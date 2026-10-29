REQUIRE_IMAGE_METADATA=1
CI_KERNPART=boot
CI_UBIPART=system
CI_ROOTPART=rootfs
# platform_check_image is also called after stage2 pivots away from squashfs.
# Its exact compatible check needs this BusyBox applet in the RAM filesystem.
RAMFS_COPY_BIN="${RAMFS_COPY_BIN:-} /usr/bin/tr"

platform_check_image() {
	case "$(board_name)" in
	greenpacket,h5-200q1)
		if ! tr '\000' '\n' </proc/device-tree/compatible |
			grep -qx 'greenpacket,h5-200q1-openwrt'; then
			echo "Use the H5 CPE initramfs installer; this profile is read-only."
			return 1
		fi
		for idx in 27 31; do
			flags="$(cat /sys/class/mtd/mtd$idx/flags)"
			[ $((flags & 1024)) -ne 0 ] || return 1
		done
		nand_do_platform_check "$(board_name)" "$1"
		;;
	*)
		echo "Unsupported board"
		return 1
		;;
	esac
}

platform_do_upgrade() {
	case "$(board_name)" in
	greenpacket,h5-200q1)
		platform_check_image "$1" || return 1
		stage=/sys/bus/platform/devices/soc:ipa-ethernet/stage
		if [ -f "$stage" ]; then
			echo 0 >"$stage" 2>/dev/null || true
		fi
		# procd stops userspace before this RAM-stage hook. Stop MPSS too,
		# before erasing system, so it cannot access shared NAND during it.
		state=/sys/class/remoteproc/remoteproc0/state
		if [ -f "$state" ]; then
			case "$(cat "$state")" in
				running) echo stop >"$state" || return 1 ;;
				offline) ;;
				*) echo 'Modem is not in a safe state for NAND upgrade'; return 1 ;;
			esac
			[ "$(cat "$state")" = offline ] || return 1
		fi
		nand_do_upgrade "$1"
		;;
	esac
}
