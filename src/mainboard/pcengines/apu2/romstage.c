/* SPDX-License-Identifier: GPL-2.0-only */

#include <stdint.h>
#include <string.h>
#include <delay.h>
#include <device/pci_def.h>
#include <arch/io.h>
#include <arch/mmio.h>
#include <amdblocks/acpimmio.h>
#include <amdblocks/gpio.h>
#include <amdblocks/gpio_defs.h>
#include <device/pci_def.h>
#include <device/pci_ops.h>
#include <console/console.h>
#include <console/uart.h>
#include <gpio.h>
#include <northbridge/amd/agesa/state_machine.h>

#include <southbridge/amd/pi/hudson/hudson.h>
#include <Fch/Fch.h>
#include <smp/node.h>

#include "bios_knobs.h"
#include "gpio_ftns.h"

static void early_lpc_init(void);
static void print_sign_of_life(void);
extern char coreboot_dmi_date[];

void board_BeforeAgesa(struct sysinfo *cb)
{
	u32 val;

	early_lpc_init();

	/* Disable SVI2 controller to wait for command completion */
	val = pci_read_config32(PCI_DEV(0, 0x18, 5), 0x12C);
	if (!(val & (1 << 30))) {
		val |= (1 << 30);
		pci_write_config32(PCI_DEV(0, 0x18, 5), 0x12C, val);
	}

	/* Release GPIO32/33 for other uses. */
	pm_write8(0xea, 1);
}

const struct soc_amd_gpio gpio_common[] = {
	PAD_GPI(GPIO_49, PULL_NONE),
	PAD_GPI(GPIO_50, PULL_NONE),
	PAD_GPI(GPIO_71, PULL_NONE),
	PAD_GPO(GPIO_57, LOW),
	PAD_GPO(GPIO_58, LOW),
	PAD_GPO(GPIO_59, LOW),
	PAD_GPO(GPIO_51, HIGH),
	PAD_GPO(GPIO_55, HIGH),
	PAD_GPO(GPIO_64, HIGH),
	PAD_GPO(GPIO_68, HIGH),
};

const struct soc_amd_gpio gpio_apu2[] = {
	PAD_GPI(GPIO_32, PULL_NONE),
};

const struct soc_amd_gpio gpio_apu34[] = {
	PAD_GPI(GPIO_32, PULL_NONE),
	PAD_GPO(GPIO_33, LOW),
};

const struct soc_amd_gpio gpio_apu5[] = {
	PAD_GPI(GPIO_22, PULL_NONE),
	PAD_GPO(GPIO_32, HIGH),
	PAD_GPO(GPIO_33, HIGH),
};

static void early_lpc_init(void)
{
	gpio_configure_pads(gpio_common, ARRAY_SIZE(gpio_common));

<<<<<<< HEAD
	if (CONFIG(BOARD_PCENGINES_APU2))
		gpio_configure_pads(gpio_apu2, ARRAY_SIZE(gpio_apu2));

	if (CONFIG(BOARD_PCENGINES_APU3) || CONFIG(BOARD_PCENGINES_APU4))
		gpio_configure_pads(gpio_apu34, ARRAY_SIZE(gpio_apu34));

	if (CONFIG(BOARD_PCENGINES_APU5))
		gpio_configure_pads(gpio_apu5, ARRAY_SIZE(gpio_apu5));
}

static void print_sign_of_life(void)
{
	char tmp[9];
	strncpy(tmp,   coreboot_dmi_date+6, 4);
	strncpy(tmp+4, coreboot_dmi_date+3, 2);
	strncpy(tmp+6, coreboot_dmi_date,   2);
	tmp[8] = '\0';
	printk(BIOS_ALERT, CONFIG_MAINBOARD_VENDOR " "
	                   CONFIG_MAINBOARD_PART_NUMBER "\n");
	printk(BIOS_ALERT, "coreboot build %s\n", tmp);
	printk(BIOS_ALERT, "BIOS version %s\n", COREBOOT_ORIGIN_GIT_TAG);
=======
	//
	// Configure output disabled, value low, pull up/down disabled
	//
#if iS_ENABLED(CONFIG_BOARD_PCENGINES_APU5)
	configure_gpio(IOMUX_GPIO_22, Function0, GPIO_22, setting);
#endif

#if IS_ENABLED(CONFIG_BOARD_PCENGINES_APU2) || IS_ENABLED(CONFIG_BOARD_PCENGINES_APU3) || IS_ENABLED(CONFIG_BOARD_PCENGINES_APU4)
	configure_gpio(IOMUX_GPIO_32, Function0, GPIO_32, setting);
#endif
	configure_gpio(IOMUX_GPIO_49, Function2, GPIO_49, setting);
	configure_gpio(IOMUX_GPIO_50, Function2, GPIO_50, setting);
	configure_gpio(IOMUX_GPIO_71, Function0, GPIO_71, setting);
	//
	// Configure output enabled, value low, pull up/down disabled
	//
	setting = GPIO_OUTPUT_ENABLE;
#if IS_ENABLED(CONFIG_BOARD_PCENGINES_APU3) || IS_ENABLED(CONFIG_BOARD_PCENGINES_APU4)
	configure_gpio(IOMUX_GPIO_33, Function0, GPIO_33, setting);
#endif
	configure_gpio(IOMUX_GPIO_57, Function1, GPIO_57, setting);
	configure_gpio(IOMUX_GPIO_58, Function1, GPIO_58, setting);
	configure_gpio(IOMUX_GPIO_59, Function3, GPIO_59, setting);
	//
	// Configure output enabled, value high, pull up/down disabled
	//
	setting = GPIO_OUTPUT_ENABLE | GPIO_OUTPUT_VALUE;
#if IS_ENABLED(CONFIG_BOARD_PCENGINES_APU5)
	configure_gpio(IOMUX_GPIO_32, Function0, GPIO_32, setting);
	configure_gpio(IOMUX_GPIO_33, Function0, GPIO_33, setting);
#endif
	configure_gpio(IOMUX_GPIO_51, Function2, GPIO_51, setting);
	configure_gpio(IOMUX_GPIO_55, Function3, GPIO_55, setting);
	configure_gpio(IOMUX_GPIO_64, Function2, GPIO_64, setting);
	configure_gpio(IOMUX_GPIO_68, Function0, GPIO_68, setting);
>>>>>>> ae68834b3346 (fix errors during test-lint)
}
