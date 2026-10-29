/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _IPA_RTK_TX_H_
#define _IPA_RTK_TX_H_
#include <linux/dma-mapping.h>
struct ipa;

/* Caller serializes with RTK stop and owns a coherent AP DMA frame buffer.
 * Returns after the command-channel transfer completes; RTK must be running.
 */
int ipa_cmd_rtk_packet_tx(struct ipa *ipa, dma_addr_t frame, u32 length);
#endif
