# H5-200Q1 / OpenWrt 25.12.6 Source Port

Experimental pre-release for the tested GreenPacket H5-200Q1 hardware only.
Read README.md and docs/STOCK_INSTALL.md before building or flashing.
Built images are not GreenPacket web-update packages.

## Contents

Source-only: target, custom packages, kernel/feed patches, build configuration,
exported vanilla patch, upstream revision pins and stock installation guide.
A read-only first-install check is included; it does not flash the device.
No prebuilt images, dependency-source archive or device backups are supplied.
SHA256SUMS covers the exported patch only. Generate and verify checksums of
your own images before transfer or flashing.

## Validation and Caveats

The clean local rebuild completed successfully. Decompressed kernels,
initramfs and squashfs passed private-data checks, and both images passed
the existing offline production-format/content validation. The patch passed
clean-index application to the exact OpenWrt and packages-feed revisions.

The initially privacy-checked candidate was not flashed. A later CPE rebuild
fixes cold boot without USB by selecting ttyMSM0, not ttyGS0, in both the DTB
and Android boot header. USB remains available for normal console login.
This rebuild passed sysupgrade with configuration preserved, main and recovery
cold boots without USB, image/NAND readback checks, and return from recovery
to the persistent system. Its runtime kernel identity is `builder@buildhost`;
the earlier privacy audit is not a complete audit of this newer build.

Release RESET about 2-3 seconds after restoring power to enter recovery.
Holding it for ten seconds can also activate Linux failsafe at `192.168.1.1`.
Normal recovery uses `192.168.0.1` with `rootfs_type: initramfs`.

The preceding build with the same driver/modem fixes passed persistent boot,
sysupgrade, mobile data, LAN routing, LTE cell-lock and modem/NAND concurrency
tests. LTE/5G NSA, including NR n78 activation under traffic, was observed.
Stock-equivalent SDX65 IPA v5.0 + Realtek RTL8125B hardware NAT and
`nf_flowtable` packet acceleration (`kmod-h5-ipa-rtk`, `stage 4`) was
subsequently validated on the device, achieving **115.28 Mbps** across 8
concurrent TCP streams at **11.8% CPU** (**0.5% `softirq`**).
That does not establish hardware testing of your own new build.

Fresh root has no password and management uses HTTP by default. Set a strong
root password and keep management on a trusted LAN. IPv6 and 5G SA cell lock
remain unvalidated. Optional IPv4 passthrough (`h5-ippt`) is off by default.

Original device-specific modem firmware, SIM credentials, device backups,
private keys and raw diagnostic logs are not included. First installation
from stock remains an advanced procedure, not a turnkey installer. The new
generalized preflight has syntax and early-rejection tests, but has not been
run through a fresh stock-to-OpenWrt installation on another unit.