/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _IPA_RTK_FLOWTABLE_H_
#define _IPA_RTK_FLOWTABLE_H_
#include <linux/list.h>
#include <net/flow_offload.h>
#include "ipa_rtk_flow.h"

struct net_device;
struct ipa_rtk_flowtable;
struct ipa_rtk_flowtable_ops {
	/* A zero replace result means hardware forwarding is actually installed.
	 * Cookies are opaque, scoped to binding, and must never be dereferenced.
	 * All calls run in process context, serialized by the backend's lock.
	 */
	int (*replace)(void *private, const void *binding, unsigned long cookie,
		       const struct ipa_rtk_flow *flow);
	int (*destroy)(void *private, const void *binding, unsigned long cookie);
	int (*stats)(void *private, const void *binding, unsigned long cookie,
		     struct flow_stats *stats);
	/* Synchronously remove all hardware rules owned by this binding. */
	void (*flush)(void *private, const void *binding);
};
struct ipa_rtk_flowtable {
	struct net_device *lan;
	const struct ipa_rtk_flowtable_ops *ops;
	void *private;
	struct list_head bindings, driver_blocks;
	bool registered;
};
/* Register only after CPU fallback and hardware teardown are operational.
 * The caller owns LAN's lifetime through unregister; registration holds a ref.
 * Both register and unregister must be called WITHOUT holding rtnl_lock or
 * the backend's lock, because flow_indr_dev_unregister invokes the flowtable's
 * cleanup callback (which flushesnf_ft_offload_del) and binding_release.
 */
int ipa_rtk_flowtable_register(struct ipa_rtk_flowtable *table,
		struct net_device *lan, const struct ipa_rtk_flowtable_ops *ops,
		void *private);
void ipa_rtk_flowtable_unregister(struct ipa_rtk_flowtable *table);
#endif
