# Validation Summary

Date: 2026-10-27. Scope: one tested H5-200Q1 unit, OpenWrt 25.12.6,
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