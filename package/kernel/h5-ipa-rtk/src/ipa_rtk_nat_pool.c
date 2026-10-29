// SPDX-License-Identifier: GPL-2.0-only
#include <linux/bitmap.h>
#include <linux/dma-mapping.h>
#include <linux/in.h>
#include <linux/slab.h>
#include "ipa.h"
#include "ipa_mem.h"
#include "ipa_rtk_nat_pool.h"

#define IPA_RTK_NAT_BASE 4096
#define IPA_RTK_NAT_EXPANSION 800
#define IPA_RTK_NAT_LIMIT 4000

struct ipa_rtk_nat_node {
	u16 next[2], previous[2];
	struct ipa_rtk_nat_slot *owner[2];
	struct ipa_rtk_nat_tuple tuple;
};

/* dir 0 is the destination NAT graph; dir 1 is the source index graph. */
static void *node_address(struct ipa_rtk_nat_pool *p, unsigned int dir, u16 n)
{
	return p->buffer + (dir ? p->total * 32 + n * 4 : n * 32);
}

static int write_node(struct ipa_rtk_nat_pool *p, unsigned int dir, u16 n,
		      unsigned int field, u16 value)
{
	return ipa_cmd_rtk_nat_dma(p->ipa, dir * 2 + (n >= p->count),
		(n >= p->count ? n - p->count : n) * (dir ? 4 : 32) + field, value);
}

static bool key_equal(const struct ipa_rtk_nat_tuple *a,
		      const struct ipa_rtk_nat_tuple *b, unsigned int dir)
{
	return a->remote_ip == b->remote_ip && a->remote_port == b->remote_port &&
		a->protocol == b->protocol && (dir ?
		(a->private_ip == b->private_ip && a->private_port == b->private_port) :
		(a->public_ip == b->public_ip && a->public_port == b->public_port));
}

/* Resolve a chain without modifying either software or DMA-visible state. */
static int reserve_node(struct ipa_rtk_nat_pool *p, unsigned int dir, u16 hash,
			const struct ipa_rtk_nat_tuple *tuple, u16 *index, u16 *tail)
{
	unsigned long *used = dir ? p->sources : p->destinations;
	u16 n = hash;
	unsigned int walked = 0;

	*tail = 0;
	if (!test_bit(n, used)) { *index = n; return 0; }
	for (;;) {
		if (!n || n >= p->total || !test_bit(n, used) || ++walked > p->total)
			return -EUCLEAN;
		if (p->nodes[n].owner[dir] && key_equal(&p->nodes[
			dir ? p->nodes[n].owner[dir]->destination : n].tuple, tuple, dir))
			return -EEXIST;
		if (!p->nodes[n].next[dir])
			break;
		n = p->nodes[n].next[dir];
	}
	*tail = n;
	n = find_next_zero_bit(used, p->total, p->count);
	if (n == p->total)
		return -ENOSPC;
	*index = n;
	return 0;
}

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
	p->count = IPA_RTK_NAT_BASE;
	p->total = p->count + IPA_RTK_NAT_EXPANSION;
	p->size = p->total * 36 + 32;
	p->nodes = kvcalloc(p->total, sizeof(*p->nodes), GFP_KERNEL);
	p->destinations = bitmap_zalloc(p->total, GFP_KERNEL);
	p->sources = bitmap_zalloc(p->total, GFP_KERNEL);
	p->buffer = dma_alloc_coherent(ipa->dev, p->size, &p->address, GFP_KERNEL);
	if (!p->nodes || !p->destinations || !p->sources || !p->buffer) {
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
	p->layout.expansion = p->address + p->count * 32;
	p->layout.index = p->address + p->total * 32;
	p->layout.index_expansion = p->layout.index + p->count * 4;
	p->layout.base_mask = p->count - 1;
	p->layout.expansion_count = IPA_RTK_NAT_EXPANSION;
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
	kvfree(p->nodes);
	memset(p, 0, sizeof(*p));
out:
	mutex_unlock(&pool_owner_lock);
	return ret;
}

int ipa_rtk_nat_pool_add(struct ipa_rtk_nat_pool *p,
		const struct ipa_rtk_nat_tuple *tuple, struct ipa_rtk_nat_slot *slot)
{
	u16 source, destination, tail[2];
	unsigned int dir;
	int ret;
	if (!p || !p->ipa || !tuple || !slot || slot->installed || tuple->pdn ||
	    tuple->public_ip != p->public_ip ||
	    (tuple->protocol != IPPROTO_UDP && tuple->protocol != IPPROTO_TCP))
		return -EINVAL;
	mutex_lock(&p->lock);
	if (!p->active || p->failed) { ret = -ESHUTDOWN; goto out; }
	if (p->active_count >= IPA_RTK_NAT_LIMIT) { ret = -ENOSPC; goto out; }
	ret = reserve_node(p, 0, ipa_rtk_nat_destination_hash(tuple, p->count - 1),
			   tuple, &destination, &tail[0]);
	if (!ret)
		ret = reserve_node(p, 1, ipa_rtk_nat_source_hash(tuple, p->count - 1),
				   tuple, &source, &tail[1]);
	if (ret) {
		if (ret == -EUCLEAN) p->failed = true;
		goto out;
	}
	ipa_rtk_nat_encode(node_address(p, 0, destination), tuple, 0, 0);
	/* An unlinked expansion index is private until its predecessor links it. */
	memset(node_address(p, 1, source), 0, 4);
	dma_wmb();
	ret = write_node(p, 0, destination, 18, 0x8000);
	if (!ret && tail[0]) ret = write_node(p, 0, tail[0], 8, destination);
	if (!ret) ret = write_node(p, 1, source, 0, destination);
	if (!ret && tail[1]) ret = write_node(p, 1, tail[1], 2, source);
	if (ret) { p->failed = true; goto out; }
	p->nodes[destination].tuple = *tuple;
	slot->source = source;
	slot->destination = destination;
	slot->installed = true;
	for (dir = 0; dir < 2; dir++) {
		u16 n = dir ? source : destination;
		p->nodes[n].owner[dir] = slot;
		p->nodes[n].previous[dir] = tail[dir];
		p->nodes[n].next[dir] = 0;
		if (tail[dir]) p->nodes[tail[dir]].next[dir] = n;
		set_bit(n, dir ? p->sources : p->destinations);
	}
	p->active_count++;
out:
	mutex_unlock(&p->lock);
	return ret;
}

int ipa_rtk_nat_pool_delete(struct ipa_rtk_nat_pool *p, struct ipa_rtk_nat_slot *slot)
{
	u16 d, s, next, previous;
	int ret = 0;
	if (!p || !p->ipa || !slot)
		return -EINVAL;
	mutex_lock(&p->lock);
	if (!slot->installed) goto out;
	d = slot->destination; s = slot->source;
	if (!d || !s || d >= p->total || s >= p->total ||
	    p->nodes[d].owner[0] != slot || p->nodes[s].owner[1] != slot) {
		ret = -EINVAL; goto out;
	}
	if (p->failed || !p->active) { ret = -ESHUTDOWN; goto out; }
	/* A base head must stay enabled for the hardware to traverse successors.
	 * Stock invalidates its protocol instead. Never reuse this sentinel until
	 * the chain becomes empty. Every command drains prior pipeline readers.
	 */
	ret = d < p->count && p->nodes[d].next[0] ?
		write_node(p, 0, d, 22, 0xff00) : write_node(p, 0, d, 18, 0);
	if (ret) goto failed;
	next = p->nodes[s].next[1]; previous = p->nodes[s].previous[1];
	if (!previous && next) {
		struct ipa_rtk_nat_slot *promoted = p->nodes[next].owner[1];
		if (!promoted) { ret = -EUCLEAN; goto failed; }
		/* Preserve the chain while promoting the first expansion element. */
		ret = write_node(p, 1, s, 2, p->nodes[next].next[1]);
		if (!ret) ret = write_node(p, 1, s, 0, promoted->destination);
		if (ret) goto failed;
		p->nodes[s].owner[1] = promoted;
		p->nodes[s].next[1] = p->nodes[next].next[1];
		if (p->nodes[s].next[1]) p->nodes[p->nodes[s].next[1]].previous[1] = s;
		promoted->source = s;
		s = next;
	} else {
		ret = write_node(p, 1, previous ? previous : s, previous ? 2 : 0, next);
		if (ret) goto failed;
		if (previous) p->nodes[previous].next[1] = next;
		if (next) p->nodes[next].previous[1] = previous;
	}
	p->nodes[s].owner[1] = NULL;
	p->nodes[s].next[1] = p->nodes[s].previous[1] = 0;
	memset(node_address(p, 1, s), 0, 4);
	clear_bit(s, p->sources);

	next = p->nodes[d].next[0]; previous = p->nodes[d].previous[0];
	p->nodes[d].owner[0] = NULL;
	if (previous || !next) {
		if (previous) {
			ret = write_node(p, 0, previous, 8, next);
			if (ret) goto failed;
			p->nodes[previous].next[0] = next;
			if (next) p->nodes[next].previous[0] = previous;
		}
		memset(node_address(p, 0, d), 0, 32);
		p->nodes[d].next[0] = p->nodes[d].previous[0] = 0;
		clear_bit(d, p->destinations);
		if (previous && !next && previous < p->count && !p->nodes[previous].owner[0]) {
			ret = write_node(p, 0, previous, 18, 0);
			if (ret) goto failed;
			memset(node_address(p, 0, previous), 0, 32);
			clear_bit(previous, p->destinations);
		}
	}
	p->active_count--;
	memset(slot, 0, sizeof(*slot));
	goto out;
failed:
	/* Unknown partially published state stays allocated until reset. */
	p->failed = true;
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
	if (p->active && slot->installed && slot->destination < p->total) {
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
	kvfree(p->nodes);
	p->buffer = NULL;
	p->nodes = NULL;
	p->ipa = NULL;
	pool_owner = NULL;
out:
	mutex_unlock(&p->lock);
	mutex_unlock(&pool_owner_lock);
	return ret;
}
