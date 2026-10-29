// SPDX-License-Identifier: GPL-2.0-only
/* SDX65/RTL8125 external DMA controller. Stage 0 is passive domain attachment. */
#include <linux/dma-mapping.h>
#include <linux/ethtool.h>
#include <linux/inetdevice.h>
#include <linux/iommu.h>
#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/pci.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/rtnetlink.h>
#include <asm/dma-iommu.h>
#include <asm/dma-mapping.h>

#include "ipa.h"
#include "ipa_endpoint_rtk.h"
#include "ipa_rtk_dma.h"
#include "ipa_uc_rtk.h"
#include "ipa_rtk_debug.h"
#include "ipa_reg.h"
#include "ipa_table_rtk.h"
#include "ipa_qmi_rtk.h"
#define H5_IPA_QMI_SCHEMA_DEFINE
#include "ipa_qmi_filter_schema.h"
#include "ipa_rtk_modem_rules.h"
#include "ipa_rtk_flow_manager.h"

#define H5_EP8_METADATA (0x101c + 8 * 0x80)
#define IPA_RTK_MAX_HEADERS 6

struct h5_dl_fltr_req_ex_msg {
	u8 filter_spec_ex_list_valid;
	u8 reserved_1[3];
	u32 filter_spec_ex_list_len;
	struct h5_filter_spec_ex_type filter_spec_ex_list[IPA_RTK_FLOW_LIMIT];
} __packed;

static const struct qmi_elem_info h5_dl_fltr_req_ex_msg_ei[] = {
	{
		.data_type = QMI_OPT_FLAG,
		.elem_len = 1,
		.elem_size = 1,
		.array_type = NO_ARRAY,
		.tlv_type = 0x10,
		.offset = offsetof(struct h5_dl_fltr_req_ex_msg, filter_spec_ex_list_valid),
	},
	{
		.data_type = QMI_DATA_LEN,
		.elem_len = 1,
		.elem_size = 1,
		.array_type = NO_ARRAY,
		.tlv_type = 0x10,
		.offset = offsetof(struct h5_dl_fltr_req_ex_msg, filter_spec_ex_list_len),
	},
	{
		.data_type = QMI_STRUCT,
		.elem_len = IPA_RTK_FLOW_LIMIT,
		.elem_size = sizeof(struct h5_filter_spec_ex_type),
		.array_type = VAR_LEN_ARRAY,
		.tlv_type = 0x10,
		.offset = offsetof(struct h5_dl_fltr_req_ex_msg, filter_spec_ex_list),
		.ei_array = h5_filter_spec_ex_type_ei,
	},
	{ .data_type = QMI_EOTI },
};

struct h5_dl_fltr_resp_ex_msg {
	struct h5_qmi_response_type resp;
	u8 rule_id_valid;
	u8 reserved_1[3];
	u32 rule_id_len;
	u32 rule_id[128];
} __packed;

static const struct qmi_elem_info h5_dl_fltr_resp_ex_msg_ei[] = {
	{
		.data_type = QMI_STRUCT,
		.elem_len = 1,
		.elem_size = sizeof(struct h5_qmi_response_type),
		.array_type = NO_ARRAY,
		.tlv_type = 0x02,
		.offset = offsetof(struct h5_dl_fltr_resp_ex_msg, resp),
		.ei_array = h5_qmi_response_type_ei,
	},
	{
		.data_type = QMI_OPT_FLAG,
		.elem_len = 1,
		.elem_size = 1,
		.array_type = NO_ARRAY,
		.tlv_type = 0x10,
		.offset = offsetof(struct h5_dl_fltr_resp_ex_msg, rule_id_valid),
	},
	{
		.data_type = QMI_DATA_LEN,
		.elem_len = 1,
		.elem_size = 1,
		.array_type = NO_ARRAY,
		.tlv_type = 0x10,
		.offset = offsetof(struct h5_dl_fltr_resp_ex_msg, rule_id_len),
	},
	{
		.data_type = QMI_UNSIGNED_4_BYTE,
		.elem_len = 128,
		.elem_size = sizeof(u32),
		.array_type = VAR_LEN_ARRAY,
		.tlv_type = 0x10,
		.offset = offsetof(struct h5_dl_fltr_resp_ex_msg, rule_id),
	},
	{ .data_type = QMI_EOTI },
};

static int send_dl_qmi(struct ipa *ipa, const struct h5_dl_fltr_req_ex_msg *req,
		       struct h5_dl_fltr_resp_ex_msg *resp)
{
	struct qmi_handle qmi = {};
	struct qmi_txn txn;
	int ret;

	(void)h5_install_fltr_rule_req_msg_ei;
	(void)h5_install_fltr_rule_resp_msg_ei;
	(void)h5_fltr_installed_notif_req_msg_ei;
	(void)h5_fltr_installed_notif_resp_msg_ei;

	ret = qmi_handle_init(&qmi, 1024, NULL, NULL);
	if (ret < 0)
		return ret;
	memset(resp, 0, sizeof(*resp));
	ret = qmi_txn_init(&qmi, &txn, h5_dl_fltr_resp_ex_msg_ei, resp);
	if (ret < 0)
		goto out;
	ret = qmi_send_request(&qmi, &ipa->qmi.modem_sq, &txn, 0x0037,
			       sizeof(*req) + 64, h5_dl_fltr_req_ex_msg_ei, req);
	if (ret < 0) {
		qmi_txn_cancel(&txn);
		goto out;
	}
	ret = qmi_txn_wait(&txn, msecs_to_jiffies(5000));
	if (ret >= 0 && resp->resp.result != 0)
		ret = -EREMOTEIO;
out:
	qmi_handle_release(&qmi);
	return ret < 0 ? ret : 0;
}

static const u32 dl_nat_eps[3] = { 5, 12, 15 };

struct ipa_rtk_header_ref {
	struct ipa_rtk_header header;
	u8 ethernet[ETH_HLEN];
	unsigned int refcount;
};

struct ipa_rtk_offload_slot {
	struct ipa_rtk_nat_tuple nat;
	unsigned int header_index;
	bool active;
};

struct ipa_rtk_controller {
	struct device *dev;
	struct platform_device *ipa_pdev;
	struct pci_dev *nic;
	struct net_device *netdev;
	struct ipa *ipa;
	struct iommu_domain *nic_domain;
	struct dma_iommu_mapping *previous_mapping;
	struct mutex lock;
	struct ipa_rtk_endpoints endpoints;
	struct ipa_rtk_dma dma[2];
	struct gsi_rtk gsi[2];
	void *uc_payload[2];
	dma_addr_t uc_address[2];
	bool uc_active[2];
	struct ipa_rtk_flow_manager flow_manager;
	struct ipa_rtk_flowtable flowtable;
	struct notifier_block netdev_nb;
	struct work_struct netdev_down_work;
	struct ipa_rtk_table ul[4], route, route_hash;
	struct ipa_rtk_header_ref headers[IPA_RTK_MAX_HEADERS];
	struct ipa_rtk_offload_slot flows[IPA_RTK_FLOW_LIMIT];
	struct h5_fltr_installed_notif_req_msg *notification;
	void *offload_buf;
	dma_addr_t offload_addr;
	int wan_ifindex;
	u32 qmi_generation;
	u32 original_mux;
	u32 original_nat[3];
	u32 installed_nat[3];
	unsigned int route_flip;
	u64 total_installs;
	u64 total_removes;
	bool mux_changed;
	bool nat_changed;
	bool notified;
	bool dl_qmi_installed;
	bool udp_rss_changed;
	bool in_stop;
	bool shared;
	bool pinned;
	bool failed;
	unsigned int stage;
};

int ipa_rtk_test_tx_copy(struct ipa *ipa, u32 index, void *data, size_t capacity)
{
	struct device *dev;
	struct ipa_rtk_controller *c;
	struct rtl8125_ring *ring;
	const __le32 *desc;
	u32 length;
	int ret = -ENODEV;

	dev = bus_find_device_by_name(&platform_bus_type, NULL, "soc:ipa-ethernet");
	if (!dev)
		return ret;
	device_lock(dev);
	c = dev_get_drvdata(dev);
	if (!c || c->ipa != ipa)
		goto unlock;
	mutex_lock(&c->lock);
	ring = c->dma[1].ring;
	if (c->stage < 2 || c->failed || !ring || !data || index >= ring->ring_size) {
		ret = -EAGAIN;
		goto state;
	}
	dma_rmb();
	desc = ring->desc_addr;
	length = le32_to_cpu(READ_ONCE(desc[index * 8])) & 0x3fff;
	if (!length || length > capacity || length > ring->buff_size) {
		ret = -EMSGSIZE;
		goto state;
	}
	memcpy(data, ring->bufs[0].addr + index * ring->buff_size, length);
	ret = length;
state:
	mutex_unlock(&c->lock);
unlock:
	device_unlock(dev);
	put_device(dev);
	return ret;
}
EXPORT_SYMBOL_GPL(ipa_rtk_test_tx_copy);

int ipa_rtk_test_nat_direction(struct ipa *ipa, bool destination)
{
	struct device *dev;
	struct ipa_rtk_controller *c;
	const struct reg *reg;
	int ret = -ENODEV;

	dev = bus_find_device_by_name(&platform_bus_type, NULL, "soc:ipa-ethernet");
	if (!dev)
		return ret;
	device_lock(dev);
	c = dev_get_drvdata(dev);
	if (!c || c->ipa != ipa)
		goto unlock;
	mutex_lock(&c->lock);
	if (c->stage != 2 || c->failed) {
		ret = -EAGAIN;
	} else {
		/* The controller owns EP8 and restores its original registers at
		 * stage0. This diagnostic never changes an unowned live WAN pipe.
		 */
		reg = ipa->regs->reg[ENDP_INIT_NAT];
		iowrite32(reg_encode(reg, NAT_EN,
			destination ? IPA_NAT_TYPE_DST : IPA_NAT_TYPE_SRC),
			ipa->reg_virt + reg_n_offset(reg, 8));
		ret = 0;
	}
	mutex_unlock(&c->lock);
unlock:
	device_unlock(dev);
	put_device(dev);
	return ret;
}
EXPORT_SYMBOL_GPL(ipa_rtk_test_nat_direction);

int ipa_rtk_test_rx_pause(struct ipa *ipa)
{
	struct device *dev;
	struct ipa_rtk_controller *c;
	int ret = -ENODEV;
	dev = bus_find_device_by_name(&platform_bus_type, NULL, "soc:ipa-ethernet");
	if (!dev)
		return ret;
	device_lock(dev);
	c = dev_get_drvdata(dev);
	if (!c || c->ipa != ipa)
		goto unlock;
	mutex_lock(&c->lock);
	rtnl_lock();
	if (c->failed)
		ret = -ESHUTDOWN;
	else if (c->stage < 2)
		ret = -EAGAIN;
	else if (c->stage == 2)
		ret = 0;
	else {
		ret = rtl8125_rss_reset(c->netdev);
		if (!ret)
			c->stage = 2;
	}
	rtnl_unlock();
	mutex_unlock(&c->lock);
unlock:
	device_unlock(dev);
	put_device(dev);
	return ret;
}
EXPORT_SYMBOL_GPL(ipa_rtk_test_rx_pause);

static int ipa_rtk_uc(struct ipa_rtk_controller *c, unsigned int dir, bool setup)
{
	struct ipa_uc_rtk_setup *payload = c->uc_payload[dir];
	u8 channel = dir == 0 ? 18 : 21;

	memset(payload, 0, sizeof(*payload));
	payload->protocol = cpu_to_le32(9);
	if (setup) {
		payload->direction = dir;
		payload->gsi_channel = channel;
	} else {
		/* Teardown channel is at byte4, unlike setup's channel at byte5. */
		payload->direction = channel;
	}
	return ipa_uc_rtk_command(c->ipa, c->uc_address[dir], setup);
}

static void ipa_rtk_set_udp_rss(struct ipa_rtk_controller *c, bool enable)
{
	struct ethtool_rxnfc rxnfc = {
		.cmd = ETHTOOL_SRXFH,
		.flow_type = UDP_V4_FLOW,
		.data = enable ?
			(RXH_IP_SRC | RXH_IP_DST | RXH_L4_B_0_1 | RXH_L4_B_2_3) :
			(RXH_IP_SRC | RXH_IP_DST),
	};

	if (c->netdev && c->netdev->ethtool_ops &&
	    c->netdev->ethtool_ops->set_rxnfc) {
		if (!c->netdev->ethtool_ops->set_rxnfc(c->netdev, &rxnfc))
			c->udp_rss_changed = enable;
	}
}

static int ipa_rtk_header_get(struct ipa_rtk_controller *c,
			      const u8 ethernet[ETH_HLEN], unsigned int *out_idx)
{
	unsigned int i, free_idx = IPA_RTK_MAX_HEADERS;
	int ret;

	for (i = 0; i < IPA_RTK_MAX_HEADERS; i++) {
		if (c->headers[i].refcount) {
			if (!memcmp(c->headers[i].ethernet, ethernet, ETH_HLEN)) {
				c->headers[i].refcount++;
				*out_idx = i;
				return 0;
			}
		} else if (free_idx == IPA_RTK_MAX_HEADERS) {
			free_idx = i;
		}
	}
	if (free_idx == IPA_RTK_MAX_HEADERS)
		return -ENOSPC;
	ret = ipa_header_rtk_acquire(c->ipa, ethernet, &c->headers[free_idx].header);
	if (ret)
		return ret;
	memcpy(c->headers[free_idx].ethernet, ethernet, ETH_HLEN);
	c->headers[free_idx].refcount = 1;
	*out_idx = free_idx;
	return 0;
}

static int ipa_rtk_header_put(struct ipa_rtk_controller *c, unsigned int idx)
{
	int ret;

	if (idx >= IPA_RTK_MAX_HEADERS || !c->headers[idx].refcount)
		return -EINVAL;
	if (--c->headers[idx].refcount > 0)
		return 0;
	ret = ipa_header_rtk_release(&c->headers[idx].header);
	if (ret) {
		c->headers[idx].refcount = 1;
		return ret;
	}
	memset(&c->headers[idx], 0, sizeof(c->headers[idx]));
	return 0;
}

static int ipa_rtk_rebuild_routes(struct ipa_rtk_controller *c)
{
	unsigned int off = 4096 + (c->route_flip ? 1024 : 0);
	unsigned int i, j, cursor = 0, prio = 0;
	int ret;

	memset(c->offload_buf + off, 0, 1024);
	for (i = 0; i < IPA_RTK_FLOW_LIMIT; i++) {
		struct ipa_rtk_equations eq = {};
		bool dup = false;
		int len;

		if (!c->flows[i].active)
			continue;
		for (j = 0; j < i; j++) {
			if (c->flows[j].active &&
			    c->flows[j].nat.private_ip == c->flows[i].nat.private_ip &&
			    c->flows[j].header_index == c->flows[i].header_index) {
				dup = true;
				break;
			}
		}
		if (dup)
			continue;
		if (cursor + 256 > 1024)
			return -ENOSPC;
		eq.bitmap = BIT(5);
		eq.meq32_count = 1;
		eq.meq32[0].offset = 16;
		eq.meq32[0].mask = U32_MAX;
		eq.meq32[0].value = be32_to_cpu(c->flows[i].nat.private_ip);
		len = ipa_rtk_route_encode(c->offload_buf + off + cursor, &eq, 26,
			c->headers[c->flows[i].header_index].header.route_offset,
			0, 0, prio, 711 + prio, 0, 0);
		if (len < 0)
			return len;
		cursor += len;
		prio++;
	}
	dma_wmb();
	ret = ipa_table_rtk_replace(&c->route, c->offload_addr + off);
	if (!ret)
		c->route_flip ^= 1;
	return ret;
}

static int ipa_rtk_reassert_ul_tables(struct ipa_rtk_controller *c)
{
	int ret;

	if (!c->ul[0].ipa)
		return 0;
	ret = ipa_table_rtk_replace(&c->ul[2], c->offload_addr + 3072);
	if (!ret)
		ret = ipa_table_rtk_replace(&c->ul[3], c->offload_addr + 3072);
	if (!ret)
		ret = ipa_table_rtk_replace(&c->ul[1], c->offload_addr + 2048);
	if (!ret)
		ret = ipa_table_rtk_replace(&c->ul[0], c->offload_addr);
	return ret;
}

static int ipa_rtk_sync_dl_qmi(struct ipa_rtk_controller *c)
{
	struct h5_dl_fltr_req_ex_msg *qreq;
	struct h5_dl_fltr_resp_ex_msg *qresp;
	unsigned int i, count = 0;
	int ret;

	if (!READ_ONCE(c->ipa->qmi.modem_ready) ||
	    !READ_ONCE(c->ipa->qmi.uc_ready) ||
	    READ_ONCE(c->ipa->qmi.initial_boot)) {
		c->dl_qmi_installed = false;
		return 0;
	}
	qreq = kzalloc(sizeof(*qreq), GFP_KERNEL);
	qresp = kzalloc(sizeof(*qresp), GFP_KERNEL);
	if (!qreq || !qresp) {
		kfree(qresp);
		kfree(qreq);
		return -ENOMEM;
	}
	for (i = 0; i < IPA_RTK_FLOW_LIMIT; i++) {
		const struct ipa_rtk_nat_tuple *t = &c->flows[i].nat;
		struct h5_filter_spec_ex_type *spec;

		if (!c->flows[i].active)
			continue;
		spec = &qreq->filter_spec_ex_list[count++];
		spec->ip_type = 1;
		spec->filter_action = 2; /* HW action=2 (DST_NAT) */
		spec->is_routing_table_index_valid = 1;
		spec->route_table_index = 11;
		spec->is_mux_id_valid = 1;
		spec->mux_id = 1;
		spec->rule_id = 0;
		spec->is_rule_hashable = 1;
		spec->filter_rule.rule_eq_bitmap =
			BIT(1) | BIT(5) | BIT(6) | BIT(10) | BIT(11);
		spec->filter_rule.protocol_eq_present = 1;
		spec->filter_rule.protocol_eq = t->protocol;
		spec->filter_rule.num_ihl_offset_range_16 = 2;
		spec->filter_rule.ihl_offset_range_16[0].offset = 0;
		spec->filter_rule.ihl_offset_range_16[0].range_low =
			be16_to_cpu(t->remote_port);
		spec->filter_rule.ihl_offset_range_16[0].range_high =
			be16_to_cpu(t->remote_port);
		spec->filter_rule.ihl_offset_range_16[1].offset = 2;
		spec->filter_rule.ihl_offset_range_16[1].range_low =
			be16_to_cpu(t->public_port);
		spec->filter_rule.ihl_offset_range_16[1].range_high =
			be16_to_cpu(t->public_port);
		spec->filter_rule.num_offset_meq_32 = 2;
		spec->filter_rule.offset_meq_32[0].offset = 12;
		spec->filter_rule.offset_meq_32[0].mask = U32_MAX;
		spec->filter_rule.offset_meq_32[0].value =
			be32_to_cpu(t->remote_ip);
		spec->filter_rule.offset_meq_32[1].offset = 16;
		spec->filter_rule.offset_meq_32[1].mask = U32_MAX;
		spec->filter_rule.offset_meq_32[1].value =
			be32_to_cpu(t->public_ip);
		if (t->protocol == IPPROTO_TCP) {
			spec->filter_rule.rule_eq_bitmap |= BIT(7);
			spec->filter_rule.num_ihl_offset_meq_32 = 1;
			spec->filter_rule.ihl_offset_meq_32[0].offset = 12;
			spec->filter_rule.ihl_offset_meq_32[0].mask = 0x00050000U;
			spec->filter_rule.ihl_offset_meq_32[0].value = 0;
		}
	}
	qreq->filter_spec_ex_list_valid = count > 0;
	qreq->filter_spec_ex_list_len = count;
	ret = send_dl_qmi(c->ipa, qreq, qresp);
	kfree(qresp);
	kfree(qreq);
	if (ret)
		return ret;
	c->dl_qmi_installed = count > 0;
	if (!(c->ipa->filtered & BIT_ULL(5)))
		return ipa_rtk_reassert_ul_tables(c);
	return 0;
}

static int ipa_rtk_flow_install(void *private, unsigned int index,
				const struct ipa_rtk_flow flow[2])
{
	struct ipa_rtk_controller *c = private;
	unsigned int hdr_idx;
	int ret;

	if (index >= IPA_RTK_FLOW_LIMIT || c->flows[index].active)
		return -EINVAL;
	ret = ipa_rtk_header_get(c, flow[1].ethernet, &hdr_idx);
	if (ret)
		return ret;
	c->flows[index].nat = flow[1].nat;
	c->flows[index].header_index = hdr_idx;
	c->flows[index].active = true;
	ret = ipa_rtk_rebuild_routes(c);
	if (!ret)
		ret = ipa_rtk_sync_dl_qmi(c);
	if (ret) {
		c->flows[index].active = false;
		ipa_rtk_rebuild_routes(c);
		ipa_rtk_header_put(c, hdr_idx);
		return ret;
	}
	c->total_installs++;
	return 0;
}

static int ipa_rtk_flow_remove(void *private, unsigned int index)
{
	struct ipa_rtk_controller *c = private;
	unsigned int hdr_idx;
	int ret, ret2;

	if (index >= IPA_RTK_FLOW_LIMIT || !c->flows[index].active)
		return 0;
	hdr_idx = c->flows[index].header_index;
	c->flows[index].active = false;
	ret = ipa_rtk_sync_dl_qmi(c);
	ret2 = ipa_rtk_rebuild_routes(c);
	if (!ret && ret2)
		ret = ret2;
	ret2 = ipa_rtk_header_put(c, hdr_idx);
	if (!ret && ret2)
		ret = ret2;
	if (!ret)
		c->total_removes++;
	return ret;
}

static int ipa_rtk_quiesce(struct ipa_rtk_controller *c)
{
	unsigned int dir;
	int ret;

	if (c->stage >= 3) {
		if (c->udp_rss_changed)
			ipa_rtk_set_udp_rss(c, false);
		ret = rtl8125_rss_reset(c->netdev);
		if (ret)
			return ret;
		c->stage = 2;
	}
	for (dir = 0; dir < 2; dir++) {
		if (!c->dma[dir].ring)
			continue;
		if (c->dma[dir].ring->event.enabled) {
			ret = rtl8125_disable_event(c->dma[dir].ring);
			if (ret)
				return ret;
		}
		if (c->dma[dir].ring->enabled)
			rtl8125_disable_ring(c->dma[dir].ring);
	}
	for (dir = 0; dir < 2; dir++) {
		ret = gsi_rtk_stop(&c->gsi[dir]);
		if (ret)
			return ret;
	}
	for (dir = 0; dir < 2; dir++) {
		if (!c->uc_active[dir])
			continue;
		ret = ipa_rtk_uc(c, dir, false);
		if (ret)
			return ret;
		c->uc_active[dir] = false;
	}
	/* Unknown uC completion requires retaining even a failed setup payload. */
	if (c->ipa->uc_command_failed)
		return -ESHUTDOWN;
	return 0;
}

static void ipa_rtk_flow_quiesce(void *private)
{
	struct ipa_rtk_controller *c = private;

	c->failed = true;
	if (!c->in_stop)
		schedule_work(&c->netdev_down_work);
	else
		ipa_rtk_quiesce(c);
}

static const struct ipa_rtk_flow_manager_ops ipa_rtk_controller_flow_ops = {
	.install = ipa_rtk_flow_install,
	.remove = ipa_rtk_flow_remove,
	.quiesce = ipa_rtk_flow_quiesce,
};

static int ipa_rtk_offload_stop(struct ipa_rtk_controller *c)
{
	unsigned int i;
	int ret;

	if (c->stage >= 3) {
		if (c->udp_rss_changed)
			ipa_rtk_set_udp_rss(c, false);
		ret = rtl8125_rss_reset(c->netdev);
		if (ret)
			return ret;
		c->stage = 2;
	}
	if (c->flow_manager.active) {
		c->in_stop = true;
		ret = ipa_rtk_flow_manager_stop(&c->flow_manager);
		c->in_stop = false;
		if (ret)
			return ret;
	}
	if (c->notified) {
		c->notification->rule_id_len = 0;
		c->notification->dst_pipe_id_len = 0;
		c->notification->num_ipv4_filters = 0;
		ret = ipa_qmi_rtk_rules_notify(c->ipa, c->notification,
					       c->qmi_generation);
		if (ret && ret != -ESTALE && ret != -ENODEV)
			return ret;
		c->notified = false;
	}
	for (i = 0; i < ARRAY_SIZE(c->ul); i++) {
		ret = ipa_table_rtk_release(&c->ul[i]);
		if (ret)
			return ret;
	}
	if (c->dl_qmi_installed) {
		struct h5_dl_fltr_req_ex_msg *qreq = kzalloc(sizeof(*qreq), GFP_KERNEL);
		struct h5_dl_fltr_resp_ex_msg *qresp = kzalloc(sizeof(*qresp), GFP_KERNEL);

		if (qreq && qresp)
			send_dl_qmi(c->ipa, qreq, qresp);
		kfree(qresp);
		kfree(qreq);
		c->dl_qmi_installed = false;
	}
	if (c->nat_changed) {
		const struct reg *reg = c->ipa->regs->reg[ENDP_INIT_NAT];

		for (i = 0; i < ARRAY_SIZE(dl_nat_eps); i++) {
			void __iomem *at = c->ipa->reg_virt +
					   reg_n_offset(reg, dl_nat_eps[i]);

			if (readl(at) == c->installed_nat[i])
				writel(c->original_nat[i], at);
		}
		c->nat_changed = false;
	}
	if (c->mux_changed) {
		writel(c->original_mux, c->ipa->reg_virt + H5_EP8_METADATA);
		c->mux_changed = false;
	}
	ret = ipa_table_rtk_release(&c->route);
	if (!ret)
		ret = ipa_table_rtk_release(&c->route_hash);
	if (ret)
		return ret;
	for (i = 0; i < IPA_RTK_MAX_HEADERS; i++) {
		if (c->headers[i].header.ipa) {
			ret = ipa_header_rtk_release(&c->headers[i].header);
			if (ret)
				return ret;
			memset(&c->headers[i], 0, sizeof(c->headers[i]));
		}
	}
	if (c->offload_buf) {
		dma_free_coherent(&c->ipa_pdev->dev, 8192,
				  c->offload_buf, c->offload_addr);
		c->offload_buf = NULL;
	}
	kfree(c->notification);
	c->notification = NULL;
	c->wan_ifindex = 0;
	return 0;
}

static int ipa_rtk_offload_start(struct ipa_rtk_controller *c)
{
	struct h5_install_fltr_rule_req_msg *rules = NULL;
	struct net_device *wan_dev, *master;
	struct ipa_rtk_equations eq = {};
	const struct reg *reg;
	__be32 wan_ip;
	int wan_ifindex, bridge_ifindex = 0;
	unsigned int i;
	int ret, length;

	if (c->stage != 2)
		return -EINVAL;
	wan_dev = __dev_get_by_name(&init_net, "qmapmux0.0");
	if (!wan_dev)
		wan_dev = __dev_get_by_name(&init_net, "rmnet_ipa0");
	if (!wan_dev)
		return -ENODEV;
	wan_ifindex = wan_dev->ifindex;
	wan_ip = inet_select_addr(wan_dev, 0, RT_SCOPE_UNIVERSE);
	if (!wan_ip)
		return -EADDRNOTAVAIL;
	c->wan_ifindex = wan_ifindex;
	rcu_read_lock();
	master = netdev_master_upper_dev_get_rcu(c->netdev);
	if (master)
		bridge_ifindex = master->ifindex;
	rcu_read_unlock();

	rules = kzalloc(sizeof(*rules), GFP_KERNEL);
	c->notification = kzalloc(sizeof(*c->notification), GFP_KERNEL);
	c->offload_buf = dma_alloc_coherent(&c->ipa_pdev->dev, 8192,
					    &c->offload_addr, GFP_KERNEL);
	if (!rules || !c->notification || !c->offload_buf) {
		ret = -ENOMEM;
		goto fail;
	}
	memset(c->offload_buf, 0, 8192);
	ret = ipa_qmi_rtk_rules_copy(c->ipa, rules, &c->qmi_generation);
	if (ret)
		goto fail;
	length = ipa_rtk_modem_ul_build(rules, c->offload_buf, 2048, 1,
					c->notification);
	if (length < 0) {
		ret = length;
		goto fail;
	}
	eq.bitmap = BIT(5);
	eq.meq32_count = 1;
	ret = ipa_rtk_filter_encode(c->offload_buf + length, &eq, 0, 3, 0, 255,
				    702, 0, 0, 0);
	if (ret < 0)
		goto fail;
	ret = ipa_rtk_filter_encode(c->offload_buf + 2048, &eq, 0, 3, 0, 255,
				    703, 0, 0, 0);
	if (ret < 0)
		goto fail;

	ret = ipa_table_rtk_acquire(c->ipa, false, false, 11, &c->route);
	if (!ret)
		ret = ipa_table_rtk_acquire_hashed(c->ipa, false, false, 11,
						   &c->route_hash);
	if (ret)
		goto fail;
	dma_wmb();
	ret = ipa_table_rtk_replace(&c->route_hash, c->offload_addr + 3072);
	if (!ret)
		ret = ipa_table_rtk_replace(&c->route, c->offload_addr + 3072);
	if (ret)
		goto fail;

	ret = ipa_rtk_flow_manager_start(&c->flow_manager, c->ipa, wan_ip, 0, 0,
					 c->netdev->ifindex, bridge_ifindex,
					 wan_ifindex,
					 &ipa_rtk_controller_flow_ops, c);
	if (ret)
		goto fail;

	c->original_mux = readl(c->ipa->reg_virt + H5_EP8_METADATA);
	writel(1U << 16, c->ipa->reg_virt + H5_EP8_METADATA);
	c->mux_changed = true;

	reg = c->ipa->regs->reg[ENDP_INIT_NAT];
	for (i = 0; i < ARRAY_SIZE(dl_nat_eps); i++) {
		u32 ep = dl_nat_eps[i];

		c->original_nat[i] = readl(c->ipa->reg_virt + reg_n_offset(reg, ep));
		c->installed_nat[i] = (c->original_nat[i] & ~reg_fmask(reg, NAT_EN)) |
				      reg_encode(reg, NAT_EN, IPA_NAT_TYPE_DST);
		writel(c->installed_nat[i], c->ipa->reg_virt + reg_n_offset(reg, ep));
	}
	c->nat_changed = true;

	ret = ipa_rtk_sync_dl_qmi(c);
	if (ret)
		goto fail;

	for (i = 0; i < ARRAY_SIZE(c->ul); i++) {
		ret = i < 2 ?
			ipa_table_rtk_acquire(c->ipa, true, i & 1, 8, &c->ul[i]) :
			ipa_table_rtk_acquire_hashed(c->ipa, true, i & 1, 8, &c->ul[i]);
		if (ret)
			goto fail;
	}
	dma_wmb();
	ret = ipa_rtk_reassert_ul_tables(c);
	if (ret)
		goto fail;
	c->notified = true;
	ret = ipa_qmi_rtk_rules_notify(c->ipa, c->notification, c->qmi_generation);
	if (ret)
		goto fail;

	ret = rtl8125_rss_redirect(c->netdev, 0, c->dma[0].ring);
	if (ret)
		goto fail;
	c->stage = 3;
	ipa_rtk_set_udp_rss(c, true);

	c->stage = 4;
	kfree(rules);
	dev_info(c->dev, "H5 hardware flowtable offload active: wan=%s(%d) ip=%pI4 lan=%s(%d) br=%d\n",
		 wan_dev->name, wan_ifindex, &wan_ip, c->netdev->name,
		 c->netdev->ifindex, bridge_ifindex);
	return 0;

fail:
	kfree(rules);
	ipa_rtk_offload_stop(c);
	return ret;
}

static int ipa_rtk_clear(struct ipa_rtk_controller *c)
{
	unsigned int dir;
	int ret;

	ret = ipa_rtk_offload_stop(c);
	if (ret)
		return ret;
	ret = ipa_rtk_quiesce(c);
	if (ret)
		return ret;
	for (dir = 0; dir < 2; dir++) {
		ret = gsi_rtk_release(&c->gsi[dir]);
		if (ret)
			return ret;
	}
	ipa_endpoint_rtk_restore(&c->endpoints);
	for (dir = 0; dir < 2; dir++) {
		ret = ipa_rtk_dma_release(&c->dma[dir], &c->gsi[dir]);
		if (ret)
			return ret;
		if (c->uc_payload[dir]) {
			dma_free_coherent(&c->ipa_pdev->dev,
					  sizeof(struct ipa_uc_rtk_setup),
					  c->uc_payload[dir], c->uc_address[dir]);
			c->uc_payload[dir] = NULL;
		}
	}
	c->stage = 0;
	if (c->pinned) {
		c->pinned = false;
		module_put(THIS_MODULE);
	}
	return 0;
}

static int ipa_rtk_prepare(struct ipa_rtk_controller *c)
{
	unsigned int dir;
	int ret;

	/* Keep all code and state resident if a hardware command cannot stop. */
	__module_get(THIS_MODULE);
	c->pinned = true;
	ret = ipa_endpoint_rtk_prepare(c->ipa, &c->endpoints);
	if (ret)
		return ret;
	for (dir = 0; dir < 2; dir++) {
		ret = ipa_rtk_dma_prepare(&c->dma[dir], &c->ipa_pdev->dev,
					   c->dev, c->netdev, dir == 0, 256, 2048);
		if (ret)
			return ret;
		ret = gsi_rtk_prepare(&c->ipa->gsi, &c->dma[dir].config, &c->gsi[dir]);
		if (ret)
			return ret;
		ret = ipa_rtk_dma_set_notification(&c->dma[dir], c->gsi[dir].msi_addr);
		if (ret)
			return ret;
		c->uc_payload[dir] = dma_alloc_coherent(&c->ipa_pdev->dev,
				sizeof(struct ipa_uc_rtk_setup), &c->uc_address[dir], GFP_KERNEL);
		if (!c->uc_payload[dir])
			return -ENOMEM;
	}
	ret = ipa_endpoint_rtk_attach_netdev(&c->endpoints, c->netdev);
	if (!ret)
		c->stage = 1;
	return ret;
}

static int ipa_rtk_start(struct ipa_rtk_controller *c)
{
	unsigned int dir;
	int ret;

	/* uc_loaded may describe the bootloader's SRAM before the modem has
	 * finished the first IPA handshake. Its initialization can replace uC
	 * state; external-channel commands must wait for the final handshake.
	 */
	if (!READ_ONCE(c->ipa->qmi.modem_ready) ||
	    !READ_ONCE(c->ipa->qmi.uc_ready) ||
	    READ_ONCE(c->ipa->qmi.initial_boot))
		return -EAGAIN;
	for (dir = 0; dir < 2; dir++) {
		ret = gsi_rtk_start(&c->gsi[dir]);
		if (ret)
			return ret;
		ret = ipa_rtk_uc(c, dir, true);
		if (ret)
			return ret;
		c->uc_active[dir] = true;
	}
	for (dir = 0; dir < 2; dir++) {
		ret = rtl8125_enable_ring(c->dma[dir].ring);
		if (ret)
			return ret;
	}
	for (dir = 0; dir < 2; dir++) {
		ret = rtl8125_enable_event(c->dma[dir].ring);
		if (ret)
			return ret;
	}
	c->stage = 2;
	return 0;
}

static ssize_t stage_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct ipa_rtk_controller *c = dev_get_drvdata(dev);
	struct ipa_rtk_rx_stats stats;
	ssize_t length;
	unsigned int dir;

	mutex_lock(&c->lock);
	ipa_endpoint_rtk_stats(&stats);
	length = sysfs_emit(buf, "%u failed=%u exception_packets=%llu bytes=%llu invalid=%llu\n",
			  READ_ONCE(c->stage), READ_ONCE(c->failed),
			  stats.packets, stats.bytes, stats.invalid);
	if (c->stage == 4) {
		unsigned int i, active_flows = 0;

		for (i = 0; i < IPA_RTK_FLOW_LIMIT; i++)
			if (READ_ONCE(c->flows[i].active))
				active_flows++;
		length += sysfs_emit_at(buf, length,
			"offload active_flows=%u installs=%llu removes=%llu wan_ip=%pI4\n",
			active_flows, READ_ONCE(c->total_installs),
			READ_ONCE(c->total_removes), &c->flow_manager.pool.public_ip);
	}
	for (dir = 0; dir < 2; dir++) {
		void __iomem *context;
		u32 state, read, write, base;

		if (!c->gsi[dir].gsi)
			continue;
		/* IPA v5.0 AP channel context4/6 contain the hardware ring RP/WP. */
		context = c->ipa->gsi.virt + 0x14000 + c->gsi[dir].channel_id * 0x80;
		state = (ioread32(context) >> 20) & 0xf;
		read = ioread32(context + 0x10);
		write = ioread32(context + 0x18);
		base = lower_32_bits(c->dma[dir].config.ring_addr);
		length += sysfs_emit_at(buf, length,
			"%s channel=%u queue=%u enabled=%u state=%u rp_offset=%#x wp_offset=%#x\n",
			dir ? "tx" : "rx", c->gsi[dir].channel_id,
			c->dma[dir].config.queue, c->dma[dir].ring->enabled,
			state, read - base, write - base);
		if (dir == 1) {
			const __le32 *desc = c->dma[dir].ring->desc_addr;
			void __iomem *bar;
			unsigned int i;

			dma_rmb();
			for (i = 0; i < 16; i++)
				length += sysfs_emit_at(buf, length,
					"tx_desc%u opts=%#x/%#x address=%#x:%08x\n",
					i, le32_to_cpu(READ_ONCE(desc[i * 8])),
					le32_to_cpu(READ_ONCE(desc[i * 8 + 1])),
					le32_to_cpu(READ_ONCE(desc[i * 8 + 3])),
					le32_to_cpu(READ_ONCE(desc[i * 8 + 2])));
			bar = pci_iomap(c->nic, 2, 0);
			if (bar) {
				length += sysfs_emit_at(buf, length,
					"nic_tx1 tail=%u close=%u config=%#x\n",
					ioread16(bar + 0x2804), ioread16(bar + 0x2806),
					ioread32(bar + 0x40));
				pci_iounmap(c->nic, bar);
			}
		}
	}
	mutex_unlock(&c->lock);
	return length;
}

static ssize_t stage_store(struct device *dev, struct device_attribute *attr,
			   const char *buf, size_t count)
{
	struct ipa_rtk_controller *c = dev_get_drvdata(dev);
	unsigned int target;
	int ret = kstrtouint(buf, 0, &target);

	if (ret || target > 4)
		return -EINVAL;
	mutex_lock(&c->lock);
	rtnl_lock();
	if (c->failed) {
		ret = -ESHUTDOWN;
		goto out;
	}
	if (target == 0) {
		ret = ipa_rtk_clear(c);
		goto out;
	}
	if (target < c->stage) {
		ret = -EINVAL;
		goto out;
	}
	if (c->stage == 0)
		ret = ipa_rtk_prepare(c);
	if (!ret && target >= 2 && c->stage == 1)
		ret = ipa_rtk_start(c);
	if (!ret && target == 3 && c->stage == 2) {
		ret = rtl8125_rss_redirect(c->netdev, 0, c->dma[0].ring);
		if (!ret)
			c->stage = 3;
	}
	if (!ret && target == 4 && c->stage == 2)
		ret = ipa_rtk_offload_start(c);
	if (ret) {
		int cleanup = ipa_rtk_clear(c);

		if (cleanup) {
			c->failed = true;
			dev_err(dev, "RTK stop incomplete (%d); retaining DMA until reset\n", cleanup);
		}
	}
out:
	if (ret && c->ipa->uc_command_failed)
		c->failed = true;
	rtnl_unlock();
	mutex_unlock(&c->lock);
	return ret ? ret : count;
}
static DEVICE_ATTR_RW(stage);

static void ipa_rtk_netdev_down_work(struct work_struct *work)
{
	struct ipa_rtk_controller *c =
		container_of(work, struct ipa_rtk_controller, netdev_down_work);
	int ret;

	mutex_lock(&c->lock);
	rtnl_lock();
	if (c->stage > 0) {
		dev_info(c->dev, "Netdev/quiesce event; returning RTK offload to stage0\n");
		ret = c->failed ? ipa_rtk_quiesce(c) : ipa_rtk_clear(c);
		if (ret) {
			c->failed = true;
			dev_err(c->dev, "RTK stop incomplete on netdev event (%d); retaining DMA\n",
				ret);
		}
	}
	rtnl_unlock();
	mutex_unlock(&c->lock);
}

static int ipa_rtk_netdev_event(struct notifier_block *nb, unsigned long event,
				void *ptr)
{
	struct net_device *dev = netdev_notifier_info_to_dev(ptr);
	struct ipa_rtk_controller *c =
		container_of(nb, struct ipa_rtk_controller, netdev_nb);

	if (event != NETDEV_GOING_DOWN && event != NETDEV_DOWN &&
	    event != NETDEV_UNREGISTER && event != NETDEV_CHANGEMTU)
		return NOTIFY_DONE;
	if (dev == c->netdev ||
	    (READ_ONCE(c->wan_ifindex) && dev->ifindex == READ_ONCE(c->wan_ifindex)) ||
	    !strcmp(dev->name, "qmapmux0.0") || !strcmp(dev->name, "rmnet_ipa0"))
		schedule_work(&c->netdev_down_work);
	return NOTIFY_DONE;
}

static int ipa_rtk_probe(struct platform_device *pdev)
{
	struct device_node *node;
	struct ipa_rtk_controller *c;
	struct device_link *link;
	struct iommu_domain *eth_domain;
	struct dma_iommu_mapping *nic_mapping, *old_mapping;
	struct sg_table test_sgt = {};
	dma_addr_t test_dma;
	void *test_page;
	const char *phase = "find IPA supplier";
	int ret;

	if (!of_machine_is_compatible("greenpacket,h5-200q1"))
		return -ENODEV;
	c = kzalloc(sizeof(*c), GFP_KERNEL);
	if (!c)
		return -ENOMEM;
	c->dev = &pdev->dev;
	mutex_init(&c->lock);
	mutex_init(&c->flow_manager.lock);
	node = of_parse_phandle(pdev->dev.of_node, "qcom,ipa", 0);
	c->ipa_pdev = of_find_device_by_node(node);
	of_node_put(node);
	if (!c->ipa_pdev) {
		ret = -EPROBE_DEFER;
		goto err_free;
	}
	c->ipa = platform_get_drvdata(c->ipa_pdev);
	if (!c->ipa || !c->ipa->setup_complete) {
		ret = -EPROBE_DEFER;
		goto err_ipa;
	}
	c->nic = pci_get_device(PCI_VENDOR_ID_REALTEK, 0x8125, NULL);
	if (!c->nic || !c->nic->dev.driver ||
	    strcmp(c->nic->dev.driver->name, "r8125")) {
		ret = -EPROBE_DEFER;
		goto err_nic;
	}
	c->netdev = rtl8125_get_netdev(&c->nic->dev);
	if (!c->netdev) {
		ret = -EPROBE_DEFER;
		goto err_nic;
	}
	dev_hold(c->netdev);
	c->nic_domain = iommu_get_domain_for_dev(&c->nic->dev);
	if (!c->nic_domain) {
		ret = -ENODEV;
		goto err_netdev;
	}
	link = device_link_add(&pdev->dev, &c->ipa_pdev->dev, DL_FLAG_AUTOREMOVE_CONSUMER);
	if (!link) {
		ret = -ENOMEM;
		goto err_netdev;
	}
	link = device_link_add(&pdev->dev, &c->nic->dev, DL_FLAG_AUTOREMOVE_CONSUMER);
	if (!link) {
		ret = -ENOMEM;
		goto err_netdev;
	}
	phase = "hold IPA runtime power";
	ret = pm_runtime_resume_and_get(&c->ipa_pdev->dev);
	if (ret < 0)
		goto err_netdev;
	phase = "hold RTL8125 runtime power";
	ret = pm_runtime_resume_and_get(&c->nic->dev);
	if (ret < 0)
		goto err_ipa_power;
	phase = "attach ETH IOMMU domain";
	eth_domain = iommu_get_domain_for_dev(&pdev->dev);
	if (!eth_domain) {
		ret = -ENODEV;
		goto err_nic_power;
	}
	dev_info(&pdev->dev, "DMA translation sample: NIC=%#llx ETH=%#llx\n",
		 (u64)iommu_iova_to_phys(c->nic_domain, 0x80000000),
		 (u64)iommu_iova_to_phys(eth_domain, 0x80000000));
	/* Test a real owned allocation before attaching a consumer to its domain. */
	test_page = dma_alloc_coherent(&c->nic->dev, PAGE_SIZE, &test_dma, GFP_KERNEL);
	if (!test_page) {
		ret = -ENOMEM;
		goto err_nic_power;
	}
	ret = dma_get_sgtable(&c->nic->dev, &test_sgt, test_page, test_dma, PAGE_SIZE);
	if (!ret) {
		phys_addr_t physical = sg_phys(test_sgt.sgl);
		phys_addr_t mapped = iommu_iova_to_phys(c->nic_domain, test_dma);

		dev_info(&pdev->dev, "Owned DMA page: IOVA=%#llx physical=%#llx translated=%#llx\n",
			 (u64)test_dma, (u64)physical, (u64)mapped);
		if (physical != mapped)
			ret = -EXDEV;
		sg_free_table(&test_sgt);
	}
	dma_free_coherent(&c->nic->dev, PAGE_SIZE, test_page, test_dma);
	if (ret)
		goto err_nic_power;
	/* ARM32 owns an attached UNMANAGED domain in addition to the core default.
	 * Share its allocator and reference-counted mapping, not just the domain.
	 */
	if (eth_domain != c->nic_domain) {
		nic_mapping = to_dma_iommu_mapping(&c->nic->dev);
		old_mapping = to_dma_iommu_mapping(&pdev->dev);
		if (!nic_mapping || !old_mapping ||
		    nic_mapping->domain != c->nic_domain || old_mapping->domain != eth_domain) {
			ret = -EXDEV;
			goto err_nic_power;
		}
		kref_get(&old_mapping->kref);
		c->previous_mapping = old_mapping;
		arm_iommu_detach_device(&pdev->dev);
		ret = arm_iommu_attach_device(&pdev->dev, nic_mapping);
		if (ret) {
			int restore = arm_iommu_attach_device(&pdev->dev, old_mapping);

			if (restore)
				dev_err(&pdev->dev, "ETH DMA restore failed: %d\n", restore);
			arm_iommu_release_mapping(old_mapping);
			c->previous_mapping = NULL;
			goto err_nic_power;
		}
		c->shared = true;
	}
	platform_set_drvdata(pdev, c);
	INIT_WORK(&c->netdev_down_work, ipa_rtk_netdev_down_work);
	ret = ipa_rtk_flowtable_register(&c->flowtable, c->netdev,
					 &ipa_rtk_managed_flow_ops,
					 &c->flow_manager);
	if (ret)
		goto err_detach;
	c->netdev_nb.notifier_call = ipa_rtk_netdev_event;
	ret = register_netdevice_notifier(&c->netdev_nb);
	if (ret)
		goto err_flowtable;
	ret = device_create_file(&pdev->dev, &dev_attr_stage);
	if (ret) {
		unregister_netdevice_notifier(&c->netdev_nb);
		cancel_work_sync(&c->netdev_down_work);
		goto err_flowtable;
	}
	dev_info(&pdev->dev, "ETH SID0x5e3 shares RTL8125 DMA domain; passive stage0\n");
	return 0;

err_flowtable:
	ipa_rtk_flowtable_unregister(&c->flowtable);
err_detach:
	if (c->shared) {
		arm_iommu_detach_device(&pdev->dev);
		arm_iommu_attach_device(&pdev->dev, c->previous_mapping);
		arm_iommu_release_mapping(c->previous_mapping);
	}
err_nic_power:
	pm_runtime_put(&c->nic->dev);
err_ipa_power:
	pm_runtime_put(&c->ipa_pdev->dev);
err_netdev:
	dev_put(c->netdev);
err_nic:
	pci_dev_put(c->nic);
err_ipa:
	put_device(&c->ipa_pdev->dev);
err_free:
	kfree(c);
	return dev_err_probe(&pdev->dev, ret, "%s\n", phase);
}

static void ipa_rtk_remove(struct platform_device *pdev)
{
	struct ipa_rtk_controller *c = platform_get_drvdata(pdev);
	int ret;

	device_remove_file(&pdev->dev, &dev_attr_stage);
	unregister_netdevice_notifier(&c->netdev_nb);
	cancel_work_sync(&c->netdev_down_work);
	if (READ_ONCE(c->flowtable.registered))
		ipa_rtk_flowtable_unregister(&c->flowtable);
	mutex_lock(&c->lock);
	rtnl_lock();
	ret = ipa_rtk_clear(c);
	rtnl_unlock();
	mutex_unlock(&c->lock);
	if (ret) {
		dev_crit(&pdev->dev, "RTK retained after stop failure; hardware reset required\n");
		return;
	}
	if (c->shared) {
		arm_iommu_detach_device(&pdev->dev);
		ret = arm_iommu_attach_device(&pdev->dev, c->previous_mapping);
		if (ret)
			dev_err(&pdev->dev, "ETH DMA restore failed: %d\n", ret);
		arm_iommu_release_mapping(c->previous_mapping);
	}
	pm_runtime_put(&c->nic->dev);
	pm_runtime_put(&c->ipa_pdev->dev);
	dev_put(c->netdev);
	pci_dev_put(c->nic);
	put_device(&c->ipa_pdev->dev);
	kfree(c);
}

static const struct of_device_id ipa_rtk_match[] = {
	{ .compatible = "greenpacket,h5-ipa-ethernet" },
	{}
};
MODULE_DEVICE_TABLE(of, ipa_rtk_match);

static struct platform_driver ipa_rtk_driver = {
	.probe = ipa_rtk_probe,
	.remove = ipa_rtk_remove,
	.driver_managed_dma = true,
	.driver = {
		.name = "h5-ipa-rtk",
		.of_match_table = ipa_rtk_match,
		.suppress_bind_attrs = true,
	},
};
module_platform_driver(ipa_rtk_driver);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("H5 SDX65 IPA RTL8125 external DMA controller");
