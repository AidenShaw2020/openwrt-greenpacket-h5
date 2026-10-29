/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _IPA_RTK_NAT_WIRE_H_
#define _IPA_RTK_NAT_WIRE_H_

#include <linux/types.h>
#ifdef __KERNEL__
#include <linux/byteorder/generic.h>
#else
#include <endian.h>
#define cpu_to_le32(x) htole32(x)
#define be32_to_cpu(x) be32toh(x)
#define be16_to_cpu(x) be16toh(x)
#endif

/* Wire layout from Qualcomm ipanat ipa_nat_rule; use words, not C bitfields. */
struct ipa_rtk_nat_entry {
	__le32 word[8];
};

struct ipa_rtk_nat_tuple {
	__be32 private_ip;
	__be32 remote_ip;
	__be32 public_ip;
	__be16 private_port;
	__be16 remote_port;
	__be16 public_port;
	__u8 protocol;
	__u8 pdn;
};

static inline __u16 ipa_rtk_nat_checksum_delta(__u32 public_ip, __u16 public_port,
					     __u32 private_ip, __u16 private_port)
{
	__u32 sum = (public_ip & 0xffff) + (public_ip >> 16) + public_port;

	sum += (~private_ip & 0xffff) + ((~private_ip) >> 16);
	sum += (__u16)~private_port;
	while (sum >> 16)
		sum = (sum & 0xffff) + (sum >> 16);
	return sum;
}

static inline __u16 ipa_rtk_nat_destination_hash(const struct ipa_rtk_nat_tuple *t,
					       __u16 mask)
{
	__u32 remote = be32_to_cpu(t->remote_ip);
	__u32 public = be32_to_cpu(t->public_ip);
	__u16 hash;

	hash = remote ^ (remote >> 16) ^ public ^ (public >> 16) ^
		be16_to_cpu(t->remote_port) ^ be16_to_cpu(t->public_port) ^ t->protocol;
	hash &= mask;
	return hash ? hash : mask;
}

static inline __u16 ipa_rtk_nat_source_hash(const struct ipa_rtk_nat_tuple *t, __u16 mask)
{
	__u32 remote = be32_to_cpu(t->remote_ip);
	__u32 private = be32_to_cpu(t->private_ip);
	__u16 hash;

	hash = remote ^ (remote >> 16) ^ private ^ (private >> 16) ^
		be16_to_cpu(t->remote_port) ^ be16_to_cpu(t->private_port) ^ t->protocol;
	hash &= mask;
	return hash ? hash : mask;
}

static inline void ipa_rtk_nat_encode(struct ipa_rtk_nat_entry *entry,
				     const struct ipa_rtk_nat_tuple *t,
				     __u16 next, __u16 flags)
{
	__u32 public = be32_to_cpu(t->public_ip);
	__u32 private = be32_to_cpu(t->private_ip);
	__u16 public_port = be16_to_cpu(t->public_port);
	__u16 private_port = be16_to_cpu(t->private_port);
	__u16 ip_delta, transport_delta;

	ip_delta = ipa_rtk_nat_checksum_delta(public, 0, private, 0);
	transport_delta = ipa_rtk_nat_checksum_delta(public, public_port, private, private_port);
	entry->word[0] = cpu_to_le32(private);
	entry->word[1] = cpu_to_le32(be32_to_cpu(t->remote_ip));
	entry->word[2] = cpu_to_le32(next | (__u32)public_port << 16);
	entry->word[3] = cpu_to_le32(private_port | (__u32)be16_to_cpu(t->remote_port) << 16);
	entry->word[4] = cpu_to_le32(ip_delta | (__u32)flags << 16);
	entry->word[5] = cpu_to_le32((__u32)t->protocol << 24);
	entry->word[6] = 0;
	entry->word[7] = cpu_to_le32((__u32)(t->pdn & 0xf) << 12 |
				   (__u32)transport_delta << 16);
}

#endif
