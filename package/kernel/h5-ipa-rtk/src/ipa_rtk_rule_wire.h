/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _IPA_RTK_RULE_WIRE_H_
#define _IPA_RTK_RULE_WIRE_H_

#include <linux/types.h>
#ifdef __KERNEL__
#include <linux/errno.h>
#include <linux/string.h>
#else
#include <errno.h>
#include <string.h>
#endif

/* IPA 5.0 equations, independent of QMI C layout and compiler bitfields. */
struct ipa_rtk_eq32 { __u8 offset; __u32 mask, value; };
struct ipa_rtk_eq128 { __u8 offset, mask[16], value[16]; };
struct ipa_rtk_range16 { __u8 offset; __u16 low, high; };
struct ipa_rtk_equations {
	__u16 bitmap;
	__u8 pure_ack, protocol_present, protocol, tc_present, tc;
	__u8 meq128_count, meq32_count, ihl_meq32_count, range16_count;
	struct ipa_rtk_eq128 meq128[2];
	struct ipa_rtk_eq32 meq32[2], ihl_meq32[2], metadata;
	struct ipa_rtk_range16 range16[2];
	__u8 metadata_present, ihl_eq32_present, ihl_eq32_offset;
	__u32 ihl_eq32;
	__u8 ihl_eq16_present, ihl_eq16_offset;
	__u16 ihl_eq16;
	__u8 flow_present, fragment;
	__u32 flow;
};

static inline void ipa_rtk_put16(__u8 *p, __u16 value)
{
	p[0] = value; p[1] = value >> 8;
}
static inline void ipa_rtk_put32(__u8 *p, __u32 value)
{
	ipa_rtk_put16(p, value); ipa_rtk_put16(p + 2, value >> 16);
}
static inline void ipa_rtk_put64(__u8 *p, __u64 value)
{
	ipa_rtk_put32(p, value); ipa_rtk_put32(p + 4, value >> 32);
}

/* Includes the eight-byte header; zero header is the eight-byte terminator.
 * Local table offsets are relative to that table's header block, not SRAM.
 */
static inline unsigned int ipa_rtk_rule_size(__u64 header)
{
	unsigned int extra = 0, payload = 0, bit;
	__u16 bitmap = header;
	if (!header)
		return 8;
	for (bit = 0; bit < 16; bit++) {
		if (!(bitmap & (1U << bit)))
			continue;
		if (bit <= 8 || (bit >= 10 && bit <= 13))
			extra++;
		if (bit == 3 || bit == 4)
			payload += 32;
		else if (bit >= 5 && bit <= 9)
			payload += 8;
		else if (bit >= 10 && bit <= 12)
			payload += 4;
		else if (bit == 13)
			payload += 2;
		else if (bit == 14)
			payload += 4;
	}
	return 8 + ((((extra + 7) & ~7U) + payload + 7) & ~7U);
}

/* dst must have 248 bytes available and start at an eight-byte boundary.
 * The caller publishes it to hardware only after this function succeeds.
 */
static inline int ipa_rtk_equations_encode(__u8 *dst,
					 const struct ipa_rtk_equations *eq)
{
	unsigned int extra, cursor, payload, i, half;
	__u16 bitmap;

	if (eq->meq128_count > 2 || eq->meq32_count > 2 ||
	    eq->ihl_meq32_count > 2 || eq->range16_count > 2 ||
	    eq->pure_ack > 1 || eq->protocol_present > 1 || eq->tc_present > 1 ||
	    eq->metadata_present > 1 || eq->ihl_eq32_present > 1 ||
	    eq->ihl_eq16_present > 1 || eq->flow_present > 1 || eq->fragment > 1)
		return -EINVAL;
	bitmap = eq->pure_ack | eq->protocol_present << 1 | eq->tc_present << 2 |
		(eq->meq128_count > 0) << 3 | (eq->meq128_count > 1) << 4 |
		(eq->meq32_count > 0) << 5 | (eq->meq32_count > 1) << 6 |
		(eq->ihl_meq32_count > 0) << 7 | (eq->ihl_meq32_count > 1) << 8 |
		eq->metadata_present << 9 | (eq->range16_count > 0) << 10 |
		(eq->range16_count > 1) << 11 | eq->ihl_eq32_present << 12 |
		eq->ihl_eq16_present << 13 | eq->flow_present << 14 | eq->fragment << 15;
	if (bitmap != eq->bitmap)
		return -EINVAL;
	extra = eq->pure_ack + eq->protocol_present + eq->tc_present +
		eq->meq128_count + eq->meq32_count + eq->ihl_meq32_count +
		eq->range16_count + eq->ihl_eq32_present + eq->ihl_eq16_present;
	if (extra > 13)
		return -EINVAL;
	memset(dst, 0, 248);
	cursor = 0;
	payload = (extra + 7) & ~7U;
	if (eq->pure_ack) dst[cursor++] = 0;
	if (eq->protocol_present) dst[cursor++] = eq->protocol;
	if (eq->tc_present) dst[cursor++] = eq->tc;
	for (i = 0; i < eq->meq128_count; i++) {
		dst[cursor++] = eq->meq128[i].offset;
		/* Hardware interleaves each 64-bit mask with its 64-bit value. */
		for (half = 0; half < 2; half++) {
			memcpy(dst + payload, eq->meq128[i].mask + half * 8, 8);
			memcpy(dst + payload + 8, eq->meq128[i].value + half * 8, 8);
			payload += 16;
		}
	}
	for (i = 0; i < eq->meq32_count; i++) {
		dst[cursor++] = eq->meq32[i].offset;
		ipa_rtk_put32(dst + payload, eq->meq32[i].mask);
		ipa_rtk_put32(dst + payload + 4, eq->meq32[i].value);
		payload += 8;
	}
	for (i = 0; i < eq->ihl_meq32_count; i++) {
		dst[cursor++] = eq->ihl_meq32[i].offset;
		ipa_rtk_put32(dst + payload, eq->ihl_meq32[i].mask);
		ipa_rtk_put32(dst + payload + 4, eq->ihl_meq32[i].value);
		payload += 8;
	}
	if (eq->metadata_present) {
		ipa_rtk_put32(dst + payload, eq->metadata.mask);
		ipa_rtk_put32(dst + payload + 4, eq->metadata.value);
		payload += 8;
	}
	for (i = 0; i < eq->range16_count; i++) {
		if (eq->range16[i].low > eq->range16[i].high)
			return -EINVAL;
		dst[cursor++] = eq->range16[i].offset;
		ipa_rtk_put16(dst + payload, eq->range16[i].high);
		ipa_rtk_put16(dst + payload + 2, eq->range16[i].low);
		payload += 4;
	}
	if (eq->ihl_eq32_present) {
		dst[cursor++] = eq->ihl_eq32_offset;
		ipa_rtk_put32(dst + payload, eq->ihl_eq32);
		payload += 4;
	}
	if (eq->ihl_eq16_present) {
		dst[cursor++] = eq->ihl_eq16_offset;
		ipa_rtk_put16(dst + payload, eq->ihl_eq16);
		payload += 4;
	}
	if (eq->flow_present) {
		ipa_rtk_put32(dst + payload, eq->flow & 0xfffff);
		payload += 4;
	}
	return (payload + 7) & ~7U;
}

/* Both rule headers are eight bytes on IPA 5.0. Rule ID 1023 is reserved. */
static inline int ipa_rtk_filter_encode(__u8 *dst, const struct ipa_rtk_equations *eq,
		__u8 route, __u8 action, __u8 pdn, __u8 priority, __u16 id,
		__u8 counter, __u8 retain, __u8 metadata)
{
	__u64 header;
	int length;
	if (id >= 1023 || action > 3 || pdn > 15 || retain > 1 || metadata > 1)
		return -EINVAL;
	length = ipa_rtk_equations_encode(dst + 8, eq);
	if (length < 0) return length;
	header = eq->bitmap | (__u64)route << 16 | (__u64)counter << 24 |
		(__u64)priority << 32 | (__u64)id << 42 | (__u64)action << 52 |
		(__u64)pdn << 57 | (__u64)metadata << 61 | (__u64)retain << 62;
	ipa_rtk_put64(dst, header);
	return length + 8;
}

static inline int ipa_rtk_route_encode(__u8 *dst, const struct ipa_rtk_equations *eq,
		__u8 endpoint, __u16 header_offset, __u8 process_context,
		__u8 system, __u8 priority, __u16 id, __u8 counter, __u8 retain)
{
	__u64 header;
	unsigned int alignment;
	int length;
	if (id >= 1023 || process_context > 1 || system > 1 || retain > 1)
		return -EINVAL;
	alignment = process_context ? 32 : 4;
	if (header_offset % alignment || header_offset / alignment > 511)
		return -EINVAL;
	length = ipa_rtk_equations_encode(dst + 8, eq);
	if (length < 0) return length;
	header = eq->bitmap | (__u64)endpoint << 16 | (__u64)counter << 24 |
		(__u64)priority << 32 | (__u64)id << 42 |
		(__u64)(header_offset / alignment) << 52 |
		(__u64)process_context << 61 | (__u64)retain << 62 | (__u64)system << 63;
	ipa_rtk_put64(dst, header);
	return length + 8;
}
#endif
