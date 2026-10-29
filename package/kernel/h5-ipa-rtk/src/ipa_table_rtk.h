/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _IPA_TABLE_RTK_H_
#define _IPA_TABLE_RTK_H_
#include <linux/dma-mapping.h>
#include <linux/list.h>
struct ipa;

struct ipa_rtk_table {
	struct list_head node;
	struct ipa *ipa;
	u32 offset;
	u64 original_pointer;
	bool installed;
	u64 installed_pointer;
	u32 base_offset;
	bool modem;
};

/* A 16-byte local AP header slot. Routes reference route_offset, relative to
 * the complete modem+AP local header table, not the raw SRAM offset.
 */
struct ipa_rtk_header {
	struct list_head node;
	struct ipa *ipa;
	u32 offset;
	u16 route_offset;
	u8 original[16];
};

/* Handles must be zero-initialized and remain alive until release succeeds.
 * Caller holds IPA power and retains rule DMA until successful replacement
 * or release. Release restores the exact original, possibly local, pointer.
 */
int ipa_table_rtk_acquire(struct ipa *ipa, bool filter, bool ipv6,
			  u32 id, struct ipa_rtk_table *table);
int ipa_table_rtk_acquire_hashed(struct ipa *ipa, bool filter, bool ipv6,
				 u32 id, struct ipa_rtk_table *table);
/* Non-hashed IPv4 modem downlink only. Preserve its original rule bytes and
 * leave its hashed table untouched. Concurrent modem replacement is ESTALE.
 */
int ipa_table_rtk_acquire_downlink(struct ipa *ipa, struct ipa_rtk_table *table);
int ipa_table_rtk_copy_local(struct ipa_rtk_table *table, void *buffer, size_t capacity);
int ipa_table_rtk_replace(struct ipa_rtk_table *table, dma_addr_t rule_address);
int ipa_table_rtk_release(struct ipa_rtk_table *table);
int ipa_header_rtk_acquire(struct ipa *ipa, const u8 ethernet[14],
			   struct ipa_rtk_header *header);
int ipa_header_rtk_release(struct ipa_rtk_header *header);
#endif
