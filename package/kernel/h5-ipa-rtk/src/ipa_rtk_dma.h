/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _IPA_RTK_DMA_H_
#define _IPA_RTK_DMA_H_

#include <linux/device.h>
#include <linux/netdevice.h>
#include <linux/scatterlist.h>
#include <linux/iommu.h>
#include "gsi_rtk.h"
#include "r8125/r8125_lib.h"

struct ipa_rtk_dma {
	struct device *ap_dev;
	struct net_device *netdev;
	struct rtl8125_ring *ring;
	struct sg_table descriptors;
	struct gsi_rtk_config config;
	dma_addr_t notification;
	bool descriptors_mapped;
	bool notification_mapped;
	bool event_claimed;
};

/*
 * Caller holds RTNL, device/module references and runtime power.  ETH SID
 * 0x5e3 must share the NIC DMA domain before prepare, as in the stock DTS.
 * No ring or event is enabled by prepare.  Release only after both DMA engines
 * and the IPA uC have stopped; no memory is freed while GSI still owns it.
 */
int ipa_rtk_dma_prepare(struct ipa_rtk_dma *dma, struct device *ap_dev,
			struct device *eth_dev, struct net_device *netdev,
			bool toward_ipa, unsigned int count, unsigned int buffer_size);
int ipa_rtk_dma_set_notification(struct ipa_rtk_dma *dma, phys_addr_t msi);
int ipa_rtk_dma_release(struct ipa_rtk_dma *dma, const struct gsi_rtk *rtk);

#endif
