# Third-Party Notices

This project builds on the work of the following projects and their contributors.
Existing notices in imported context and patches are retained.

- [OpenWrt](https://openwrt.org/): build system, base system and package recipes;
  the pinned base is identified in README.md. OpenWrt is a mixed-license
  distribution; its component licenses are not replaced by this repository's LICENSE.
- [Linux](https://kernel.org/): Linux 6.12.112 with the OpenWrt patch set and
  the included SDX65/H5 changes. Kernel code follows its upstream licensing,
  principally GPL-2.0-only with explicit exceptions and per-file licenses.
- [LuCI](https://github.com/openwrt/luci): OpenWrt web interface and modem
  configuration integration, with its original per-component licenses.
- [ModemManager](https://modemmanager.org/), libqmi and libqrtr-glib:
  cellular control and transport libraries. Their original GPL/LGPL licensing
  and notices apply to downloaded upstream source, not a new project license.
- [tqftpserv](https://github.com/linux-msm/tqftpserv) and QRTR:
  modem firmware-support transport; see the recipe and upstream BSD license.
- [linux-firmware](https://git.kernel.org/pub/scm/linux/kernel/git/firmware/linux-firmware.git/):
  any firmware selected by OpenWrt, including RTL8125 Ethernet firmware, is
  governed by the applicable upstream firmware license and redistribution terms.

The integrated modem uses signed firmware already present on the device.
This repository does not redistribute GreenPacket/Quectel/Qualcomm modem
firmware, calibration, EFS, identity partitions or device backups.

Product and organization names identify compatibility and provenance only.
They are not endorsements. This notice is an overview, not an exhaustive
license inventory for a future compiled firmware distribution.