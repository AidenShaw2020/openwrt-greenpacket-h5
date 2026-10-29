// SPDX-License-Identifier: GPL-2.0-only
#include <linux/dma-mapping.h>
#include <linux/iommu.h>
#include <linux/log2.h>
#include <linux/pci.h>
#include <linux/rtnetlink.h>

#include "ipa_rtk_dma.h"

int ipa_rtk_dma_prepare(struct ipa_rtk_dma *dma, struct device *ap_dev,
			struct device *eth_dev, struct net_device *netdev,
			bool toward_ipa, unsigned int count, unsigned int buffer_size)
{
	struct device *nic_dev = netdev->dev.parent;
	struct iommu_domain *nic_domain;
	struct iommu_domain *eth_domain;
	struct rtl8125_ring *ring;
	struct pci_dev *pdev;
	unsigned int offset;
	int ret;

	ASSERT_RTNL();
	if (dma->ring || !nic_dev || !dev_is_pci(nic_dev) ||
	    !nic_dev->driver || strcmp(nic_dev->driver->name, "r8125"))
		return -EINVAL;
	if (!is_power_of_2(count) || count < 64 || count > 4096 ||
	    !is_power_of_2(buffer_size) || buffer_size < 2048 || buffer_size > PAGE_SIZE)
		return -EINVAL;
	nic_domain = iommu_get_domain_for_dev(nic_dev);
	eth_domain = iommu_get_domain_for_dev(eth_dev);
	/* No best-effort fallback to incompatible DMA address spaces. */
	if (!nic_domain || nic_domain != eth_domain)
		return -EXDEV;
	pdev = to_pci_dev(nic_dev);
	ring = rtl8125_request_ring(netdev, count, buffer_size,
			toward_ipa ? RTL8125_CH_DIR_RX : RTL8125_CH_DIR_TX,
			RTL8125_CONTIG_BUFS, NULL);
	if (!ring)
		return -ENOMEM;
	dma->ring = ring;
	dma->ap_dev = ap_dev;
	dma->netdev = netdev;
	/* Verify every data-buffer page in both hardware address spaces. */
	for (offset = 0; offset < count * buffer_size; offset += PAGE_SIZE) {
		dma_addr_t address = ring->bufs[0].dma_addr + offset;
		phys_addr_t physical = iommu_iova_to_phys(nic_domain, address);

		if (!physical || iommu_iova_to_phys(eth_domain, address) != physical) {
			ret = -EXDEV;
			goto err_release;
		}
	}
	/* Both directions must use the stock 32-byte descriptor ABI. */
	if (ring->desc_size != count * 32 || ring->enabled) {
		ret = -EINVAL;
		goto err_release;
	}
	ret = dma_get_sgtable(nic_dev, &dma->descriptors, ring->desc_addr,
			     ring->desc_daddr, ring->desc_size);
	if (ret)
		goto err_release;
	ret = dma_map_sgtable(ap_dev, &dma->descriptors, DMA_BIDIRECTIONAL, 0);
	if (ret)
		goto err_sg_free;
	dma->descriptors_mapped = true;
	if (dma->descriptors.nents != 1 ||
	    sg_dma_len(dma->descriptors.sgl) < ring->desc_size) {
		ret = -ERANGE;
		goto err_unmap;
	}
	dma->config.ring_addr = sg_dma_address(dma->descriptors.sgl);
	dma->config.ring_size = ring->desc_size;
	dma->config.buffer_addr = ring->bufs[0].dma_addr;
	dma->config.buffer_size = buffer_size;
	dma->config.bar_addr = pci_resource_start(pdev, 2);
	dma->config.bar_size = pci_resource_len(pdev, 2);
	dma->config.queue = ring->queue_num;
	dma->config.toward_ipa = toward_ipa;
	dma->config.tail_offset = toward_ipa ? 0 : 0x2800 + 4 * ring->queue_num;
	if (!IS_ALIGNED(dma->config.ring_addr, ring->desc_size) ||
	    !IS_ALIGNED(dma->config.buffer_addr, buffer_size)) {
		ret = -ERANGE;
		goto err_unmap;
	}
	return 0;

err_unmap:
	dma_unmap_sgtable(ap_dev, &dma->descriptors, DMA_BIDIRECTIONAL, 0);
	dma->descriptors_mapped = false;
err_sg_free:
	sg_free_table(&dma->descriptors);
err_release:
	rtl8125_release_ring(ring);
	memset(dma, 0, sizeof(*dma));
	return ret;
}

int ipa_rtk_dma_set_notification(struct ipa_rtk_dma *dma, phys_addr_t msi)
{
	struct device *nic_dev;
	int ret;

	ASSERT_RTNL();
	if (!dma->ring || dma->notification_mapped || !IS_ALIGNED(msi, 4))
		return -EINVAL;
	nic_dev = dma->netdev->dev.parent;
	/* MSI-X writes travel through the PCIe SMMU domain too. */
	dma->notification = dma_map_resource(nic_dev, msi, 4, DMA_FROM_DEVICE, 0);
	if (dma_mapping_error(nic_dev, dma->notification))
		return -EIO;
	dma->notification_mapped = true;
	ret = rtl8125_request_event(dma->ring, MSIX_event_type, dma->notification, 0);
	if (!ret) {
		dma->event_claimed = true;
		return 0;
	}
	dma_unmap_resource(nic_dev, dma->notification, 4, DMA_FROM_DEVICE, 0);
	dma->notification_mapped = false;
	return ret;
}

int ipa_rtk_dma_release(struct ipa_rtk_dma *dma, const struct gsi_rtk *rtk)
{
	struct device *nic_dev;

	ASSERT_RTNL();
	if (!rtk)
		return -EINVAL;
	if (rtk->gsi)
		return -EBUSY;
	if (!dma->ring)
		return 0;
	/* Refuse to free anything if the NIC is still an active DMA owner. */
	if (WARN_ON(dma->ring->enabled || dma->ring->event.enabled))
		return -EBUSY;
	nic_dev = dma->netdev->dev.parent;
	if (dma->event_claimed)
		rtl8125_release_event(dma->ring);
	if (dma->notification_mapped)
		dma_unmap_resource(nic_dev, dma->notification, 4, DMA_FROM_DEVICE, 0);
	if (dma->descriptors_mapped)
		dma_unmap_sgtable(dma->ap_dev, &dma->descriptors, DMA_BIDIRECTIONAL, 0);
	sg_free_table(&dma->descriptors);
	rtl8125_release_ring(dma->ring);
	memset(dma, 0, sizeof(*dma));
	return 0;
}
