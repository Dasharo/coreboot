/* SPDX-License-Identifier: GPL-2.0-only */

#include <cpu/x86/smm.h>
#include <device/pnp_ops.h>
#include <superio/nuvoton/nct6687d/nct6687d.h>

#define POWER_DEV PNP_DEV(0x4e, NCT6687D_SLEEP_PWR)
#define ACPI_DEV PNP_DEV(0x4e, NCT6687D_ACPI)

#define NUVOTON_ENTRY_KEY 0x87
#define NUVOTON_EXIT_KEY 0xAA

static void nuvoton_pnp_enter_conf_state(pnp_devfn_t dev)
{
	u16 port = dev >> 8;
	outb(NUVOTON_ENTRY_KEY, port);
	outb(NUVOTON_ENTRY_KEY, port);
}

static void nuvoton_pnp_exit_conf_state(pnp_devfn_t dev)
{
	u16 port = dev >> 8;
	outb(NUVOTON_EXIT_KEY, port);
}

static void disable_ps2_wake(void)
{
	uint8_t reg;

	pnp_set_logical_device(ACPI_DEV);
	reg = pnp_read_config(ACPI_DEV, 0xe4);
	reg &= ~0xc0; /* Disable keyboard and mouse wake via PSOUT# */
	reg &= ~0x08; /* Disable keyboard wake with any key */
	pnp_write_config(ACPI_DEV, 0xe4, reg);
}

void mainboard_smi_sleep(u8 slp_typ)
{
	nuvoton_pnp_enter_conf_state(POWER_DEV);
	pnp_set_logical_device(POWER_DEV);

	switch (slp_typ) {
	case 3:
		pnp_write_config(POWER_DEV, 0xe7, 0x20); /* Keep LED freq at S3-S5 */
		pnp_write_config(POWER_DEV, 0xe8, 0x05); /* Blink LED at 1Hz */
		break;
	case 4:
		pnp_write_config(POWER_DEV, 0xe7, 0x88); /* Set AUTO_EN */
		pnp_write_config(POWER_DEV, 0xe8, 0x07); /* LED to low */
		break;
	case 5:
		pnp_write_config(POWER_DEV, 0xe7, 0x88); /* Set AUTO_EN */
		pnp_write_config(POWER_DEV, 0xe8, 0x07); /* LED to low */
		disable_ps2_wake();
		break;
	}

	nuvoton_pnp_exit_conf_state(POWER_DEV);
}
