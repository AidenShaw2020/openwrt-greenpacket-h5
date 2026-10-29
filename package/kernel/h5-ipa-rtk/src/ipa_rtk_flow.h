/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _IPA_RTK_FLOW_H_
#define _IPA_RTK_FLOW_H_
#include <linux/if_ether.h>
#include "ipa_rtk_nat_wire.h"

struct flow_rule;
struct ipa_rtk_flow {
	struct ipa_rtk_nat_tuple nat;
	int ingress_ifindex, egress_ifindex;
	u8 ethernet[ETH_HLEN];
	bool destination;
};
/* Only decode; never program hardware or retain a borrowed net_device. */
int ipa_rtk_flow_decode(const struct flow_rule *rule, struct ipa_rtk_flow *out);
#endif
