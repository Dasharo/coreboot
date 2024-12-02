/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <arch/mmio.h>
#include <device/device.h>
#include <fmap.h>
#include <gpio.h>
#include <pc80/i8254.h>
#include <soc/gpio.h>
#include <soc/intel/common/reset.h>
#include <soc/iomap.h>
#include <soc/ramstage.h>

#define IFD_FIA_COMBO_PORT0	0x189
#define  FIA_PCIE		5
#define  FIA_SATA		7

/* Flash Master 1 : HOST/BIOS */
#define FLMSTR1			0x80
/* Flash signature Offset */
#define FLASH_SIGN_OFFSET	0x10
#define FLMSTR_WR_SHIFT_V2	20
#define FLASH_VAL_SIGN		0xFF0A55A
#define SI_DESC_SIZE		0x1000
#define SI_DESC_REGION		"SI_DESC"

/* It checks whether host (Flash Master 1) has write access to the Descriptor Region or not */
static bool is_descriptor_writeable(uint8_t *desc)
{
	/* Check flash has valid signature */
	if (read32((void *)(desc + FLASH_SIGN_OFFSET)) != FLASH_VAL_SIGN) {
		printk(BIOS_ERR, "Flash Descriptor is not valid\n");
		return 0;
	}
	/* Check host has write access to the Descriptor Region */
	if (!((read32((void *)(desc + FLMSTR1)) >> FLMSTR_WR_SHIFT_V2) & BIT(0))) {
		printk(BIOS_ERR, "Host doesn't have write access to Descriptor Region\n");
		return 0;
	}
	return 1;
}

static void descriptor_patch_pcie8_lane(int is_nvme)
{
	uint8_t *si_desc_buf;
	struct region_device desc_rdev;
	uint8_t state = is_nvme ? FIA_PCIE : FIA_SATA;

	si_desc_buf = (uint8_t *)malloc(SI_DESC_SIZE);

	if (!si_desc_buf) {
		printk(BIOS_ERR, "Failed to allocate buffer for %s\n", SI_DESC_REGION);
		return;
	}

	if (fmap_locate_area_as_rdev_rw(SI_DESC_REGION, &desc_rdev) < 0) {
		printk(BIOS_ERR, "Failed to locate %s in the FMAP\n", SI_DESC_REGION);
		free(si_desc_buf);
		return;
	}
	if (rdev_readat(&desc_rdev, si_desc_buf, 0, SI_DESC_SIZE) != SI_DESC_SIZE) {
		printk(BIOS_ERR, "Failed to read Descriptor Region from SPI Flash\n");
		free(si_desc_buf);
		return;
	}
	if (!is_descriptor_writeable(si_desc_buf)) {
		free(si_desc_buf);
		return;
	}

	/*
	 * Offset 0x189 bits [0:3]: 5=PCIe, 7=SATA.
	 * Not patching the descriptor will result in SATA or NVMe not working.
	 */
	if ((si_desc_buf[IFD_FIA_COMBO_PORT0] & 0xf) == state) {
		printk(BIOS_DEBUG, "Update of Descriptor is not required!\n");
		free(si_desc_buf);
		return;
	}

	si_desc_buf[IFD_FIA_COMBO_PORT0] &= 0xf0;
	si_desc_buf[IFD_FIA_COMBO_PORT0] |= state;

	/* FIT als sets reserved offset 0x1a4 bits [0:3]: 5=PCIe, 1=SATA */
	if (state == FIA_PCIE) {
		si_desc_buf[0x1a4] &= 0xf0;
		si_desc_buf[0x1a4] |= 5;
	} else {
		si_desc_buf[0x1a4] &= 0xf0;
		si_desc_buf[0x1a4] |= 1;
	}

	if (rdev_eraseat(&desc_rdev, 0, SI_DESC_SIZE) != SI_DESC_SIZE) {
		printk(BIOS_ERR, "Failed to erase Descriptor Region area\n");
		free(si_desc_buf);
		return;
	}

	if (rdev_writeat(&desc_rdev, si_desc_buf, 0, SI_DESC_SIZE) != SI_DESC_SIZE) {
		printk(BIOS_ERR, "Failed to update Descriptor Region\n");
		free(si_desc_buf);
		return;
	}

	printk(BIOS_DEBUG, "Update of Descriptor successful\n");
	free(si_desc_buf);
	do_global_reset();
}

void mainboard_silicon_init_params(FSP_S_CONFIG *params)
{
	params->UsbPdoProgramming = 0;

	for (uint8_t i = 0; i < CONFIG_MAX_ROOT_PORTS; i++) {
		params->PcieRpAcsEnabled[i] = 1;
		params->PcieRpAdvancedErrorReporting[i] = 1;
		params->PcieRpLtrEnable[i] = 1;
		params->PcieRpMaxPayload[i] = 1; // 256B
		params->PcieRpSlotImplemented[i] = 1;
	}

	if (CONFIG(BOARD_PROTECTLI_V1610)) {
		/* Read GPP_C6 (M2_PEDET) to check for disk type: 1=NVME, 0=SATA */
		gpio_input_pullup(GPP_C6);
		/*
		 * Patch the descriptor accordingly, by default the lanes 7 and 8
		 * are in 1x2 mode. Lane 8 can be statically assigned to either
		 * PCIe or SATA
		 */
		descriptor_patch_pcie8_lane(gpio_get(GPP_C6));
	}

	/*
	 * HWP is too aggressive in power savings and does not let using full
	 * bandwidth of Ethernet controllers without additional stressing of
	 * the CPUs (2Gb/s vs 2.35Gb/s with stressing, measured with iperf3).
	 * Let the Linux use acpi-cpufreq governor driver instead of
	 * intel_pstate by disabling HWP.
	 */
	params->Hwp = 0;
}

static void mainboard_final(void *unused)
{
	if (CONFIG(BEEP_ON_BOOT))
		beep(1500, 100);
}

struct chip_operations mainboard_ops = {
	.final = mainboard_final,
};
