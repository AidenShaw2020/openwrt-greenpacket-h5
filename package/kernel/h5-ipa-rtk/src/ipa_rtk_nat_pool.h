/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _IPA_RTK_NAT_POOL_H_
#define _IPA_RTK_NAT_POOL_H_
#include <linux/mutex.h>
#include "ipa_rtk_nat.h"
#include "ipa_rtk_nat_wire.h"

struct ipa;
struct ipa_rtk_nat_slot {
	u16 source, destination;
	bool installed;
};
struct ipa_rtk_nat_pool {
	struct ipa *ipa;
	struct mutex lock;
	struct ipa_rtk_nat_layout layout;
	void *buffer;
	dma_addr_t address;
	size_t size;
	unsigned long *destinations, *sources;
	u16 count;
	__be32 public_ip;
	bool active, failed;
};
/* Caller holds IPA power and stops/removes every referencing filter before
 * destroy. Collision returns ENOSPC and leaves the packet on CPU fallback.
 */
int ipa_rtk_nat_pool_create(struct ipa_rtk_nat_pool *pool, struct ipa *ipa,
			   __be32 public_ip, u32 source_metadata, u32 destination_metadata);
/* TCP classifiers MUST exclude FIN/RST; bit14 redirects ordinary data on H5.
 * The pool only enables the NAT entry (bit15), never TCP redirect.
 */
int ipa_rtk_nat_pool_add(struct ipa_rtk_nat_pool *pool,
		const struct ipa_rtk_nat_tuple *tuple, struct ipa_rtk_nat_slot *slot);
int ipa_rtk_nat_pool_delete(struct ipa_rtk_nat_pool *pool, struct ipa_rtk_nat_slot *slot);
u32 ipa_rtk_nat_pool_timestamp(struct ipa_rtk_nat_pool *pool, const struct ipa_rtk_nat_slot *slot);
int ipa_rtk_nat_pool_destroy(struct ipa_rtk_nat_pool *pool);
#endif
