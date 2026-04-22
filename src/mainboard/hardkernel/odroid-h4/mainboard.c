/* SPDX-License-Identifier: GPL-2.0-only */

#include <bootstate.h>
#include <dasharo/options.h>
#include <device/device.h>
#include <device/pnp.h>
#include <device/pnp_def.h>
#include <soc/ramstage.h>
#include <soc/gpio.h>
#include <gpio.h>
#include <intelblocks/cse.h>
#include <superio/ite/it8613e/it8613e.h>
#include <smbios.h>
#include <string.h>

#define ITE_GPIO_PIN(x)		(1 << ((x) % 10))
#define ITE_GPIO_SET(x)		(((x) / 10) - 1)
#define ITE_GPIO_IO_ADDR(x)	(iobase + ITE_GPIO_SET(x))

void mainboard_silicon_init_params(FSP_S_CONFIG *params)
{
	memset(params->PcieRpEnableCpm, 0, sizeof(params->PcieRpEnableCpm));
	memset(params->PcieRpPmSci, 0, sizeof(params->PcieRpPmSci));

	params->PcieRpEnableCpm[2] = 1; // LAN1
	params->PcieRpEnableCpm[3] = 1; // LAN2
	params->PcieRpEnableCpm[6] = 1; // ASMedia PCIe to SATA
	if (!CONFIG(ODROID_H4_NETCARD_SUPPORT))
		params->PcieRpEnableCpm[8] = 1; // NVMe

	// Max payload 256B
	memset(params->PcieRpMaxPayload, 1, sizeof(params->PcieRpMaxPayload));

	// I2C
	params->PchSerialIoI2cSdaPinMux[0] = 0x1947c404; // GPP_H4
	params->PchSerialIoI2cSclPinMux[0] = 0x1947a405; // GPP_H5
	params->PchSerialIoI2cSdaPinMux[1] = 0x1947c606; // GPP_H6
	params->PchSerialIoI2cSclPinMux[1] = 0x1947a607; // GPP_H7

	params->CnviRfResetPinMux = 0;
	params->CnviClkreqPinMux = 0;
}

#if CONFIG(GENERATE_SMBIOS_TABLES)
static int mainboard_smbios_data(struct device *dev, int *handle, unsigned long *current)
{
	int len = 0;

	len += cse_write_smbios_type14(handle, current);

	return len;
}
#endif

static void mainboard_enable(struct device *dev)
{
	config_t *config = config_of_soc();
#if CONFIG(GENERATE_SMBIOS_TABLES)
	dev->ops->get_smbios_data = mainboard_smbios_data;
#endif
}

static void mainboard_final(void *unused)
{
	struct device *gpio_dev = dev_find_slot_pnp(0x2e, IT8613E_GPIO);
	struct resource *gpio_iobase = NULL;
	uint16_t iobase;

	if (!gpio_dev) {
		printk(BIOS_WARNING, "%s: ITE GPIO PNP device not found\n",
		       __func__);
		return;
	}

	gpio_iobase = probe_resource(gpio_dev, PNP_IDX_IO1);

	if (!gpio_iobase || gpio_iobase->base == 0) {
		printk(BIOS_WARNING, "%s: ITE GPIO I/O resource not found or not assigned\n",
		       __func__);
		return;
	}

	iobase = gpio_iobase->base & 0xffff;

	/* GP21 and GP23 to low to enable USB ports VBUS */
	outb(inb(ITE_GPIO_IO_ADDR(21)) & ~ITE_GPIO_PIN(21), ITE_GPIO_IO_ADDR(21));
	outb(inb(ITE_GPIO_IO_ADDR(23)) & ~ITE_GPIO_PIN(23), ITE_GPIO_IO_ADDR(23));
}

/* Ensure the USB ports power is applied on S3 resume too */
BOOT_STATE_INIT_ENTRY(BS_OS_RESUME, BS_ON_ENTRY, mainboard_final, NULL);

struct chip_operations mainboard_ops = {
	.enable_dev = mainboard_enable,
	.final = mainboard_final
};
