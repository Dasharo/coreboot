/* SPDX-License-Identifier: GPL-2.0-only */

#include <arch/io.h>
#include <stdint.h>
#include <console/console.h>
#include <cpu/power/istep_8.h>
#include <cpu/power/proc.h>

#include "fsi.h"

static void set_fsi_gp_shadow(uint8_t chip)
{
	enum {
		PERV_PERV_CTRL0_COPY_FSI = 0x0000291A,
		PERV_PERV_CTRL0_TP_OTP_SCOM_FUSED_CORE_MODE = 23,
	};

	uint32_t ctrl0_copy = read_cfam(chip, PERV_PERV_CTRL0_COPY_FSI);

	/* ATTR_FUSED_CORE_MODE (seems to be zero by default) */
	PPC_INSERT(ctrl0_copy, 0, PERV_PERV_CTRL0_TP_OTP_SCOM_FUSED_CORE_MODE, 1);

	write_cfam(chip, PERV_PERV_CTRL0_COPY_FSI, ctrl0_copy);
}

void istep_8_2(uint8_t chips)
{
	printk(BIOS_EMERG, "starting istep 8.2\n");
	report_istep(8,2);

	/* Skipping master chip */
	for (uint8_t chip = 1; chip < MAX_CHIPS; chip++) {
		if (chips & (1 << chip))
			set_fsi_gp_shadow(chip);
	}

	printk(BIOS_EMERG, "ending istep 8.2\n");
}
