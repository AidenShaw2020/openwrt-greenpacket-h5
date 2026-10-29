// SPDX-License-Identifier: GPL-2.0-only
#include <linux/err.h>
#include <linux/netdevice.h>
#include <linux/rtnetlink.h>
#include <linux/slab.h>
#include "ipa_rtk_flowtable.h"

struct ipa_rtk_binding {
	struct list_head node;
	struct ipa_rtk_flowtable *table;
	void *data;
	struct net_device *bound_dev;
};

static int ipa_rtk_flowtable_command(enum tc_setup_type type, void *data,
				   void *private)
{
	struct ipa_rtk_binding *binding = private;
	struct ipa_rtk_flowtable *table = binding->table;
	struct flow_cls_offload *cls = data;
	struct ipa_rtk_flow flow;
	int ret;

	if (type != TC_SETUP_CLSFLOWER || cls->common.chain_index)
		return -EOPNOTSUPP;
	switch (cls->command) {
	case FLOW_CLS_REPLACE:
		ret = ipa_rtk_flow_decode(cls->rule, &flow);
		if (ret)
			return ret;
		return table->ops->replace(table->private, binding, cls->cookie, &flow);
	case FLOW_CLS_DESTROY:
		return table->ops->destroy(table->private, binding, cls->cookie);
	case FLOW_CLS_STATS:
		return table->ops->stats(table->private, binding, cls->cookie, &cls->stats);
	default:
		return -EOPNOTSUPP;
	}
}

static void ipa_rtk_binding_release(void *private)
{
	struct ipa_rtk_binding *binding = private;

	binding->table->ops->flush(binding->table->private, binding);
	list_del(&binding->node);
	kfree(binding);
}

static int ipa_rtk_flowtable_setup(struct net_device *dev, struct Qdisc *sch,
		void *private, enum tc_setup_type type, void *type_data, void *data,
		void (*cleanup)(struct flow_block_cb *))
{
	struct ipa_rtk_flowtable *table = private;
	struct flow_block_offload *offload = type_data;
	struct ipa_rtk_binding *binding, *found = NULL;
	struct flow_block_cb *cb;
	bool lan;

	/* fw4 may name br-lan while NF resolves the actual path to its physical
	 * Ethernet port. Accept that master as well, once per flowtable block.
	 * NF invokes callbacks for both directions; do not bind again on WAN.
	 */
	if (type != TC_SETUP_FT || !dev)
		return -EOPNOTSUPP;
	rcu_read_lock();
	lan = dev == table->lan ||
		(netif_is_bridge_master(dev) && netdev_master_upper_dev_get_rcu(table->lan) == dev);
	rcu_read_unlock();
	if (!lan ||
	    offload->binder_type != FLOW_BLOCK_BINDER_TYPE_CLSACT_INGRESS)
		return -EOPNOTSUPP;
	offload->unlocked_driver_cb = true;
	offload->driver_block_list = &table->driver_blocks;
	list_for_each_entry(binding, &table->bindings, node)
		if (binding->data == data && binding->bound_dev == dev) {
			found = binding;
			break;
		}
	switch (offload->command) {
	case FLOW_BLOCK_BIND:
		if (found)
			return -EEXIST;
		binding = kzalloc(sizeof(*binding), GFP_KERNEL);
		if (!binding)
			return -ENOMEM;
		binding->table = table;
		binding->data = data;
		binding->bound_dev = dev;
		cb = flow_indr_block_cb_alloc(ipa_rtk_flowtable_command, binding,
			binding, ipa_rtk_binding_release, offload, dev, sch,
			data, table, cleanup);
		if (IS_ERR(cb)) {
			kfree(binding);
			return PTR_ERR(cb);
		}
		list_add_tail(&binding->node, &table->bindings);
		flow_block_cb_add(cb, offload);
		list_add_tail(&cb->driver_list, &table->driver_blocks);
		return 0;
	case FLOW_BLOCK_UNBIND:
		if (!found || found->bound_dev != dev)
			return -ENOENT;
		cb = flow_block_cb_lookup(offload->block,
					 ipa_rtk_flowtable_command, found);
		if (!cb)
			return -ENOENT;
		flow_indr_block_cb_remove(cb, offload);
		list_del(&cb->driver_list);
		return 0;
	default:
		return -EOPNOTSUPP;
	}
}

int ipa_rtk_flowtable_register(struct ipa_rtk_flowtable *table,
		struct net_device *lan, const struct ipa_rtk_flowtable_ops *ops,
		void *private)
{
	int ret;
	if (!table || !lan || !ops || !ops->replace || !ops->destroy ||
	    !ops->stats || !ops->flush || table->registered)
		return -EINVAL;
	table->lan = lan;
	table->ops = ops;
	table->private = private;
	INIT_LIST_HEAD(&table->bindings);
	INIT_LIST_HEAD(&table->driver_blocks);
	dev_hold(lan);
	ret = flow_indr_dev_register(ipa_rtk_flowtable_setup, table);
	if (ret) {
		dev_put(lan);
		return ret;
	}
	table->registered = true;
	return 0;
}

void ipa_rtk_flowtable_unregister(struct ipa_rtk_flowtable *table)
{
	if (!table || !table->registered)
		return;
	flow_indr_dev_unregister(ipa_rtk_flowtable_setup, table, ipa_rtk_binding_release);
	table->registered = false;
	dev_put(table->lan);
	table->lan = NULL;
}
