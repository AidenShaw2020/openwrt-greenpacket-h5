# H5-200Q1 / OpenWrt 25.12.6 Public Rebuild

Experimental pre-release for the tested GreenPacket H5-200Q1 hardware only.
Read README.md before flashing. These are not GreenPacket web-update packages.

## Downloads

- `cpe-initramfs-kernel.bin`: full RAM/recovery image, not a sysupgrade file.
- `cpe-squashfs-sysupgrade.bin`: update for devices already running this CPE port.
- `h5-openwrt-25.12.6-bringup.patch`: source overlay against the pinned vanilla release.
- `openwrt-h5-25.12.6-corresponding-source.tar.zst`: full corresponding sources,
  original dependency source archives, configuration and upstream license texts.
- The package manifest and `SHA256SUMS` for all of the above.

The actual download names include the full OpenWrt target/profile prefix.
Verify their SHA256 checksums before use. Source and binary assets belong
together when redistributing this release under the applicable licenses.

## Validation and Caveats

The clean public rebuild completed successfully. Decompressed kernels,
initramfs and squashfs passed private-data checks, and both images passed
the existing offline production-format/content validation. The patch passed
clean-index application to the exact OpenWrt and packages-feed revisions.

**These rebuilt binaries have not themselves been flashed or boot-tested.**
The preceding build with the same driver/modem fixes passed persistent boot,
sysupgrade, mobile data, LAN routing, LTE cell-lock and modem/NAND concurrency
tests. LTE/5G NSA, including NR n78 activation under traffic, was observed.
That does not establish hardware testing of these new binary hashes.

Fresh root has no password and management uses HTTP by default. Set a strong
root password and keep management on a trusted LAN. IPv6 and 5G SA cell lock
remain unvalidated; bridge/IP passthrough is not implemented.

Original device-specific modem firmware, SIM credentials, device backups,
private keys and raw diagnostic logs are not included. First installation
from stock remains an advanced procedure, not a turnkey installer.