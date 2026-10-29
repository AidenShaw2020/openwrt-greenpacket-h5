/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _IPA_RTK_DEBUG_H_
#define _IPA_RTK_DEBUG_H_
#include <linux/types.h>
struct ipa;
/* Temporary kernel-only diagnostic. No packet bytes are exposed to userspace. */
int ipa_rtk_test_tx_copy(struct ipa *ipa, u32 index, void *data, size_t capacity);
int ipa_rtk_test_nat_direction(struct ipa *ipa, bool destination);
/* Stop new external RX while keeping NIC TX/GSI alive to drain DL rules. */
int ipa_rtk_test_rx_pause(struct ipa *ipa);
#endif
