#!/bin/sh
set -eu

fail() {
    printf 'STOP: %s\n' "$1" >&2
    exit 1
}

if [ "$#" -ne 2 ]; then
    echo 'Usage: sh h5-install-check.sh SYSUPGRADE_IMAGE EXPECTED_SHA256' >&2
    exit 2
fi
image=$1
expected=$2
case "$expected" in
    ''|*[!0-9a-f]*) fail 'Expected SHA256 must be 64 lowercase hexadecimal characters.' ;;
esac
[ "${#expected}" -eq 64 ] || fail 'Expected SHA256 has the wrong length.'
[ -f "$image" ] && [ ! -L "$image" ] || fail 'A regular, non-symlink sysupgrade file is required.'
[ "$(id -u)" = 0 ] || fail 'Run as root in the H5 CPE initramfs.'
[ "$(cat /tmp/sysinfo/board_name)" = greenpacket,h5-200q1 ] || fail 'Unsupported board.'
[ "$(ubus call system board | jsonfilter -e '@.rootfs_type')" = initramfs ] || fail 'First install requires RAM/initramfs, not stock or persistent OpenWrt.'
[ "$(uname -r)" = 6.12.112 ] || fail 'Unexpected kernel version.'
tr '\000' '\n' </proc/device-tree/compatible |
    grep -qx greenpacket,h5-200q1-openwrt || fail 'The writable CPE profile is required.'
[ "$(cat /sys/class/mtd/mtd27/name)" = boot ] || fail 'Unexpected boot partition.'
[ "$(cat /sys/class/mtd/mtd28/name)" = recovery ] || fail 'Unexpected recovery partition.'
[ "$(cat /sys/class/mtd/mtd31/name)" = system ] || fail 'Unexpected system partition.'
[ "$(cat /sys/class/mtd/mtd27/size)" = 16777216 ] || fail 'Unexpected boot size.'
[ "$(cat /sys/class/mtd/mtd28/size)" = 16777216 ] || fail 'Unexpected recovery size.'
[ "$(cat /sys/class/mtd/mtd31/size)" = 131072000 ] || fail 'Unexpected system size.'
[ "$(cat /sys/class/mtd/mtd31/writesize)" = 4096 ] || fail 'Unexpected NAND page size.'
for index in 27 28 31; do
    [ "$(cat "/sys/class/mtd/mtd$index/bad_blocks")" = 0 ] || fail 'Bad blocks require a separately validated backup/restore procedure.'
done
count=0
writable=0
for path in /sys/class/mtd/mtd[0-9]*; do
    [ -f "$path/name" ] || continue
    count=$((count + 1))
    flags=$(cat "$path/flags")
    if [ $((flags & 1024)) -ne 0 ]; then
        case "$(basename "$path"):$(cat "$path/name")" in
            mtd27:boot|mtd31:system) writable=$((writable + 1)) ;;
            *) fail 'An unexpected NAND partition is writable.' ;;
        esac
    fi
done
[ "$count" -eq 33 ] && [ "$writable" -eq 2 ] || fail 'Unexpected partition map or write protection.'
[ "$(sha256sum "$image" | cut -d' ' -f1)" = "$expected" ] || fail 'Image SHA256 mismatch.'
sysupgrade -T "$image" || fail 'OpenWrt rejected the sysupgrade image.'
echo 'H5_FIRST_INSTALL_CHECK_PASSED: no flash was written.'
echo 'This does not verify your backups or prove recovery was cold-boot tested.'