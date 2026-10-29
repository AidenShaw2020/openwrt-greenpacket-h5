/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _IPA_RTK_MODEM_RULES_H_
#define _IPA_RTK_MODEM_RULES_H_
#include "ipa_qmi_filter_schema.h"
#include "ipa_rtk_rule_wire.h"

/* Translate decoded QMI fields; do not confuse the QMI C layout with HW wire.
 * IPA5 repurposes the legacy TOS equation bit as pure TCP ACK (value ignored).
 */
static inline int ipa_rtk_modem_equations(struct ipa_rtk_equations *eq,
					 const struct h5_filter_rule_type *qmi)
{
	unsigned int i;
	if (qmi->num_offset_meq_128 > 2 || qmi->num_offset_meq_32 > 2 ||
	    qmi->num_ihl_offset_meq_32 > 2 || qmi->num_ihl_offset_range_16 > 2)
		return -EINVAL;
	memset(eq, 0, sizeof(*eq));
	eq->bitmap = qmi->rule_eq_bitmap;
	eq->pure_ack = qmi->tos_eq_present;
	eq->protocol_present = qmi->protocol_eq_present;
	eq->protocol = qmi->protocol_eq;
	eq->tc_present = qmi->tc_eq_present;
	eq->tc = qmi->tc_eq;
	eq->meq128_count = qmi->num_offset_meq_128;
	eq->meq32_count = qmi->num_offset_meq_32;
	eq->ihl_meq32_count = qmi->num_ihl_offset_meq_32;
	eq->range16_count = qmi->num_ihl_offset_range_16;
	for (i = 0; i < eq->meq128_count; i++) {
		eq->meq128[i].offset = qmi->offset_meq_128[i].offset;
		memcpy(eq->meq128[i].mask, qmi->offset_meq_128[i].mask, 16);
		memcpy(eq->meq128[i].value, qmi->offset_meq_128[i].value, 16);
	}
	for (i = 0; i < eq->meq32_count; i++) {
		eq->meq32[i].offset = qmi->offset_meq_32[i].offset;
		eq->meq32[i].mask = qmi->offset_meq_32[i].mask;
		eq->meq32[i].value = qmi->offset_meq_32[i].value;
	}
	for (i = 0; i < eq->ihl_meq32_count; i++) {
		eq->ihl_meq32[i].offset = qmi->ihl_offset_meq_32[i].offset;
		eq->ihl_meq32[i].mask = qmi->ihl_offset_meq_32[i].mask;
		eq->ihl_meq32[i].value = qmi->ihl_offset_meq_32[i].value;
	}
	for (i = 0; i < eq->range16_count; i++) {
		eq->range16[i].offset = qmi->ihl_offset_range_16[i].offset;
		eq->range16[i].low = qmi->ihl_offset_range_16[i].range_low;
		eq->range16[i].high = qmi->ihl_offset_range_16[i].range_high;
	}
	eq->metadata_present = qmi->metadata_meq32_present;
	eq->metadata.offset = qmi->metadata_meq32.offset;
	eq->metadata.mask = qmi->metadata_meq32.mask;
	eq->metadata.value = qmi->metadata_meq32.value;
	eq->ihl_eq32_present = qmi->ihl_offset_eq_32_present;
	eq->ihl_eq32_offset = qmi->ihl_offset_eq_32.offset;
	eq->ihl_eq32 = qmi->ihl_offset_eq_32.value;
	eq->ihl_eq16_present = qmi->ihl_offset_eq_16_present;
	eq->ihl_eq16_offset = qmi->ihl_offset_eq_16.offset;
	eq->ihl_eq16 = qmi->ihl_offset_eq_16.value;
	eq->flow_present = qmi->flow_eq_present;
	eq->flow = qmi->flow_eq;
	eq->fragment = qmi->ipv4_frag_eq_present;
	return 0;
}

/* Build IPv4 stock-style SRC_NAT UL policy, but never activate a NAT entry.
 * Empty/missing hardware NAT tuples must fall back to the Linux firewall.
 * Caller adds CPU fallback after the returned byte count, then publishes and
 * ONLY THEN sends this notification. IPv6 is deliberately not authorized here.
 */
static inline int ipa_rtk_modem_ul_build(const struct h5_install_fltr_rule_req_msg *rules,
		u8 *buffer, size_t capacity, u8 mux,
		struct h5_fltr_installed_notif_req_msg *notification)
{
	unsigned int i, cursor = 0, count = 0;
	if (!rules || !buffer || !notification ||
	    !rules->filter_spec_ex_list_valid || rules->filter_spec_ex_list_len > 64 ||
	    rules->filter_spec_ex2_list_valid || !mux)
		return -EOPNOTSUPP;
	memset(notification, 0, sizeof(*notification));
	/* Prepend TCP FIN and RST exception rules (priorities 1..2) so TCP
	 * teardown packets always go to AP_LAN_RX (EP16) for nf_flowtable
	 * teardown instead of hitting SRC_NAT.
	 */
	for (i = 0; i < 2; i++) {
		struct ipa_rtk_equations fin_rst = {};
		int length;

		if (cursor > capacity || capacity - cursor < 256)
			return -ENOSPC;
		fin_rst.bitmap = BIT(1) | BIT(7);
		fin_rst.protocol_present = 1;
		fin_rst.protocol = 6; /* IPPROTO_TCP */
		fin_rst.ihl_meq32_count = 1;
		fin_rst.ihl_meq32[0].offset = 12;
		fin_rst.ihl_meq32[0].mask = i == 0 ? 0x00010000U : 0x00040000U;
		fin_rst.ihl_meq32[0].value = fin_rst.ihl_meq32[0].mask;
		length = ipa_rtk_filter_encode(buffer + cursor, &fin_rst,
			0, 3 /* EXCEPTION */, 0, 1 + i, 704 + i, 0, 0, 0);
		if (length < 0)
			return length;
		cursor += length;
	}
	for (i = 0; i < rules->filter_spec_ex_list_len; i++) {
		const struct h5_filter_spec_ex_type *r = &rules->filter_spec_ex_list[i];
		struct ipa_rtk_equations eq;
		int length, ret;
		if (r->ip_type != 0)
			continue;
		if (!r->is_routing_table_index_valid || r->route_table_index > 10 ||
		    r->rule_id >= 1023 || (r->is_mux_id_valid && r->mux_id != mux))
			return -EOPNOTSUPP;
		if (cursor > capacity || capacity - cursor < 256)
			return -ENOSPC;
		ret = ipa_rtk_modem_equations(&eq, &r->filter_rule);
		if (ret)
			return ret;
		length = ipa_rtk_filter_encode(buffer + cursor, &eq,
			r->route_table_index, 1 /* SRC_NAT */, 0, 16 + count,
			r->rule_id, 0, 0, 0);
		if (length < 0)
			return length;
		cursor += length;
		notification->rule_id[count++] = r->rule_id;
	}
	if (!count)
		return -ENODATA;
	notification->source_pipe_index = 8;
	notification->install_status = 0; /* QMI_RESULT_SUCCESS */
	notification->rule_id_valid = 1;
	notification->rule_id_len = count;
	notification->embedded_pipe_index_valid = 1;
	notification->embedded_pipe_index = 2; /* AP_LAN_WAN_PROD */
	notification->retain_header_valid = 1;
	notification->retain_header = 0;
	notification->embedded_call_mux_id_valid = 1;
	notification->embedded_call_mux_id = mux;
	return cursor;
}
#endif
