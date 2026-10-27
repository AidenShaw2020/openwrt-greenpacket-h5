# Installing from Stock

This is the tested migration sequence for H5-200Q1 stock `912.001.604.145`,
adapted for a user's own build. It is not a vendor-supported installation or
an unattended installer. Other hardware/stock revisions are not validated.
The generalized instructions/helper have not been retested as a complete
installation on a second unit. Do not flash a production or irreplaceable unit.

There are two deliberate writes: replace independent recovery first, test it,
then replace the main `boot` and `system` through standard sysupgrade.
**Do not upload an OpenWrt image to the stock web updater.**

## 1. Prerequisites

- A Linux PC with OpenWrt build prerequisites, `ssh`, `scp`, `sha256sum` and
  Google's official Android platform-tools (`fastboot`). Commands below are
  Linux shell commands, not Windows PowerShell binary redirections.
- A completed build following [README](../README.md#build), using the CPE seed.
- Direct trusted Ethernet, reliable power and physical access to RESET.
- Access to the internal USB interface. This is not a proven installation
  through the external management web UI alone. Do not probe unidentified
  connectors or pins; obtain the correct board-specific USB connection first.
- An authenticated stock root shell. The independent
  [GreenPacket-Jailbreak project](https://github.com/Troll338cz/GreenPacket-Jailbreak)
  documents access for this model/revision. Review its instructions and terms
  separately; it is not bundled here and does not itself install this port.
- Fastboot must already report `unlocked: yes`. No unlocking, fuse changes,
  bootloader flashing, EDL or modem firmware conversion is part of this guide.

If any prerequisite is missing, stop. Do not try another board's commands.
Keep all backups private. Never copy EFS, calibration or identity from another
unit. The port depends on the original signed modem firmware remaining intact.

## 2. Identify Images and Enter RAM OpenWrt

On the PC, from the OpenWrt build directory:

```sh
INITRAMFS=$(realpath bin/targets/sdx/generic/openwrt-sdx-generic-greenpacket_h5-200q1-cpe-initramfs-kernel.bin)
SYSUPGRADE=$(realpath bin/targets/sdx/generic/openwrt-sdx-generic-greenpacket_h5-200q1-cpe-squashfs-sysupgrade.bin)
sha256sum "$INITRAMFS" "$SYSUPGRADE" > h5-local-images.sha256
sha256sum -c h5-local-images.sha256
```

Save these files/checksums on the PC. In the **stock root shell**:

```sh
systemctl reboot bootloader
```

The tested stock shutdown can take about 150 seconds. On the PC, inspect
`fastboot devices`, require exactly the intended unit, then set its actual
serial locally (do not publish it):

```sh
export H5_FASTBOOT_SERIAL='YOUR_DEVICE_SERIAL'
fastboot -s "$H5_FASTBOOT_SERIAL" getvar unlocked
fastboot -s "$H5_FASTBOOT_SERIAL" reboot bootloader
```

Require `unlocked: yes`. Wait for that same device to reappear, then RAM boot:

```sh
fastboot -s "$H5_FASTBOOT_SERIAL" boot "$INITRAMFS"
```

This command downloads/boots RAM; it is not a flash command. Require an actual
Linux USB console and login, not merely fastboot's `OKAY`. Open the enumerated
USB ACM console at 115200 baud and log in as root. On the tested Windows
machine this was a COM port, but the port number is not portable.

In that console verify:

```sh
uname -r
ubus call system board
cat /proc/cmdline
cat /proc/mtd
for key in /etc/dropbear/*_host_key; do
    dropbearkey -y -f "$key" | grep '^ssh-'
done
```

Require Linux `6.12.112`, board `greenpacket,h5-200q1`, and
`rootfs_type: initramfs`. Compare the displayed public SSH key against the
PC's connection prompt. Do not blindly suppress host-key checking. A fresh
RAM boot may generate a different host key. Set a root password with `passwd`.
Wait for LAN services at `192.168.0.1`; initial startup can take two minutes.

## 3. Back Up Before the First Write

Still in RAM, the expected map has 33 partitions. `mtd27` is `boot` (16777216
bytes), `mtd28` is `recovery` (16777216), and `mtd31` is `system` (131072000).
Only `boot` and `system` may be writable in the CPE kernel. Stop on any mismatch.
Check `bad_blocks` for these three partitions under `/sys/class/mtd/`; stop
on any nonzero value or read/ECC error. These simple backups are not a
bad-block-aware raw-NAND restoration method.

On the **Linux PC**, using a new private directory:

```sh
umask 077
mkdir h5-stock-backup
ssh root@192.168.0.1 'cat /proc/mtd' > h5-stock-backup/proc-mtd.txt
ssh root@192.168.0.1 'dd if=/dev/mtd27ro bs=65536' > h5-stock-backup/boot.bin
ssh root@192.168.0.1 'dd if=/dev/mtd28ro bs=65536' > h5-stock-backup/recovery.bin
ssh root@192.168.0.1 'dd if=/dev/mtd31ro bs=65536' > h5-stock-backup/system.bin
wc -c h5-stock-backup/*.bin
ssh root@192.168.0.1 'sha256sum /dev/mtd27ro /dev/mtd28ro /dev/mtd31ro'
sha256sum h5-stock-backup/*.bin > h5-stock-backup/SHA256SUMS
cat h5-stock-backup/SHA256SUMS
```

Check every command's exit status and compare each local size/hash with the
corresponding device result. Keep a second verified copy off the PC. Stop on
any mismatch. Preserve original stock settings/export separately if available.
These are ECC-corrected data reads of three partitions, **not** a full-chip
clone: EFS/NV, raw OOB, bad-block metadata and other partitions are excluded.
Do not read protected EFS merely to complete this guide.

## 4. Install and Prove Independent Recovery

From the RAM console, `reboot` back into unchanged stock, then use its root
shell and `systemctl reboot bootloader` again. On the PC:

```sh
sha256sum -c h5-local-images.sha256
fastboot -s "$H5_FASTBOOT_SERIAL" getvar unlocked
fastboot -s "$H5_FASTBOOT_SERIAL" getvar partition-size:recovery
```

Require the same device, unlocked state and an initramfs that fits the
reported usable recovery size. The tested bootloader reported `0xEC0000`
(15466496 bytes), smaller than the raw 16 MiB Linux MTD size. Do not assume
that a raw MTD backup fits fastboot. Stop if your image or partition differs.

**The next command replaces stock recovery. Proceed only after verified
backups and successful RAM boot. Never substitute `boot`, `system`, `misc`,
a bootloader partition or a modem partition for `recovery`.**

```sh
fastboot -s "$H5_FASTBOOT_SERIAL" flash recovery "$INITRAMFS"
```

Only after a successful flash, perform this mandatory normal reboot:

```sh
fastboot -s "$H5_FASTBOOT_SERIAL" reboot
```

The H5 bootloader sets a persistent `fastboot-abnormal` flag during flashing.
**Normal `fastboot reboot` clears it; `reboot bootloader` and power cycling
are not substitutes.** Do not edit `misc` by hand. Wait for normal stock boot.

Now remove **all power sources**, including USB. Hold RESET, restore normal
power, release RESET after about ten seconds, wait about 45 seconds, and
reconnect USB. Require an actual OpenWrt initramfs login and usable Ethernet.
This must be a cold NAND recovery boot, not another `fastboot boot` RAM boot.

Verify the new recovery image by readback. On the PC, after checking the
new RAM session's SSH public key through USB and setting a password:

```sh
pages=$((($(stat -c %s "$INITRAMFS") + 4095) / 4096))
ssh root@192.168.0.1 "dd if=/dev/mtd28ro bs=4096 count=$pages" > h5-recovery-readback.bin
cmp "$INITRAMFS" h5-recovery-readback.bin
```

Require a successful read and exact comparison. Also verify the full `boot`
and `system` SHA256 values still match your stock backups. Do not proceed if
recovery does not cold-boot, readback differs, or the main stock partitions
changed unexpectedly. At this point main stock firmware is still intact.

## 5. Install the Persistent Main System

Remain in the verified NAND-booted OpenWrt initramfs. On the PC, transfer your
own sysupgrade image and the repository's check-only helper:

```sh
scp -O "$SYSUPGRADE" root@192.168.0.1:/tmp/h5-sysupgrade.bin
scp -O ../openwrt-greenpacket-h5/tools/h5-install-check.sh root@192.168.0.1:/tmp/
sha256sum "$SYSUPGRADE"
```

In the USB console, substitute that actual local checksum:

```sh
sh /tmp/h5-install-check.sh /tmp/h5-sysupgrade.bin YOUR_LOCAL_SHA256
```

Require `H5_FIRST_INSTALL_CHECK_PASSED`. This helper does not write flash and
cannot verify that you saved valid backups or physically tested recovery.
Keep the image unchanged after the check. The next command is the separate,
destructive confirmation; it replaces main stock `boot` and `system` and
starts with fresh OpenWrt settings:

```sh
sysupgrade -n /tmp/h5-sysupgrade.bin
```

Do not disconnect power. The platform upgrade hook stops the modem before
shared-NAND writes and leaves recovery/other partitions protected. Monitor
the USB console. After reboot, require `rootfs_type: squashfs` from
`ubus call system board` and a mounted writable UBIFS `/overlay`. An initramfs
login alone is not proof of persistent installation. Set a new root password,
verify the new SSH host key through USB, and configure your own APN/PIN in LuCI.
Never guess a SIM PIN. Reboot and verify persistence before relying on the unit.

## Recovery and Returning to Stock

RESET-at-power-on starts the independent recovery installed in step 4. It does
not restore stock or erase the main system. From working OpenWrt recovery,
check a newly built matching sysupgrade image before installing it again.

Returning to stock is **not** an OpenWrt factory reset. This project does not
supply a generic stock restore flasher: raw ECC data backups, usable fastboot
sizes, bad blocks and vendor signature/trailer bytes require device-specific
handling. Do not truncate a stock image just to fit or feed a raw dump to
`sysupgrade`. Preserve your own full recovery dump, including any bytes beyond
the Android header's declared payload. Arrange a proven restoration plan
before the first write if guaranteed stock rollback is a requirement.