# SPDX-License-Identifier: GPL-2.0-only

define KernelPackage/h5-modem-transport
  SUBMENU:=Network Support
  TITLE:=H5 Qualcomm GLINK SMEM and QRTR transport
  DEPENDS:=@TARGET_sdx +kmod-qrtr
  KCONFIG:= \
	CONFIG_RPMSG \
	CONFIG_RPMSG_QCOM_GLINK \
	CONFIG_RPMSG_QCOM_GLINK_SMEM \
	CONFIG_QRTR_SMD
  FILES:= \
	$(LINUX_DIR)/drivers/rpmsg/rpmsg_core.ko \
	$(LINUX_DIR)/drivers/rpmsg/qcom_glink.ko \
	$(LINUX_DIR)/drivers/rpmsg/qcom_glink_smem.ko \
	$(LINUX_DIR)/net/qrtr/qrtr-smd.ko
  AUTOLOAD:=$(call AutoProbe,rpmsg_core qcom_glink qcom_glink_smem qrtr-smd)
endef

define KernelPackage/h5-modem-transport/description
 Upstream Linux transport modules for experimental communication with the
 integrated SDX65 modem. This package does not start remoteproc, load modem
 firmware, unlock the SIM, establish a data session or enable NAND writes.
endef

$(eval $(call KernelPackage,h5-modem-transport))

define KernelPackage/h5-mpss
  SUBMENU:=Network Support
  TITLE:=H5 upstream modem remoteproc support
  DEPENDS:=@TARGET_sdx +kmod-h5-modem-transport
  KCONFIG:= \
	CONFIG_REMOTEPROC=y \
	CONFIG_QCOM_SMP2P \
	CONFIG_QCOM_Q6V5_PAS \
	CONFIG_QCOM_Q6V5_COMMON \
	CONFIG_QCOM_RPROC_COMMON \
	CONFIG_QCOM_PIL_INFO \
	CONFIG_QCOM_MDT_LOADER \
	CONFIG_QCOM_QMI_HELPERS \
	CONFIG_QCOM_SYSMON
  FILES:= \
	$(LINUX_DIR)/drivers/soc/qcom/smp2p.ko \
	$(LINUX_DIR)/drivers/soc/qcom/mdt_loader.ko \
	$(LINUX_DIR)/drivers/soc/qcom/qmi_helpers.ko \
	$(LINUX_DIR)/drivers/remoteproc/qcom_pil_info.ko \
	$(LINUX_DIR)/drivers/remoteproc/qcom_common.ko \
	$(LINUX_DIR)/drivers/remoteproc/qcom_q6v5.ko \
	$(LINUX_DIR)/drivers/remoteproc/qcom_sysmon.ko \
	$(LINUX_DIR)/drivers/remoteproc/qcom_q6v5_pas.ko
  # Keep experimental MPSS drivers manual until board power/reset is verified.
endef

define KernelPackage/h5-mpss/description
 Experimental upstream Qualcomm PAS/remoteproc drivers. The modem profile
 requires matching firmware copied separately from this device. It does
 not bundle proprietary modem firmware or enable NAND or cellular data.
endef

$(eval $(call KernelPackage,h5-mpss))

define KernelPackage/h5-ipa
  SUBMENU:=Network Devices
  TITLE:=H5 upstream Qualcomm IPA data transport
  DEPENDS:=@TARGET_sdx +kmod-h5-mpss +kmod-rmnet
  KCONFIG:=CONFIG_QCOM_IPA
  FILES:=$(LINUX_DIR)/drivers/net/ipa/ipa.ko
  # Manual loading while the board's data path is being verified.
endef

define KernelPackage/h5-ipa/description
 Experimental upstream SDX65 IPA and standard RMNET/QMAP data transport.
 Uses the IPA GSI firmware already initialized by the stock boot firmware.
endef

$(eval $(call KernelPackage,h5-ipa))

define KernelPackage/h5-rpmsg-at
  SUBMENU:=Network Support
  TITLE:=H5 standard RPMSG character endpoint for modem AT commands
  DEPENDS:=@TARGET_sdx +kmod-h5-modem-transport
  KCONFIG:=CONFIG_RPMSG_CHAR
  FILES:=$(LINUX_DIR)/drivers/rpmsg/rpmsg_char.ko
  # Bind the H5 DS channel explicitly after PAS starts; no WWAN AT probing.
endef

define KernelPackage/h5-rpmsg-at/description
 Unmodified upstream RPMSG character driver. The board startup helper binds
 only the DS channel and exposes /dev/h5-at with root-only access. Its default
 endpoint survives application close, avoiding repeated GLINK channel opens.
endef

$(eval $(call KernelPackage,h5-rpmsg-at))
