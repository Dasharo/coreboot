
/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <acpi/acpi.h>
#include <arch/cpu.h>
#include <device/device.h>
#include <pc80/i8254.h>
#include <soc/gpio.h>
#include <soc/ramstage.h>
#include <smbios.h>
#include <stdlib.h>
#include <string.h>

#include "gpio.h"

const char *smbios_mainboard_product_name(void)
{
	u32 tmp[13];
	const char *str = "Unknown Processor Name";

	if (cpu_have_cpuid()) {
		int i;
		struct cpuid_result res = cpuid(0x80000000);
		if (res.eax >= 0x80000004) {
			int j = 0;
			for (i = 0; i < 3; i++) {
				res = cpuid(0x80000002 + i);
				tmp[j++] = res.eax;
				tmp[j++] = res.ebx;
				tmp[j++] = res.ecx;
				tmp[j++] = res.edx;
			}
			tmp[12] = 0;
			str = (const char *)tmp;
		}
	}

	if (strstr(str, "i3-10110U") != NULL)
		return "VP4630";
	else if (strstr(str, "i5-10210U") != NULL)
		return "VP4650";
	else if (strstr(str, "i7-10810U") != NULL)
		return "VP4670";
	else
		return CONFIG_MAINBOARD_SMBIOS_PRODUCT_NAME;
}

static void mainboard_final(void *unused)
{
	if (CONFIG(BEEP_ON_BOOT))
		beep(1500, 100);
}

static unsigned long mainboard_write_acpi_tables(const struct device *device,
					  unsigned long start,
					  acpi_rsdp_t *rsdp)
{
	unsigned long current;
	acpi_dbg2_header_t *dbg2;
	acpi_dbgp_t *dbgp;
	acpi_addr_t address;

	printk(BIOS_DEBUG, "ACPI:     * DBG2\n");
	current = acpi_align_current(start);
	dbg2 = (acpi_dbg2_header_t *)current;

	memset(&address, 0, sizeof(address));
	address.space_id = ACPI_ADDRESS_SPACE_IO;
	address.addrl = (uint32_t)0x3f8;

	acpi_create_dbg2(dbg2,
			 ACPI_DBG2_PORT_SERIAL,
			 ACPI_DBG2_PORT_SERIAL_16550_DBGP,
			 &address, 8, NULL);

	if (dbg2->header.length) {
		current += dbg2->header.length;
		current = acpi_align_current(current);
		acpi_add_table(rsdp, dbg2);
	}

	printk(BIOS_DEBUG, "ACPI:     * DBGP\n");
	current = acpi_align_current(current);
	dbgp = (acpi_dbgp_t *)current;

	acpi_create_dbgp(dbgp,
			 ACPI_DBGP_PORT_SERIAL_16550_DBGP,
			 &address);

	if (dbgp->header.length) {
		current += dbgp->header.length;
		current = acpi_align_current(current);
		acpi_add_table(rsdp, dbgp);
	}

	return current;
}

static void mainboard_enable(struct device *dev)
{
	dev->ops->write_acpi_tables = mainboard_write_acpi_tables;
}

struct chip_operations mainboard_ops = {
	.enable_dev = mainboard_enable,
	.final = mainboard_final,
};
