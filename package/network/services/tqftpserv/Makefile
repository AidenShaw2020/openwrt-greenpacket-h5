# SPDX-License-Identifier: GPL-2.0-only
include $(TOPDIR)/rules.mk

PKG_NAME:=tqftpserv
PKG_VERSION:=1.3
PKG_RELEASE:=2
PKG_LICENSE:=BSD-3-Clause
PKG_LICENSE_FILES:=LICENSE qrtr/LICENSE
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk

define Download/tqftpserv-source
  FILE:=tqftpserv-1.3.tar.gz
  URL:=https://codeload.github.com/linux-msm/tqftpserv/tar.gz/refs/tags
  URL_FILE:=v1.3
  HASH:=87a3845e8f47b42f31d8aaad04c5e0707e391b3954d096aeff6f0a03467dc33f
endef
$(eval $(call Download,tqftpserv-source))

define Download/tqftpserv-qrtr
  FILE:=qrtr-1.2.tar.gz
  URL:=https://codeload.github.com/linux-msm/qrtr/tar.gz/refs/tags
  URL_FILE:=v1.2
  HASH:=7a82bf80246fe71287b13c66c0466208822e8337fcd4aaf302eee6c5fcb48a52
endef
$(eval $(call Download,tqftpserv-qrtr))

$(STAMP_PREPARED): $(DL_DIR)/tqftpserv-1.3.tar.gz $(DL_DIR)/qrtr-1.2.tar.gz

define Package/tqftpserv
  SECTION:=net
  CATEGORY:=Network
  TITLE:=Qualcomm QRTR firmware file service
  DEPENDS:=+kmod-qrtr +libpthread
  URL:=https://github.com/linux-msm/tqftpserv
endef

define Package/tqftpserv/description
 Upstream TFTP service over QRTR for Qualcomm remote processors. Firmware
 is read from /lib/firmware; writable service files stay in RAM under /var.
 No proprietary firmware is included.
endef

define Build/Prepare
	$(INSTALL_DIR) $(PKG_BUILD_DIR)/qrtr
	$(TAR) -C $(PKG_BUILD_DIR) --strip-components=1 -xzf $(DL_DIR)/tqftpserv-1.3.tar.gz
	$(TAR) -C $(PKG_BUILD_DIR)/qrtr --strip-components=1 -xzf $(DL_DIR)/qrtr-1.2.tar.gz
	$(Build/Patch)
endef

define Build/Compile
	$(TARGET_CC) $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) -D_GNU_SOURCE \
		-ffunction-sections -fdata-sections -Wl,--gc-sections \
		-I$(PKG_BUILD_DIR)/qrtr/include -I$(PKG_BUILD_DIR) \
		$(PKG_BUILD_DIR)/tqftpserv.c $(PKG_BUILD_DIR)/translate.c \
		$(PKG_BUILD_DIR)/qrtr/lib/qrtr.c $(PKG_BUILD_DIR)/qrtr/lib/qmi.c \
		$(PKG_BUILD_DIR)/qrtr/lib/logging.c $(TARGET_LDFLAGS) \
		-o $(PKG_BUILD_DIR)/tqftpserv
endef

define Package/tqftpserv/install
	$(INSTALL_DIR) $(1)/usr/sbin $(1)/etc/init.d
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/tqftpserv $(1)/usr/sbin/tqftpserv
	$(INSTALL_BIN) ./files/tqftpserv.init $(1)/etc/init.d/tqftpserv
endef

$(eval $(call BuildPackage,tqftpserv))
