## SPDX-License-Identifier: GPL-2.0-only
ifeq ($(CONFIG_SOC_AMD_COMMON_BLOCK_SPI),y)

all_x86-$(CONFIG_SOC_AMD_COMMON_BLOCK_SPI_MMAP) += mmap_boot.c
smm-$(CONFIG_SOC_AMD_COMMON_BLOCK_SPI_MMAP) += mmap_boot.c

all-$(CONFIG_SOC_AMD_COMMON_BLOCK_SPI_MMAP_USE_ROM3) += mmap_boot_rom3.c
smm-$(CONFIG_SOC_AMD_COMMON_BLOCK_SPI_MMAP_USE_ROM3) += mmap_boot_rom3.c

ifeq ($(CONFIG_SOC_AMD_COMMON_BLOCK_SPI_MMAP_USE_ROM3),y)
# The soft fuse bit 14 must be clear for ROM3 to work properly in coreboot.
ifneq ($(findstring 14,$(CONFIG_PSP_SOFTFUSE_BITS)),)
$(error "Soft fuse bit 14 must be clear to use ROM3.")
endif
endif

bootblock-y += fch_spi_ctrl.c
romstage-y += fch_spi_ctrl.c
verstage-y += fch_spi_ctrl.c
postcar-y += fch_spi_ctrl.c
ramstage-y += fch_spi_ctrl.c
smm-$(CONFIG_SPI_FLASH_SMM) += fch_spi_ctrl.c

bootblock-y += fch_spi.c
romstage-y += fch_spi.c
postcar-y += fch_spi.c
ramstage-y += fch_spi.c
verstage-y += fch_spi.c
smm-$(CONFIG_SPI_FLASH_SMM) += fch_spi.c

bootblock-y += fch_spi_util.c
romstage-y += fch_spi_util.c
postcar-y += fch_spi_util.c
ramstage-y += fch_spi_util.c
verstage-y += fch_spi_util.c
smm-$(CONFIG_SPI_FLASH_SMM) += fch_spi_util.c

ifneq ($(CONFIG_SOC_AMD_COMMON_BLOCK_PSP_ROM_ARMOR_DISABLED),y)
all_x86-y += rom_armor_boot_device_rw_nommap.c
smm-y += rom_armor_boot_device_rw_nommap.c
endif

endif # CONFIG_SOC_AMD_COMMON_BLOCK_SPI
