/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <console/console.h>
#include <device/device.h>
#include <drivers/intel/gma/int15.h>
#include <gpio.h>
#include <soc/gpio.h>

#define GPIO_CHASSIS_ID1 GPP_H20
#define GPIO_CHASSIS_ID2 GPP_H21

static void mainboard_enable(struct device *dev)
{
	int pin_sts;
	install_intel_vga_int15_handler(GMA_INT15_ACTIVE_LFP_NONE,
					GMA_INT15_PANEL_FIT_DEFAULT,
					GMA_INT15_BOOT_DISPLAY_DEFAULT, 0);

	/* Chassis type */
	pin_sts = gpio_get(GPIO_CHASSIS_ID1);
	pin_sts |= gpio_get(GPIO_CHASSIS_ID2) << 1;

	printk(BIOS_DEBUG, "Chassis type: ");
	switch (pin_sts) {
	case 0:
		printk(BIOS_DEBUG, "11L / SFF\n");
		break;
	case 1:
		printk(BIOS_DEBUG, "9.7L\n");
		break;
	case 2:
		printk(BIOS_DEBUG, "20L / Tower\n");
		break;
	case 3:
		printk(BIOS_DEBUG, "25L\n");
		break;
	default:
		printk(BIOS_DEBUG, "Unknown chassis type %u\n", pin_sts);
		break;
	}

	/* BOM option */
	pin_sts = gpio_get(GPP_G0);
	pin_sts |= gpio_get(GPP_G1) << 1;
	pin_sts |= gpio_get(GPP_G2) << 2;
	pin_sts |= gpio_get(GPP_G3) << 3;
	pin_sts |= gpio_get(GPP_G4) << 4;
	pin_sts |= gpio_get(GPP_G5) << 5;

	printk(BIOS_DEBUG, "BOM option: ");
	switch (pin_sts) {
	case 0:
		printk(BIOS_DEBUG, "NEC\n");
		break;
	case 0x0f:
		printk(BIOS_DEBUG, "KT\n");
		break;
	case 0x1f:
		printk(BIOS_DEBUG, "CONSUMER\n");
		break;
	case 0x3f:
		printk(BIOS_DEBUG, "TCM/TCE\n");
		break;
	default:
		printk(BIOS_DEBUG, "Unknown BOM option %u\n", pin_sts);
		break;
	}
}

struct chip_operations mainboard_ops = {
	.enable_dev = mainboard_enable,
};
