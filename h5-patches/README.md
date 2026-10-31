# GreenPacket H5-200Q1 on vanilla OpenWrt 25.12.6

This adds a new SDX65 target to unmodified OpenWrt; it does not use a RutOS
kernel, target, SDK or build environment. The tested device has stock software
912.001.604.145. Other H5 hardware or stock firmware revisions are not validated.

## Reproducible build

OpenWrt base: 64900e756efa667774214f9c6b17c113222e01f7 (v25.12.6),
Linux 6.12.112. Tested feed revisions:

- packages: 093d551675b1119a06ef224d419905e8d6b2af1a
- luci: 75a306df84a020c57a631ef5b4c02ace56d93387

From a clean OpenWrt checkout, apply the exported target patch, update feeds,
check out the above feed revisions, then install feeds. Run
`sh h5-patches/apply-packages.sh` to apply the included ModemManager fixes
and the LuCI ucode dispatcher fix (a per-request ubus connection; with the
shared inherited one, concurrent LuCI requests under uhttpd-mod-ucode hang).
The tested feed snapshot has an unrelated weechat-full/weechat-minimal Kconfig
cycle through the unused luci-app-weechat package; remove that feed symlink with
`./scripts/feeds uninstall luci-app-weechat` if all packages were installed.

Select the CPE profile:

```sh
cp target/linux/sdx/h5-cpe.config .config
make defconfig
make -j8
```

Outputs under bin/targets/sdx/generic:

- openwrt-sdx-generic-greenpacket_h5-200q1-cpe-initramfs-kernel.bin
- openwrt-sdx-generic-greenpacket_h5-200q1-cpe-squashfs-sysupgrade.bin

The initramfs includes the complete LuCI/modem stack. Both boot kernels use an
Android v0 header, 4096-byte pages and an appended DTB, as required by this ABL.
These are not GreenPacket web-update packages. Direct fastboot RAM boot of
the full Linux/LuCI/modem initramfs has been validated on the tested H5.
Earlier bring-up profiles remain available for diagnostics.

The CPE profile uses ttyMSM0 as its only boot console in both the DTB and
Android image header. USB ttyGS0 remains available for authenticated login,
but is not a boot console: OpenWrt preinit can otherwise block while writing
the failsafe prompt to a disconnected USB TTY. With this fix, both the main
system and independent NAND recovery completed physical cold boots without
USB. Main LAN forwarding started after about 50 seconds; recovery after about
70 seconds. At power-on, release RESET after about 2-3 seconds to select
recovery; holding it for ten seconds can also activate Linux failsafe at
192.168.1.1 instead of completing normal recovery startup at 192.168.0.1.

## Default operation

The default is a normal cellular CPE router: Ethernet LAN 192.168.0.1/24,
DHCP/DNS, and a firewalled/NAT cellular WAN named mobile. Automatic mobile
startup is enabled. Optional IPv4 passthrough to one LAN router is available
under Network -> IP Passthrough and is not enabled by default.

LuCI is available over HTTP. Fresh root has no password and Dropbear permits
password authentication. The USB CDC ACM and UART consoles use the standard
OpenWrt login helper with ttylogin enabled, so setting a root password also
protects console login. No private SSH keys, authorized keys, test SIM PIN,
autologin shell or private diagnostic overlay are part of the public target.

The native ModemManager interface handles APN, PIN and connection settings.
Configure only a verified SIM PIN; the standard netifd handler blocks repeated
restart attempts after a PIN error. No carrier identifiers or PINs are embedded
in the build.

Network -> Cellular Network adds standard LuCI tables for serving cells,
carrier aggregation and per-chain signal measurements, plus validated vendor
cell-lock controls. It uses a restricted rpcd API and /dev/h5-at, not a generic
web command shell. LTE cell lock to the current anchor and removal were tested
with working data in both states. The advertised NR lock is for 5G SA; it has
not been tested on the current NSA network. NSA uses the LTE anchor. Cell-lock
persistence across modem restarts is firmware dependent and is not promised.

## Hardware and firmware

RTL8125B uses upstream r8169 with the SDX65 GCC/PCIe bootstrap and stock PCIe
policy. 1 Gb/s LAN, upstream IPA/rmnet, QRTR/QMI and ModemManager provide the
integrated RG520N-EU transport. The DS AT service uses unmodified upstream
rpmsg_char through driver_override, avoiding ModemManager WWAN auto-probing.

Signed modem firmware is loaded from the original modem partition, mounted
read-only and checked against the tested modem.mdt hash. No modem firmware is
distributed in this patch. QRTR TFTP service4096 version3 is required for this
firmware; the generic package default remains unchanged. Runtime support files
are cached in RAM before starting MPSS.

BAM command-pipe LOCK/UNLOCK serializes AP/modem NAND access. The lock dummy
command uses the physical NAND_ADDR0 address and a zero write mask. A dummy
write to NAND_VERSION caused resets and must not be substituted. Both block
integrity and runtime firmware access were tested while the modem was active.
Patch 118 also backports the upstream QPIC v2 last-codeword read-location fix
for the v6.14 cleanup included by OpenWrt. The uncorrected driver returned
stale final 480-byte page tails during a captured modem-start test, despite
zero ECC errors. The corrected kernel passed three stop/start concurrency
rounds: 14 complete rootfs reads and 171 three-page samples all matched their
expected hashes, with zero ECC or squashfs errors.

The NAND master stays read-only. Only the CPE compatible and an exact verified
512 MiB/4 KiB-page/256 KiB-erase/33-partition map expose writable children:
boot (mtd27) and system (mtd31). All other children remain read-only. The secure
EFS range 0x00380000..0x01980000 is blocked through secure-regions. The sysupgrade
platform requires metadata, the CPE compatible and both writable children.
It uses the standard NAND sysupgrade tar with boot, system and rootfs volumes.

SYS blue/red GPIOs are 18/63, Ethernet 17, 4G 16 and 5G 19. Ethernet follows
link/activity; cellular LEDs follow ModemManager RAT. Reset is GPIO32, active
low with pull-up and 20 ms polling. Physical press/release events were verified;
the short press reboots while preserving configuration. The user completed
the long-press factory-reset test and confirmed that SIM settings were restored
afterwards. A post-test check found a new SSH host key, no test marker, and
working mobile data; the configuration-free interval was not captured during
the subsequent handoff.

## Validation and installation status

The first clean CPE initramfs booted through the stock-kernel RAM handoff.
HTTP LuCI and Cellular Network browser checks passed, including LTE/NR values,
cell-lock form rendering and absence of JavaScript errors. Root blank-password
SSH worked. Mobile autostart connected to O2 CZ with the previously verified
private SIM configuration. Live NAND flags allowed only boot/system; concurrent
4 KiB and 1 MiB boot reads matched the original backup. sysupgrade -T accepted
the generated image without writing flash.

A previous integrated RAM image also passed a separate LAN client DHCP, DNS,
HTTP and NAT test, and LTE cell-lock apply/readback/data/restore tests. IPv6
service was not supplied by the test carrier and is not hardware validated.

The GZIP CPE initramfs now also boots directly through a freshly restarted
fastboot, without the stock kernel. USB console root login, blank-password SSH,
HTTP LuCI, 1 Gb/s Ethernet, modem startup, SIM unlock with verified private
settings and O2 data passed. A separate LAN namespace passed DHCP, DNS, NAT
and HTTP through that mobile connection. The image builder places aliases and
chosen before memory, preserving all DTB properties, and gives initramfs an
explicit RAM root. Failed image attempts can affect subsequent bootloader
tests; restart directly into fastboot before each independent RAM trial.

The full standard initramfs also booted from NAND recovery after clearing the
vendor fastboot flag described below. USB root login, blank-password SSH,
HTTP LuCI/Cellular Network login, 1 Gb/s Ethernet, LTE/5G NSA data and a separate
LAN client's DHCP/DNS/NAT/HTTP passed. No native-DTB handoff shim was required.
Persistent installation into boot/system has passed: the installed kernel,
rootfs and preserved recovery readback hashes match; normal startup uses
squashfs with a writable UBIFS overlay. A software reboot preserved the
configuration and automatically started the modem and AT statistics.
Stock boot/system have been replaced; the verified original backups remain
private. A subsequent standard sysupgrade to the corrected kernel preserved
the overlay marker, SSH host key and verified private SIM configuration;
kernel/rootfs/recovery readbacks passed again. Mobile data, a separate LAN
client's DHCP/DNS/NAT/HTTP, native AT statistics, and LTE cell-lock
apply/readback/data/unlock passed on the installed corrected system.
The short physical RESET press rebooted and preserved configuration.
The user also confirmed the long-press RESET test and subsequent restoration
of SIM settings. Post-test kernel/rootfs/recovery readbacks matched, mobile
data worked, and all three PIN1 attempts remained available.
The CPE DT supplies the original board's
SCM download register (TCSR + 0x13000), so the upstream SCM shutdown handler can
clear it. With this correction, normal software reboot from the RAM image
with active LTE/5G NSA data returned to the unchanged stock system without
any physical power cycle. The bootloader restart target passed with MPSS
offline. Original-stock software recovery entry and physical RESET recovery
entry passed; OpenWrt's software recovery target remains unvalidated.

The original ABL reads GPIO32 during startup. Two low readings select LAN
recovery (boot mode6 / recovery=2); this was established from this device's
saved ABL, not just a related vendor reference. Physical recovery entry is
verified with the original recovery service listening on TCP 5000.

The vendor's fastboot flash/erase handlers persist `fastboot-abnormal` in
misc. At the next startup this flag bypasses kernel loading, including RESET
recovery selection, and launches fastboot directly. After flashing recovery,
issue a normal `fastboot reboot` before a physical recovery test. The normal
reboot handler clears this flag; `fastboot reboot bootloader` does not.
Negative cold tests performed without clearing it do not establish a Linux,
DTB or image-size failure. This behavior was verified in the saved device's
ABL and by reading misc; the original recovery booted after clearing it.

Installation was performed only after the independent OpenWrt NAND recovery
and stock restoration prechecks had passed. The standard upgrade hook stops
MPSS before changing boot/system and includes its tools in the stage2 RAM root.

All 32 nonsecure original NAND partitions have private backups with gzip CRC
and decompressed SHA256 verification. EFS and raw OOB are excluded; this is not
a raw full-chip clone. Keep device-specific backups outside the public patch.

## Porting to another OpenWrt version

The exported patch is validated against the pinned 25.12.6 revision, not every
future branch. Start from a clean checkout of the desired version and run
`git apply --check` before applying it. If that target or any included package
already exists upstream, reconcile the implementation instead of overwriting
it. Review the target's kernel version, files-6.12 and patches-6.12 directories,
kernel APIs/configuration, and packages/LuCI feed compatibility. Backports may
already be upstream and must not be applied twice. Keep the DTB and Android
header boot-console selections consistent, including the recovery initramfs.

After adapting the overlay, export a new patch against that clean version,
verify application in a second clean checkout, build both CPE images, and test
sysupgrade, cold boot without USB, recovery and modem/NAND integrity. A clean
patch-application check alone is not proof of runtime compatibility.
