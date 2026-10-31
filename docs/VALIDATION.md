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

### Capacity implementation (2026-10-29)

The revised IPv4 offload implementation replaces the original 16-flow limit with
an admission limit of 4000 paired connections and 200 distinct LAN IPv4 clients.
These are resource limits, not a measured guarantee of throughput for 200 users.
IPv6 continues through the software path.

The exact H5 stock configuration requests 4000 NAT entries in HYBRID mode. Its
original NAT library calculates a DDR layout of 4096 base and 800 expansion
entries. The revised port uses that DDR layout with independent destination NAT
and source-index collision chains. Exhaustion of either expansion graph can
occur before the 4000 admission limit; those connections remain in software.
Duplicate lookup keys are rejected, and uncertain DMA publication failures
quarantine the pool until reset. Head deletion preserves surviving successors.

Two fixed modem downlink policies (UDP and TCP excluding FIN/RST) replace the
per-connection QMI table updates. Exact translations are still authorized by
Linux flowtable callbacks. Hardware NAT miss handling and unsolicited traffic
must be verified during the device test before treating this policy as validated.

Connection cookies and normalized tuples use separate hash indexes; periodic
hardware statistics do not scan all 4000 entries. Unpaired directions use a
bounded pending queue and expire without evicting active Linux-authorized flows.

Client routes are reference-counted separately from connections and rebuilt
only when a client appears or disappears. Two 8192-byte route buffers replace
the original 1024-byte buffers. Two hundred system-memory header processing
contexts use the stock IPA5 NONE/HDR_ADD format with absolute header addresses.
The system context base is leased only when unused; modem-owned DDR is never
followed or replaced. Routes are drained before a context slot is reused.

The production NAT pool and manager passed host tests with ASan/UBSan:

- 200 clients deliberately sharing both hash buckets;
- 15000 insertion/deletion churn operations with independent wire-graph lookups;
- all 800 expansion entries, exhaustion, duplicate keys and head promotion;
- 4000 paired connections, deletion, pending-pair expiry and callbacks after stop;
- injected publication-command failures retaining uncertain DMA state;
- production client/header/route code with 200 distinct MAC/IP destinations and
  4000 referencing flows, all system-context offsets, randomized teardown and
  failed route-publication retention.

The backend/controller passed ARM kernel compilation and the complete source
export passed a clean-index application check against the pinned vanilla OpenWrt.
A complete r4 image (expanding the external Realtek TX1 ring to 1024 descriptors,
adding a 2-second TCP hardware NAT teardown grace period across `NF_FLOW_TEARDOWN`,
and raising `net.core.netdev_max_backlog` to 4096 to eliminate `gro_cells_receive`
drops on `qmapmux0.0`) was installed with configuration preserved by sysupgrade:

- image SHA256: `0a7798049b37b179f2413ffda872018202b5750d0f89baff7666cc7f447cac10`;
- installed controller SHA256: `a7d19b94bc25fefbffc36491ae993bade6611f58436c52097d93a62c83581fb9`;
- installed IPA SHA256: `2a36612aad0ef3d07bda4df3e892bda098843a87fa1238171a26a0c940be37a9`.

The hashes were checked in the image and again in the device's read-only `/rom`.
Main-system boot, mobile registration, WAN data, stage4/failed0 and connection
cleanup passed. Independent recovery remained read-only and matched its earlier
verified SHA256 `63ee6f820c5eb66862bfc0fa85a966683e00d0ce7bb126151fba338cb663a968`.
Do not replace IPA while restarting an already-running modem: that diagnostic
attempt hit the modem HWP initialization timeout. Complete-image boots passed.

Two live batches of 200 simultaneous HTTPS downloads (1 MB each) completed
200/200. Both the driver and conntrack reported 200 simultaneous hardware flows.
After the r4 sysupgrade, a live 200-stream HTTPS batch transferred 200 MB in
7.431 s (215.32 Mbps including 200 TLS handshakes and connection setup/teardown)
with zero `qmapmux0.0` `rx_dropped` packets. In two earlier fully populated steady
samples, CPU was 23.42% and 11.93%, softirq 0.90% and 0.92%. Other samples include
setup, teardown and concurrent management work; aggregate CPU peaks alone are not a
measurement of forwarding cost. The device owner observed peaks while opening
LuCI.

A separate 4 MB HTTPS transfer advanced the dedicated Realtek external TX1
hardware tail/close counters from 63062 to 282, i.e. 2756 completions modulo
65536. This verifies actual payload movement through the IPA-to-Realtek ring in
addition to the software HW_OFFLOAD flags. Deliberately colliding TCP source
ports also completed 12/12 HTTP transfers in the preceding capacity revision.

Live tests used one physical LAN computer. Two hundred different MAC/IP clients
were exercised in the production-code model, not on 200 physical devices.

In 200-stream cleartext HTTP close/FIN bursts, raising `netdev_max_backlog` to
4096 eliminated `qmapmux0.0` `gro_cells_receive` queue drops (`rx_dropped` delta 0).
With the 1024-descriptor TX1 ring, single runs reached 200/200 (130.64 Mbps in
12.25 s) and 198/200. With the 2-second teardown grace, two runs completed only
185/200 and 181/200 without new queue drops. The cleartext HTTP failure remains
unresolved and its cause is not established.

Release r5 restricts the diagnostic `stage` attribute to root (mode 0600) and
stops the board default script from clearing a locked root password or
overwriting existing SSH/HTTPS settings after a config-preserving sysupgrade.
Its image (SHA256 `0a7c840aea52456375193452060c39c8010d2501c1ef23f7dd547424ba484e30`,
controller `ae40985be8928ad99700b1710e3e8e771b9929e80243152cf927a7d1f76bb46c`)
was installed by sysupgrade with configuration preserved. It reached stage 4 with
failed=0 automatically and completed a 200-stream HTTPS batch 200/200 at
183.07 Mbps in 8.740 s. The new default script was verified on copies of the
live configuration.

Unsolicited WAN UDP was tested on r5 through the carrier CGNAT with RFC 5780
STUN CHANGE-REQUEST replies from each server's alternate port/address while an
offloaded flow to its primary address was active. Temporary nftables counters
on `qmapmux0.0` confirmed arrival at H5. Against two independent servers, 20/20
probes arrived each time; all took the IPA exception path (`exception_delta`
20/21), Realtek TX1 did not advance, and the LAN client received none. A third
server's probes did not reach H5. This covers UDP from a new remote port to an
existing public mapping, not TCP, inbound traffic without a CGNAT mapping, or a
public-IP subscription.

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
- IPv6 passthrough; a public mobile IPv4 address in passthrough mode.
- Automatic stock-to-OpenWrt installation for third-party devices.
- Bit-for-bit reproducibility or privacy of a new third-party build.

## Native LuCI handler (2026-10-29)

The device owner reported 436/70 Mbps through the accelerated CPE and a brief
CPU peak when opening even the LuCI login page. The device exposes one CPU at
1497600 kHz with the performance governor. uhttpd was serving LuCI through
`/www/cgi-bin/luci`, starting the ucode CGI interpreter for requests; its config
retained the old Lua prefix and had no native ucode prefix/module.

Installed the matching `uhttpd-mod-ucode` package and configured the standard
`/cgi-bin/luci=/usr/share/ucode/luci/uhttpd.uc` handler. Only uhttpd restarted;
radio, kernel modules, firewall and IPA were unchanged. Five baseline login
requests averaged 0.368851 s. After switching, the first request was 0.314672 s
and four warm requests averaged 0.183894 s. Both modes returned the normal
HTTP403 login form; authentication fields remained present. The owner confirmed
that the page became substantially faster. These are response-time results,
not a measurement proving that all CPU peaks have disappeared.

The package is selected by all LuCI H5 image profiles and the three seed configs.
The idempotent board default adds the native handler to preserved configurations
without replacing a custom handler. The currently running device received the
package/config fix live; a complete image is also built for subsequent upgrades.
No causal connection with the separate cleartext HTTP transfer issue is established.

## IPv4 passthrough and LuCI concurrency (2026-10-31)

Passthrough is optional; the default remains the NAT router. It is selected
under Network -> IP Passthrough (`/etc/config/h5ippt`); saving triggers
`/etc/init.d/h5-ippt`, which applies the firewall, offload and DHCP changes.
The mobile address is removed from `qmapmux0.0`, the H5 takes the mobile
gateway address on br-lan and leases the mobile address to the configured
MAC with a two-minute lease. Unsolicited inbound IPv4 is forwarded to that
host only; traffic originated by the H5 is SNATed to the mobile address.

Tested with a test SIM behind carrier NAT and an OpenWrt router as the
passthrough host:

- The router received the mobile address and gateway, with working DNS and
  internet access; the H5 stayed reachable on its LAN address through it.
- After a reboot and a new mobile address (/29 to /30), the router obtained
  the new address automatically. A router holding a LAN lease from before the
  modem came up is forced to restart DHCP by a short LAN carrier drop.
- Switching router -> passthrough -> router through the LuCI save path (uci
  commit over ubus) restored masquerading and the mobile address on the H5
  in router mode, and the passthrough state on the way back, within ~25 s.

`kmod-h5-ipa-rtk` release 6 offloads the routed flows as identity NAT entries
(public = private = mobile address), selected by the new `passthrough_ip`
parameter because the modem netdev has no address. Hardware installs were
observed in the controller counters and `[HW_OFFLOAD]` conntrack entries.
Two 100 MiB downloads through the router returned identical SHA256 values;
OVH reached 186 Mb/s with negligible H5 softirq time, and `speedtest-go` on
the router measured 296.9/99.3 Mb/s with 0% packet loss. NAT-mode offload is
unchanged when the parameter is empty. Not tested: a public mobile address,
IPv6 passthrough, and inbound connections from the internet.

LuCI ucode handler: with `uhttpd-mod-ucode`, six concurrent requests to
`/cgi-bin/luci/` hung (two after 30 s, one 502 after 60 s, three never
finished), and a Chrome hard reload kept loading. `luci.dispatcher` opened
its ubus connection at import time in the uhttpd parent, so all forked request
processes shared one socket. With the per-request connection patch, eight
concurrent requests completed in 0.5-1.4 s and five successive hard reloads
in headless Chrome 156 rendered the menu with no pending requests.

## Boot time, idle load and reconnect robustness (2026-10-31)

Idle CPU load fell from about 3.5% to 0.6%. The radio LED helper polled
`mmcli` every 10 s (about 0.22 s CPU per poll, including ModemManager and
D-Bus). It now follows the mobile interface through hotplug and, only while
connected, asks the modem for the serving cell (`h5-at serving`, about 0.02 s
CPU) every 30 s. The 5G LED tracks an actual NR5G-NSA/SA serving cell.

Measured boot to mobile data: 174 s before, about 132 s after:

- NAND reads were 2.2 MB/s because `qcom_nandc` only enables the RPMh QPIC
  clock, leaving the minimum vote. The stock driver sets 100 MHz
  (`MSM_NAND_BUS_VOTE_MAX_RATE`); the DT now assigns that rate, giving
  4.9 MB/s raw and 2.1 MB/s through the stock firmware squashfs.
- `h5-modem` starts right after the network, asynchronously. Loading the
  modem firmware no longer holds back the remaining init scripts. Starting it
  before the network only delayed the LAN, as the load competes for NAND/CPU.
- ModemManager spent two 10 s timeouts on SUPL/assistance data requests the
  SoC QMI LOC service never answers. The qcom-soc plugin now reports no
  location capabilities (GNSS is unused on this CPE).
- The modem recreates its glink channels while starting, so a one-time DS
  bind could attach to a vanished channel. A rpmsg hotplug script binds DS to
  rpmsg_char on every add and creates `/dev/h5-at`.

After a ModemManager restart its mux link came back as `qmapmux1.0`. The
offload controller looked for `qmapmux0.0`, fell back to `rmnet_ipa0` and
Ethernet receive stopped reaching the CPU until reboot. netifd's l3_device is
now passed in (`wan_ifname`), and `h5-ippt` uses it as well. Repeated with the
fix: offload followed `qmapmux1.0`, the H5 stayed reachable and mobile data
returned 25 s after the restart. Each offload stage change still drops the
LAN link for about 3 s.
