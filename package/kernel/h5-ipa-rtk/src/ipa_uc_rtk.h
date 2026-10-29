/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _IPA_UC_RTK_H_
#define _IPA_UC_RTK_H_

#include <linux/dma-mapping.h>
#include <linux/types.h>

struct ipa;

/* Exact H5 stock ipa3_eth_config_uc ABI: setup 28 bytes, teardown 8 bytes. */
struct ipa_uc_rtk_setup {
	__le32 protocol;		/* 9: RTK */
	u8 direction;		/* 0: Ethernet RX, 1: Ethernet TX */
	u8 gsi_channel;
	__le16 reserved;
	u8 padding[20];
} __packed;

struct ipa_uc_rtk_teardown {
	__le32 protocol;
	u8 gsi_channel;
	u8 reserved[3];
} __packed;

/*
 * Address is a coherent DMA allocation through ipa->dev.  On SDX65 this
 * device maps both AP SID 0x5e0 and UC SID 0x5e2 into the same IOMMU domain.
 * The caller fills the appropriate zero-padded structure and holds a runtime
 * power reference covering the entire external-ring lifetime.
 *
 * -ETIMEDOUT/-EPROTO: completion is unknown; retain payload and ring mappings
 * until hardware reset. Further commands are refused. Do not attempt fallback
 * by freeing memory still owned by the microcontroller.
 */
int ipa_uc_rtk_command(struct ipa *ipa, dma_addr_t address, bool setup);

#endif
