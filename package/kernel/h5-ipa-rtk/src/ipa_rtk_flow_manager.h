/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _IPA_RTK_FLOW_MANAGER_H_
#define _IPA_RTK_FLOW_MANAGER_H_
#include "ipa_rtk_flowtable.h"
#include "ipa_rtk_nat_pool.h"
#include <linux/list.h>
#include <linux/workqueue.h>

#define IPA_RTK_FLOW_LIMIT 4000
#define IPA_RTK_FLOW_BUCKETS 1024
#define IPA_RTK_TEARDOWN_GRACE (2 * HZ)
struct ipa_rtk_managed_flow;
struct ipa_rtk_cookie_ref {
	struct hlist_node node;
	struct ipa_rtk_managed_flow *entry;
	unsigned int direction;
};
struct ipa_rtk_flow_manager_ops {
	/* Publish/remove ONLY exact per-flow classifiers and route/header state.
	 * NAT pool owns translation and remains alive until every filter is gone.
	 */
	int (*install)(void *private, unsigned int index, const struct ipa_rtk_flow flow[2]);
	int (*remove)(void *private, unsigned int index);
	/* Fail closed: synchronously disable external NIC DMA without freeing
	 * referenced buffers. Owner must remain pinned until a confirmed reset.
	 */
	void (*quiesce)(void *private);
};
struct ipa_rtk_managed_flow {
	struct hlist_node tuple_node;
	struct ipa_rtk_cookie_ref refs[2];
	struct list_head pending, retiring;
	const void *binding;
	unsigned long cookie[2], lastused, retire_at;
	struct ipa_rtk_flow flow[2];
	struct ipa_rtk_nat_slot nat;
	u32 timestamp;
	bool present[2], used, hardware, closing;
};
struct ipa_rtk_flow_manager {
	struct mutex lock;
	struct ipa_rtk_nat_pool pool;
	struct ipa_rtk_managed_flow *entries;
	struct hlist_head *tuples, *cookies;
	unsigned long *used;
	struct list_head pending, retiring;
	struct delayed_work retire_work;
	const struct ipa_rtk_flow_manager_ops *ops;
	void *private;
	int lan, bridge, wan;
	bool active, failed;
};
extern const struct ipa_rtk_flowtable_ops ipa_rtk_managed_flow_ops;
/* Manager is initially zeroed; caller holds IPA power throughout its life. */
int ipa_rtk_flow_manager_start(struct ipa_rtk_flow_manager *manager, struct ipa *ipa,
		__be32 public_ip, u32 source_metadata, u32 destination_metadata,
		int lan, int bridge, int wan, const struct ipa_rtk_flow_manager_ops *ops,
		void *private);
/* Unregister flowtable first. Failure retains memory and requires reset. */
int ipa_rtk_flow_manager_stop(struct ipa_rtk_flow_manager *manager);
#endif
