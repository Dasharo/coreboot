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
#include <spd_cache.h>
#include <smbios.h>
#include <string.h>
#include <cpu/amd/amdfam16.h>
#include <cpuRegisters.h>
#include <build.h>
#include "bios_knobs.h"
#include "s1_button.h"

/**********************************************
 * enable the dedicated function in mainboard.
 **********************************************/

static void mainboard_enable(device_t dev)
{
	struct device *sio_dev;

	bool scon = check_console();
	setup_bsp_ramtop();
	u32 total_mem = bsp_topmem() / (1024 * 1024);
	if (bsp_topmem2() > 0)
		total_mem += (bsp_topmem2() / (1024 * 1024)) - 4 * 1024;
	if (scon) {
		printk(BIOS_ALERT, "%d MB", total_mem);
	}

	u8 spd_buffer[SPD_SIZE];
	int	index = 0;

	/* One SPD file contains all 4 options, determine which index to read here, then call into the standard routines*/

	if ( ReadFchGpio(APU2_SPD_STRAP0_GPIO) ) index |= BIT0;
	if ( ReadFchGpio(APU2_SPD_STRAP1_GPIO) ) index |= BIT1;

	printk(BIOS_SPEW, "Reading SPD index %d to get ECC info \n", index);
	if (read_spd_from_cbfs(spd_buffer, index) < 0)
		spd_buffer[3]=3;	// Indicate no ECC

	if (scon) {
		if ( spd_buffer[3] == 8 ) 	printk(BIOS_ALERT, " ECC");
		printk(BIOS_ALERT, " DRAM\n\n");
	}

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
	// SIO CONFIG, enable and disable UARTC and UARTD depending on the configuration
	//
	if (check_uartc()) {
		printk(BIOS_INFO, "UARTC enabled\n");

		sio_dev = dev_find_slot_pnp(0x2E, NCT5104D_SP3);
		if ( sio_dev ) sio_dev->enabled = 1;
		sio_dev = dev_find_slot_pnp(0x2E, NCT5104D_GPIO0);
		if ( sio_dev ) sio_dev->enabled = 0;
	} else {
		printk(BIOS_INFO, "UARTC disabled\n");

		sio_dev = dev_find_slot_pnp(0x2E, NCT5104D_SP3);
		if ( sio_dev ) sio_dev->enabled = 0;
		sio_dev = dev_find_slot_pnp(0x2E, NCT5104D_GPIO0);
		if ( sio_dev ) sio_dev->enabled = 1;
	}

	if (check_uartd()) {
		printk(BIOS_INFO, "UARTD enabled\n");

		sio_dev = dev_find_slot_pnp(0x2E, NCT5104D_SP4);
		if ( sio_dev ) sio_dev->enabled = 1;
		sio_dev = dev_find_slot_pnp(0x2E, NCT5104D_GPIO1);
		if ( sio_dev ) sio_dev->enabled = 0;
	} else {
		printk(BIOS_INFO, "UARTD disabled\n");

		sio_dev = dev_find_slot_pnp(0x2E, NCT5104D_SP4);
		if ( sio_dev ) sio_dev->enabled = 0;
		sio_dev = dev_find_slot_pnp(0x2E, NCT5104D_GPIO1);
		if ( sio_dev ) sio_dev->enabled = 1;
	}
}

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

#if CONFIG_BOARD_PCENGINES_APU2 || CONFIG_BOARD_PCENGINES_APU3 || CONFIG_BOARD_PCENGINES_APU4
	bool console_enabled = check_console( );	// Get console setting from bootorder file.

	if ( !console_enabled ) {

		//
		// The console is disabled, check if S1 is pressed and enable if so
		//
		if ( !ReadFchGpio(APU2_BIOS_CONSOLE_GPIO) ) {

			printk(BIOS_INFO, "S1 PRESSED\n");

			enable_console();
		}
	}
#endif // CONFIG_BOARD_PCENGINES_APU2/3/4
}

struct chip_operations mainboard_ops = {
	.enable_dev = mainboard_enable,
	.final = mainboard_final,
};

const char *smbios_mainboard_serial_number(void)
{
    static char serial[10];
    msr_t msr;
    u32 mac_addr = 0;
    device_t nic_dev;

    // Allows the IO configuration space access method, IOCF8 and IOCFC, to be
    // used to generate extended configuration cycles
    msr = rdmsr(NB_CFG_MSR);
    msr.hi |= (ENABLE_CF8_EXT_CFG);
    wrmsr(NB_CFG_MSR, msr);

    nic_dev = dev_find_slot(1, PCI_DEVFN(0, 0));

    if ((serial[0] != 0) || !nic_dev)
        return serial;

    // Read 4 bytes starting from 0x144 offset
    mac_addr = pci_read_config32(nic_dev, 0x144);
    // MSB here is always 0xff
    // Discard it so only bottom 3b of mac address are left
    mac_addr &= 0x00ffffff;

    // Set bit EnableCf8ExtCfg back to 0
    msr.hi &= ~(ENABLE_CF8_EXT_CFG);
    wrmsr(NB_CFG_MSR, msr);

    // Calculate serial value
    mac_addr /= 4;
    mac_addr -= 64;

    snprintf(serial, sizeof(serial), "%d", mac_addr);

    return serial;
}

const char *smbios_mainboard_sku(void)
{
	static char sku[5];
	if (sku[0] != 0)
		return sku;

	if (!ReadFchGpio(APU2_SPD_STRAP0_GPIO))
		snprintf(sku, sizeof(sku), "2 GB");
	else
		snprintf(sku, sizeof(sku), "4 GB");
	return sku;
}
