# Validation Summary

Date: 2026-10-29. Scope: one tested H5-200Q1 unit, OpenWrt 25.12.6,
Linux 6.12.112, and an LTE/5G NSA network. This is not hardware certification.

## Runtime Evidence

- Persistent squashfs root with a writable UBIFS overlay.
- Installed kernel/rootfs hashes matched their build artifacts; the independent
  recovery partition matched its preserved image.
- Standard sysupgrade preserved configuration. A short physical RESET press
  rebooted and retained settings.
- The owner confirmed the long RESET test and restoration of SIM settings.
  At handoff the test marker was absent, the new SSH host key was independently
  verified through the physical USB console, and mobile data worked. The
  interval without SIM configuration was not directly observed at handoff.
- All three SIM PIN1 attempts remained available after the tests.
- Separate LAN-client tests passed DHCP, DNS, NAT, ping and HTTP transfer.
- LTE cell lock and subsequent unlock passed readback and connectivity tests.

The corrected NAND driver passed three modem stop/start concurrency rounds:
14 complete rootfs reads and 171 three-page samples matched expected hashes,
with no ECC, xz or squashfs errors. Patch 118 backports the upstream QPIC v2
last-codeword read-location fix required by the OpenWrt kernel patch set.

A later live check showed LTE alone while idle. During a successful bounded
10 MB download, serving-cell and aggregation queries reported NR5G-NSA and an
NR band n78 secondary carrier alongside LTE B1/B3. This establishes NR carrier
activation, not a measurement of how individual bytes were split across LTE
and NR or a throughput benchmark. No radio configuration was changed.

## IPA v5.0 + Realtek RTL8125B Hardware Offload & Stress Test

Full stock-equivalent hardware NAT and `nf_flowtable` packet offload (`stage 4`,
`[HW_OFFLOAD]`) was implemented across `drivers/net/ipa/` (`120-ipa-h5-rtk-backend.patch`),
`package/kernel/r8125` (`ENABLE_LIB_SUPPORT=y`), `package/kernel/h5-ipa-rtk`
(`h5_ipa_rtk.ko`), and `firewall4` (`010-fw4-filter-hw-offload-devices.patch`).

Key hardware and firmware requirements verified on H5:
- **Modem DL Endpoint (`EP 15`)**: On SDX65 firmware `912.001.604.145`, downlink
  cellular IPv4 packets arrive on `EP 15` (`Q6_DL_NLO_DATA_PROD`), not `EP 12`
  (`Q6_DL_DATA_PROD`), while `EP 5` (`Q6_DL_NLO_LL_DATA_PROD`) also claims a filter
  bitmap slot (`BIT(5)`), shifting `EP 8` (`AP_LAN_TX`) to filter slot 3.
- **Exception Status LPM Wake Fix (`op=8` on `dst=19`)**: When `EP 8` (`AP_LAN_TX`)
  has `status_enable=true`, packets routed by hardware rules to `EP 19` (`Q6_UL_PROD`)
  generate status `opcode=8` (low-power/sleep wake notification) on `EP 9` (`AP_LAN_RX`)
  with `STATUS_DST_ENDPOINT=19`. Reinjecting `dst=19` status frames into `netif_receive_skb()`
  caused duplicate TCP/UDP packets and masquerade port collisions (`sport` rewritten to
  `1024`). Dropping `dst == 19` in `ipa_endpoint_rtk_exception()` keeps the first packet
  on the normal `br-lan` software path while `nf_flowtable` transitions the connection to
  `[HW_OFFLOAD]`.
- **QMI `0x0037` Extended Filter Rule Control Plane**: Downlink filter rules on `EP 15`
  are installed via `QMI_IPA_INSTALL_FILTER_RULE_EX_REQ_V01` (`0x0037`) with stock-exact
  fields (`ip_type=1`, `filter_action=2` `QMI_IPA_FILTER_ACTION_NAT`, `is_rule_hashable=1`,
  `route_table_index=11`, `rule_id=513+slot`) followed by `0x0024`
  `QMI_IPA_FLTR_INSTALLED_NOTIF_REQ_V01`.

### Multi-Stream Throughput & CPU Stress Benchmark

A concurrent 8-stream TCP HTTP download stress test (`8 x 10 MB` from `162.159.140.220:80`
to a LAN client at `192.168.0.2` through H5's `qmimux0` WAN and `eth0` LAN) was executed
and compared against the earlier software forwarding baseline:

| Mode | Workload | Wire Throughput | Transfer Volume / Time | H5 CPU Busy | H5 `softirq` | H5 `idle` | Offload State |
| --- | --- | --- | --- | --- | --- | --- | --- |
| Software routing (`r8169` / CPU) | Multi-stream HTTP download | 85.1 – 97.6 Mbps | 40 – 64 MB | ~95 – 100% | Majority of CPU | ~0 – 5% | Software only |
| IPA + RTL8125B HW Offload (`stage 4`, warmup) | 4 concurrent TCP streams | **62.43 Mbps** | 12.95 MB in 1.66 s | Low | Minimal | High | `active_flows=4` |
| IPA + RTL8125B HW Offload (`stage 4`, stress) | 8 concurrent TCP streams (`8 x 10 MB`) | **115.28 Mbps** | 53.73 MB (`35,504` pkts) in 3.73 s | **11.8%** | **0.5%** | **88.2%** | `active_flows=8`, `invalid=0` |

During the **115.28 Mbps** transfer (`stage 4`), `/proc/stat` recorded
`ticks=[user=52, nice=0, sys=11, idle=491, iowait=0, irq=0, softirq=3, steal=0]`
across the sampling window, confirming that data-plane packets bypass the Cortex-A7
kernel network stack (`0.5% softirq`) and travel directly between the modem, IPA v5.0,
and RTL8125B hardware rings (`lib_rx_ring[1]` / `lib_tx_ring[1]`).

A complete `cpe-squashfs-sysupgrade.bin` image built from this overlay (bundling
`kmod-r8125-rss`, `kmod-h5-ipa`, `kmod-h5-ipa-rtk`, and the `firewall4` L3 device
offload fix directly in `/rom`) was flashed via standard `sysupgrade`, booted cleanly
from NAND with zero module overrides in `/overlay`, automatically entered `stage 4`
with `flowtable ft { devices = { "eth0", "qmapmux0.0" }; flags offload; }`, and passed
live TCP/UDP `[HW_OFFLOAD]` forwarding as well as `stage 4 -> 0 -> 4` cycling under
active offloaded flows.

### Constraints & Pitfalls of Higher Parallel Flow Limits (`IPA_RTK_FLOW_LIMIT`)

The driver defaults to `IPA_RTK_FLOW_LIMIT = 16` simultaneous hardware-offloaded
5-tuples (excess flows transparently remain on Linux software `nf_flowtable` / normal
routing). Raising `IPA_RTK_FLOW_LIMIT` significantly higher carries four hardware and
protocol trade-offs:

1. **NAT Table Birthday-Paradox Hash Collisions (`ipa_rtk_nat_pool`)**:
   The hardware IPv4 NAT table allocated in DMA memory has `4096` entries (`12-bit` index)
   with `expansion_count = 0` (no collision chaining in the direct index table). Each flow
   hashes both `(private_ip, private_port, target_ip, target_port, proto)` and
   `(target_ip, target_port, public_ip, public_port, proto)`. By the birthday paradox,
   the probability that at least one new flow collides with an existing slot is
   approximately $1 - e^{-N^2 / 4096}$ (~6% at $N=16$, ~32% at $N=40$, ~63% at $N=64$).
   Colliding flows are safely rejected with `-ENOSPC` and fall back to software forwarding.
2. **QMI `0x0037` Full-Table Rewrite Latency**:
   `QMI_IPA_INSTALL_FILTER_RULE_EX_REQ_V01` (`0x0037`) is a full-table replacement RPC:
   every flow insertion or removal sends the entire active rule set (each `h5_filter_spec_ex_type`
   is 216 bytes, up to `H5_QMI_IPA_MAX_FILTERS_EX = 192` / ~33 KB payload) over QRTR to
   the modem DSP, which rebuilds the modem-side IMEM filter table and responds with `0x0024`.
   At high flow counts ($N \ge 64$), connection bursts incur higher per-flow QMI serialization
   and DSP table-rebuild latency.
3. **AP Header Insertion SRAM Capacity (`IPA_MEM_AP_HEADER`, 480 bytes)**:
   The AP-owned header insertion partition (`offset 0x0528`, `size 0x01e0` = 480 bytes)
   holds 30 slots of 16 bytes (slot 0 reserved, leaving **29 usable 14-byte Ethernet L2
   header slots**). Because `h5_ipa_rtk` deduplicates and reference-counts headers by
   `(dst_mac, src_mac)`, all flows to the same LAN host or downstream L2 switch share a
   single header slot; however, more than 29 simultaneous distinct LAN client MAC addresses
   cannot be offloaded in hardware at the same time.
4. **AP Route Table Ping-Pong Buffer Size (`offload_buf`, 1024 bytes)**:
   Route table index `11` (`LAN_RTK`) uses two 1024-byte DMA ping-pong buffers. Each unique
   `(private_ip, header_index)` pair emits one 24-byte route rule (plus an 8-byte terminator),
   bounding simultaneous distinct hardware-offloaded LAN IPv4 hosts to `42` per 1 KB buffer
   (multiple flows from the same LAN IPv4 share one route rule).

## Source Checks

The exported patch applied to a temporary clean index at the exact OpenWrt
revision in README.md. All three packages-feed patches passed clean-index
application checks. Existing whitespace warnings in feed patches 020 and 030
did not prevent application. The original build artifact manifest matched.

The test applies patches without changing the build server's working sources.

## Local Rebuilds

A clean kernel rebuild completed with exit code 0 using the same CPE profile
and driver/modem fixes, a neutral builder/host identity, and compiler prefix
mapping. Both images passed decompressed-content privacy checks and the
existing production image validator. The NAND source file and Linux kernel
configuration matched the preceding build. Images and dependency archives are
not distributed; the project publishes its source overlay and build instructions.

The initial privacy-checked candidate was not flashed. A subsequent CPE
rebuild corrected a USB-dependent cold-start failure: preinit could block on
its synchronous failsafe-prompt write to an active but disconnected ttyGS0.
Both the DTB and Android header now select ttyMSM0 as the CPE boot console;
USB CDC ACM remains available through the normal authenticated login helper.

The console-fixed rebuild was installed using standard sysupgrade with
configuration preserved. Independent recovery was updated through fastboot,
followed by the required normal reboot. Installed kernel/rootfs hashes matched;
the recovery image's complete byte range matched a NAND readback. Do not assume
that the unused partition tail is all 0xff when comparing a whole-partition hash.

Physical cold boots without USB passed for both the persistent squashfs/UBIFS
system and recovery initramfs. LAN forwarding started at about 50 and 70 seconds,
respectively; HTTP and SSH worked. Recovery had no active failsafe, did not mount
the main overlay, and returned by normal reboot to the persistent system with
its preserved SSH host key. Runtime kernel identity was `builder@buildhost`.
Holding RESET for ten seconds also selected Linux failsafe; releasing it after
2-3 seconds completed normal NAND recovery startup on the tested device.

The newer build passed the production image checks, including UART-only CPE
boot arguments in both images and matching DTBs. The new console regression
check rejected the old USB-console candidate. This is not a repeat of every
earlier radio/concurrency test or a completed new privacy audit. No binaries
are distributed. Later OpenWrt versions still need their own port/build/tests.

The generalized first-install check passed shell syntax and early-rejection
tests for missing arguments, malformed checksums and a missing image. Its
successful hardware path and the complete published stock installation guide
have not been retested on a fresh stock device. It is check-only; the guide
keeps the final sysupgrade as a separate manual command.

## Not Validated

- Different hardware or original firmware revisions.
- IPv6 service on a carrier that actually provides it.
- 5G SA registration or SA cell lock.
- Bridge/IP passthrough.
- Automatic stock-to-OpenWrt installation for third-party devices.
- Bit-for-bit reproducibility or privacy of a new third-party build.