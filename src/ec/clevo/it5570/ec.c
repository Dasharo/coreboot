/* SPDX-License-Identifier: GPL-2.0-only */

#include <acpi/acpi.h>
#include <console/console.h>
#include <cpu/x86/smm.h>
#include <ec/acpi/ec.h>

#define CLEVO_EC_CMD_ENABLE_ACPI_MODE	0x90
#define CLEVO_EC_CMD_DISABLE_ACPI_MODE	0x91

int clevo_it5570_ec_smi_apmc(int apmc)
{
	switch (apmc) {
	case APM_CNT_ACPI_ENABLE:
		return send_ec_command(CLEVO_EC_CMD_ENABLE_ACPI_MODE);
	case APM_CNT_ACPI_DISABLE:
		return send_ec_command(CLEVO_EC_CMD_DISABLE_ACPI_MODE);
	default:
		break;
	}

	return 0;
}