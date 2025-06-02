/* SPDX-License-Identifier: GPL-2.0-only */

#include <arch/romstage.h>
#include <cbmem.h>
#include <console/console.h>
#include <cpu/x86/mtrr.h>
#include <main_decl.h>
#include <program_loading.h>
#include <timestamp.h>

/*
 * Systems without a native coreboot cache-as-ram teardown may implement
 * this to use an alternate method.
 */
__weak void late_car_teardown(void) { /* do nothing */ }

void main(void)
{

	post_code(0x99);
	printk(BIOS_INFO, "Postcar: hello\n");
	late_car_teardown();
	post_code(0x01);
	printk(BIOS_INFO, "car torn down\n");

	console_init();
	post_code(0x02);
	printk(BIOS_INFO, "console initted\n");

	/*
	 * CBMEM needs to be recovered because timestamps rely on
	 * the cbmem infrastructure being around. Explicitly recover it.
	 *
	 * On some platforms CBMEM needs to be initialized earlier.
	 * Use cbmem_online() to avoid init CBMEM twice.
	 */
	if (!cbmem_online())
	post_code(0x03);
	printk(BIOS_INFO, "cbmem offline\n");
		cbmem_initialize();
	post_code(0x04);
	printk(BIOS_INFO, "cbmem initted\n");

	timestamp_add_now(TS_POSTCAR_START);

	display_mtrrs();

	/* Load and run ramstage. */
	run_ramstage();
}
