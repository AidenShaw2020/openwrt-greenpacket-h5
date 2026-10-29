// SPDX-License-Identifier: GPL-2.0-only
#include <linux/bitmap.h>
#include <linux/dma-mapping.h>
#include <linux/in.h>
#include "ipa.h"
#include "ipa_mem.h"
#include "ipa_rtk_nat_pool.h"

static DEFINE_MUTEX(pool_owner_lock);
static struct ipa_rtk_nat_pool *pool_owner;

int ipa_rtk_nat_pool_create(struct ipa_rtk_nat_pool *p, struct ipa *ipa,
			   __be32 public_ip, u32 source_metadata, u32 destination_metadata)
{
	const struct ipa_mem *pdn = NULL;
	struct ipa_rtk_pdn *record;
	unsigned int i;
	int ret = -EBUSY;
	if (!p || p->ipa || !ipa || !public_ip || ipa->version != IPA_VERSION_5_0)
		return -EINVAL;
	mutex_lock(&pool_owner_lock);
	if (pool_owner)
		goto out;
	for (i = 0; i < ipa->mem_count; i++)
		if (ipa->mem[i].id == IPA_MEM_PDN_CONFIG)
			pdn = &ipa->mem[i];
	if (!pdn || pdn->size < 16) { ret = -ENODEV; goto out; }
	p->count = 4096;
	p->size = p->count * 36 + 32 + 8 + 32;
	p->destinations = bitmap_zalloc(p->count, GFP_KERNEL);
	p->sources = bitmap_zalloc(p->count, GFP_KERNEL);
	p->buffer = dma_alloc_coherent(ipa->dev, p->size, &p->address, GFP_KERNEL);
	if (!p->destinations || !p->sources || !p->buffer) {
		ret = -ENOMEM;
		goto free;
	}
	memset(p->buffer, 0, p->size);
	memcpy_fromio(p->buffer + p->size - 16,
		ipa->mem_virt + ipa->mem_offset + pdn->offset, 16);
	record = p->buffer + p->size - 32;
	record->public_ip = cpu_to_le32(be32_to_cpu(public_ip));
	record->source_metadata = cpu_to_le32(source_metadata);
	record->destination_metadata = cpu_to_le32(destination_metadata);
	p->layout.base = p->address;
	p->layout.index = p->address + p->count * 32;
	p->layout.expansion = p->layout.index + p->count * 4;
	p->layout.index_expansion = p->layout.expansion + 32;
	p->layout.base_mask = p->count - 1;
	p->layout.expansion_count = 0;
	p->failed = false;
	mutex_init(&p->lock);
	ret = ipa_cmd_rtk_nat_init(ipa, &p->layout, p->address + p->size - 32);
	if (ret)
		goto free;
	p->ipa = ipa;
	p->public_ip = public_ip;
	p->active = true;
	pool_owner = p;
	goto out;
free:
	if (p->buffer)
		dma_free_coherent(ipa->dev, p->size, p->buffer, p->address);
	bitmap_free(p->sources);
	bitmap_free(p->destinations);
	memset(p, 0, sizeof(*p));
out:
	mutex_unlock(&pool_owner_lock);
	return ret;
}

int ipa_rtk_nat_pool_add(struct ipa_rtk_nat_pool *p,
		const struct ipa_rtk_nat_tuple *tuple, struct ipa_rtk_nat_slot *slot)
{
	struct ipa_rtk_nat_entry *entry;
	u16 source, destination;
	int ret;
	if (!p || !p->ipa || !tuple || !slot || slot->installed || tuple->pdn ||
	    tuple->public_ip != p->public_ip ||
	    (tuple->protocol != IPPROTO_UDP && tuple->protocol != IPPROTO_TCP))
		return -EINVAL;
	mutex_lock(&p->lock);
	if (!p->active || p->failed) { ret = -ESHUTDOWN; goto out; }
	source = ipa_rtk_nat_source_hash(tuple, p->count - 1);
	destination = ipa_rtk_nat_destination_hash(tuple, p->count - 1);
	if (test_bit(source, p->sources) || test_bit(destination, p->destinations)) {
		ret = -ENOSPC;
		goto out;
	}
	entry = p->buffer + destination * 32;
	ipa_rtk_nat_encode(entry, tuple, 0, 0);
	dma_wmb();
	ret = ipa_cmd_rtk_nat_dma(p->ipa, 2, source * 4, destination);
	if (!ret)
		ret = ipa_cmd_rtk_nat_dma(p->ipa, 0, destination * 32 + 18,
					 0x8000);
	if (ret) {
		/* Unknown partially published state must survive until reset. */
		p->failed = true;
		goto out;
	}
	set_bit(source, p->sources);
	set_bit(destination, p->destinations);
	slot->source = source;
	slot->destination = destination;
	slot->installed = true;
out:
	mutex_unlock(&p->lock);
	return ret;
}

int ipa_rtk_nat_pool_delete(struct ipa_rtk_nat_pool *p, struct ipa_rtk_nat_slot *slot)
{
	int ret = 0;
	if (!p || !p->ipa || !slot)
		return -EINVAL;
	mutex_lock(&p->lock);
	if (!slot->installed)
		goto out;
	if (slot->source >= p->count || slot->destination >= p->count) {
		ret = -EINVAL;
		goto out;
	}
	if (p->failed || !p->active) { ret = -ESHUTDOWN; goto out; }
	/* First make the entry unusable and drain lookup readers, then unlink. */
	ret = ipa_cmd_rtk_nat_dma(p->ipa, 0, slot->destination * 32 + 18, 0);
	if (!ret)
		ret = ipa_cmd_rtk_nat_dma(p->ipa, 2, slot->source * 4, 0);
	if (ret) { p->failed = true; goto out; }
	memset(p->buffer + slot->destination * 32, 0, 32);
	clear_bit(slot->source, p->sources);
	clear_bit(slot->destination, p->destinations);
	memset(slot, 0, sizeof(*slot));
out:
	mutex_unlock(&p->lock);
	return ret;
}

u32 ipa_rtk_nat_pool_timestamp(struct ipa_rtk_nat_pool *p, const struct ipa_rtk_nat_slot *slot)
{
	const struct ipa_rtk_nat_entry *entry;
	u32 timestamp = 0;
	if (!p || !p->ipa || !slot)
		return 0;
	mutex_lock(&p->lock);
	if (p->active && slot->installed && slot->destination < p->count) {
		entry = p->buffer + slot->destination * 32;
		dma_rmb();
		timestamp = le32_to_cpu(READ_ONCE(entry->word[5])) & 0xffffff;
	}
	mutex_unlock(&p->lock);
	return timestamp;
}

int ipa_rtk_nat_pool_destroy(struct ipa_rtk_nat_pool *p)
{
	int ret = 0;
	if (!p || !p->ipa)
		return -EINVAL;
	mutex_lock(&pool_owner_lock);
	mutex_lock(&p->lock);
	if (pool_owner != p) { ret = -EINVAL; goto out; }
	ret = ipa_cmd_rtk_nat_init(p->ipa, &p->layout, p->address + p->size - 16);
	if (!ret)
		ret = ipa_cmd_rtk_nat_disable(p->ipa);
	if (ret) { p->failed = true; goto out; }
	p->active = false;
	dma_free_coherent(p->ipa->dev, p->size, p->buffer, p->address);
	bitmap_free(p->sources);
	bitmap_free(p->destinations);
	p->buffer = NULL;
	p->ipa = NULL;
	pool_owner = NULL;
out:
	mutex_unlock(&p->lock);
	mutex_unlock(&pool_owner_lock);
	return ret;
}
