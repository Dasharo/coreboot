/* SPDX-License-Identifier: GPL-2.0-or-later */

/*
 * SMI handler for Hudson southbridges
 */

#include <amdblocks/acpi.h>
#include <amdblocks/smm.h>
#include <amdblocks/smi.h>
#include <soc/smi.h>
#include <arch/io.h>
#include <cpu/x86/smm.h>

static void fch_apmc_smi_handler_no_psp(void)
{
	const uint8_t cmd = inb(pm_acpi_smi_cmd_port());

	switch (cmd) {
	case APM_CNT_ACPI_ENABLE:
		acpi_clear_pm_gpe_status();
		acpi_enable_sci();
		break;
	case APM_CNT_ACPI_DISABLE:
		acpi_disable_sci();
		break;
	case APM_CNT_SMMSTORE:
		if (CONFIG(SMMSTORE))
			handle_smi_store();
		break;
	}

	mainboard_smi_apmc(cmd);
}

static const struct smi_sources_t smi_sources[] = {
	{ .type = SMITYPE_SMI_CMD_PORT, .handler = fch_apmc_smi_handler_no_psp },
};

void *get_smi_source_handler(int source)
{
	size_t i;

	for (i = 0 ; i < ARRAY_SIZE(smi_sources) ; i++)
		if (smi_sources[i].type == source)
			return smi_sources[i].handler;
	return NULL;
}
