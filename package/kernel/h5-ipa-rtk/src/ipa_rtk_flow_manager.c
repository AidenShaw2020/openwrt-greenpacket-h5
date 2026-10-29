// SPDX-License-Identifier: GPL-2.0-only
#include <linux/in.h>
#include <linux/jiffies.h>
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
	memset(entry, 0, sizeof(*entry));
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
	unsigned int dir, i, free_index = IPA_RTK_FLOW_LIMIT;
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
	for (i = 0; i < IPA_RTK_FLOW_LIMIT; i++) {
		struct ipa_rtk_managed_flow *candidate = &m->entries[i];
		if (!candidate->used) { free_index = i; continue; }
		if (candidate->binding != binding)
			continue;
		if (candidate->present[dir] && candidate->cookie[dir] == cookie &&
		    !same_tuple(&candidate->flow[dir].nat, &flow->nat)) {
			ret = -ESTALE;
			goto out;
		}
		if (same_tuple(&candidate->flow[candidate->present[0] ? 0 : 1].nat, &flow->nat)) {
			index = i;
			entry = candidate;
			break;
		}
	}
	if (!entry) {
		if (free_index == IPA_RTK_FLOW_LIMIT) { ret = -ENOSPC; goto out; }
		index = free_index;
		entry = &m->entries[index];
		entry->binding = binding;
		entry->used = true;
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
	if (!entry->present[!dir]) { ret = -EAGAIN; goto out; }
	/* No success is reported for a pending half-pair. Only the callback that
	 * has both independently supplied directions may activate hardware.
	 */
	ret = ipa_rtk_nat_pool_add(&m->pool, &flow->nat, &entry->nat);
	if (ret) {
		if (m->pool.failed)
			fail_closed(m);
		else
			memset(entry, 0, sizeof(*entry));
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
	unsigned int i, dir;
	int ret = -ENOENT;
	mutex_lock(&m->lock);
	if (!m->active || m->failed) { ret = m->failed ? -ESHUTDOWN : -ENOENT; goto out; }
	for (i = 0; i < IPA_RTK_FLOW_LIMIT; i++) {
		struct ipa_rtk_managed_flow *entry = &m->entries[i];
		if (!entry->used || entry->binding != binding)
			continue;
		for (dir = 0; dir < 2; dir++)
			if (entry->present[dir] && entry->cookie[dir] == cookie) {
				ret = remove_entry(m, i);
				goto out;
			}
	}
out:
	mutex_unlock(&m->lock);
	return ret;
}

static int managed_stats(void *private, const void *binding, unsigned long cookie,
			 struct flow_stats *stats)
{
	struct ipa_rtk_flow_manager *m = private;
	unsigned int i;
	int ret = -ENOENT;
	mutex_lock(&m->lock);
	if (!m->active || m->failed) { ret = m->failed ? -ESHUTDOWN : -ENOENT; goto out; }
	for (i = 0; i < IPA_RTK_FLOW_LIMIT; i++) {
		struct ipa_rtk_managed_flow *entry = &m->entries[i];
		u32 timestamp;
		if (!entry->used || !entry->hardware || entry->binding != binding ||
		    (entry->cookie[0] != cookie && entry->cookie[1] != cookie))
			continue;
		timestamp = ipa_rtk_nat_pool_timestamp(&m->pool, &entry->nat);
		if (timestamp != entry->timestamp) {
			entry->timestamp = timestamp;
			entry->lastused = jiffies;
		}
		/* IPA NAT timestamp is shared by the two directions. Never invent
		 * packet/byte counters; report only the activity actually observed.
		 */
		flow_stats_update(stats, 0, 0, 0, entry->lastused, FLOW_ACTION_HW_STATS_DELAYED);
		ret = 0;
		break;
	}
out:
	mutex_unlock(&m->lock);
	return ret;
}

static void managed_flush(void *private, const void *binding)
{
	struct ipa_rtk_flow_manager *m = private;
	unsigned int i;
	mutex_lock(&m->lock);
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
	ret = ipa_rtk_nat_pool_create(&m->pool, ipa, public_ip, source_metadata, destination_metadata);
	if (ret) {
		mutex_unlock(&m->lock);
		return ret;
	}
	m->lan = lan; m->bridge = bridge; m->wan = wan;
	m->ops = ops; m->private = private;
	m->failed = false;
	m->active = true;
	mutex_unlock(&m->lock);
	return 0;
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
	else
		m->active = false;
out:
	mutex_unlock(&m->lock);
	return ret;
}
