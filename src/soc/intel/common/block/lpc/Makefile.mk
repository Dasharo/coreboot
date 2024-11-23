## SPDX-License-Identifier: GPL-2.0-only
bootblock-$(CONFIG_SOC_INTEL_COMMON_BLOCK_LPC) += lpc_lib.c

romstage-$(CONFIG_SOC_INTEL_COMMON_BLOCK_LPC) += lpc_lib.c

ramstage-$(CONFIG_SOC_INTEL_COMMON_BLOCK_LPC) += lpc_lib.c
ramstage-$(CONFIG_SOC_INTEL_COMMON_BLOCK_LPC) += lpc.c

ifeq ($(CONFIG_SOC_INTEL_COMMON_SPI_LOCKDOWN_SMM),y)
smm-$(CONFIG_SOC_INTEL_COMMON_BLOCK_LPC) += lpc_lib.c
endif
