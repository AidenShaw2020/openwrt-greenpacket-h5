# Corresponding Source and Binary Release

The release includes `openwrt-h5-25.12.6-corresponding-source.tar.zst`
alongside the two firmware images. Keep this archive available with the
binaries when redistributing them, subject to each component's license.

It contains:

- The exact OpenWrt, packages and LuCI revisions listed in README.md.
- The H5 target and custom packages, and the actual tracked build-tree changes.
- The applied packages-feed changes, including added ModemManager patches.
- The complete build configuration with personal paths mapped to /build/openwrt.
- The Linux configuration and firmware package manifest.
- The original downloaded source archives used by the build and host toolchain,
  including their upstream license/copyright notices.
- The upstream OpenWrt license texts and package recipes describing licenses.

The archive excludes build caches, compiled toolchains, Git metadata, private
package signing keys, device backups and device-specific modem firmware.
The dependency download cache can include public upstream firmware distributed
by OpenWrt; its original redistribution notices remain inside its archive.

## Building from the Source Archive

On a Linux host with OpenWrt's normal build prerequisites:

```sh
tar --zstd -xf openwrt-h5-25.12.6-corresponding-source.tar.zst
cd public-corresponding-source-20261027-v2/openwrt
./scripts/feeds update -i packages luci
./scripts/feeds install -a -p packages
./scripts/feeds install -a -p luci
./scripts/feeds uninstall luci-app-weechat
make defconfig
make -j"$(nproc)"
```

These are source archives without Git metadata. The feed changes are already
applied; do not run the Git-dependent `h5-patches/apply-packages.sh` again.
The source archive includes a saved version string and source-date epoch.
Its sanitized config maps the canonical /build/openwrt source prefix; if using
another directory, use the dynamic `CONFIG_KERNEL_CFLAGS` prefix-mapping
argument shown in README.md for the compilation command.

The source archive was assembled from the actual successful build and checked
for known private identities. A second full build starting solely from that
archive, with network access disabled, has not been performed. Bit-for-bit
reproducibility is not claimed. A new APK signing key may be generated for a
local build; the private release signing key is intentionally not distributed.

## Firmware Notice

The pre-release images were rebuilt for public distribution and passed offline
format/content checks. They have not themselves been flashed or boot-tested.
The preceding build with the same driver fixes passed the documented hardware
tests. Review README.md before flashing; stock migration is not turnkey.