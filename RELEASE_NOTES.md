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

**The local rebuilt binaries have not themselves been flashed or boot-tested.**
The preceding build with the same driver/modem fixes passed persistent boot,
sysupgrade, mobile data, LAN routing, LTE cell-lock and modem/NAND concurrency
tests. LTE/5G NSA, including NR n78 activation under traffic, was observed.
That does not establish hardware testing of your own new build.

Fresh root has no password and management uses HTTP by default. Set a strong
root password and keep management on a trusted LAN. IPv6 and 5G SA cell lock
remain unvalidated; bridge/IP passthrough is not implemented.

Original device-specific modem firmware, SIM credentials, device backups,
private keys and raw diagnostic logs are not included. First installation
from stock remains an advanced procedure, not a turnkey installer. The new
generalized preflight has syntax and early-rejection tests, but has not been
run through a fresh stock-to-OpenWrt installation on another unit.