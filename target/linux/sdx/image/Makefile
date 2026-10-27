# SPDX-License-Identifier: GPL-2.0-only

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/image.mk
DEVICE_VARS += H5_CMDLINE

define Build/h5-bootimg
	python3 $(TOPDIR)/target/linux/sdx/image/h5-bootimg.py $@ $@.new "$(strip $(H5_CMDLINE) $(1))"
	mv $@.new $@
endef

define Device/greenpacket_h5-200q1
  DEVICE_VENDOR := GreenPacket
  DEVICE_MODEL := H5-200Q1
  KERNEL_NAME := zImage
  DEVICE_DTS_DIR := $$(DTS_DIR)/qcom
  DEVICE_DTS := qcom-sdx65-greenpacket-h5
  DEVICE_DTC_FLAGS := -@
  DEVICE_PACKAGES := kmod-gpio-button-hotplug
  KERNEL_INITRAMFS := kernel-bin | append-dtb
  IMAGES :=
endef
TARGET_DEVICES += greenpacket_h5-200q1

define Device/greenpacket_h5-200q1-pcie
  $(call Device/greenpacket_h5-200q1)
  DEVICE_VARIANT := Experimental PCIe RAM test
  SUPPORTED_DEVICES := greenpacket,h5-200q1
  DEVICE_DTS := qcom-sdx65-greenpacket-h5-pcie
  DEVICE_PACKAGES += kmod-r8169 pciutils
endef
TARGET_DEVICES += greenpacket_h5-200q1-pcie

define Device/greenpacket_h5-200q1-usb-debug
  $(call Device/greenpacket_h5-200q1-pcie)
  DEVICE_VARIANT := Experimental USB serial RAM test
  DEVICE_DTS := qcom-sdx65-greenpacket-h5-usb-debug
endef
TARGET_DEVICES += greenpacket_h5-200q1-usb-debug

define Device/greenpacket_h5-200q1-usb-pcie-compat
  $(call Device/greenpacket_h5-200q1-usb-debug)
  DEVICE_VARIANT := Experimental USB RAM test with stock PCIe policy
  DEVICE_DTS := qcom-sdx65-greenpacket-h5-usb-pcie-compat
endef
TARGET_DEVICES += greenpacket_h5-200q1-usb-pcie-compat

define Device/greenpacket_h5-200q1-web
  $(call Device/greenpacket_h5-200q1-usb-pcie-compat)
  DEVICE_VARIANT := Experimental LuCI HTTP RAM test
  DEVICE_PACKAGES += luci
endef
TARGET_DEVICES += greenpacket_h5-200q1-web

define Device/greenpacket_h5-200q1-modem
  $(call Device/greenpacket_h5-200q1-web)
  DEVICE_VARIANT := Experimental MPSS firmware RAM test
  DEVICE_DTS := qcom-sdx65-greenpacket-h5-modem
  DEVICE_PACKAGES += kmod-h5-mpss
endef
TARGET_DEVICES += greenpacket_h5-200q1-modem

define Device/greenpacket_h5-200q1-ipa
  $(call Device/greenpacket_h5-200q1-modem)
  DEVICE_VARIANT := Experimental IPA cellular data RAM test
  DEVICE_DTS := qcom-sdx65-greenpacket-h5-ipa
  DEVICE_PACKAGES += kmod-h5-ipa kmod-h5-rpmsg-at h5-modem-tools luci-app-h5-modem ip-full modemmanager modemmanager-rpcd luci-proto-modemmanager ubi-utils tqftpserv
endef
TARGET_DEVICES += greenpacket_h5-200q1-ipa

define Device/greenpacket_h5-200q1-cpe
  $(call Device/greenpacket_h5-200q1-ipa)
  DEVICE_VARIANT := CPE router
  DEVICE_DTS := qcom-sdx65-greenpacket-h5-cpe
  BOARD_NAME := greenpacket_h5-200q1
  KERNEL_SIZE := 16384k
  IMAGE_SIZE := 128000k
  BLOCKSIZE := 256k
  PAGESIZE := 4096
  # Select RAM or the new OpenWrt system volume explicitly, without stock rootfs arguments.
  H5_CMDLINE := noinitrd ro rootwait console=ttyMSM0,115200n8 pcie_ports=compat pci=pcie_bus_safe
  KERNEL := kernel-bin | append-dtb | h5-bootimg ubi.mtd=system root=/dev/ubiblock0_0 rootfstype=squashfs | check-size $$(KERNEL_SIZE)
  KERNEL_INITRAMFS := kernel-bin | append-dtb | h5-bootimg root=/dev/ram0 | check-size $$(KERNEL_SIZE)
  FILESYSTEMS := squashfs
  IMAGES := sysupgrade.bin
  IMAGE/sysupgrade.bin := sysupgrade-tar | append-metadata | check-size
endef
TARGET_DEVICES += greenpacket_h5-200q1-cpe

$(eval $(call BuildImage))
