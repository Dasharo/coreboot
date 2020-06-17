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
#include <smp/node.h>

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

	// /* Enable or disable watchdog depending on the configuration*/
	// if (boot_cpu()){
	// 	volatile u32 *ptr = (u32 *)(ACPI_MMIO_BASE + WATCHDOG_BASE);
	// 	u16 watchdog_timeout = get_watchdog_timeout();

	// 	if (watchdog_timeout == 0) {
	// 		//disable watchdog
	// 		printk(BIOS_WARNING, "Watchdog is disabled\n");
	// 		*ptr |= (1 << 3);
	// 	} else {
	// 		// enable
	// 		*ptr &= ~(1 << 3);
	// 		*ptr |= (1 << 0);
	// 		// configure timeout
	// 		*(ptr + 1) = (u16) watchdog_timeout;
	// 		// trigger
	// 		*ptr |= (1 << 7);
	// 		printk(BIOS_WARNING, "Watchdog is enabled, state = 0x%x, time = %d\n", *ptr, *(ptr + 1));
	// 	}
	// }
}

static void mainboard_final(void *chip_info) {

	/* ECC was not working because all memory was marked
	 * as excluded from ECC checking and error reporting.
	 * Normally one contiguous range is excluded to cover
	 * framebuffer region in systems with internal GPUs.
	 * AGESA set bit 0, EccExclEn, in register D18F5x240 as 1.
	 * Range was incorrectly enabled to cover all memory
	 * for cases without UMA (no integrated graphics).
	 *
	 * In order to clear EccExclEn DRAM scrubber needs to be
	 * disabled temporarily by setting D18F3x88[DisDramScrub]
	 */
	u32 val;
	device_t D18F3 = dev_find_device(0x1022, 0x1583, NULL);
	device_t D18F5 = dev_find_device(0x1022, 0x1585, NULL);

	/* Disable DRAM ECC scrubbing */
	val = pci_read_config32(D18F3, 0x88);
	val |= (1 << 27);
	pci_write_config32(D18F3, 0x88, val);

	/* Clear reserved bits in register D18F3xB8
	 *
	 * D18F3xB8 NB Array Address has reserved bits [27:10]
	 * Multiple experiments showed that ECC injection
	 * doesn't work when these bits are set.
	 */
	val = pci_read_config32(D18F3, 0xB8);
	val &= 0xF000003F;
	pci_write_config32(D18F3, 0xB8, val);

	/* Disable ECC exclusion range */
	val = pci_read_config32(D18F5, 0x240);
	val &= ~1;
	pci_write_config32(D18F5, 0x240, val);

	/*  Re-enable DRAM ECC scrubbing */
	val = pci_read_config32(D18F3, 0x88);
	val &= ~(1 << 27);
	pci_write_config32(D18F3, 0x88, val);

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

	bool console_enabled = check_console();	// Get console setting from bootorder file.

	if ( !console_enabled ) {

		//
		// The console is disabled, check if S1 is pressed and enable if so
		//
# if CONFIG_BOARD_PCENGINES_APU5
		if ( !ReadFchGpio(APU5_BIOS_CONSOLE_GPIO) ) {
#else
		if ( !ReadFchGpio(APU2_BIOS_CONSOLE_GPIO) ) {
#endif

			printk(BIOS_INFO, "S1 PRESSED\n");

			enable_console();
		}
	}
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
	u32 bus_no;
	device_t dev;

	// Allows the IO configuration space access method, IOCF8 and IOCFC, to be
	// used to generate extended configuration cycles
	msr = rdmsr(NB_CFG_MSR);
	msr.hi |= (ENABLE_CF8_EXT_CFG);
	wrmsr(NB_CFG_MSR, msr);

	// In case we have PCIe module connected to mPCIe2 slot, BDF 1:0.0 may
	// not be a NIC, because mPCIe2 slot is routed to the very first PCIe
	// bridge and the first NIC is connected to the second PCIe bridge.
	// Read secondary bus number from the PCIe bridge where the first NIC is
	// connected.
	dev = dev_find_slot(0, PCI_DEVFN(2, 2));
	if ((serial[0] != 0) || !dev)
		return serial;

	bus_no = dev->link_list->secondary;
	dev = dev_find_slot(bus_no, PCI_DEVFN(0, 0));
	if (!dev)
		return serial;

	// Read 4 bytes starting from 0x144 offset
	mac_addr = pci_read_config32(dev, 0x144);
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

int fill_mainboard_smbios_type16(unsigned long *current, int *handle)
{
	u8 spd_index = 0;
	if ( ReadFchGpio(APU2_SPD_STRAP0_GPIO) ) spd_index |= BIT0;
	if ( ReadFchGpio(APU2_SPD_STRAP1_GPIO) ) spd_index |= BIT1;
	
	u8 spd_buffer[SPD_SIZE];
	if (read_spd_from_cbfs(spd_buffer, spd_index) < 0) {
		return 0;
	}

	struct smbios_type16 *t = (struct smbios_type16 *)*current;
	int len = sizeof(struct smbios_type16) - 2;
	memset(t, 0, sizeof(struct smbios_type16));

	t->handle = *handle;
	t->length = len;
	t->type = SMBIOS_PHYS_MEMORY_ARRAY;
	t->use = MEMORY_ARRAY_USE_SYSTEM;
	t->location = MEMORY_ARRAY_LOCATION_SYSTEM_BOARD;
	t->maximum_capacity = 4 * 1024 * 1024; 		// 4GB (in kB) due to board design
	t->extended_maximum_capacity = 0;
	t->memory_error_information_handle = 0xFFFE;
	t->number_of_memory_devices = 1; 			// only 1 device soldered down to 1 channel

	switch(spd_buffer[3]){
		case 0x08:
			t->memory_error_correction = MEMORY_ARRAY_ECC_MULTI_BIT;
			break;
		case 0x03:
			t->memory_error_correction = MEMORY_ARRAY_ECC_NONE;
			break;
		default:
			t->memory_error_correction = MEMORY_ARRAY_ECC_UNKNOWN;
			break;
	}
	len = t->length + smbios_string_table_len(t->eos);
	return len;
}

int fill_mainboard_smbios_type17(unsigned long *current, int *handle, int *type16_handle){

	u8 spd_index = 0;
	if ( ReadFchGpio(APU2_SPD_STRAP0_GPIO) ) spd_index |= BIT0;
	if ( ReadFchGpio(APU2_SPD_STRAP1_GPIO) ) spd_index |= BIT1;
	
	u8 spd_buffer[SPD_SIZE];
	if (read_spd_from_cbfs(spd_buffer, spd_index) < 0) {
		;
	}

	char locator[40];

	struct smbios_type17 *t = (struct smbios_type17 *)*current;
	int len = sizeof(struct smbios_type17) - 2;
	memset(t, 0, sizeof(struct smbios_type17));

	t->memory_error_information_handle = 0xFFFE;

	/* There can be 2GB or 4GB RAM */
	if((spd_buffer[4] & 0x07) == 3)
	{
		t->size = 0x800;				/* 2048 MB size */
	}
	else if((spd_buffer[4] & 0x07) == 4)
	{
		t->size = 0x1000;				/* 4096 MB size */
	}

	/* Memory speed */
	switch (spd_buffer[12]) 
	{
		case 0x0a:
			t->speed = 1600;
			t->clock_speed = 1600;
			break;
		case 0x0c:
			t->speed = 1333;
			t->clock_speed = 1333;
			break;
		default:
			t->speed = 0;
			t->clock_speed = 0;
			break;
	}

	/* Data width bus is always 64 bits */
	t->data_width = 64;
	
	/* Checking if there is ECC correction 
		If there is - total width is 72b
		If not total width is the same as data width - 64b */
	if(((spd_buffer[8] & 0x18) >> 3) == 1)
	{
		t->total_width = t->data_width + 8;		
	}
	else
	{
		t->total_width = t->data_width;			
	}

	t->type_detail = 0x0080;
	t->memory_type = MEMORY_TYPE_DDR3;

	/* Retrieve information about memory form factor*/
	switch(spd_buffer[3] & 0x0f)
	{
	case 0x01:
	case 0x10:
		t->form_factor = MEMORY_FORMFACTOR_RIMM;
		break;
	case 0x02:
	case 0x08:
	case 0x20:
		t->form_factor = MEMORY_FORMFACTOR_DIMM;
		break;
	case 0x04:
		t->form_factor = MEMORY_FORMFACTOR_SODIMM;
		break;
	default:
		t->form_factor = MEMORY_FORMFACTOR_UNKNOWN;
		break;
	}

	snprintf(locator, sizeof(locator), "Channel-%d-DIMM-%d", 0, 0);
	t->device_locator = smbios_add_string(t->eos, locator);

	snprintf(locator, sizeof(locator), "BANK %d", 0);
	t->bank_locator = smbios_add_string(t->eos, locator);

	t->device_set = 0;			/* None information about device set */			
	t->type = SMBIOS_MEMORY_DEVICE;
	t->length = len;
	t->handle = *handle;		/* Handle is incremented in outside function */
	t->phys_memory_array_handle = *type16_handle;
	
	return t->length + smbios_string_table_len(t->eos);
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
