# SPDX-License-Identifier: GPL-2.0-only

include $(TOPDIR)/rules.mk

ARCH:=arm
BOARD:=sdx
BOARDNAME:=Qualcomm SDX modems
CPU_TYPE:=cortex-a7
CPU_SUBTYPE:=neon-vfpv4
FEATURES:=fpu ramdisk pci pcie nand squashfs
SUBTARGETS:=generic
KERNEL_PATCHVER:=6.12
KERNELNAME:=zImage dtbs

include $(INCLUDE_DIR)/target.mk

$(eval $(call BuildTarget))
