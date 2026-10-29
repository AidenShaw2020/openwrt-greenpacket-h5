# SPDX-License-Identifier: GPL-2.0-only

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=h5-ipa-rtk
PKG_VERSION:=1
PKG_RELEASE:=1
PKG_LICENSE:=GPL-2.0-only

include $(INCLUDE_DIR)/package.mk

define KernelPackage/h5-ipa-rtk
  SUBMENU:=Network Devices
  TITLE:=GreenPacket H5 SDX65 IPA RTL8125 hardware offload controller
  DEPENDS:=@TARGET_sdx +kmod-h5-ipa +kmod-r8125-rss +kmod-nft-offload
  FILES:=$(PKG_BUILD_DIR)/h5_ipa_rtk.ko
  AUTOLOAD:=$(call AutoProbe,h5_ipa_rtk,85)
endef

define KernelPackage/h5-ipa-rtk/description
 Stock-equivalent Qualcomm SDX65 IPA v5.0 + Realtek RTL8125B external ring,
 microcontroller, QMI 0x0021/0x0023/0x0024/0x0037 filter/NAT, and Linux
 nf_flowtable hardware offload controller for the GreenPacket H5-200Q1.
endef

define Build/Compile
	+$(KERNEL_MAKE) $(PKG_JOBS) \
		M="$(PKG_BUILD_DIR)" \
		modules
endef

define KernelPackage/h5-ipa-rtk/install
	$(INSTALL_DIR) $(1)/etc/init.d $(1)/etc/hotplug.d/iface
	$(INSTALL_BIN) ./files/h5-ipa-offload.init $(1)/etc/init.d/h5-ipa-offload
	$(INSTALL_DATA) ./files/15-h5-ipa-offload.hotplug $(1)/etc/hotplug.d/iface/15-h5-ipa-offload
endef

$(eval $(call KernelPackage,h5-ipa-rtk))
