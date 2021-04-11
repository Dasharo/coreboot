/* SPDX-License-Identifier: GPL-2.0-only */

#include <console/console.h>
#include <cpu/power/vpd.h>
#include <program_loading.h>

void main(void)
{
	console_init();

	vpd_pnor_main();

	run_ramstage();
}
