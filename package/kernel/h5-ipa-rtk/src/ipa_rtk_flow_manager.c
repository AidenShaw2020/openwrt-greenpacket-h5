// SPDX-License-Identifier: GPL-2.0-only
#include <linux/in.h>
#include <linux/jiffies.h>
#include <linux/slab.h>
#include <linux/bitmap.h>
#include <linux/jhash.h>
#include "ipa_rtk_flow_manager.h"

static bool same_tuple(const struct ipa_rtk_nat_tuple *a, const struct ipa_rtk_nat_tuple *b)
{
	return a->private_ip == b->private_ip && a->remote_ip == b->remote_ip &&
		a->public_ip == b->public_ip && a->private_port == b->private_port &&
		a->remote_port == b->remote_port && a->public_port == b->public_port &&
		a->protocol == b->protocol && a->pdn == b->pdn;
}

static bool same_flow(const struct ipa_rtk_flow *a, const struct ipa_rtk_flow *b)
{
	return same_tuple(&a->nat, &b->nat) && a->ingress_ifindex == b->ingress_ifindex &&
		a->egress_ifindex == b->egress_ifindex && a->destination == b->destination &&
		!memcmp(a->ethernet, b->ethernet, ETH_HLEN);
}

static u32 cookie_bucket(const void *binding, unsigned long cookie)
{
	return jhash_2words((u32)(unsigned long)binding, (u32)cookie, 0) &
		(IPA_RTK_FLOW_BUCKETS - 1);
}

static u32 tuple_bucket(const void *binding, const struct ipa_rtk_nat_tuple *tuple)
{
	return jhash(tuple, sizeof(*tuple), (u32)(unsigned long)binding) &
		(IPA_RTK_FLOW_BUCKETS - 1);
}

static struct ipa_rtk_managed_flow *find_cookie(struct ipa_rtk_flow_manager *m,
		const void *binding, unsigned long cookie, unsigned int direction)
{
	struct ipa_rtk_cookie_ref *ref;
	hlist_for_each_entry(ref, &m->cookies[cookie_bucket(binding, cookie)], node)
		if ((direction > 1 || ref->direction == direction) &&
		    ref->entry->binding == binding &&
		    ref->entry->cookie[ref->direction] == cookie)
			return ref->entry;
	return NULL;
}

static struct ipa_rtk_managed_flow *find_tuple(struct ipa_rtk_flow_manager *m,
		const void *binding, const struct ipa_rtk_nat_tuple *tuple)
{
	struct ipa_rtk_managed_flow *entry;
	hlist_for_each_entry(entry, &m->tuples[tuple_bucket(binding, tuple)], tuple_node)
		if (entry->binding == binding &&
		    same_tuple(&entry->flow[entry->present[0] ? 0 : 1].nat, tuple))
			return entry;
	return NULL;
}

/* Called only after hardware references are gone, or for a never-published
 * half-pair. No installed flow is evicted to make room for a new connection.
 */
static void forget_entry(struct ipa_rtk_flow_manager *m, struct ipa_rtk_managed_flow *entry)
{
	unsigned int dir;
	hlist_del_init(&entry->tuple_node);
	for (dir = 0; dir < 2; dir++)
		hlist_del_init(&entry->refs[dir].node);
	if (!list_empty(&entry->pending))
		list_del_init(&entry->pending);
	clear_bit(entry - m->entries, m->used);
	memset(entry, 0, sizeof(*entry));
}

static void expire_pending(struct ipa_rtk_flow_manager *m)
{
	while (!list_empty(&m->pending)) {
		struct ipa_rtk_managed_flow *entry =
			list_first_entry(&m->pending, struct ipa_rtk_managed_flow, pending);
		if (!time_after(jiffies, entry->lastused + 30 * HZ))
			break;
		forget_entry(m, entry);
	}
}

static void fail_closed(struct ipa_rtk_flow_manager *m)
{
	m->failed = true;
	m->ops->quiesce(m->private);
}

static int remove_entry(struct ipa_rtk_flow_manager *m, unsigned int index)
{
	struct ipa_rtk_managed_flow *entry = &m->entries[index];
	int ret;
	if (!entry->used)
		return 0;
	/* Disable translation first: an expired connection cannot continue to
	 * forward while its classifiers are being removed. Referenced storage
	 * stays allocated until all remove commands have completed.
	 */
	if (entry->nat.installed) {
		ret = ipa_rtk_nat_pool_delete(&m->pool, &entry->nat);
		if (ret)
			goto fail;
	}
	if (entry->hardware) {
		ret = m->ops->remove(m->private, index);
		if (ret)
			goto fail;
	}
	forget_entry(m, entry);
	return 0;
fail:
	fail_closed(m);
	return ret;
}

static int managed_replace(void *private, const void *binding, unsigned long cookie,
			   const struct ipa_rtk_flow *flow)
{
	struct ipa_rtk_flow_manager *m = private;
	struct ipa_rtk_managed_flow *entry = NULL;
	unsigned int dir;
	int index = -1, ret = -EOPNOTSUPP;
	bool lan_in, lan_out;

	if (!flow || (flow->nat.protocol != IPPROTO_UDP &&
		      flow->nat.protocol != IPPROTO_TCP))
		return ret;
	dir = flow->destination;
	mutex_lock(&m->lock);
	if (!m->active || m->failed) { ret = -ESHUTDOWN; goto out; }
	lan_in = flow->ingress_ifindex == m->lan || flow->ingress_ifindex == m->bridge;
	lan_out = flow->egress_ifindex == m->lan || flow->egress_ifindex == m->bridge;
	if (flow->nat.public_ip != m->pool.public_ip ||
	    (!dir && (!lan_in || flow->egress_ifindex != m->wan)) ||
	    (dir && (flow->ingress_ifindex != m->wan || !lan_out)))
		goto out;
	expire_pending(m);
	entry = find_cookie(m, binding, cookie, dir);
	if (entry && !same_tuple(&entry->flow[dir].nat, &flow->nat)) {
		ret = -ESTALE;
		goto out;
	}
	if (!entry)
		entry = find_tuple(m, binding, &flow->nat);
	if (!entry) {
		index = find_first_zero_bit(m->used, IPA_RTK_FLOW_LIMIT);
		if (index == IPA_RTK_FLOW_LIMIT) { ret = -ENOSPC; goto out; }
		entry = &m->entries[index];
		INIT_LIST_HEAD(&entry->pending);
		entry->binding = binding;
		entry->used = true;
		entry->lastused = jiffies;
		set_bit(index, m->used);
		list_add_tail(&entry->pending, &m->pending);
	} else {
		index = entry - m->entries;
	}
	if (entry->present[dir]) {
		/* A cookie is one direction of one NF flow, not a replaceable tuple
		 * pointer. Do not merge distinct connections that happen to share NAT.
		 */
		ret = entry->cookie[dir] == cookie &&
			same_flow(&entry->flow[dir], flow) ?
			(entry->hardware ? 0 : -EAGAIN) : -EEXIST;
		goto out;
	}
	entry->flow[dir] = *flow;
	entry->cookie[dir] = cookie;
	entry->present[dir] = true;
	entry->refs[dir].entry = entry;
	entry->refs[dir].direction = dir;
	hlist_add_head(&entry->refs[dir].node, &m->cookies[cookie_bucket(binding, cookie)]);
	if (!entry->present[!dir]) {
		hlist_add_head(&entry->tuple_node, &m->tuples[tuple_bucket(binding, &flow->nat)]);
		ret = -EAGAIN;
		goto out;
	}
	list_del_init(&entry->pending);
	/* No success is reported for a pending half-pair. Only the callback that
	 * has both independently supplied directions may activate hardware.
	 */
	ret = ipa_rtk_nat_pool_add(&m->pool, &flow->nat, &entry->nat);
	if (ret) {
		if (m->pool.failed)
			fail_closed(m);
		else
			forget_entry(m, entry);
		goto out;
	}
	/* install must roll back partial classifiers or remove must handle them. */
	entry->hardware = true;
	ret = m->ops->install(m->private, index, entry->flow);
	if (ret) {
		int cleanup = remove_entry(m, index);
		if (cleanup)
			ret = cleanup;
		goto out;
	}
	entry->lastused = jiffies;
	entry->timestamp = 0;
out:
	mutex_unlock(&m->lock);
	return ret;
}

static int managed_destroy(void *private, const void *binding, unsigned long cookie)
{
	struct ipa_rtk_flow_manager *m = private;
	struct ipa_rtk_managed_flow *entry;
	int ret = -ENOENT;
	mutex_lock(&m->lock);
	if (!m->active || m->failed) { ret = m->failed ? -ESHUTDOWN : -ENOENT; goto out; }
	entry = find_cookie(m, binding, cookie, 2);
	if (entry)
		ret = remove_entry(m, entry - m->entries);
out:
	mutex_unlock(&m->lock);
	return ret;
}

static int managed_stats(void *private, const void *binding, unsigned long cookie,
			 struct flow_stats *stats)
{
	struct ipa_rtk_flow_manager *m = private;
	struct ipa_rtk_managed_flow *entry;
	u32 timestamp;
	int ret = -ENOENT;
	mutex_lock(&m->lock);
	if (!m->active || m->failed) { ret = m->failed ? -ESHUTDOWN : -ENOENT; goto out; }
	entry = find_cookie(m, binding, cookie, 2);
	if (!entry || !entry->hardware)
		goto out;
	timestamp = ipa_rtk_nat_pool_timestamp(&m->pool, &entry->nat);
	if (timestamp != entry->timestamp) {
		entry->timestamp = timestamp;
		entry->lastused = jiffies;
	}
	/* IPA timestamps are shared by both directions; never invent counters. */
	flow_stats_update(stats, 0, 0, 0, entry->lastused, FLOW_ACTION_HW_STATS_DELAYED);
	ret = 0;
out:
	mutex_unlock(&m->lock);
	return ret;
}

static void managed_flush(void *private, const void *binding)
{
	struct ipa_rtk_flow_manager *m = private;
	unsigned int i;
	mutex_lock(&m->lock);
	if (!m->active) {
		mutex_unlock(&m->lock);
		return;
	}
	for (i = 0; i < IPA_RTK_FLOW_LIMIT; i++)
		if (m->entries[i].used && m->entries[i].binding == binding) {
			if (!m->failed)
				remove_entry(m, i);
			/* A failed entry stays quarantined, but cannot alias a newly
			 * allocated binding when the old callback object is released.
			 */
			if (m->failed)
				m->entries[i].binding = NULL;
		}
	mutex_unlock(&m->lock);
}

const struct ipa_rtk_flowtable_ops ipa_rtk_managed_flow_ops = {
	.replace = managed_replace, .destroy = managed_destroy,
	.stats = managed_stats, .flush = managed_flush,
};

int ipa_rtk_flow_manager_start(struct ipa_rtk_flow_manager *m, struct ipa *ipa,
		__be32 public_ip, u32 source_metadata, u32 destination_metadata,
		int lan, int bridge, int wan, const struct ipa_rtk_flow_manager_ops *ops,
		void *private)
{
	int ret;
	if (!m || m->active || !ops || !ops->install || !ops->remove || !ops->quiesce ||
	    lan <= 0 || wan <= 0 || lan == wan || bridge == wan)
		return -EINVAL;
	mutex_lock(&m->lock);
	m->entries = kvcalloc(IPA_RTK_FLOW_LIMIT, sizeof(*m->entries), GFP_KERNEL);
	m->tuples = kvcalloc(IPA_RTK_FLOW_BUCKETS, sizeof(*m->tuples), GFP_KERNEL);
	m->cookies = kvcalloc(IPA_RTK_FLOW_BUCKETS, sizeof(*m->cookies), GFP_KERNEL);
	m->used = bitmap_zalloc(IPA_RTK_FLOW_LIMIT, GFP_KERNEL);
	if (!m->entries || !m->tuples || !m->cookies || !m->used) {
		ret = -ENOMEM;
		goto free;
	}
	INIT_LIST_HEAD(&m->pending);
	ret = ipa_rtk_nat_pool_create(&m->pool, ipa, public_ip, source_metadata, destination_metadata);
	if (ret)
		goto free;
	m->lan = lan; m->bridge = bridge; m->wan = wan;
	m->ops = ops; m->private = private;
	m->failed = false;
	m->active = true;
	mutex_unlock(&m->lock);
	return 0;
free:
	kvfree(m->entries); kvfree(m->tuples); kvfree(m->cookies);
	bitmap_free(m->used);
	m->entries = NULL; m->tuples = NULL; m->cookies = NULL; m->used = NULL;
	mutex_unlock(&m->lock);
	return ret;
}

int ipa_rtk_flow_manager_stop(struct ipa_rtk_flow_manager *m)
{
	unsigned int i;
	int ret = 0;
	if (!m || !m->active)
		return -EINVAL;
	mutex_lock(&m->lock);
	if (m->failed) { ret = -ESHUTDOWN; goto out; }
	for (i = 0; i < IPA_RTK_FLOW_LIMIT; i++) {
		ret = remove_entry(m, i);
		if (ret)
			goto out;
	}
	ret = ipa_rtk_nat_pool_destroy(&m->pool);
	if (ret)
		fail_closed(m);
	else {
		m->active = false;
		kvfree(m->entries); kvfree(m->tuples); kvfree(m->cookies);
		bitmap_free(m->used);
		m->entries = NULL; m->tuples = NULL; m->cookies = NULL; m->used = NULL;
	}
out:
	mutex_unlock(&m->lock);
	return ret;
}
