# OpenWrt for GreenPacket H5-200Q1

An independent OpenWrt 25.12.6 / Linux 6.12.112 port for the GreenPacket
H5-200Q1 with a Qualcomm SDX65 platform and integrated Quectel RG520N-EU modem.
Maintained by [AidenShaw2020](https://github.com/AidenShaw2020).

The tested device runs from its main NAND boot/system partitions with a
read-only squashfs root and writable UBIFS overlay. The default configuration
is a cellular router, not a bridge. This project is not affiliated with
GreenPacket, Quectel, Qualcomm, or the OpenWrt project.

## Release Contents

This is a **source-only project**: the target, custom packages, kernel/feed
patches, build configuration, an exported vanilla OpenWrt patch, and build
and installation instructions. Build your own images using the pinned
upstream sources. No prebuilt firmware, dependency source archive, vendor
firmware or device backup is distributed here.

A follow-up local CPE rebuild with the USB boot-console fix was installed by
sysupgrade with configuration preserved. Both the main system and independent
NAND recovery passed physical cold boots without USB. Earlier radio and
concurrency results refer to the preceding build with the same driver/modem
fixes. This remains an experimental port; a new third-party build is not certified.
See [Privacy Review](docs/PRIVACY.md) and [Build Inputs](docs/SOURCES.md).

- `target/linux/sdx/`: target, board descriptions, image builder and kernel patches.
- `package/`: custom modem tools, LuCI application and TFTP service integration.
- `h5-patches/`: patches for the packages feed and their application script.
- `h5-openwrt-25.12.6-bringup.patch`: complete overlay as a patch against vanilla OpenWrt.
- `tools/h5-install-check.sh`: read-only first-install preflight for OpenWrt RAM boot.
- `docs/STOCK_INSTALL.md`: stock-to-OpenWrt installation and recovery procedure.
- `SHA256SUMS`: checksum of the exported patch, not a checksum of your own build.

The repository layout is an overlay, **not** a complete OpenWrt checkout.
Use the exported patch once; do not also copy the same overlay on top.

## Tested Functionality

- Persistent boot from NAND and standard sysupgrade preserving configuration.
- Independent OpenWrt initramfs recovery and readback verification.
- Main and recovery cold boot without USB; USB serial remains a login console.
- RTL8125B Ethernet at 1 Gb/s, LAN DHCP/DNS, firewall, NAT and client HTTP traffic.
- Mobile data through IPA/rmnet, QRTR/QMI and ModemManager.
- LTE and 5G NSA, including an active NR band n78 secondary carrier under load.
- LuCI modem configuration, serving cells, carrier aggregation and signal tables.
- LTE cell lock, readback, data connectivity and return to the unlocked state.
- Short RESET press rebooting without losing configuration.
- Long RESET test and subsequent settings restoration confirmed by the device owner.

Only the tested H5-200Q1 hardware with original firmware revision
`912.001.604.145` has been validated. Other revisions are not certified.
IPv6 service and 5G SA cell lock are not hardware validated. Bridge/IP
passthrough is not implemented. See [Validation](docs/VALIDATION.md).

5G NSA uses an LTE anchor. In an idle state the modem may report only LTE
serving/carrier information; NR can be added during traffic. A ModemManager
`lte, 5gnr` label alone is not proof of an active NR carrier. Confirm actual
NR attachment in the Cellular Network serving-cell and carrier tables.

## Build

Use a Linux host with the normal
[OpenWrt build prerequisites](https://openwrt.org/docs/guide-developer/toolchain/install-buildsystem).
Pin these revisions:

| Source | Revision |
| --- | --- |
| OpenWrt v25.12.6 | `64900e756efa667774214f9c6b17c113222e01f7` |
| packages feed | `093d551675b1119a06ef224d419905e8d6b2af1a` |
| LuCI feed | `75a306df84a020c57a631ef5b4c02ace56d93387` |

```sh
git clone https://github.com/AidenShaw2020/openwrt-greenpacket-h5.git
git clone https://git.openwrt.org/openwrt/openwrt.git
cd openwrt
git checkout 64900e756efa667774214f9c6b17c113222e01f7
git apply --check ../openwrt-greenpacket-h5/h5-openwrt-25.12.6-bringup.patch
git apply ../openwrt-greenpacket-h5/h5-openwrt-25.12.6-bringup.patch
./scripts/feeds update packages luci
git -C feeds/packages checkout 093d551675b1119a06ef224d419905e8d6b2af1a
git -C feeds/luci checkout 75a306df84a020c57a631ef5b4c02ace56d93387
./scripts/feeds install -a -p packages
./scripts/feeds install -a -p luci
./scripts/feeds uninstall luci-app-weechat
sh h5-patches/apply-packages.sh
cp target/linux/sdx/h5-cpe.config .config
make defconfig
make -j"$(nproc)" download
make -j"$(nproc)" CONFIG_KERNEL_CFLAGS="-ffile-prefix-map=$PWD=/build/openwrt -fdebug-prefix-map=$PWD=/build/openwrt -fmacro-prefix-map=$PWD=/build/openwrt"
```

The unused `luci-app-weechat` feed entry is removed to avoid a dependency cycle
in the pinned feed snapshot. The seed sets the neutral kernel build identity.
The build downloads the original upstream sources directly. Dependency source
archives and prebuilt images are not uploaded to this repository or its releases.
Device-specific modem firmware is read from the unit, not distributed here.

Expected outputs in `bin/targets/sdx/generic/`:

- `openwrt-sdx-generic-greenpacket_h5-200q1-cpe-initramfs-kernel.bin`
- `openwrt-sdx-generic-greenpacket_h5-200q1-cpe-squashfs-sysupgrade.bin`

Use a generic build directory and audit both images before redistributing
them. Setting the kernel build user/domain does not by itself guarantee that
all absolute build paths or third-party metadata are removed. A new build is
not automatically byte-identical to the privately tested images.

## Porting to Newer OpenWrt

The patch filename records its **tested base**, not automatic compatibility
with later versions. Keep this repository and its overlay as the local source
of truth. From a clean checkout of the desired OpenWrt revision, first run:

```sh
git apply --check ../openwrt-greenpacket-h5/h5-openwrt-25.12.6-bringup.patch
```

If the check fails, adapt the overlay to that revision in a disposable checkout;
do not force it over existing target/package files. Review the target kernel
version, `files-6.12`/`patches-6.12`, kernel APIs and configuration, and each
packages/LuCI feed patch. Drop backports already present upstream. Preserve
the UART boot-console selection in both the CPE DTB and Android image header;
USB login must not become a required boot dependency again.

After staging only the intended port changes in that disposable checkout,
generate a version-specific patch with `git diff --cached --binary`. Check it
in a second clean checkout of the same revision, build both images, and repeat
sysupgrade, main/recovery cold boot without USB, and modem/NAND integrity tests.
Record the new OpenWrt/feed revisions and patch checksum. Successful patch
application alone does not establish build or hardware compatibility.

## Installation and Recovery

**Do not upload these files to the original GreenPacket web updater.**
They are not vendor update packages. Incorrect NAND writes can brick the unit.

For a first installation from stock, follow
[Stock Installation](docs/STOCK_INSTALL.md). It covers stock access prerequisites,
RAM boot, private backups, independent recovery verification, preflight and
the final sysupgrade. The helper is deliberately check-only, not a one-click
stock exploit or automatic flasher. Never use another unit's calibration or
identity partitions. Original signed modem firmware stays on the device.

For a device **already running this persistent CPE port**, use the
`cpe-squashfs-sysupgrade.bin` image through LuCI's standard firmware upgrade,
keeping configuration when desired. Do not select the initramfs image for
sysupgrade. Do not disconnect power while flashing. The platform upgrade
hook stops the modem and writes only the main boot/system partitions;
the independent recovery and other partitions remain protected.

On the tested installation, independent OpenWrt recovery is entered by
powering off the unit, holding RESET while restoring power, and releasing
RESET after approximately **2-3 seconds**. Holding it for ten seconds can also
trigger OpenWrt failsafe (rapid red SYS blinking, `192.168.1.1`). Ensure all
power sources, including USB, are removed before starting. Normal recovery
uses `192.168.0.1` and reports `rootfs_type: initramfs`. This only boots whatever
is installed in the recovery
partition; it does not install OpenWrt recovery or restore stock firmware.

## Operation

- LAN and HTTP LuCI: `http://192.168.0.1/`.
- SSH: `root@192.168.0.1`; fresh defaults have no root password.
- Set a strong root password immediately and keep management on a trusted LAN.
- Configure APN and your verified PIN under Network > Interfaces > mobile.
- View radio information and cell lock under Network > Cellular Network.
- Short RESET press, less than one second: reboot, preserving settings.
- RESET held for at least five seconds while OpenWrt is running, then released:
  factory reset of OpenWrt settings, including saved PIN/APN.

An OpenWrt factory reset does not restore the vendor firmware. Never guess a
SIM PIN or repeatedly retry a rejected PIN. HTTP is intentionally enabled;
use a trusted management network and configure HTTPS before untrusted access.

## License and Credits

The overlay's original, otherwise unmarked code and documentation are
GPL-2.0-only. Explicit per-file licenses take precedence, including
BSD-3-Clause device-tree files. Upstream patches and dependencies retain their
original licenses and notices. This does not relicense OpenWrt or dependencies.
See [LICENSE](LICENSE), [Licensing](LICENSES/README.md), and
[Third-Party Notices](THIRD_PARTY_NOTICES.md).

No device backups, SIM credentials, SSH private keys, vendor modem firmware,
or raw personal diagnostic logs are included. Hardware modification and
firmware installation are at your own risk; there is no warranty.