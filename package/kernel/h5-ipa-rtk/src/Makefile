# SPDX-License-Identifier: GPL-2.0-only
obj-m += h5_ipa_rtk.o
h5_ipa_rtk-y := ipa_rtk_platform.o ipa_rtk_dma.o ipa_rtk_nat_pool.o \
		ipa_rtk_flow.o ipa_rtk_flowtable.o ipa_rtk_flow_manager.o
ccflags-y += -I$(srctree)/drivers/net/ipa -I$(src)
