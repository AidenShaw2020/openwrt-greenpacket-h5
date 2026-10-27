# GreenPacket H5-200Q1 / Qualcomm SDX65

See h5-patches/README.md in the patched OpenWrt tree for the complete build
instructions, pinned source revisions, current validation and installation
limits. The target is independent of RutOS and vendor kernel build systems.

The CPE profile provides a normal router on 192.168.0.1 with DHCP/DNS,
firewall/NAT, HTTP LuCI, native ModemManager and Network -> Cellular Network.
Bridge/IP passthrough is deferred. Fresh root has an empty password and
Dropbear supports password authentication. USB CDC ACM uses standard console
login. No private credentials, host keys, SIM settings or modem firmware
are embedded in the source patch.

Direct fastboot RAM startup without the stock kernel has been verified with
Linux 6.12.112, 1 Gb/s RTL8125B Ethernet, USB root login, blank-password SSH,
HTTP LuCI and O2 CZ LTE/5G NSA data. A separate LAN client passed DHCP, DNS,
ping and HTTP through NAT. LTE cell-lock apply/readback/data/unlock was
verified on an earlier integrated RAM build. 5G SA lock and carrier IPv6
remain untested. The GZIP outer kernel contains an XZ-compressed initramfs.

Restart directly into fastboot before independent RAM trials: prior failed
image attempts can affect bootloader state. The Android v0 builder places
aliases and chosen before memory without altering DTB property values.
Initramfs explicitly selects RAM root; the persistent kernel explicitly
selects the new OpenWrt system UBI/squashfs root. These images are not stock
GreenPacket web update packages.

Signed modem firmware is read from the original modem UBI partition, checked
and cached in RAM. QRTR TFTP service 4096 version 3, upstream IPA/rmnet/QMI
and unmodified rpmsg_char provide modem data and restricted vendor AT
operations. BAM locking serializes AP/modem NAND access. Secure EFS is
blocked; all partitions are read-only except verified CPE boot/system
children on the exact backed-up layout.

GPIOs: SYS blue 18, SYS red 63, Ethernet 17, 4G 16, 5G 19. Reset GPIO32
is active low with pull-up and 20 ms polling; physical press/release and
SYS/ETH indications have been verified. Cellular indication follows actual
ModemManager RAT. Persistent factory-reset behavior remains untested.

The vendor's fastboot flash/erase handlers set a persistent `fastboot-abnormal`
flag in misc, which bypasses kernel loading even when RESET selects recovery.
Run a normal `fastboot reboot` after flashing to clear it before a physical
recovery test; `fastboot reboot bootloader` does not clear it. The original
recovery service on TCP 5000 booted after this correction. Full standard
OpenWrt then booted from NAND recovery, with USB/SSH, HTTP LuCI, gigabit LAN,
LTE/5G NSA data and separate LAN-client DHCP/DNS/NAT/HTTP verified. Earlier
flag-blocked failures do not establish a DTB or image-size problem.
The CPE DT supplies the original SCM download-mode register at TCSR + 0x13000.
Normal software reboot with active mobile data then returned to the then
installed stock system without a power cycle. The bootloader restart target
also passed with MPSS offline. Physical recovery entry is verified;
OpenWrt's software recovery target remains unvalidated.
Standard PDC, PMIC
PON, SPMI SDAM and nvmem reboot-mode dependencies are included in the latest
build. Persistent installation into boot/system, exact kernel/rootfs/recovery
readback and a configuration-preserving software reboot have passed.
The main system now uses squashfs with a writable UBIFS overlay; stock
boot/system are replaced and original private backups are retained.
A subsequent standard sysupgrade preserved configuration and host keys;
the updated kernel/rootfs and preserved recovery readbacks passed.
Three stop/start tests of patch 118 passed all 14 complete rootfs reads and
171 three-page samples without ECC or squashfs errors. Mobile data,
LAN-client DHCP/DNS/NAT/HTTP, AT statistics and LTE cell-lock apply/data/unlock
passed afterwards. A short RESET press rebooted with settings preserved.
Long-press factory reset remains under test.
