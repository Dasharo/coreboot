/* SPDX-License-Identifier: GPL-2.0-only */

#include <acpi/acpi_device.h>
#include <amdblocks/acpi.h>
#include <amdblocks/amd_pci_util.h>
#include <bootstate.h>
#include <cbmem.h>
#include <commonlib/helpers.h>
#include <device/azalia_device.h>
#include <device/device.h>
#include <device/pnp_def.h>
#include <drivers/amd/opensil/opensil.h>
#include <static.h>
#include <stdio.h>
#include <superio/nuvoton/nct6687d/nct6687d.h>
#include <superio/nuvoton/nct6687d/nct6687d_gpio.h>
#include <superio/nuvoton/nct6687d/nct6687d_hwm.h>
#include <types.h>
#include "gpio.h"

/* The IRQ mapping in fch_irq_map ends up getting written to the indirect address space that is
   accessed via I/O ports 0xc00/0xc01. */

/*
 * This controls the device -> IRQ routing.
 *
 * Hardcoded IRQs:
 *  0: timer < soc/amd/common/acpi/lpc.asl
 *  1: i8042 - Keyboard
 *  2: cascade
 *  8: rtc0 <- soc/amd/common/acpi/lpc.asl
 *  9: acpi <- soc/amd/common/acpi/lpc.asl
 */
static const struct fch_irq_routing fch_irq_map[] = {
	{ PIRQ_A,	0x03,		0x10 },
	{ PIRQ_B,	0x04,		0x11 },
	{ PIRQ_C,	0x05,		0x12 },
	{ PIRQ_D,	0x06,		0x13 },
	{ PIRQ_E,	0x0a,		0x14 },
	{ PIRQ_F,	0x0b,		0x15 },
	{ PIRQ_G,	0x0e,		0x16 },
	{ PIRQ_H,	0x0f,		0x17 },
	{ PIRQ_SCI,	ACPI_SCI_IRQ,	ACPI_SCI_IRQ },
	{ PIRQ_SD,	PIRQ_NC,	0x10 },
	{ PIRQ_SDIO,	PIRQ_NC,	0x10 },
	{ PIRQ_GPIO,	0x07,		0x07 },
	{ PIRQ_EMMC,	PIRQ_NC,	0x05 },
	{ PIRQ_I2C0,	PIRQ_NC,	0x03 },
	{ PIRQ_I2C1,	PIRQ_NC,	PIRQ_NC },
	{ PIRQ_I2C2,	PIRQ_NC,	PIRQ_NC },
	{ PIRQ_I2C3,	PIRQ_NC,	PIRQ_NC },
	{ PIRQ_UART0,	PIRQ_NC,	PIRQ_NC },
	{ PIRQ_UART1,	PIRQ_NC,	PIRQ_NC },
	{ PIRQ_UART2,	PIRQ_NC,	PIRQ_NC },
	{ PIRQ_UART3,	PIRQ_NC,	PIRQ_NC },
	{ PIRQ_UART4,	PIRQ_NC,	PIRQ_NC },
	/* The MISC registers are not interrupt numbers */
	{ PIRQ_MISC,	0xfa,		0x00 },
	{ PIRQ_MISC0,	0x91,		0x00 },
	{ PIRQ_HPET_L,	0x00,		0x00 },
	{ PIRQ_HPET_H,	0x00,		0x00 },
};


const struct fch_irq_routing *mb_get_fch_irq_mapping(size_t *length)
{
	*length = ARRAY_SIZE(fch_irq_map);
	return fch_irq_map;
}

static const char *hda_acpi_name(const struct device *dev)
{
	return "AZAL";
}

static const char *gfx_hda_acpi_name(const struct device *dev)
{
	return "HDAU";
}

static const char *crypto_acpi_name(const struct device *dev)
{
	return "APSP";
}

#define SET_AUDIO_DEV_OPS(dev) \
	struct device *dev = (struct device *)DEV_PTR(dev); \
	if (is_dev_enabled(dev)) { \
		(dev)->ops = &phx_ ## dev ## _audio_ops; \
		(dev)->ops->acpi_name = dev ## _acpi_name; \
		(dev)->ops->acpi_fill_ssdt = acpi_device_write_pci_dev; \
	}

static struct device_operations phx_hda_audio_ops;
static struct device_operations phx_gfx_hda_audio_ops;

static void mainboard_init(void *chip_info)
{
	struct device *psp = (struct device *)DEV_PTR(crypto);
	struct memory_info *mem_info;


	mainboard_program_gpios();

	memcpy(&phx_hda_audio_ops, &default_azalia_audio_ops,
	       sizeof(default_azalia_audio_ops));
	memcpy(&phx_gfx_hda_audio_ops, &default_azalia_audio_ops,
	       sizeof(default_azalia_audio_ops));

	SET_AUDIO_DEV_OPS(gfx_hda);
	SET_AUDIO_DEV_OPS(hda);

	if (is_dev_enabled(psp)) {
		psp->ops->acpi_name = crypto_acpi_name;
		psp->ops->acpi_fill_ssdt = acpi_device_write_pci_dev;
	}

	/*
	 * Update maximum memory capacity for SMBIOS type 16 for this board.
	 * Phoenix can support up to 256GB of memory on 4 DIMMs. This
	 * board populates all 2 channels with 2 DIMMs per channel.
	 */
	mem_info = cbmem_find(CBMEM_ID_MEMINFO);
	if (mem_info)
		mem_info->max_capacity_mib = 256 * (GiB / MiB);
}

struct chip_operations mainboard_ops = {
	.init = mainboard_init
};

bool mainboard_dimm_slot_exists(uint8_t socket, uint8_t channel, uint8_t slot)
{
	if (socket > 0 || channel > 1)
		return false;

	/* First 2 channels fully populated */
	return true;
}

void smbios_fill_dimm_locator(const struct dimm_info *dimm, struct smbios_type17 *t)
{
	char locator[40];

	/*
	 * DIMM slots are named by channel number and dimm number:
	 * Channels: A, B.
	 * DIMMs: A1, A2, B1, B2.
	 */
	snprintf(locator, sizeof(locator), "DIMM%c%u",
		'A' + dimm->channel_num, dimm->dimm_num + 1);
	t->device_locator = smbios_add_string(t->eos, locator);

	snprintf(locator, sizeof(locator), "BANK %d", dimm->bank_locator);
	t->bank_locator = smbios_add_string(t->eos, locator);
}

static void nct6687d_misc_init(void *unused)
{
	struct device *ec_dev = dev_find_slot_pnp(0x4e, NCT6687D_EC);
	const struct resource *res;
	uint16_t ec_iobase;

	if (!ec_dev)
		return;

	res = probe_resource(ec_dev, PNP_IDX_IO0);
	if (!res || !res->base)
		return;

	ec_iobase = (uint16_t)res->base;

	hwm_reg_write(EC_PAGE_REG(2, 0x42), 0x40);
	hwm_reg_write(EC_PAGE_REG(2, 0x43), 0x40);
	hwm_reg_write(EC_PAGE_REG(2, 0x44), 0x40);
	hwm_reg_write(EC_PAGE_REG(2, 0x45), 0x40);

	hwm_reg_write(EC_PAGE_REG(2, 0x62), 0x99);
	hwm_reg_write(EC_PAGE_REG(2, 0x63), 0x99);
	hwm_reg_write(EC_PAGE_REG(2, 0x64), 0x99);
	hwm_reg_write(EC_PAGE_REG(2, 0x65), 0x99);

	hwm_reg_write(EC_PAGE_REG(2, 0xa8), 0x10);

	hwm_reg_write(SENSOR_INT_CFG_REG(2), TMPIN_INT_OVT_COMP_MODE);
	hwm_reg_write(TMPIN_HYSTERSIS_REG(2), 0x5a);
	hwm_reg_write(TMPIN_OVERTEMP_REG(2), 0x7d);

	hwm_reg_write(SENSOR_INT_CFG_REG(31), TMPIN_INT_OVT_COMP_MODE);
	hwm_reg_write(TMPIN_HYSTERSIS_REG(31), 0x5a);
	hwm_reg_write(TMPIN_OVERTEMP_REG(31), 0x7d);

	hwm_reg_write(EC_PAGE_REG(3, 0xda), 0x04);
	hwm_reg_write(EC_PAGE_REG(7, 0x63), 0x80);

	/* Turn off GPIO54: EZ Debug LED - CPU */
	nct6687d_gpio_set(ec_iobase, 54, 0, NCT6687D_GPIO_OPENDRAIN);
}

BOOT_STATE_INIT_ENTRY(BS_DEV_INIT, BS_ON_EXIT, nct6687d_misc_init, NULL);
