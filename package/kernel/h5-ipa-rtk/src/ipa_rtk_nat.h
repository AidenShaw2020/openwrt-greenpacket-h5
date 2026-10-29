/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _IPA_RTK_NAT_H_
#define _IPA_RTK_NAT_H_

#include <linux/dma-mapping.h>
#include <linux/types.h>

struct ipa;

struct ipa_rtk_nat_layout {
	dma_addr_t base;
	dma_addr_t expansion;
	dma_addr_t index;
	dma_addr_t index_expansion;
	u16 base_mask;
	u16 expansion_count;
};

/* A single 16-byte PDN record, in IPA wire order. */
struct ipa_rtk_pdn {
	__le32 public_ip;
	__le32 source_metadata;
	__le32 destination_metadata;
	__le32 reserved;
};

/* Coherent buffers remain owned by hardware until a replacement init and drain. */
int ipa_cmd_rtk_nat_init(struct ipa *ipa, const struct ipa_rtk_nat_layout *layout,
			 dma_addr_t pdn_record);
int ipa_cmd_rtk_nat_dma(struct ipa *ipa, u8 table, u32 offset, u16 value);
int ipa_cmd_rtk_nat_disable(struct ipa *ipa);

#endif
