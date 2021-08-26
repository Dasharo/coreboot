/* SPDX-License-Identifier: GPL-2.0-only */

#include <acpi/acpi.h>
#include <console/console.h>
#include <cpu/x86/smm.h>
#include <ec/acpi/ec.h>

#include "ec.h"

#define CLEVO_EC_CMD_ENABLE_ACPI_MODE	0x90
#define CLEVO_EC_CMD_DISABLE_ACPI_MODE	0x91

void clevo_it5570_ec_smi_apmc(int apmc)
{
	switch (apmc) {
	case APM_CNT_ACPI_ENABLE:
		send_ec_command(CLEVO_EC_CMD_ENABLE_ACPI_MODE);
		send_ec_command(0x98);
		ec_clr_bit(0xe8, 2);
		break;
	case APM_CNT_ACPI_DISABLE:
		send_ec_command(CLEVO_EC_CMD_DISABLE_ACPI_MODE);
		break;
	default:
		break;
	}

}
