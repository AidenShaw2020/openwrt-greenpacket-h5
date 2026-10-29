/* SPDX-License-Identifier: GPL-2.0-only */
/* Generated from Qualcomm QMI wire descriptions and verified stock H5 offsets. */
#ifndef _H5_IPA_QMI_FILTER_SCHEMA_H_
#define _H5_IPA_QMI_FILTER_SCHEMA_H_
#include <linux/build_bug.h>
#include <linux/soc/qcom/qmi.h>

struct h5_ipfltr_range_eq_16_type {
	u8 offset;
	u8 reserved_1[1];
	u16 range_low;
	u16 range_high;
} __packed;
static_assert(sizeof(struct h5_ipfltr_range_eq_16_type) == 6);
static_assert(offsetof(struct h5_ipfltr_range_eq_16_type, offset) == 0);
static_assert(offsetof(struct h5_ipfltr_range_eq_16_type, range_low) == 2);
static_assert(offsetof(struct h5_ipfltr_range_eq_16_type, range_high) == 4);
#ifdef H5_IPA_QMI_SCHEMA_DEFINE
static const struct qmi_elem_info h5_ipfltr_range_eq_16_type_ei[] = {
	{
		.data_type = QMI_UNSIGNED_1_BYTE,
		.elem_len = 1,
		.elem_size = 1,
		.array_type = 0,
		.tlv_type = 0,
		.offset = offsetof(struct h5_ipfltr_range_eq_16_type, offset),
	},
	{
		.data_type = QMI_UNSIGNED_2_BYTE,
		.elem_len = 1,
		.elem_size = 2,
		.array_type = 0,
		.tlv_type = 0,
		.offset = offsetof(struct h5_ipfltr_range_eq_16_type, range_low),
	},
	{
		.data_type = QMI_UNSIGNED_2_BYTE,
		.elem_len = 1,
		.elem_size = 2,
		.array_type = 0,
		.tlv_type = 0,
		.offset = offsetof(struct h5_ipfltr_range_eq_16_type, range_high),
	},
	{ .data_type = QMI_EOTI },
};
#endif

struct h5_ipfltr_mask_eq_32_type {
	u8 offset;
	u8 reserved_1[3];
	u32 mask;
	u32 value;
} __packed;
static_assert(sizeof(struct h5_ipfltr_mask_eq_32_type) == 12);
static_assert(offsetof(struct h5_ipfltr_mask_eq_32_type, offset) == 0);
static_assert(offsetof(struct h5_ipfltr_mask_eq_32_type, mask) == 4);
static_assert(offsetof(struct h5_ipfltr_mask_eq_32_type, value) == 8);
#ifdef H5_IPA_QMI_SCHEMA_DEFINE
static const struct qmi_elem_info h5_ipfltr_mask_eq_32_type_ei[] = {
	{
		.data_type = QMI_UNSIGNED_1_BYTE,
		.elem_len = 1,
		.elem_size = 1,
		.array_type = 0,
		.tlv_type = 0,
		.offset = offsetof(struct h5_ipfltr_mask_eq_32_type, offset),
	},
	{
		.data_type = QMI_UNSIGNED_4_BYTE,
		.elem_len = 1,
		.elem_size = 4,
		.array_type = 0,
		.tlv_type = 0,
		.offset = offsetof(struct h5_ipfltr_mask_eq_32_type, mask),
	},
	{
		.data_type = QMI_UNSIGNED_4_BYTE,
		.elem_len = 1,
		.elem_size = 4,
		.array_type = 0,
		.tlv_type = 0,
		.offset = offsetof(struct h5_ipfltr_mask_eq_32_type, value),
	},
	{ .data_type = QMI_EOTI },
};
#endif

struct h5_ipfltr_eq_16_type {
	u8 offset;
	u8 reserved_1[1];
	u16 value;
} __packed;
static_assert(sizeof(struct h5_ipfltr_eq_16_type) == 4);
static_assert(offsetof(struct h5_ipfltr_eq_16_type, offset) == 0);
static_assert(offsetof(struct h5_ipfltr_eq_16_type, value) == 2);
#ifdef H5_IPA_QMI_SCHEMA_DEFINE
static const struct qmi_elem_info h5_ipfltr_eq_16_type_ei[] = {
	{
		.data_type = QMI_UNSIGNED_1_BYTE,
		.elem_len = 1,
		.elem_size = 1,
		.array_type = 0,
		.tlv_type = 0,
		.offset = offsetof(struct h5_ipfltr_eq_16_type, offset),
	},
	{
		.data_type = QMI_UNSIGNED_2_BYTE,
		.elem_len = 1,
		.elem_size = 2,
		.array_type = 0,
		.tlv_type = 0,
		.offset = offsetof(struct h5_ipfltr_eq_16_type, value),
	},
	{ .data_type = QMI_EOTI },
};
#endif

struct h5_ipfltr_eq_32_type {
	u8 offset;
	u8 reserved_1[3];
	u32 value;
} __packed;
static_assert(sizeof(struct h5_ipfltr_eq_32_type) == 8);
static_assert(offsetof(struct h5_ipfltr_eq_32_type, offset) == 0);
static_assert(offsetof(struct h5_ipfltr_eq_32_type, value) == 4);
#ifdef H5_IPA_QMI_SCHEMA_DEFINE
static const struct qmi_elem_info h5_ipfltr_eq_32_type_ei[] = {
	{
		.data_type = QMI_UNSIGNED_1_BYTE,
		.elem_len = 1,
		.elem_size = 1,
		.array_type = 0,
		.tlv_type = 0,
		.offset = offsetof(struct h5_ipfltr_eq_32_type, offset),
	},
	{
		.data_type = QMI_UNSIGNED_4_BYTE,
		.elem_len = 1,
		.elem_size = 4,
		.array_type = 0,
		.tlv_type = 0,
		.offset = offsetof(struct h5_ipfltr_eq_32_type, value),
	},
	{ .data_type = QMI_EOTI },
};
#endif

struct h5_ipfltr_mask_eq_128_type {
	u8 offset;
	u8 mask[16];
	u8 value[16];
} __packed;
static_assert(sizeof(struct h5_ipfltr_mask_eq_128_type) == 33);
static_assert(offsetof(struct h5_ipfltr_mask_eq_128_type, offset) == 0);
static_assert(offsetof(struct h5_ipfltr_mask_eq_128_type, mask) == 1);
static_assert(offsetof(struct h5_ipfltr_mask_eq_128_type, value) == 17);
#ifdef H5_IPA_QMI_SCHEMA_DEFINE
static const struct qmi_elem_info h5_ipfltr_mask_eq_128_type_ei[] = {
	{
		.data_type = QMI_UNSIGNED_1_BYTE,
		.elem_len = 1,
		.elem_size = 1,
		.array_type = 0,
		.tlv_type = 0,
		.offset = offsetof(struct h5_ipfltr_mask_eq_128_type, offset),
	},
	{
		.data_type = QMI_UNSIGNED_1_BYTE,
		.elem_len = 16,
		.elem_size = 1,
		.array_type = 1,
		.tlv_type = 0,
		.offset = offsetof(struct h5_ipfltr_mask_eq_128_type, mask),
	},
	{
		.data_type = QMI_UNSIGNED_1_BYTE,
		.elem_len = 16,
		.elem_size = 1,
		.array_type = 1,
		.tlv_type = 0,
		.offset = offsetof(struct h5_ipfltr_mask_eq_128_type, value),
	},
	{ .data_type = QMI_EOTI },
};
#endif

struct h5_filter_rule_type {
	u16 rule_eq_bitmap;
	u8 tos_eq_present;
	u8 tos_eq;
	u8 protocol_eq_present;
	u8 protocol_eq;
	u8 num_ihl_offset_range_16;
	u8 reserved_6[1];
	struct h5_ipfltr_range_eq_16_type ihl_offset_range_16[2];
	u8 num_offset_meq_32;
	u8 reserved_8[3];
	struct h5_ipfltr_mask_eq_32_type offset_meq_32[2];
	u8 tc_eq_present;
	u8 tc_eq;
	u8 flow_eq_present;
	u8 reserved_12[1];
	u32 flow_eq;
	u8 ihl_offset_eq_16_present;
	u8 reserved_14[1];
	struct h5_ipfltr_eq_16_type ihl_offset_eq_16;
	u8 ihl_offset_eq_32_present;
	u8 reserved_16[1];
	struct h5_ipfltr_eq_32_type ihl_offset_eq_32;
	u8 num_ihl_offset_meq_32;
	u8 reserved_18[3];
	struct h5_ipfltr_mask_eq_32_type ihl_offset_meq_32[2];
	u8 num_offset_meq_128;
	struct h5_ipfltr_mask_eq_128_type offset_meq_128[2];
	u8 metadata_meq32_present;
	struct h5_ipfltr_mask_eq_32_type metadata_meq32;
	u8 ipv4_frag_eq_present;
	u8 reserved_tail[3];
} __packed;
static_assert(sizeof(struct h5_filter_rule_type) == 184);
static_assert(offsetof(struct h5_filter_rule_type, rule_eq_bitmap) == 0);
static_assert(offsetof(struct h5_filter_rule_type, tos_eq_present) == 2);
static_assert(offsetof(struct h5_filter_rule_type, tos_eq) == 3);
static_assert(offsetof(struct h5_filter_rule_type, protocol_eq_present) == 4);
static_assert(offsetof(struct h5_filter_rule_type, protocol_eq) == 5);
static_assert(offsetof(struct h5_filter_rule_type, num_ihl_offset_range_16) == 6);
static_assert(offsetof(struct h5_filter_rule_type, ihl_offset_range_16) == 8);
static_assert(offsetof(struct h5_filter_rule_type, num_offset_meq_32) == 20);
static_assert(offsetof(struct h5_filter_rule_type, offset_meq_32) == 24);
static_assert(offsetof(struct h5_filter_rule_type, tc_eq_present) == 48);
static_assert(offsetof(struct h5_filter_rule_type, tc_eq) == 49);
static_assert(offsetof(struct h5_filter_rule_type, flow_eq_present) == 50);
static_assert(offsetof(struct h5_filter_rule_type, flow_eq) == 52);
static_assert(offsetof(struct h5_filter_rule_type, ihl_offset_eq_16_present) == 56);
static_assert(offsetof(struct h5_filter_rule_type, ihl_offset_eq_16) == 58);
static_assert(offsetof(struct h5_filter_rule_type, ihl_offset_eq_32_present) == 62);
static_assert(offsetof(struct h5_filter_rule_type, ihl_offset_eq_32) == 64);
static_assert(offsetof(struct h5_filter_rule_type, num_ihl_offset_meq_32) == 72);
static_assert(offsetof(struct h5_filter_rule_type, ihl_offset_meq_32) == 76);
static_assert(offsetof(struct h5_filter_rule_type, num_offset_meq_128) == 100);
static_assert(offsetof(struct h5_filter_rule_type, offset_meq_128) == 101);
static_assert(offsetof(struct h5_filter_rule_type, metadata_meq32_present) == 167);
static_assert(offsetof(struct h5_filter_rule_type, metadata_meq32) == 168);
static_assert(offsetof(struct h5_filter_rule_type, ipv4_frag_eq_present) == 180);
#ifdef H5_IPA_QMI_SCHEMA_DEFINE
static const struct qmi_elem_info h5_filter_rule_type_ei[] = {
	{
		.data_type = QMI_UNSIGNED_2_BYTE,
		.elem_len = 1,
		.elem_size = 2,
		.array_type = 0,
		.tlv_type = 0,
		.offset = offsetof(struct h5_filter_rule_type, rule_eq_bitmap),
	},
	{
		.data_type = QMI_UNSIGNED_1_BYTE,
		.elem_len = 1,
		.elem_size = 1,
		.array_type = 0,
		.tlv_type = 0,
		.offset = offsetof(struct h5_filter_rule_type, tos_eq_present),
	},
	{
		.data_type = QMI_UNSIGNED_1_BYTE,
		.elem_len = 1,
		.elem_size = 1,
		.array_type = 0,
		.tlv_type = 0,
		.offset = offsetof(struct h5_filter_rule_type, tos_eq),
	},
	{
		.data_type = QMI_UNSIGNED_1_BYTE,
		.elem_len = 1,
		.elem_size = 1,
		.array_type = 0,
		.tlv_type = 0,
		.offset = offsetof(struct h5_filter_rule_type, protocol_eq_present),
	},
	{
		.data_type = QMI_UNSIGNED_1_BYTE,
		.elem_len = 1,
		.elem_size = 1,
		.array_type = 0,
		.tlv_type = 0,
		.offset = offsetof(struct h5_filter_rule_type, protocol_eq),
	},
	{
		.data_type = QMI_UNSIGNED_1_BYTE,
		.elem_len = 1,
		.elem_size = 1,
		.array_type = 0,
		.tlv_type = 0,
		.offset = offsetof(struct h5_filter_rule_type, num_ihl_offset_range_16),
	},
	{
		.data_type = QMI_STRUCT,
		.elem_len = 2,
		.elem_size = 6,
		.array_type = 1,
		.tlv_type = 0,
		.offset = offsetof(struct h5_filter_rule_type, ihl_offset_range_16),
		.ei_array = h5_ipfltr_range_eq_16_type_ei,
	},
	{
		.data_type = QMI_UNSIGNED_1_BYTE,
		.elem_len = 1,
		.elem_size = 1,
		.array_type = 0,
		.tlv_type = 0,
		.offset = offsetof(struct h5_filter_rule_type, num_offset_meq_32),
	},
	{
		.data_type = QMI_STRUCT,
		.elem_len = 2,
		.elem_size = 12,
		.array_type = 1,
		.tlv_type = 0,
		.offset = offsetof(struct h5_filter_rule_type, offset_meq_32),
		.ei_array = h5_ipfltr_mask_eq_32_type_ei,
	},
	{
		.data_type = QMI_UNSIGNED_1_BYTE,
		.elem_len = 1,
		.elem_size = 1,
		.array_type = 0,
		.tlv_type = 0,
		.offset = offsetof(struct h5_filter_rule_type, tc_eq_present),
	},
	{
		.data_type = QMI_UNSIGNED_1_BYTE,
		.elem_len = 1,
		.elem_size = 1,
		.array_type = 0,
		.tlv_type = 0,
		.offset = offsetof(struct h5_filter_rule_type, tc_eq),
	},
	{
		.data_type = QMI_UNSIGNED_1_BYTE,
		.elem_len = 1,
		.elem_size = 1,
		.array_type = 0,
		.tlv_type = 0,
		.offset = offsetof(struct h5_filter_rule_type, flow_eq_present),
	},
	{
		.data_type = QMI_UNSIGNED_4_BYTE,
		.elem_len = 1,
		.elem_size = 4,
		.array_type = 0,
		.tlv_type = 0,
		.offset = offsetof(struct h5_filter_rule_type, flow_eq),
	},
	{
		.data_type = QMI_UNSIGNED_1_BYTE,
		.elem_len = 1,
		.elem_size = 1,
		.array_type = 0,
		.tlv_type = 0,
		.offset = offsetof(struct h5_filter_rule_type, ihl_offset_eq_16_present),
	},
	{
		.data_type = QMI_STRUCT,
		.elem_len = 1,
		.elem_size = 4,
		.array_type = 0,
		.tlv_type = 0,
		.offset = offsetof(struct h5_filter_rule_type, ihl_offset_eq_16),
		.ei_array = h5_ipfltr_eq_16_type_ei,
	},
	{
		.data_type = QMI_UNSIGNED_1_BYTE,
		.elem_len = 1,
		.elem_size = 1,
		.array_type = 0,
		.tlv_type = 0,
		.offset = offsetof(struct h5_filter_rule_type, ihl_offset_eq_32_present),
	},
	{
		.data_type = QMI_STRUCT,
		.elem_len = 1,
		.elem_size = 8,
		.array_type = 0,
		.tlv_type = 0,
		.offset = offsetof(struct h5_filter_rule_type, ihl_offset_eq_32),
		.ei_array = h5_ipfltr_eq_32_type_ei,
	},
	{
		.data_type = QMI_UNSIGNED_1_BYTE,
		.elem_len = 1,
		.elem_size = 1,
		.array_type = 0,
		.tlv_type = 0,
		.offset = offsetof(struct h5_filter_rule_type, num_ihl_offset_meq_32),
	},
	{
		.data_type = QMI_STRUCT,
		.elem_len = 2,
		.elem_size = 12,
		.array_type = 1,
		.tlv_type = 0,
		.offset = offsetof(struct h5_filter_rule_type, ihl_offset_meq_32),
		.ei_array = h5_ipfltr_mask_eq_32_type_ei,
	},
	{
		.data_type = QMI_UNSIGNED_1_BYTE,
		.elem_len = 1,
		.elem_size = 1,
		.array_type = 0,
		.tlv_type = 0,
		.offset = offsetof(struct h5_filter_rule_type, num_offset_meq_128),
	},
	{
		.data_type = QMI_STRUCT,
		.elem_len = 2,
		.elem_size = 33,
		.array_type = 1,
		.tlv_type = 0,
		.offset = offsetof(struct h5_filter_rule_type, offset_meq_128),
		.ei_array = h5_ipfltr_mask_eq_128_type_ei,
	},
	{
		.data_type = QMI_UNSIGNED_1_BYTE,
		.elem_len = 1,
		.elem_size = 1,
		.array_type = 0,
		.tlv_type = 0,
		.offset = offsetof(struct h5_filter_rule_type, metadata_meq32_present),
	},
	{
		.data_type = QMI_STRUCT,
		.elem_len = 1,
		.elem_size = 12,
		.array_type = 0,
		.tlv_type = 0,
		.offset = offsetof(struct h5_filter_rule_type, metadata_meq32),
		.ei_array = h5_ipfltr_mask_eq_32_type_ei,
	},
	{
		.data_type = QMI_UNSIGNED_1_BYTE,
		.elem_len = 1,
		.elem_size = 1,
		.array_type = 0,
		.tlv_type = 0,
		.offset = offsetof(struct h5_filter_rule_type, ipv4_frag_eq_present),
	},
	{ .data_type = QMI_EOTI },
};
#endif

struct h5_filter_spec_type {
	u32 filter_spec_identifier;
	u32 ip_type;
	struct h5_filter_rule_type filter_rule;
	u32 filter_action;
	u8 is_routing_table_index_valid;
	u8 reserved_5[3];
	u32 route_table_index;
	u8 is_mux_id_valid;
	u8 reserved_7[3];
	u32 mux_id;
} __packed;
static_assert(sizeof(struct h5_filter_spec_type) == 212);
static_assert(offsetof(struct h5_filter_spec_type, filter_spec_identifier) == 0);
static_assert(offsetof(struct h5_filter_spec_type, ip_type) == 4);
static_assert(offsetof(struct h5_filter_spec_type, filter_rule) == 8);
static_assert(offsetof(struct h5_filter_spec_type, filter_action) == 192);
static_assert(offsetof(struct h5_filter_spec_type, is_routing_table_index_valid) == 196);
static_assert(offsetof(struct h5_filter_spec_type, route_table_index) == 200);
static_assert(offsetof(struct h5_filter_spec_type, is_mux_id_valid) == 204);
static_assert(offsetof(struct h5_filter_spec_type, mux_id) == 208);
#ifdef H5_IPA_QMI_SCHEMA_DEFINE
static const struct qmi_elem_info h5_filter_spec_type_ei[] = {
	{
		.data_type = QMI_UNSIGNED_4_BYTE,
		.elem_len = 1,
		.elem_size = 4,
		.array_type = 0,
		.tlv_type = 0,
		.offset = offsetof(struct h5_filter_spec_type, filter_spec_identifier),
	},
	{
		.data_type = QMI_SIGNED_4_BYTE_ENUM,
		.elem_len = 1,
		.elem_size = 4,
		.array_type = 0,
		.tlv_type = 0,
		.offset = offsetof(struct h5_filter_spec_type, ip_type),
	},
	{
		.data_type = QMI_STRUCT,
		.elem_len = 1,
		.elem_size = 184,
		.array_type = 0,
		.tlv_type = 0,
		.offset = offsetof(struct h5_filter_spec_type, filter_rule),
		.ei_array = h5_filter_rule_type_ei,
	},
	{
		.data_type = QMI_SIGNED_4_BYTE_ENUM,
		.elem_len = 1,
		.elem_size = 4,
		.array_type = 0,
		.tlv_type = 0,
		.offset = offsetof(struct h5_filter_spec_type, filter_action),
	},
	{
		.data_type = QMI_UNSIGNED_1_BYTE,
		.elem_len = 1,
		.elem_size = 1,
		.array_type = 0,
		.tlv_type = 0,
		.offset = offsetof(struct h5_filter_spec_type, is_routing_table_index_valid),
	},
	{
		.data_type = QMI_UNSIGNED_4_BYTE,
		.elem_len = 1,
		.elem_size = 4,
		.array_type = 0,
		.tlv_type = 0,
		.offset = offsetof(struct h5_filter_spec_type, route_table_index),
	},
	{
		.data_type = QMI_UNSIGNED_1_BYTE,
		.elem_len = 1,
		.elem_size = 1,
		.array_type = 0,
		.tlv_type = 0,
		.offset = offsetof(struct h5_filter_spec_type, is_mux_id_valid),
	},
	{
		.data_type = QMI_UNSIGNED_4_BYTE,
		.elem_len = 1,
		.elem_size = 4,
		.array_type = 0,
		.tlv_type = 0,
		.offset = offsetof(struct h5_filter_spec_type, mux_id),
	},
	{ .data_type = QMI_EOTI },
};
#endif

struct h5_filter_spec_ex_type {
	u32 ip_type;
	struct h5_filter_rule_type filter_rule;
	u32 filter_action;
	u8 is_routing_table_index_valid;
	u8 reserved_4[3];
	u32 route_table_index;
	u8 is_mux_id_valid;
	u8 reserved_6[3];
	u32 mux_id;
	u32 rule_id;
	u8 is_rule_hashable;
	u8 reserved_tail[3];
} __packed;
static_assert(sizeof(struct h5_filter_spec_ex_type) == 216);
static_assert(offsetof(struct h5_filter_spec_ex_type, ip_type) == 0);
static_assert(offsetof(struct h5_filter_spec_ex_type, filter_rule) == 4);
static_assert(offsetof(struct h5_filter_spec_ex_type, filter_action) == 188);
static_assert(offsetof(struct h5_filter_spec_ex_type, is_routing_table_index_valid) == 192);
static_assert(offsetof(struct h5_filter_spec_ex_type, route_table_index) == 196);
static_assert(offsetof(struct h5_filter_spec_ex_type, is_mux_id_valid) == 200);
static_assert(offsetof(struct h5_filter_spec_ex_type, mux_id) == 204);
static_assert(offsetof(struct h5_filter_spec_ex_type, rule_id) == 208);
static_assert(offsetof(struct h5_filter_spec_ex_type, is_rule_hashable) == 212);
#ifdef H5_IPA_QMI_SCHEMA_DEFINE
static const struct qmi_elem_info h5_filter_spec_ex_type_ei[] = {
	{
		.data_type = QMI_SIGNED_4_BYTE_ENUM,
		.elem_len = 1,
		.elem_size = 4,
		.array_type = 0,
		.tlv_type = 0,
		.offset = offsetof(struct h5_filter_spec_ex_type, ip_type),
	},
	{
		.data_type = QMI_STRUCT,
		.elem_len = 1,
		.elem_size = 184,
		.array_type = 0,
		.tlv_type = 0,
		.offset = offsetof(struct h5_filter_spec_ex_type, filter_rule),
		.ei_array = h5_filter_rule_type_ei,
	},
	{
		.data_type = QMI_SIGNED_4_BYTE_ENUM,
		.elem_len = 1,
		.elem_size = 4,
		.array_type = 0,
		.tlv_type = 0,
		.offset = offsetof(struct h5_filter_spec_ex_type, filter_action),
	},
	{
		.data_type = QMI_UNSIGNED_1_BYTE,
		.elem_len = 1,
		.elem_size = 1,
		.array_type = 0,
		.tlv_type = 0,
		.offset = offsetof(struct h5_filter_spec_ex_type, is_routing_table_index_valid),
	},
	{
		.data_type = QMI_UNSIGNED_4_BYTE,
		.elem_len = 1,
		.elem_size = 4,
		.array_type = 0,
		.tlv_type = 0,
		.offset = offsetof(struct h5_filter_spec_ex_type, route_table_index),
	},
	{
		.data_type = QMI_UNSIGNED_1_BYTE,
		.elem_len = 1,
		.elem_size = 1,
		.array_type = 0,
		.tlv_type = 0,
		.offset = offsetof(struct h5_filter_spec_ex_type, is_mux_id_valid),
	},
	{
		.data_type = QMI_UNSIGNED_4_BYTE,
		.elem_len = 1,
		.elem_size = 4,
		.array_type = 0,
		.tlv_type = 0,
		.offset = offsetof(struct h5_filter_spec_ex_type, mux_id),
	},
	{
		.data_type = QMI_UNSIGNED_4_BYTE,
		.elem_len = 1,
		.elem_size = 4,
		.array_type = 0,
		.tlv_type = 0,
		.offset = offsetof(struct h5_filter_spec_ex_type, rule_id),
	},
	{
		.data_type = QMI_UNSIGNED_1_BYTE,
		.elem_len = 1,
		.elem_size = 1,
		.array_type = 0,
		.tlv_type = 0,
		.offset = offsetof(struct h5_filter_spec_ex_type, is_rule_hashable),
	},
	{ .data_type = QMI_EOTI },
};
#endif

struct h5_filter_rule_req2_type {
	u16 rule_eq_bitmap;
	u8 pure_ack_eq_present;
	u8 pure_ack_eq;
	u8 protocol_eq_present;
	u8 protocol_eq;
	u8 num_ihl_offset_range_16;
	u8 reserved_6[1];
	struct h5_ipfltr_range_eq_16_type ihl_offset_range_16[2];
	u8 num_offset_meq_32;
	u8 reserved_8[3];
	struct h5_ipfltr_mask_eq_32_type offset_meq_32[2];
	u8 tc_eq_present;
	u8 tc_eq;
	u8 flow_eq_present;
	u8 reserved_12[1];
	u32 flow_eq;
	u8 ihl_offset_eq_16_present;
	u8 reserved_14[1];
	struct h5_ipfltr_eq_16_type ihl_offset_eq_16;
	u8 ihl_offset_eq_32_present;
	u8 reserved_16[1];
	struct h5_ipfltr_eq_32_type ihl_offset_eq_32;
	u8 num_ihl_offset_meq_32;
	u8 reserved_18[3];
	struct h5_ipfltr_mask_eq_32_type ihl_offset_meq_32[2];
	u8 num_offset_meq_128;
	struct h5_ipfltr_mask_eq_128_type offset_meq_128[2];
	u8 metadata_meq32_present;
	struct h5_ipfltr_mask_eq_32_type metadata_meq32;
	u8 ipv4_frag_eq_present;
	u8 reserved_tail[3];
} __packed;
static_assert(sizeof(struct h5_filter_rule_req2_type) == 184);
static_assert(offsetof(struct h5_filter_rule_req2_type, rule_eq_bitmap) == 0);
static_assert(offsetof(struct h5_filter_rule_req2_type, pure_ack_eq_present) == 2);
static_assert(offsetof(struct h5_filter_rule_req2_type, pure_ack_eq) == 3);
static_assert(offsetof(struct h5_filter_rule_req2_type, protocol_eq_present) == 4);
static_assert(offsetof(struct h5_filter_rule_req2_type, protocol_eq) == 5);
static_assert(offsetof(struct h5_filter_rule_req2_type, num_ihl_offset_range_16) == 6);
static_assert(offsetof(struct h5_filter_rule_req2_type, ihl_offset_range_16) == 8);
static_assert(offsetof(struct h5_filter_rule_req2_type, num_offset_meq_32) == 20);
static_assert(offsetof(struct h5_filter_rule_req2_type, offset_meq_32) == 24);
static_assert(offsetof(struct h5_filter_rule_req2_type, tc_eq_present) == 48);
static_assert(offsetof(struct h5_filter_rule_req2_type, tc_eq) == 49);
static_assert(offsetof(struct h5_filter_rule_req2_type, flow_eq_present) == 50);
static_assert(offsetof(struct h5_filter_rule_req2_type, flow_eq) == 52);
static_assert(offsetof(struct h5_filter_rule_req2_type, ihl_offset_eq_16_present) == 56);
static_assert(offsetof(struct h5_filter_rule_req2_type, ihl_offset_eq_16) == 58);
static_assert(offsetof(struct h5_filter_rule_req2_type, ihl_offset_eq_32_present) == 62);
static_assert(offsetof(struct h5_filter_rule_req2_type, ihl_offset_eq_32) == 64);
static_assert(offsetof(struct h5_filter_rule_req2_type, num_ihl_offset_meq_32) == 72);
static_assert(offsetof(struct h5_filter_rule_req2_type, ihl_offset_meq_32) == 76);
static_assert(offsetof(struct h5_filter_rule_req2_type, num_offset_meq_128) == 100);
static_assert(offsetof(struct h5_filter_rule_req2_type, offset_meq_128) == 101);
static_assert(offsetof(struct h5_filter_rule_req2_type, metadata_meq32_present) == 167);
static_assert(offsetof(struct h5_filter_rule_req2_type, metadata_meq32) == 168);
static_assert(offsetof(struct h5_filter_rule_req2_type, ipv4_frag_eq_present) == 180);
#ifdef H5_IPA_QMI_SCHEMA_DEFINE
static const struct qmi_elem_info h5_filter_rule_req2_type_ei[] = {
	{
		.data_type = QMI_UNSIGNED_2_BYTE,
		.elem_len = 1,
		.elem_size = 2,
		.array_type = 0,
		.tlv_type = 0,
		.offset = offsetof(struct h5_filter_rule_req2_type, rule_eq_bitmap),
	},
	{
		.data_type = QMI_UNSIGNED_1_BYTE,
		.elem_len = 1,
		.elem_size = 1,
		.array_type = 0,
		.tlv_type = 0,
		.offset = offsetof(struct h5_filter_rule_req2_type, pure_ack_eq_present),
	},
	{
		.data_type = QMI_UNSIGNED_1_BYTE,
		.elem_len = 1,
		.elem_size = 1,
		.array_type = 0,
		.tlv_type = 0,
		.offset = offsetof(struct h5_filter_rule_req2_type, pure_ack_eq),
	},
	{
		.data_type = QMI_UNSIGNED_1_BYTE,
		.elem_len = 1,
		.elem_size = 1,
		.array_type = 0,
		.tlv_type = 0,
		.offset = offsetof(struct h5_filter_rule_req2_type, protocol_eq_present),
	},
	{
		.data_type = QMI_UNSIGNED_1_BYTE,
		.elem_len = 1,
		.elem_size = 1,
		.array_type = 0,
		.tlv_type = 0,
		.offset = offsetof(struct h5_filter_rule_req2_type, protocol_eq),
	},
	{
		.data_type = QMI_UNSIGNED_1_BYTE,
		.elem_len = 1,
		.elem_size = 1,
		.array_type = 0,
		.tlv_type = 0,
		.offset = offsetof(struct h5_filter_rule_req2_type, num_ihl_offset_range_16),
	},
	{
		.data_type = QMI_STRUCT,
		.elem_len = 2,
		.elem_size = 6,
		.array_type = 1,
		.tlv_type = 0,
		.offset = offsetof(struct h5_filter_rule_req2_type, ihl_offset_range_16),
		.ei_array = h5_ipfltr_range_eq_16_type_ei,
	},
	{
		.data_type = QMI_UNSIGNED_1_BYTE,
		.elem_len = 1,
		.elem_size = 1,
		.array_type = 0,
		.tlv_type = 0,
		.offset = offsetof(struct h5_filter_rule_req2_type, num_offset_meq_32),
	},
	{
		.data_type = QMI_STRUCT,
		.elem_len = 2,
		.elem_size = 12,
		.array_type = 1,
		.tlv_type = 0,
		.offset = offsetof(struct h5_filter_rule_req2_type, offset_meq_32),
		.ei_array = h5_ipfltr_mask_eq_32_type_ei,
	},
	{
		.data_type = QMI_UNSIGNED_1_BYTE,
		.elem_len = 1,
		.elem_size = 1,
		.array_type = 0,
		.tlv_type = 0,
		.offset = offsetof(struct h5_filter_rule_req2_type, tc_eq_present),
	},
	{
		.data_type = QMI_UNSIGNED_1_BYTE,
		.elem_len = 1,
		.elem_size = 1,
		.array_type = 0,
		.tlv_type = 0,
		.offset = offsetof(struct h5_filter_rule_req2_type, tc_eq),
	},
	{
		.data_type = QMI_UNSIGNED_1_BYTE,
		.elem_len = 1,
		.elem_size = 1,
		.array_type = 0,
		.tlv_type = 0,
		.offset = offsetof(struct h5_filter_rule_req2_type, flow_eq_present),
	},
	{
		.data_type = QMI_UNSIGNED_4_BYTE,
		.elem_len = 1,
		.elem_size = 4,
		.array_type = 0,
		.tlv_type = 0,
		.offset = offsetof(struct h5_filter_rule_req2_type, flow_eq),
	},
	{
		.data_type = QMI_UNSIGNED_1_BYTE,
		.elem_len = 1,
		.elem_size = 1,
		.array_type = 0,
		.tlv_type = 0,
		.offset = offsetof(struct h5_filter_rule_req2_type, ihl_offset_eq_16_present),
	},
	{
		.data_type = QMI_STRUCT,
		.elem_len = 1,
		.elem_size = 4,
		.array_type = 0,
		.tlv_type = 0,
		.offset = offsetof(struct h5_filter_rule_req2_type, ihl_offset_eq_16),
		.ei_array = h5_ipfltr_eq_16_type_ei,
	},
	{
		.data_type = QMI_UNSIGNED_1_BYTE,
		.elem_len = 1,
		.elem_size = 1,
		.array_type = 0,
		.tlv_type = 0,
		.offset = offsetof(struct h5_filter_rule_req2_type, ihl_offset_eq_32_present),
	},
	{
		.data_type = QMI_STRUCT,
		.elem_len = 1,
		.elem_size = 8,
		.array_type = 0,
		.tlv_type = 0,
		.offset = offsetof(struct h5_filter_rule_req2_type, ihl_offset_eq_32),
		.ei_array = h5_ipfltr_eq_32_type_ei,
	},
	{
		.data_type = QMI_UNSIGNED_1_BYTE,
		.elem_len = 1,
		.elem_size = 1,
		.array_type = 0,
		.tlv_type = 0,
		.offset = offsetof(struct h5_filter_rule_req2_type, num_ihl_offset_meq_32),
	},
	{
		.data_type = QMI_STRUCT,
		.elem_len = 2,
		.elem_size = 12,
		.array_type = 1,
		.tlv_type = 0,
		.offset = offsetof(struct h5_filter_rule_req2_type, ihl_offset_meq_32),
		.ei_array = h5_ipfltr_mask_eq_32_type_ei,
	},
	{
		.data_type = QMI_UNSIGNED_1_BYTE,
		.elem_len = 1,
		.elem_size = 1,
		.array_type = 0,
		.tlv_type = 0,
		.offset = offsetof(struct h5_filter_rule_req2_type, num_offset_meq_128),
	},
	{
		.data_type = QMI_STRUCT,
		.elem_len = 2,
		.elem_size = 33,
		.array_type = 1,
		.tlv_type = 0,
		.offset = offsetof(struct h5_filter_rule_req2_type, offset_meq_128),
		.ei_array = h5_ipfltr_mask_eq_128_type_ei,
	},
	{
		.data_type = QMI_UNSIGNED_1_BYTE,
		.elem_len = 1,
		.elem_size = 1,
		.array_type = 0,
		.tlv_type = 0,
		.offset = offsetof(struct h5_filter_rule_req2_type, metadata_meq32_present),
	},
	{
		.data_type = QMI_STRUCT,
		.elem_len = 1,
		.elem_size = 12,
		.array_type = 0,
		.tlv_type = 0,
		.offset = offsetof(struct h5_filter_rule_req2_type, metadata_meq32),
		.ei_array = h5_ipfltr_mask_eq_32_type_ei,
	},
	{
		.data_type = QMI_UNSIGNED_1_BYTE,
		.elem_len = 1,
		.elem_size = 1,
		.array_type = 0,
		.tlv_type = 0,
		.offset = offsetof(struct h5_filter_rule_req2_type, ipv4_frag_eq_present),
	},
	{ .data_type = QMI_EOTI },
};
#endif

struct h5_filter_spec_ex2_type {
	u32 ip_type;
	struct h5_filter_rule_req2_type filter_rule;
	u32 filter_action;
	u8 is_routing_table_index_valid;
	u8 reserved_4[3];
	u32 route_table_index;
	u8 is_mux_id_valid;
	u8 reserved_6[3];
	u32 mux_id;
	u32 rule_id;
	u8 is_rule_hashable;
	u8 reserved_tail[3];
} __packed;
static_assert(sizeof(struct h5_filter_spec_ex2_type) == 216);
static_assert(offsetof(struct h5_filter_spec_ex2_type, ip_type) == 0);
static_assert(offsetof(struct h5_filter_spec_ex2_type, filter_rule) == 4);
static_assert(offsetof(struct h5_filter_spec_ex2_type, filter_action) == 188);
static_assert(offsetof(struct h5_filter_spec_ex2_type, is_routing_table_index_valid) == 192);
static_assert(offsetof(struct h5_filter_spec_ex2_type, route_table_index) == 196);
static_assert(offsetof(struct h5_filter_spec_ex2_type, is_mux_id_valid) == 200);
static_assert(offsetof(struct h5_filter_spec_ex2_type, mux_id) == 204);
static_assert(offsetof(struct h5_filter_spec_ex2_type, rule_id) == 208);
static_assert(offsetof(struct h5_filter_spec_ex2_type, is_rule_hashable) == 212);
#ifdef H5_IPA_QMI_SCHEMA_DEFINE
static const struct qmi_elem_info h5_filter_spec_ex2_type_ei[] = {
	{
		.data_type = QMI_SIGNED_4_BYTE_ENUM,
		.elem_len = 1,
		.elem_size = 4,
		.array_type = 0,
		.tlv_type = 0,
		.offset = offsetof(struct h5_filter_spec_ex2_type, ip_type),
	},
	{
		.data_type = QMI_STRUCT,
		.elem_len = 1,
		.elem_size = 184,
		.array_type = 0,
		.tlv_type = 0,
		.offset = offsetof(struct h5_filter_spec_ex2_type, filter_rule),
		.ei_array = h5_filter_rule_req2_type_ei,
	},
	{
		.data_type = QMI_SIGNED_4_BYTE_ENUM,
		.elem_len = 1,
		.elem_size = 4,
		.array_type = 0,
		.tlv_type = 0,
		.offset = offsetof(struct h5_filter_spec_ex2_type, filter_action),
	},
	{
		.data_type = QMI_UNSIGNED_1_BYTE,
		.elem_len = 1,
		.elem_size = 1,
		.array_type = 0,
		.tlv_type = 0,
		.offset = offsetof(struct h5_filter_spec_ex2_type, is_routing_table_index_valid),
	},
	{
		.data_type = QMI_UNSIGNED_4_BYTE,
		.elem_len = 1,
		.elem_size = 4,
		.array_type = 0,
		.tlv_type = 0,
		.offset = offsetof(struct h5_filter_spec_ex2_type, route_table_index),
	},
	{
		.data_type = QMI_UNSIGNED_1_BYTE,
		.elem_len = 1,
		.elem_size = 1,
		.array_type = 0,
		.tlv_type = 0,
		.offset = offsetof(struct h5_filter_spec_ex2_type, is_mux_id_valid),
	},
	{
		.data_type = QMI_UNSIGNED_4_BYTE,
		.elem_len = 1,
		.elem_size = 4,
		.array_type = 0,
		.tlv_type = 0,
		.offset = offsetof(struct h5_filter_spec_ex2_type, mux_id),
	},
	{
		.data_type = QMI_UNSIGNED_4_BYTE,
		.elem_len = 1,
		.elem_size = 4,
		.array_type = 0,
		.tlv_type = 0,
		.offset = offsetof(struct h5_filter_spec_ex2_type, rule_id),
	},
	{
		.data_type = QMI_UNSIGNED_1_BYTE,
		.elem_len = 1,
		.elem_size = 1,
		.array_type = 0,
		.tlv_type = 0,
		.offset = offsetof(struct h5_filter_spec_ex2_type, is_rule_hashable),
	},
	{ .data_type = QMI_EOTI },
};
#endif

struct h5_install_fltr_rule_req_msg {
	u8 filter_spec_list_valid;
	u8 reserved_1[3];
	u32 filter_spec_list_len;
	struct h5_filter_spec_type filter_spec_list[64];
	u8 source_pipe_index_valid;
	u8 reserved_4[3];
	u32 source_pipe_index;
	u8 num_ipv4_filters_valid;
	u8 reserved_6[3];
	u32 num_ipv4_filters;
	u8 num_ipv6_filters_valid;
	u8 reserved_8[3];
	u32 num_ipv6_filters;
	u8 xlat_filter_indices_list_valid;
	u8 reserved_10[3];
	u32 xlat_filter_indices_list_len;
	u32 xlat_filter_indices_list[64];
	u8 filter_spec_ex_list_valid;
	u8 reserved_13[3];
	u32 filter_spec_ex_list_len;
	struct h5_filter_spec_ex_type filter_spec_ex_list[64];
	u8 filter_spec_ex2_list_valid;
	u8 reserved_16[3];
	u32 filter_spec_ex2_list_len;
	struct h5_filter_spec_ex2_type filter_spec_ex2_list[64];
	u8 ul_firewall_indices_list_valid;
	u8 reserved_19[3];
	u32 ul_firewall_indices_list_len;
	u32 ul_firewall_indices_list[64];
} __packed;
static_assert(sizeof(struct h5_install_fltr_rule_req_msg) == 41792);
static_assert(offsetof(struct h5_install_fltr_rule_req_msg, filter_spec_list_valid) == 0);
static_assert(offsetof(struct h5_install_fltr_rule_req_msg, filter_spec_list_len) == 4);
static_assert(offsetof(struct h5_install_fltr_rule_req_msg, filter_spec_list) == 8);
static_assert(offsetof(struct h5_install_fltr_rule_req_msg, source_pipe_index_valid) == 13576);
static_assert(offsetof(struct h5_install_fltr_rule_req_msg, source_pipe_index) == 13580);
static_assert(offsetof(struct h5_install_fltr_rule_req_msg, num_ipv4_filters_valid) == 13584);
static_assert(offsetof(struct h5_install_fltr_rule_req_msg, num_ipv4_filters) == 13588);
static_assert(offsetof(struct h5_install_fltr_rule_req_msg, num_ipv6_filters_valid) == 13592);
static_assert(offsetof(struct h5_install_fltr_rule_req_msg, num_ipv6_filters) == 13596);
static_assert(offsetof(struct h5_install_fltr_rule_req_msg, xlat_filter_indices_list_valid) == 13600);
static_assert(offsetof(struct h5_install_fltr_rule_req_msg, xlat_filter_indices_list_len) == 13604);
static_assert(offsetof(struct h5_install_fltr_rule_req_msg, xlat_filter_indices_list) == 13608);
static_assert(offsetof(struct h5_install_fltr_rule_req_msg, filter_spec_ex_list_valid) == 13864);
static_assert(offsetof(struct h5_install_fltr_rule_req_msg, filter_spec_ex_list_len) == 13868);
static_assert(offsetof(struct h5_install_fltr_rule_req_msg, filter_spec_ex_list) == 13872);
static_assert(offsetof(struct h5_install_fltr_rule_req_msg, filter_spec_ex2_list_valid) == 27696);
static_assert(offsetof(struct h5_install_fltr_rule_req_msg, filter_spec_ex2_list_len) == 27700);
static_assert(offsetof(struct h5_install_fltr_rule_req_msg, filter_spec_ex2_list) == 27704);
static_assert(offsetof(struct h5_install_fltr_rule_req_msg, ul_firewall_indices_list_valid) == 41528);
static_assert(offsetof(struct h5_install_fltr_rule_req_msg, ul_firewall_indices_list_len) == 41532);
static_assert(offsetof(struct h5_install_fltr_rule_req_msg, ul_firewall_indices_list) == 41536);
#ifdef H5_IPA_QMI_SCHEMA_DEFINE
static const struct qmi_elem_info h5_install_fltr_rule_req_msg_ei[] = {
	{
		.data_type = QMI_OPT_FLAG,
		.elem_len = 1,
		.elem_size = 1,
		.array_type = 0,
		.tlv_type = 16,
		.offset = offsetof(struct h5_install_fltr_rule_req_msg, filter_spec_list_valid),
	},
	{
		.data_type = QMI_DATA_LEN,
		.elem_len = 1,
		.elem_size = 1,
		.array_type = 0,
		.tlv_type = 16,
		.offset = offsetof(struct h5_install_fltr_rule_req_msg, filter_spec_list_len),
	},
	{
		.data_type = QMI_STRUCT,
		.elem_len = 64,
		.elem_size = 212,
		.array_type = 2,
		.tlv_type = 16,
		.offset = offsetof(struct h5_install_fltr_rule_req_msg, filter_spec_list),
		.ei_array = h5_filter_spec_type_ei,
	},
	{
		.data_type = QMI_OPT_FLAG,
		.elem_len = 1,
		.elem_size = 1,
		.array_type = 0,
		.tlv_type = 17,
		.offset = offsetof(struct h5_install_fltr_rule_req_msg, source_pipe_index_valid),
	},
	{
		.data_type = QMI_UNSIGNED_4_BYTE,
		.elem_len = 1,
		.elem_size = 4,
		.array_type = 0,
		.tlv_type = 17,
		.offset = offsetof(struct h5_install_fltr_rule_req_msg, source_pipe_index),
	},
	{
		.data_type = QMI_OPT_FLAG,
		.elem_len = 1,
		.elem_size = 1,
		.array_type = 0,
		.tlv_type = 18,
		.offset = offsetof(struct h5_install_fltr_rule_req_msg, num_ipv4_filters_valid),
	},
	{
		.data_type = QMI_UNSIGNED_4_BYTE,
		.elem_len = 1,
		.elem_size = 4,
		.array_type = 0,
		.tlv_type = 18,
		.offset = offsetof(struct h5_install_fltr_rule_req_msg, num_ipv4_filters),
	},
	{
		.data_type = QMI_OPT_FLAG,
		.elem_len = 1,
		.elem_size = 1,
		.array_type = 0,
		.tlv_type = 19,
		.offset = offsetof(struct h5_install_fltr_rule_req_msg, num_ipv6_filters_valid),
	},
	{
		.data_type = QMI_UNSIGNED_4_BYTE,
		.elem_len = 1,
		.elem_size = 4,
		.array_type = 0,
		.tlv_type = 19,
		.offset = offsetof(struct h5_install_fltr_rule_req_msg, num_ipv6_filters),
	},
	{
		.data_type = QMI_OPT_FLAG,
		.elem_len = 1,
		.elem_size = 1,
		.array_type = 0,
		.tlv_type = 20,
		.offset = offsetof(struct h5_install_fltr_rule_req_msg, xlat_filter_indices_list_valid),
	},
	{
		.data_type = QMI_DATA_LEN,
		.elem_len = 1,
		.elem_size = 1,
		.array_type = 0,
		.tlv_type = 20,
		.offset = offsetof(struct h5_install_fltr_rule_req_msg, xlat_filter_indices_list_len),
	},
	{
		.data_type = QMI_UNSIGNED_4_BYTE,
		.elem_len = 64,
		.elem_size = 4,
		.array_type = 2,
		.tlv_type = 20,
		.offset = offsetof(struct h5_install_fltr_rule_req_msg, xlat_filter_indices_list),
	},
	{
		.data_type = QMI_OPT_FLAG,
		.elem_len = 1,
		.elem_size = 1,
		.array_type = 0,
		.tlv_type = 21,
		.offset = offsetof(struct h5_install_fltr_rule_req_msg, filter_spec_ex_list_valid),
	},
	{
		.data_type = QMI_DATA_LEN,
		.elem_len = 1,
		.elem_size = 1,
		.array_type = 0,
		.tlv_type = 21,
		.offset = offsetof(struct h5_install_fltr_rule_req_msg, filter_spec_ex_list_len),
	},
	{
		.data_type = QMI_STRUCT,
		.elem_len = 64,
		.elem_size = 216,
		.array_type = 2,
		.tlv_type = 21,
		.offset = offsetof(struct h5_install_fltr_rule_req_msg, filter_spec_ex_list),
		.ei_array = h5_filter_spec_ex_type_ei,
	},
	{
		.data_type = QMI_OPT_FLAG,
		.elem_len = 1,
		.elem_size = 1,
		.array_type = 0,
		.tlv_type = 22,
		.offset = offsetof(struct h5_install_fltr_rule_req_msg, filter_spec_ex2_list_valid),
	},
	{
		.data_type = QMI_DATA_LEN,
		.elem_len = 1,
		.elem_size = 1,
		.array_type = 0,
		.tlv_type = 22,
		.offset = offsetof(struct h5_install_fltr_rule_req_msg, filter_spec_ex2_list_len),
	},
	{
		.data_type = QMI_STRUCT,
		.elem_len = 64,
		.elem_size = 216,
		.array_type = 2,
		.tlv_type = 22,
		.offset = offsetof(struct h5_install_fltr_rule_req_msg, filter_spec_ex2_list),
		.ei_array = h5_filter_spec_ex2_type_ei,
	},
	{
		.data_type = QMI_OPT_FLAG,
		.elem_len = 1,
		.elem_size = 1,
		.array_type = 0,
		.tlv_type = 23,
		.offset = offsetof(struct h5_install_fltr_rule_req_msg, ul_firewall_indices_list_valid),
	},
	{
		.data_type = QMI_DATA_LEN,
		.elem_len = 1,
		.elem_size = 1,
		.array_type = 0,
		.tlv_type = 23,
		.offset = offsetof(struct h5_install_fltr_rule_req_msg, ul_firewall_indices_list_len),
	},
	{
		.data_type = QMI_UNSIGNED_4_BYTE,
		.elem_len = 64,
		.elem_size = 4,
		.array_type = 2,
		.tlv_type = 23,
		.offset = offsetof(struct h5_install_fltr_rule_req_msg, ul_firewall_indices_list),
	},
	{ .data_type = QMI_EOTI },
};
#endif

#define H5_INSTALL_FLTR_RULE_REQ_MSG_MAX_LEN 33705
#define H5_INSTALL_FLTR_RULE_REQ_MSG_ID 35
struct h5_qmi_response_type {
	u16 result;
	u16 error;
} __packed;
static_assert(sizeof(struct h5_qmi_response_type) == 4);
static_assert(offsetof(struct h5_qmi_response_type, result) == 0);
static_assert(offsetof(struct h5_qmi_response_type, error) == 2);
#ifdef H5_IPA_QMI_SCHEMA_DEFINE
static const struct qmi_elem_info h5_qmi_response_type_ei[] = {
	{
		.data_type = QMI_SIGNED_2_BYTE_ENUM,
		.elem_len = 1,
		.elem_size = 2,
		.array_type = 0,
		.tlv_type = 0,
		.offset = offsetof(struct h5_qmi_response_type, result),
	},
	{
		.data_type = QMI_SIGNED_2_BYTE_ENUM,
		.elem_len = 1,
		.elem_size = 2,
		.array_type = 0,
		.tlv_type = 0,
		.offset = offsetof(struct h5_qmi_response_type, error),
	},
	{ .data_type = QMI_EOTI },
};
#endif

struct h5_filter_rule_identifier_to_handle_map {
	u32 filter_spec_identifier;
	u32 filter_handle;
} __packed;
static_assert(sizeof(struct h5_filter_rule_identifier_to_handle_map) == 8);
static_assert(offsetof(struct h5_filter_rule_identifier_to_handle_map, filter_spec_identifier) == 0);
static_assert(offsetof(struct h5_filter_rule_identifier_to_handle_map, filter_handle) == 4);
#ifdef H5_IPA_QMI_SCHEMA_DEFINE
static const struct qmi_elem_info h5_filter_rule_identifier_to_handle_map_ei[] = {
	{
		.data_type = QMI_UNSIGNED_4_BYTE,
		.elem_len = 1,
		.elem_size = 4,
		.array_type = 0,
		.tlv_type = 0,
		.offset = offsetof(struct h5_filter_rule_identifier_to_handle_map, filter_spec_identifier),
	},
	{
		.data_type = QMI_UNSIGNED_4_BYTE,
		.elem_len = 1,
		.elem_size = 4,
		.array_type = 0,
		.tlv_type = 0,
		.offset = offsetof(struct h5_filter_rule_identifier_to_handle_map, filter_handle),
	},
	{ .data_type = QMI_EOTI },
};
#endif

struct h5_install_fltr_rule_resp_msg {
	struct h5_qmi_response_type resp;
	u8 filter_handle_list_valid;
	u8 reserved_2[3];
	u32 filter_handle_list_len;
	struct h5_filter_rule_identifier_to_handle_map filter_handle_list[64];
	u8 rule_id_valid;
	u8 reserved_5[3];
	u32 rule_id_len;
	u32 rule_id[64];
} __packed;
static_assert(sizeof(struct h5_install_fltr_rule_resp_msg) == 788);
static_assert(offsetof(struct h5_install_fltr_rule_resp_msg, resp) == 0);
static_assert(offsetof(struct h5_install_fltr_rule_resp_msg, filter_handle_list_valid) == 4);
static_assert(offsetof(struct h5_install_fltr_rule_resp_msg, filter_handle_list_len) == 8);
static_assert(offsetof(struct h5_install_fltr_rule_resp_msg, filter_handle_list) == 12);
static_assert(offsetof(struct h5_install_fltr_rule_resp_msg, rule_id_valid) == 524);
static_assert(offsetof(struct h5_install_fltr_rule_resp_msg, rule_id_len) == 528);
static_assert(offsetof(struct h5_install_fltr_rule_resp_msg, rule_id) == 532);
#ifdef H5_IPA_QMI_SCHEMA_DEFINE
static const struct qmi_elem_info h5_install_fltr_rule_resp_msg_ei[] = {
	{
		.data_type = QMI_STRUCT,
		.elem_len = 1,
		.elem_size = 4,
		.array_type = 0,
		.tlv_type = 2,
		.offset = offsetof(struct h5_install_fltr_rule_resp_msg, resp),
		.ei_array = h5_qmi_response_type_ei,
	},
	{
		.data_type = QMI_OPT_FLAG,
		.elem_len = 1,
		.elem_size = 1,
		.array_type = 0,
		.tlv_type = 16,
		.offset = offsetof(struct h5_install_fltr_rule_resp_msg, filter_handle_list_valid),
	},
	{
		.data_type = QMI_DATA_LEN,
		.elem_len = 1,
		.elem_size = 1,
		.array_type = 0,
		.tlv_type = 16,
		.offset = offsetof(struct h5_install_fltr_rule_resp_msg, filter_handle_list_len),
	},
	{
		.data_type = QMI_STRUCT,
		.elem_len = 64,
		.elem_size = 8,
		.array_type = 2,
		.tlv_type = 16,
		.offset = offsetof(struct h5_install_fltr_rule_resp_msg, filter_handle_list),
		.ei_array = h5_filter_rule_identifier_to_handle_map_ei,
	},
	{
		.data_type = QMI_OPT_FLAG,
		.elem_len = 1,
		.elem_size = 1,
		.array_type = 0,
		.tlv_type = 17,
		.offset = offsetof(struct h5_install_fltr_rule_resp_msg, rule_id_valid),
	},
	{
		.data_type = QMI_DATA_LEN,
		.elem_len = 1,
		.elem_size = 1,
		.array_type = 0,
		.tlv_type = 17,
		.offset = offsetof(struct h5_install_fltr_rule_resp_msg, rule_id_len),
	},
	{
		.data_type = QMI_UNSIGNED_4_BYTE,
		.elem_len = 64,
		.elem_size = 4,
		.array_type = 2,
		.tlv_type = 17,
		.offset = offsetof(struct h5_install_fltr_rule_resp_msg, rule_id),
	},
	{ .data_type = QMI_EOTI },
};
#endif

#define H5_INSTALL_FLTR_RULE_RESP_MSG_MAX_LEN 783
struct h5_filter_handle_to_index_map {
	u32 filter_handle;
	u32 filter_index;
} __packed;
static_assert(sizeof(struct h5_filter_handle_to_index_map) == 8);
static_assert(offsetof(struct h5_filter_handle_to_index_map, filter_handle) == 0);
static_assert(offsetof(struct h5_filter_handle_to_index_map, filter_index) == 4);
#ifdef H5_IPA_QMI_SCHEMA_DEFINE
static const struct qmi_elem_info h5_filter_handle_to_index_map_ei[] = {
	{
		.data_type = QMI_UNSIGNED_4_BYTE,
		.elem_len = 1,
		.elem_size = 4,
		.array_type = 0,
		.tlv_type = 0,
		.offset = offsetof(struct h5_filter_handle_to_index_map, filter_handle),
	},
	{
		.data_type = QMI_UNSIGNED_4_BYTE,
		.elem_len = 1,
		.elem_size = 4,
		.array_type = 0,
		.tlv_type = 0,
		.offset = offsetof(struct h5_filter_handle_to_index_map, filter_index),
	},
	{ .data_type = QMI_EOTI },
};
#endif

struct h5_fltr_installed_notif_req_msg {
	u32 source_pipe_index;
	u16 install_status;
	u8 reserved_2[2];
	u32 filter_index_list_len;
	struct h5_filter_handle_to_index_map filter_index_list[64];
	u8 embedded_pipe_index_valid;
	u8 reserved_5[3];
	u32 embedded_pipe_index;
	u8 retain_header_valid;
	u8 retain_header;
	u8 embedded_call_mux_id_valid;
	u8 reserved_9[1];
	u32 embedded_call_mux_id;
	u8 num_ipv4_filters_valid;
	u8 reserved_11[3];
	u32 num_ipv4_filters;
	u8 num_ipv6_filters_valid;
	u8 reserved_13[3];
	u32 num_ipv6_filters;
	u8 start_ipv4_filter_idx_valid;
	u8 reserved_15[3];
	u32 start_ipv4_filter_idx;
	u8 start_ipv6_filter_idx_valid;
	u8 reserved_17[3];
	u32 start_ipv6_filter_idx;
	u8 rule_id_valid;
	u8 reserved_19[3];
	u32 rule_id_len;
	u32 rule_id[64];
	u8 dst_pipe_id_valid;
	u8 reserved_22[3];
	u32 dst_pipe_id_len;
	u32 dst_pipe_id[8];
	u8 rule_id_ex_valid;
	u8 reserved_25[3];
	u32 rule_id_ex_len;
	u32 rule_id_ex[256];
} __packed;
static_assert(sizeof(struct h5_fltr_installed_notif_req_msg) == 1908);
static_assert(offsetof(struct h5_fltr_installed_notif_req_msg, source_pipe_index) == 0);
static_assert(offsetof(struct h5_fltr_installed_notif_req_msg, install_status) == 4);
static_assert(offsetof(struct h5_fltr_installed_notif_req_msg, filter_index_list_len) == 8);
static_assert(offsetof(struct h5_fltr_installed_notif_req_msg, filter_index_list) == 12);
static_assert(offsetof(struct h5_fltr_installed_notif_req_msg, embedded_pipe_index_valid) == 524);
static_assert(offsetof(struct h5_fltr_installed_notif_req_msg, embedded_pipe_index) == 528);
static_assert(offsetof(struct h5_fltr_installed_notif_req_msg, retain_header_valid) == 532);
static_assert(offsetof(struct h5_fltr_installed_notif_req_msg, retain_header) == 533);
static_assert(offsetof(struct h5_fltr_installed_notif_req_msg, embedded_call_mux_id_valid) == 534);
static_assert(offsetof(struct h5_fltr_installed_notif_req_msg, embedded_call_mux_id) == 536);
static_assert(offsetof(struct h5_fltr_installed_notif_req_msg, num_ipv4_filters_valid) == 540);
static_assert(offsetof(struct h5_fltr_installed_notif_req_msg, num_ipv4_filters) == 544);
static_assert(offsetof(struct h5_fltr_installed_notif_req_msg, num_ipv6_filters_valid) == 548);
static_assert(offsetof(struct h5_fltr_installed_notif_req_msg, num_ipv6_filters) == 552);
static_assert(offsetof(struct h5_fltr_installed_notif_req_msg, start_ipv4_filter_idx_valid) == 556);
static_assert(offsetof(struct h5_fltr_installed_notif_req_msg, start_ipv4_filter_idx) == 560);
static_assert(offsetof(struct h5_fltr_installed_notif_req_msg, start_ipv6_filter_idx_valid) == 564);
static_assert(offsetof(struct h5_fltr_installed_notif_req_msg, start_ipv6_filter_idx) == 568);
static_assert(offsetof(struct h5_fltr_installed_notif_req_msg, rule_id_valid) == 572);
static_assert(offsetof(struct h5_fltr_installed_notif_req_msg, rule_id_len) == 576);
static_assert(offsetof(struct h5_fltr_installed_notif_req_msg, rule_id) == 580);
static_assert(offsetof(struct h5_fltr_installed_notif_req_msg, dst_pipe_id_valid) == 836);
static_assert(offsetof(struct h5_fltr_installed_notif_req_msg, dst_pipe_id_len) == 840);
static_assert(offsetof(struct h5_fltr_installed_notif_req_msg, dst_pipe_id) == 844);
static_assert(offsetof(struct h5_fltr_installed_notif_req_msg, rule_id_ex_valid) == 876);
static_assert(offsetof(struct h5_fltr_installed_notif_req_msg, rule_id_ex_len) == 880);
static_assert(offsetof(struct h5_fltr_installed_notif_req_msg, rule_id_ex) == 884);
#ifdef H5_IPA_QMI_SCHEMA_DEFINE
static const struct qmi_elem_info h5_fltr_installed_notif_req_msg_ei[] = {
	{
		.data_type = QMI_UNSIGNED_4_BYTE,
		.elem_len = 1,
		.elem_size = 4,
		.array_type = 0,
		.tlv_type = 1,
		.offset = offsetof(struct h5_fltr_installed_notif_req_msg, source_pipe_index),
	},
	{
		.data_type = QMI_UNSIGNED_2_BYTE,
		.elem_len = 1,
		.elem_size = 2,
		.array_type = 0,
		.tlv_type = 2,
		.offset = offsetof(struct h5_fltr_installed_notif_req_msg, install_status),
	},
	{
		.data_type = QMI_DATA_LEN,
		.elem_len = 1,
		.elem_size = 1,
		.array_type = 0,
		.tlv_type = 3,
		.offset = offsetof(struct h5_fltr_installed_notif_req_msg, filter_index_list_len),
	},
	{
		.data_type = QMI_STRUCT,
		.elem_len = 64,
		.elem_size = 8,
		.array_type = 2,
		.tlv_type = 3,
		.offset = offsetof(struct h5_fltr_installed_notif_req_msg, filter_index_list),
		.ei_array = h5_filter_handle_to_index_map_ei,
	},
	{
		.data_type = QMI_OPT_FLAG,
		.elem_len = 1,
		.elem_size = 1,
		.array_type = 0,
		.tlv_type = 16,
		.offset = offsetof(struct h5_fltr_installed_notif_req_msg, embedded_pipe_index_valid),
	},
	{
		.data_type = QMI_UNSIGNED_4_BYTE,
		.elem_len = 1,
		.elem_size = 4,
		.array_type = 0,
		.tlv_type = 16,
		.offset = offsetof(struct h5_fltr_installed_notif_req_msg, embedded_pipe_index),
	},
	{
		.data_type = QMI_OPT_FLAG,
		.elem_len = 1,
		.elem_size = 1,
		.array_type = 0,
		.tlv_type = 17,
		.offset = offsetof(struct h5_fltr_installed_notif_req_msg, retain_header_valid),
	},
	{
		.data_type = QMI_UNSIGNED_1_BYTE,
		.elem_len = 1,
		.elem_size = 1,
		.array_type = 0,
		.tlv_type = 17,
		.offset = offsetof(struct h5_fltr_installed_notif_req_msg, retain_header),
	},
	{
		.data_type = QMI_OPT_FLAG,
		.elem_len = 1,
		.elem_size = 1,
		.array_type = 0,
		.tlv_type = 18,
		.offset = offsetof(struct h5_fltr_installed_notif_req_msg, embedded_call_mux_id_valid),
	},
	{
		.data_type = QMI_UNSIGNED_4_BYTE,
		.elem_len = 1,
		.elem_size = 4,
		.array_type = 0,
		.tlv_type = 18,
		.offset = offsetof(struct h5_fltr_installed_notif_req_msg, embedded_call_mux_id),
	},
	{
		.data_type = QMI_OPT_FLAG,
		.elem_len = 1,
		.elem_size = 1,
		.array_type = 0,
		.tlv_type = 19,
		.offset = offsetof(struct h5_fltr_installed_notif_req_msg, num_ipv4_filters_valid),
	},
	{
		.data_type = QMI_UNSIGNED_4_BYTE,
		.elem_len = 1,
		.elem_size = 4,
		.array_type = 0,
		.tlv_type = 19,
		.offset = offsetof(struct h5_fltr_installed_notif_req_msg, num_ipv4_filters),
	},
	{
		.data_type = QMI_OPT_FLAG,
		.elem_len = 1,
		.elem_size = 1,
		.array_type = 0,
		.tlv_type = 20,
		.offset = offsetof(struct h5_fltr_installed_notif_req_msg, num_ipv6_filters_valid),
	},
	{
		.data_type = QMI_UNSIGNED_4_BYTE,
		.elem_len = 1,
		.elem_size = 4,
		.array_type = 0,
		.tlv_type = 20,
		.offset = offsetof(struct h5_fltr_installed_notif_req_msg, num_ipv6_filters),
	},
	{
		.data_type = QMI_OPT_FLAG,
		.elem_len = 1,
		.elem_size = 1,
		.array_type = 0,
		.tlv_type = 21,
		.offset = offsetof(struct h5_fltr_installed_notif_req_msg, start_ipv4_filter_idx_valid),
	},
	{
		.data_type = QMI_UNSIGNED_4_BYTE,
		.elem_len = 1,
		.elem_size = 4,
		.array_type = 0,
		.tlv_type = 21,
		.offset = offsetof(struct h5_fltr_installed_notif_req_msg, start_ipv4_filter_idx),
	},
	{
		.data_type = QMI_OPT_FLAG,
		.elem_len = 1,
		.elem_size = 1,
		.array_type = 0,
		.tlv_type = 22,
		.offset = offsetof(struct h5_fltr_installed_notif_req_msg, start_ipv6_filter_idx_valid),
	},
	{
		.data_type = QMI_UNSIGNED_4_BYTE,
		.elem_len = 1,
		.elem_size = 4,
		.array_type = 0,
		.tlv_type = 22,
		.offset = offsetof(struct h5_fltr_installed_notif_req_msg, start_ipv6_filter_idx),
	},
	{
		.data_type = QMI_OPT_FLAG,
		.elem_len = 1,
		.elem_size = 1,
		.array_type = 0,
		.tlv_type = 23,
		.offset = offsetof(struct h5_fltr_installed_notif_req_msg, rule_id_valid),
	},
	{
		.data_type = QMI_DATA_LEN,
		.elem_len = 1,
		.elem_size = 1,
		.array_type = 0,
		.tlv_type = 23,
		.offset = offsetof(struct h5_fltr_installed_notif_req_msg, rule_id_len),
	},
	{
		.data_type = QMI_UNSIGNED_4_BYTE,
		.elem_len = 64,
		.elem_size = 4,
		.array_type = 2,
		.tlv_type = 23,
		.offset = offsetof(struct h5_fltr_installed_notif_req_msg, rule_id),
	},
	{
		.data_type = QMI_OPT_FLAG,
		.elem_len = 1,
		.elem_size = 1,
		.array_type = 0,
		.tlv_type = 24,
		.offset = offsetof(struct h5_fltr_installed_notif_req_msg, dst_pipe_id_valid),
	},
	{
		.data_type = QMI_DATA_LEN,
		.elem_len = 1,
		.elem_size = 1,
		.array_type = 0,
		.tlv_type = 24,
		.offset = offsetof(struct h5_fltr_installed_notif_req_msg, dst_pipe_id_len),
	},
	{
		.data_type = QMI_UNSIGNED_4_BYTE,
		.elem_len = 8,
		.elem_size = 4,
		.array_type = 2,
		.tlv_type = 24,
		.offset = offsetof(struct h5_fltr_installed_notif_req_msg, dst_pipe_id),
	},
	{
		.data_type = QMI_OPT_FLAG,
		.elem_len = 1,
		.elem_size = 1,
		.array_type = 0,
		.tlv_type = 25,
		.offset = offsetof(struct h5_fltr_installed_notif_req_msg, rule_id_ex_valid),
	},
	{
		.data_type = QMI_DATA_LEN,
		.elem_len = 1,
		.elem_size = 4,
		.array_type = 0,
		.tlv_type = 25,
		.offset = offsetof(struct h5_fltr_installed_notif_req_msg, rule_id_ex_len),
	},
	{
		.data_type = QMI_UNSIGNED_4_BYTE,
		.elem_len = 256,
		.elem_size = 4,
		.array_type = 2,
		.tlv_type = 25,
		.offset = offsetof(struct h5_fltr_installed_notif_req_msg, rule_id_ex),
	},
	{ .data_type = QMI_EOTI },
};
#endif

#define H5_FLTR_INSTALLED_NOTIF_REQ_MSG_MAX_LEN 1899
#define H5_FLTR_INSTALLED_NOTIF_REQ_MSG_ID 36
struct h5_fltr_installed_notif_resp_msg {
	struct h5_qmi_response_type resp;
} __packed;
static_assert(sizeof(struct h5_fltr_installed_notif_resp_msg) == 4);
static_assert(offsetof(struct h5_fltr_installed_notif_resp_msg, resp) == 0);
#ifdef H5_IPA_QMI_SCHEMA_DEFINE
static const struct qmi_elem_info h5_fltr_installed_notif_resp_msg_ei[] = {
	{
		.data_type = QMI_STRUCT,
		.elem_len = 1,
		.elem_size = 4,
		.array_type = 0,
		.tlv_type = 2,
		.offset = offsetof(struct h5_fltr_installed_notif_resp_msg, resp),
		.ei_array = h5_qmi_response_type_ei,
	},
	{ .data_type = QMI_EOTI },
};
#endif

#define H5_FLTR_INSTALLED_NOTIF_RESP_MSG_MAX_LEN 7
#endif
