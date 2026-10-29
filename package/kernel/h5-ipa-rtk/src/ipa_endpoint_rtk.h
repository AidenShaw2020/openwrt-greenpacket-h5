/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _IPA_ENDPOINT_RTK_H_
#define _IPA_ENDPOINT_RTK_H_

#include <linux/types.h>

struct ipa;
struct net_device;

#define IPA_RTK_SAVED_REGS 16
struct ipa_rtk_endpoints {
	struct ipa *ipa;
	u32 saved[2][IPA_RTK_SAVED_REGS];
	struct net_device *netdev;
};

struct ipa_rtk_rx_stats {
	u64 packets;
	u64 bytes;
	u64 invalid;
};
void ipa_endpoint_rtk_stats(struct ipa_rtk_rx_stats *stats);

/* Caller retains IPA power; prepare while both external DMA channels are off. */
int ipa_endpoint_rtk_prepare(struct ipa *ipa, struct ipa_rtk_endpoints *rtk);
int ipa_endpoint_rtk_attach_netdev(struct ipa_rtk_endpoints *rtk,
				  struct net_device *netdev);
/* Stop both engines before restore. This also drains exception RX readers. */
void ipa_endpoint_rtk_restore(struct ipa_rtk_endpoints *rtk);

#endif
