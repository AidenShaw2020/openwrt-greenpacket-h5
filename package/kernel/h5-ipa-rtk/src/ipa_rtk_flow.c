// SPDX-License-Identifier: GPL-2.0-only
#include <linux/etherdevice.h>
#include <linux/in.h>
#include <linux/ip.h>
#include <linux/tc_act/tc_csum.h>
#include <net/flow_offload.h>
#include "ipa_rtk_flow.h"

/* Native flow_action MANGLE words are byte copies, not host-order IPs.
 * Applying them as native words also handles the two half-word MAC writes.
 */
static void mangle_word(void *bytes, u32 mask, u32 value)
{
	u32 word;
	memcpy(&word, bytes, sizeof(word));
	word = (word & mask) | value;
	memcpy(bytes, &word, sizeof(word));
}

int ipa_rtk_flow_decode(const struct flow_rule *rule, struct ipa_rtk_flow *out)
{
	const unsigned long long required = BIT_ULL(FLOW_DISSECTOR_KEY_META) |
		BIT_ULL(FLOW_DISSECTOR_KEY_CONTROL) | BIT_ULL(FLOW_DISSECTOR_KEY_BASIC) |
		BIT_ULL(FLOW_DISSECTOR_KEY_IPV4_ADDRS) | BIT_ULL(FLOW_DISSECTOR_KEY_PORTS);
	struct flow_match_meta meta;
	struct flow_match_control control;
	struct flow_match_basic basic;
	struct flow_match_ipv4_addrs ipv4;
	struct flow_match_ports ports;
	struct flow_match_tcp tcp;
	struct flow_dissector_key_meta meta_mask;
	struct flow_dissector_key_control control_mask;
	struct flow_dissector_key_basic basic_mask;
	struct ipa_rtk_flow decoded = {};
	const struct flow_action_entry *action;
	__be32 addresses[2], translated[2];
	__be16 original_ports[2], translated_ports[2];
	u8 written[ETH_HLEN] = {};
	unsigned int i;
	bool redirect = false, checksum = false, source, destination;
	bool is_tcp;
	u32 checksum_flags;

	if (!rule || !out || !rule->match.dissector)
		return -EOPNOTSUPP;
	if (!flow_rule_match_key(rule, FLOW_DISSECTOR_KEY_BASIC))
		return -EOPNOTSUPP;
	flow_rule_match_basic(rule, &basic);
	is_tcp = basic.key->ip_proto == IPPROTO_TCP;
	if (rule->match.dissector->used_keys !=
	    (required | (is_tcp ? BIT_ULL(FLOW_DISSECTOR_KEY_TCP) : 0)))
		return -EOPNOTSUPP;
	flow_rule_match_meta(rule, &meta);
	flow_rule_match_control(rule, &control);
	flow_rule_match_basic(rule, &basic);
	flow_rule_match_ipv4_addrs(rule, &ipv4);
	flow_rule_match_ports(rule, &ports);
	meta_mask = *meta.mask;
	control_mask = *control.mask;
	basic_mask = *basic.mask;
	if (meta_mask.ingress_ifindex != U32_MAX ||
	    !meta.key->ingress_ifindex ||
	    control_mask.addr_type != U16_MAX ||
	    control.key->addr_type != FLOW_DISSECTOR_KEY_IPV4_ADDRS ||
	    basic_mask.n_proto != (__force __be16)U16_MAX ||
	    basic.key->n_proto != htons(ETH_P_IP) ||
	    basic_mask.ip_proto != U8_MAX ||
	    (!is_tcp && basic.key->ip_proto != IPPROTO_UDP) ||
	    ipv4.mask->src != (__force __be32)U32_MAX ||
	    ipv4.mask->dst != (__force __be32)U32_MAX ||
	    ports.mask->src != (__force __be16)U16_MAX ||
	    ports.mask->dst != (__force __be16)U16_MAX)
		return -EOPNOTSUPP;
	if (is_tcp) {
		flow_rule_match_tcp(rule, &tcp);
		/* Preserve native NF's FIN/RST exclusion in hardware equations.
		 * H5 NAT redirect bit14 also redirects normal TCP data; leave it
		 * clear and send control packets through the CPU classifier. */
		if (tcp.key->flags || tcp.mask->flags != htons(0x0005))
			return -EOPNOTSUPP;
	}
	checksum_flags = TCA_CSUM_UPDATE_FLAG_IPV4HDR |
		(is_tcp ? TCA_CSUM_UPDATE_FLAG_TCP : TCA_CSUM_UPDATE_FLAG_UDP);
	/* Reject extra metadata/control/basic restrictions we cannot enforce. */
	meta_mask.ingress_ifindex = 0;
	control_mask.addr_type = 0;
	basic_mask.n_proto = 0;
	basic_mask.ip_proto = 0;
	if (memchr_inv(&meta_mask, 0, sizeof(meta_mask)) ||
	    memchr_inv(&control_mask, 0, sizeof(control_mask)) ||
	    memchr_inv(&basic_mask, 0, sizeof(basic_mask)))
		return -EOPNOTSUPP;
	addresses[0] = ipv4.key->src;
	addresses[1] = ipv4.key->dst;
	original_ports[0] = ports.key->src;
	original_ports[1] = ports.key->dst;
	memcpy(translated, addresses, sizeof(translated));
	memcpy(translated_ports, original_ports, sizeof(translated_ports));
	decoded.ingress_ifindex = meta.key->ingress_ifindex;
	decoded.ethernet[12] = ETH_P_IP >> 8;
	decoded.ethernet[13] = ETH_P_IP & 0xff;
	flow_action_for_each(i, action, &rule->action) {
		u32 mask, value, offset;
		if (redirect)
			return -EOPNOTSUPP;
		switch (action->id) {
		case FLOW_ACTION_MANGLE:
			mask = action->mangle.mask;
			value = action->mangle.val;
			offset = action->mangle.offset;
			if (value & mask)
				return -EOPNOTSUPP;
			switch (action->mangle.htype) {
			case FLOW_ACT_MANGLE_HDR_TYPE_ETH:
				if (offset > 8 || offset % 4)
					return -EOPNOTSUPP;
				mangle_word(decoded.ethernet + offset, mask, value);
				mangle_word(written + offset, mask, ~mask);
				break;
			case FLOW_ACT_MANGLE_HDR_TYPE_IP4:
				if (mask || (offset != offsetof(struct iphdr, saddr) &&
				    offset != offsetof(struct iphdr, daddr)))
					return -EOPNOTSUPP;
				mangle_word(&translated[(offset - offsetof(struct iphdr, saddr)) / 4],
					    mask, value);
				break;
			case FLOW_ACT_MANGLE_HDR_TYPE_UDP:
			case FLOW_ACT_MANGLE_HDR_TYPE_TCP:
				if (action->mangle.htype != (is_tcp ? FLOW_ACT_MANGLE_HDR_TYPE_TCP :
							   FLOW_ACT_MANGLE_HDR_TYPE_UDP))
					return -EOPNOTSUPP;
				if (offset || (mask != (__force u32)htonl(0xffff) &&
				    mask != (__force u32)htonl(0xffff0000)))
					return -EOPNOTSUPP;
				mangle_word(translated_ports, mask, value);
				break;
			default:
				return -EOPNOTSUPP;
			}
			break;
		case FLOW_ACTION_CSUM:
			if (checksum || action->csum_flags != checksum_flags)
				return -EOPNOTSUPP;
			checksum = true;
			break;
		case FLOW_ACTION_REDIRECT:
			if (!action->dev)
				return -EINVAL;
			decoded.egress_ifindex = action->dev->ifindex;
			redirect = true;
			break;
		default:
			return -EOPNOTSUPP;
		}
	}
	if (!redirect || !checksum || memchr_inv(written, 0xff, 12))
		return -EOPNOTSUPP;
	source = addresses[0] != translated[0] || original_ports[0] != translated_ports[0];
	destination = addresses[1] != translated[1] || original_ports[1] != translated_ports[1];
	/* One IPA NAT entry cannot express simultaneous source and destination NAT. */
	if (source == destination)
		return -EOPNOTSUPP;
	decoded.destination = destination;
	decoded.nat.protocol = basic.key->ip_proto;
	if (source) {
		decoded.nat.private_ip = addresses[0];
		decoded.nat.private_port = original_ports[0];
		decoded.nat.public_ip = translated[0];
		decoded.nat.public_port = translated_ports[0];
		decoded.nat.remote_ip = addresses[1];
		decoded.nat.remote_port = original_ports[1];
	} else {
		decoded.nat.private_ip = translated[1];
		decoded.nat.private_port = translated_ports[1];
		decoded.nat.public_ip = addresses[1];
		decoded.nat.public_port = original_ports[1];
		decoded.nat.remote_ip = addresses[0];
		decoded.nat.remote_port = original_ports[0];
	}
	*out = decoded;
	return 0;
}
