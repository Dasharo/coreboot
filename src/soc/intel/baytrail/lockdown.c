#include <stdint.h>
#include <acpi/acpi_gnvs.h>
#include <arch/io.h>
#include <device/pci_ops.h>
#include <console/console.h>
#include <cpu/x86/cache.h>
#include <cpu/x86/smm.h>
#include <cpu/intel/em64t100_save_state.h>
#include <device/pci_def.h>
#include <elog.h>
#include <halt.h>
#include <spi-generic.h>
#include <smmstore.h>
#include <soc/spi.h>
#include <soc/iomap.h>
#include <soc/iosf.h>
#include <soc/pci_devs.h>
#include <soc/pm.h>
#include <soc/nvs.h>
#include <soc/device_nvs.h>
#include <soc/lockdown.h>
#include <bootstate.h>
#include <dasharo/options.h>

void enable_smm_bwp(void){
	void *scs = (void *)(SPI_BASE_ADDRESS + SCS);
	void *bcr = (void *)(SPI_BASE_ADDRESS + BCR);
	uint32_t reg;

	// Set to enable SMI generation on WP attempts
	reg = read32(scs);
	reg |= SMIWPEN;  // Set SMIWPEN
	write32(scs, reg);

	reg = read32(bcr);
	// Set to enforce SMM-based protection
	reg |= EISS;
	// Set to lock the BIOS write protection settings
	reg |= BCR_LE;
	write32(bcr, reg);
}

void disable_smm_bwp(void){
	void *scs = (void *)(SPI_BASE_ADDRESS + SCS);
	void *bcr = (void *)(SPI_BASE_ADDRESS + BCR);
	uint32_t reg;

	write32(scs, read32(scs) & ~SMIWPEN);
	reg = (read32(bcr) & ~SRC_MASK) | BCR_WPD;
	reg &= ~EISS;
	write32(bcr, reg);
}

bool wpd_status(void)
{
	void *bcr = (void *)(SPI_BASE_ADDRESS + BCR);
	uint32_t reg;
	reg = read32(bcr);
	return (reg & BCR_WPD);
}

void platform_lockdown_config(void *unused){
	if(CONFIG(BOOTMEDIA_SMM_BWP) && is_smm_bwp_permitted()){
		printk(BIOS_DEBUG, "Enabling SMM BWP\n");
		enable_smm_bwp();
	}
}

BOOT_STATE_INIT_ENTRY(BS_DEV_RESOURCES, BS_ON_EXIT, platform_lockdown_config, NULL);
