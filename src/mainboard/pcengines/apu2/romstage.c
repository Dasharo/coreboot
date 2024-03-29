/* SPDX-License-Identifier: GPL-2.0-only */

#include <stdint.h>
#include <Fch/Fch.h>
#include <amdblocks/acpimmio.h>
#include <amdblocks/gpio.h>
#include <amdblocks/gpio_defs.h>
#include <device/pci_def.h>
#include <device/pci_ops.h>
#include <gpio.h>
#include <northbridge/amd/agesa/state_machine.h>
#include <smp/node.h>

#include "gpio_ftns.h"

static void early_lpc_init(void);

void board_BeforeAgesa(struct sysinfo *cb)
{
	u32 val;

	/* CF9 shadow */
	pm_write8(FCH_PMIOA_REGC5, 0);

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

void board_BeforeInitReset(struct sysinfo *cb, AMD_RESET_PARAMS *Reset)
{
	u32 val;

	if (boot_cpu()) {
		/* CF9 shadow: 0 */
		pm_write8(FCH_PMIOA_REGC5, 0);

		/* Check if cold boot was requested */
		val = pci_read_config32(PCI_DEV(0, 0x18, 0), 0x6C);
		if (val & (1 << 4)) {
			printk(BIOS_ALERT, "Forcing cold boot path\n");
			val &= ~(0x630); // ColdRstDet[4], BiosRstDet[10:9, 5]
			pci_write_config32(PCI_DEV(0, 0x18, 0), 0x6C, val);

			/* S5ResetStatus */
			pm_write32(0xc0, 0x3fff003f); // Write-1-to-clear resets

			/* CF9 shadow: FullRst, SysRst, RstCmd */
			pm_write8(FCH_PMIOA_REGC5, 0xe);
			printk(BIOS_ALERT, "Did not reset (yet)\n");
		}
	}
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

const struct soc_amd_gpio gpio_apu346[] = {
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

	if (CONFIG(BOARD_PCENGINES_APU2))
		gpio_configure_pads(gpio_apu2, ARRAY_SIZE(gpio_apu2));

	if (CONFIG(BOARD_PCENGINES_APU3) || CONFIG(BOARD_PCENGINES_APU4) ||
	    CONFIG(BOARD_PCENGINES_APU6))
		gpio_configure_pads(gpio_apu346, ARRAY_SIZE(gpio_apu346));

	if (CONFIG(BOARD_PCENGINES_APU5))
		gpio_configure_pads(gpio_apu5, ARRAY_SIZE(gpio_apu5));
}
