## SPDX-License-Identifier: GPL-2.0-only
ramstage-$(CONFIG_SOC_INTEL_COMMON_PCH_LOCKDOWN) += lockdown.c
ramstage-$(CONFIG_SOC_INTEL_COMMON_PCH_LOCKDOWN) += lockdown_lpc.c
ramstage-$(CONFIG_SOC_INTEL_COMMON_PCH_LOCKDOWN) += lockdown_spi.c

smm-$(CONFIG_SOC_INTEL_COMMON_SPI_LOCKDOWN_SMM) += lockdown_lpc.c
smm-$(CONFIG_SOC_INTEL_COMMON_SPI_LOCKDOWN_SMM) += lockdown_spi.c
