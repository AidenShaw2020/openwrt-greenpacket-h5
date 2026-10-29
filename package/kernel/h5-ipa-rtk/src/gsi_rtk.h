/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _GSI_RTK_H_
#define _GSI_RTK_H_

#include <linux/types.h>

struct gsi;

/* IPA-side IOVAs, not PCI DMA addresses.  The caller owns all mappings. */
struct gsi_rtk_config {
	u64 ring_addr;
	u64 buffer_addr;
	u64 bar_addr;
	u32 ring_size;
	u32 buffer_size;
	u32 bar_size;
	u32 tail_offset;
	u8 queue;
	bool toward_ipa;
};

/* Zero initialize before first use.  Keep alive until release succeeds. */
struct gsi_rtk {
	struct gsi *gsi;
	u32 channel_id;
	u32 event_id;
	u64 msi_addr;
	bool prepared;
};

/*
 * The caller holds IPA runtime power and serializes the handle lifetime.
 * prepare does not start DMA.  A failed prepare may retain hardware ownership;
 * always call release, and retain the rings/mappings until release returns 0.
 * Start only after IPA endpoint/uC setup and both NIC rings are ready.
 * Before release, disable the NIC offload queue and IPA endpoint first.
 */
int gsi_rtk_prepare(struct gsi *gsi, const struct gsi_rtk_config *config,
		    struct gsi_rtk *rtk);
int gsi_rtk_start(struct gsi_rtk *rtk);
int gsi_rtk_stop(struct gsi_rtk *rtk);
int gsi_rtk_release(struct gsi_rtk *rtk);

#endif
