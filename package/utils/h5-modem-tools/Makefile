include $(TOPDIR)/rules.mk

PKG_NAME:=h5-modem-tools
PKG_VERSION:=1
PKG_RELEASE:=1
PKG_LICENSE:=GPL-2.0-only

include $(INCLUDE_DIR)/package.mk

define Package/h5-modem-tools
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=GreenPacket H5 modem diagnostics
  DEPENDS:=@TARGET_sdx +kmod-h5-rpmsg-at +rpcd +jshn
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Build/Compile
	$(TARGET_CC) $(TARGET_CFLAGS) -Wall -Wextra -Werror \
		-o $(PKG_BUILD_DIR)/h5-at $(PKG_BUILD_DIR)/h5-at.c $(TARGET_LDFLAGS)
endef

define Package/h5-modem-tools/install
	$(INSTALL_DIR) $(1)/usr/libexec
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/h5-at $(1)/usr/libexec/h5-at
	$(INSTALL_DIR) $(1)/usr/libexec/rpcd $(1)/usr/share/rpcd/acl.d
	$(INSTALL_BIN) ./files/h5modem $(1)/usr/libexec/rpcd/h5modem
	$(INSTALL_DATA) ./files/h5-modem-acl.json $(1)/usr/share/rpcd/acl.d/h5-modem.json
endef

$(eval $(call BuildPackage,h5-modem-tools))
