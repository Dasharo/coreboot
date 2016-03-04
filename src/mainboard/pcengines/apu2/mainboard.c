/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2012 Advanced Micro Devices, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <console/console.h>
#include <device/device.h>
#include <device/pci.h>
#include <arch/io.h>
#include <device/pci_def.h>
#include <arch/acpi.h>
#include <northbridge/amd/pi/BiosCallOuts.h>
#include <cpu/amd/pi/s3_resume.h>
#include <northbridge/amd/pi/agesawrapper.h>
#include <cpu/x86/msr.h>
#include <cpu/amd/mtrr.h>
#if CONFIG_USE_OPTION_TABLE
#include <pc80/mc146818rtc.h>
#endif //CONFIG_USE_OPTION_TABLE
#if CONFIG_HAVE_OPTION_TABLE
#include "option_table.h"
#endif //CONFIG_HAVE_OPTION_TABLE
#include <eltanhudson.h>
#include <timestamp.h>
#include <fchgpio.h>
#include "apu2.h"
#include <superio/nuvoton/nct5104d/nct5104d.h>
#include <northbridge/amd/pi/00730F01/eltannorthbridge.h>
#if CONFIG_USE_CBMEM_FILE_OVERRIDE
#include <boot/cbmemfile.h>
#endif //CONFIG_USE_CBMEM_FILE_OVERRIDE
#include <cbfs_core.h>
#include <spd_cache.h>

/**********************************************
 * enable the dedicated function in mainboard.
 **********************************************/

static void mainboard_enable(device_t dev)
{
	struct device *sio_dev;

	setup_bsp_ramtop();
	u32 TOM1 = bsp_topmem() / (1024 *1024);	// Tom1 in Mbyte
	u32 TOM2 = ( bsp_topmem2() / (1024 *1024)) - 4 * 1024;	// Tom2 in Mbyte
	printk(BIOS_ERR, "%d MB", TOM1+TOM2);

	u8 spd_buffer[SPD_SIZE];
	int	index = 0;

	/* One SPD file contains all 4 options, determine which index to read here, then call into the standard routines*/

	if ( ReadFchGpio(APU2_SPD_STRAP0_GPIO) ) index |= BIT0;
	if ( ReadFchGpio(APU2_SPD_STRAP1_GPIO) ) index |= BIT1;
	
	printk(BIOS_SPEW, "Reading SPD index %d to get ECC info \n", index);
	if (read_spd_from_cbfs(spd_buffer, index) < 0)
		spd_buffer[3]=3;	// Indicate no ECC

	if ( spd_buffer[3] == 8 ) 	printk(BIOS_ERR, " ECC");
	printk(BIOS_ERR, " DRAM\n\n");

	//
	// Enable the RTC output
	//
	pm_write16 ( PM_RTC_CONTROL, pm_read16( PM_RTC_CONTROL ) | (1 << 11));

	//
	// Enable power on from WAKE#
	//
	pm_write16 ( PM_S_STATE_CONTROL, pm_read16( PM_S_STATE_CONTROL ) | (1 << 14));

	if (acpi_is_wakeup_s3())
		agesawrapper_fchs3earlyrestore();

//
// SIO CONFIG, enable and disable UARTC and UARTD depending on the selection
//
#if CONFIG_SUPERIO_NUVOTON_NCT5104D_UARTC_ENABLE
	printk(BIOS_INFO, "UARTC enabled\n");

	sio_dev = dev_find_slot_pnp(0x2E, NCT5104D_SP3);
	if ( sio_dev ) sio_dev->enabled = 1;
	sio_dev = dev_find_slot_pnp(0x2E, NCT5104D_GPIO0);
	if ( sio_dev ) sio_dev->enabled = 0;

#else //CONFIG_SUPERIO_NUVOTON_NCT5104D_UARTC_ENABLE
	printk(BIOS_INFO, "UARTC disabled\n");

	sio_dev = dev_find_slot_pnp(0x2E, NCT5104D_SP3);
	if ( sio_dev ) sio_dev->enabled = 0;
	sio_dev = dev_find_slot_pnp(0x2E, NCT5104D_GPIO0);
	if ( sio_dev ) sio_dev->enabled = 1;
#endif //CONFIG_SUPERIO_NUVOTON_NCT5104D_UARTC_ENABLE


#if CONFIG_SUPERIO_NUVOTON_NCT5104D_UARTD_ENABLE
	printk(BIOS_INFO, "UARTD enabled\n");

	sio_dev = dev_find_slot_pnp(0x2E, NCT5104D_SP4);
	if ( sio_dev ) sio_dev->enabled = 1;
	sio_dev = dev_find_slot_pnp(0x2E, NCT5104D_GPIO1);
	if ( sio_dev ) sio_dev->enabled = 0;

#else //CONFIG_SUPERIO_NUVOTON_NCT5104D_UARTD_ENABLE
	printk(BIOS_INFO, "UARTD disabled\n");

	sio_dev = dev_find_slot_pnp(0x2E, NCT5104D_SP4);
	if ( sio_dev ) sio_dev->enabled = 0;
	sio_dev = dev_find_slot_pnp(0x2E, NCT5104D_GPIO1);
	if ( sio_dev ) sio_dev->enabled = 1;

#endif //CONFIG_SUPERIO_NUVOTON_NCT5104D_UARTD_ENABLE
}
#if CONFIG_FORCE_CONSOLE
#else //CONFIG_FORCE_CONSOLE

static char * findstr(const char *s, const char *pattern)
{
	char *result = (char *) s;
	char *lpattern = (char *) pattern;

	while (*result && *pattern ) {

		if ( *lpattern == 0)	return result;		// the pattern matches return the pointer
		if ( *result == 0) 		return NULL;		// We're at the end of the file content but don't have a patter match yet
		if (*result == *lpattern ) {
			result++;								// The string matches, simply advance
			lpattern++;
		} else {
			result++;								// The string doesn't match restart the pattern
			lpattern = (char *) pattern;
		}
	}

	return NULL;
}

static bool
check_console( void )
{
	const char *boot_file;
	size_t boot_file_len = 0;
	char * scon;

	printk(BIOS_INFO, "read bootorder\n");
	boot_file = cbfs_get_file_content(CBFS_DEFAULT_MEDIA, "bootorder", CBFS_TYPE_RAW, &boot_file_len);
	if (!boot_file)
		printk(BIOS_EMERG, "file [bootorder] not found in CBFS\n");
	if (boot_file_len < 4096)
		printk(BIOS_EMERG, "Missing bootorder data.\n");
	if (!boot_file || boot_file_len < 4096)
		return TRUE;

	//
	// Find the serial console item
	//
	scon = findstr( boot_file, "scon");

	if ( scon ) {

		if ( *scon == '0') return FALSE;
		if ( *scon == '1') return TRUE;

		printk(BIOS_EMERG, "invalid scon item, enable terminal.\n");
		return TRUE;

	} else {

		printk(BIOS_EMERG, "Missing scon item, enable terminal.\n");
		return TRUE;
	}
	return TRUE;
}
#endif //CONFIG_FORCE_CONSOLE

static void mainboard_final(void *chip_info) {

	printk(BIOS_INFO, "Mainboard " CONFIG_MAINBOARD_PART_NUMBER "final\n");

/* Disabling LPCCLK0 which is unused according to the schematic doesn't work. The system is stuck if we do this
 * So we don't do this.

 */

#if CONFIG_DUMP_GPIO_CONFIGURATION
	DumpGpioConfiguration( );
#endif //CONFIG_DUMP_GPIO_CONFIGURATION

#if CONFIG_DUMP_CLOCK_CONFIGURATION
	DumpClockConfiguration( );
#endif //CONFIG_DUMP_GPIO_CONFIGURATION

#if CONFIG_DUMP_LINK_CONFIGURATION
	DumpLinkConfiguration( );
#endif //CONFIG_DUMP_LINK_CONFIGURATION

	//
	// Turn off LED 2 and 3
	//
	printk(BIOS_INFO, "Turn off LED 2 and 3\n");
	WriteFchGpio( APU2_LED2_L_GPIO, 1);
	WriteFchGpio( APU2_LED3_L_GPIO, 1);

	printk(BIOS_INFO, "USB PORT ROUTING = 0x%08x\n", *((u8 *)(ACPI_MMIO_BASE + PMIO_BASE + FCH_PMIOA_REGEF )));
	if ( *((u8 *)(ACPI_MMIO_BASE + PMIO_BASE + FCH_PMIOA_REGEF )) & (1<<7) ) {

		printk(BIOS_INFO, "USB PORT ROUTING = XHCI PORTS ENABLED\n");
	} else {

		printk(BIOS_INFO, "USB PORT ROUTING = EHCI PORTS ENABLED\n");
	}
#if CONFIG_USE_CBMEM_FILE_OVERRIDE

#if CONFIG_FORCE_CONSOLE
	bool console_enabled = TRUE;
#else //CONFIG_FORCE_CONSOLE
	bool console_enabled = check_console( );	// Get console setting from bootorder file.
#endif //CONFIG_FORCE_CONSOLE

	if ( !console_enabled ) {

		//
		// The console is disabled, check if S1 is pressed and enable if so
		//
		if ( !ReadFchGpio(APU2_BIOS_CONSOLE_GPIO) ) {

			printk(BIOS_INFO, "S1 PRESSED\n");
			console_enabled = TRUE;
		}
	}

	if ( !console_enabled ) {

		//
		// The console should be disabled
		//
		unsigned char data = 1;

		//
		// Indicated to SeaBIOS it should display console output itself
		//
		add_cbmem_file(
				"etc/screen-and-debug",
				1,
				&data );

		//
		// Hide the sgabios to disable SeaBIOS console
		//
		hide_cbmem_file(
				"vgaroms/sgabios.bin" );

	}

#endif //CONFIG_USE_CBMEM_FILE_OVERRIDE

}

struct chip_operations mainboard_ops = {
	.enable_dev = mainboard_enable,
	.final = mainboard_final,
};


