/* SPDX-License-Identifier: GPL-2.0-only */

#include <arch/mmio.h>
#include <console/console.h>
#include <cpu/amd/common/common.h>
#include <southbridge/amd/sb700/sb700.h>
#include <northbridge/amd/amdfam10/amdfam10.h>
#include <northbridge/amd/amdfam10/raminit.h>
#include <delay.h>
#include <option.h>

/* DIMM SPD addresses */
#define DIMM0	0x50
#define DIMM1	0x51
#define DIMM2	0x52
#define DIMM3	0x53
#define DIMM4	0x54
#define DIMM5	0x55
#define DIMM6	0x56
#define DIMM7	0x57


/*
 * ASUS KGPE-D16 specific SPD enable/disable magic.
 *
 * Setting SP5100 GPIOs 59 and 60 controls an SPI mux with four settings:
 * 0: Disabled
 * 1: Normal SPI access
 * 2: CPU0 SPD
 * 3: CPU1 SPD
 *
 * Disable SPD access after RAM init to allow access to standard SMBus/I2C offsets
 * which is required e.g. by lm-sensors.
 */

/* Relevant GPIO register information is available in the
 * AMD SP5100 Register Reference Guide rev. 3.03, page 130
 */
static void switch_spd_mux(u8 channel)
{
	u8 byte;

	byte = pci_read_config8(PCI_DEV(0, 0x14, 0), 0x54);
	byte &= ~0xc;			/* Clear SPD mux GPIOs */
	byte &= ~0xc0;			/* Enable SPD mux GPIO output drivers */
	byte |= (channel << 2) & 0xc;	/* Set SPD mux GPIOs */
	pci_write_config8(PCI_DEV(0, 0x14, 0), 0x54, byte);

	/* Temporary AST PCI mapping */
	const u32 memory_base = 0xfc000000;
	const u32 memory_limit = 0xfc800000;

#define TEMP_PCI_BUS 0x2
	/* Save S100 PCI bridge settings */
	u16 prev_sec_cfg = pci_read_config16(PCI_DEV(0, 0x14, 4),
						PCI_COMMAND);
	u8 prev_sec_bus = pci_read_config8(PCI_DEV(0, 0x14, 4),
						PCI_SECONDARY_BUS);
	u8 prev_sec_sub_bus = pci_read_config8(PCI_DEV(0, 0x14, 4),
						PCI_SUBORDINATE_BUS);
	u16 prev_sec_mem_base = pci_read_config16(PCI_DEV(0, 0x14, 4),
						PCI_MEMORY_BASE);
	u16 prev_sec_mem_limit = pci_read_config16(PCI_DEV(0, 0x14, 4),
							PCI_MEMORY_LIMIT);
	/* Temporarily enable the SP5100 PCI bridge */
	pci_write_config8(PCI_DEV(0, 0x14, 4), PCI_SECONDARY_BUS, TEMP_PCI_BUS);
	pci_write_config8(PCI_DEV(0, 0x14, 4), PCI_SUBORDINATE_BUS, 0xff);
	pci_write_config16(PCI_DEV(0, 0x14, 4), PCI_MEMORY_BASE,
			(memory_base >> 20));
	pci_write_config16(PCI_DEV(0, 0x14, 4), PCI_MEMORY_LIMIT,
			(memory_limit >> 20));
	pci_write_config16(PCI_DEV(0, 0x14, 4), PCI_COMMAND,
			PCI_COMMAND_MEMORY);

	/* Temporarily enable AST BAR1 */
	u16 prev_ast_cmd = pci_read_config16(PCI_DEV(TEMP_PCI_BUS, 0x1, 0),
						PCI_COMMAND);
	u16 prev_ast_sts = pci_read_config16(PCI_DEV(TEMP_PCI_BUS, 0x1, 0),
						PCI_STATUS);
	u32 prev_ast_bar1 = pci_read_config32(
		PCI_DEV(TEMP_PCI_BUS, 0x1, 0), PCI_BASE_ADDRESS_1);
	pci_write_config32(PCI_DEV(TEMP_PCI_BUS, 0x1, 0), PCI_BASE_ADDRESS_1,
			memory_base);
	pci_write_config16(PCI_DEV(TEMP_PCI_BUS, 0x1, 0), PCI_COMMAND,
			PCI_COMMAND_MEMORY);
	pci_write_config16(PCI_DEV(TEMP_PCI_BUS, 0x1, 0), PCI_STATUS,
			PCI_STATUS_CAP_LIST | PCI_STATUS_DEVSEL_MEDIUM);

	/* Use the P2A bridge to set ASpeed SPD mux GPIOs to the same values as the SP5100 */
	void* ast_bar1 = (void*)memory_base;
	/* Enable access to GPIO controller */
	write32(ast_bar1 + 0xf004, 0x1e780000);
	write32(ast_bar1 + 0xf000, 0x1);
	/* Enable SPD mux GPIO output drivers */
	write32(ast_bar1 + 0x10024, read32(ast_bar1 + 0x10024) | 0x3000);
	/* Set SPD mux GPIOs */
	write32(ast_bar1 + 0x10020, (read32(ast_bar1 + 0x10020) & ~0x3000)
		| ((channel & 0x3) << 12));
	write32(ast_bar1 + 0xf000, 0x0);

	/* Deconfigure AST BAR1 */
	pci_write_config16(PCI_DEV(TEMP_PCI_BUS, 0x1, 0), PCI_COMMAND,
			prev_ast_cmd);
	pci_write_config16(PCI_DEV(TEMP_PCI_BUS, 0x1, 0), PCI_STATUS,
			prev_ast_sts);
	pci_write_config32(PCI_DEV(TEMP_PCI_BUS, 0x1, 0), PCI_BASE_ADDRESS_1,
			prev_ast_bar1);

	/* Deconfigure SP5100 PCI bridge */
	pci_write_config16(PCI_DEV(0, 0x14, 4), PCI_COMMAND, prev_sec_cfg);
	pci_write_config16(PCI_DEV(0, 0x14, 4), PCI_MEMORY_LIMIT,
			prev_sec_mem_limit);
	pci_write_config16(PCI_DEV(0, 0x14, 4), PCI_MEMORY_BASE,
			prev_sec_mem_base);
	pci_write_config8(PCI_DEV(0, 0x14, 4), PCI_SUBORDINATE_BUS,
			prev_sec_sub_bus);
	pci_write_config8(PCI_DEV(0, 0x14, 4), PCI_SECONDARY_BUS, prev_sec_bus);
}

void activate_spd_rom(const struct mem_controller *ctrl)
{
	struct sys_info *sysinfo = get_sysinfo();
	printk(BIOS_DEBUG, "activate_spd_rom() for node %02x\n", ctrl->node_id);
	if (ctrl->node_id == 0) {
		printk(BIOS_DEBUG, "enable_spd_node0()\n");
		switch_spd_mux(2);
	} else if (ctrl->node_id == 1) {
		printk(BIOS_DEBUG, "enable_spd_node1()\n");
		switch_spd_mux((is_fam15h() || (sysinfo->nodes <= 2)) ? 2 : 3);
	} else if (ctrl->node_id == 2) {
		printk(BIOS_DEBUG, "enable_spd_node2()\n");
		switch_spd_mux((is_fam15h() || (sysinfo->nodes <= 2)) ? 3 : 2);
	} else if (ctrl->node_id == 3) {
		printk(BIOS_DEBUG, "enable_spd_node3()\n");
		switch_spd_mux(3);
	}
}

void mainboard_sysinfo_hook(struct sys_info *sysinfo)
{
	sysinfo->ht_link_cfg.ht_speed_limit = 2600;
}

static void set_peripheral_control_lines(void) {
	u8 byte;

	if (get_uint_option("ieee1394_controller", 0)) {
		/* Enable PCICLK5 (onboard FireWire device) */
		outb(0x41, 0xcd6);
		outb(0x02, 0xcd7);
	} else {
		/* Disable PCICLK5 (onboard FireWire device) */
		outb(0x41, 0xcd6);
		outb(0x00, 0xcd7);
	}

	/* Enable the RTC AltCentury register */
	outb(0x41, 0xcd6);
	byte = inb(0xcd7);
	byte |= 0x10;
	outb(byte, 0xcd7);
}

/* Voltages are specified by index
 * Valid indices for this platform are:
 * 0: 1.5V
 * 1: 1.35V
 * 2: 1.25V
 * 3: 1.15V
 */
static void set_ddr3_voltage(u8 node, u8 index) {
	u8 byte;
	u8 value = 0;

	if (index == 0)
		value = 0x0;
	else if (index == 1)
		value = 0x1;
	else if (index == 2)
		value = 0x4;
	else if (index == 3)
		value = 0x5;
	if (node == 1)
		value <<= 1;

	/* Set GPIOs */
	byte = pci_read_config8(PCI_DEV(0, 0x14, 3), 0xd1);
	if (node == 0)
		byte &= ~0x5;
	if (node == 1)
		byte &= ~0xa;
	byte |= value;
	pci_write_config8(PCI_DEV(0, 0x14, 3), 0xd1, byte);

	/* Enable GPIO output drivers */
	byte = pci_read_config8(PCI_DEV(0, 0x14, 3), 0xd0);
	byte &= 0x0f;
	pci_write_config8(PCI_DEV(0, 0x14, 3), 0xd0, byte);

	printk(BIOS_DEBUG, "Node %02d DIMM voltage set to index %02x\n", node, index);
}

void DIMMSetVoltages(struct MCTStatStruc *pMCTstat, struct DCTStatStruc *pDCTstatA)
{
	/* This mainboard allows the DIMM voltage to be set per-socket.
	 * Therefore, for each socket, iterate over all DIMMs to find the
	 * lowest supported voltage common to all DIMMs on that socket.
	 */
	u8 dimm;
	u8 node;
	u8 socket;
	u8 allowed_voltages = 0xf;	/* The mainboard VRMs allow 1.15V, 1.25V, 1.35V, and 1.5V */
	u8 socket_allowed_voltages = allowed_voltages;
	u32 set_voltage = 0;
	u8 min_voltage = get_uint_option("minimum_memory_voltage", 0);

	switch (min_voltage) {
		case 2:
			allowed_voltages = 0x7;	/* Allow 1.25V, 1.35V, and 1.5V */
			break;
		case 1:
			allowed_voltages = 0x3;	/* Allow 1.35V and 1.5V */
			break;
		case 0:
		default:
			allowed_voltages = 0x1;	/* Allow 1.5V only */
			break;
	}

	for (node = 0; node < MAX_NODES_SUPPORTED; node++) {
		socket = node / 2;
		struct DCTStatStruc *pDCTstat;
		pDCTstat = pDCTstatA + node;

		/* reset socket_allowed_voltages before processing each socket */
		if (!(node % 2))
			socket_allowed_voltages = allowed_voltages;

		if (pDCTstat->NodePresent) {
			for (dimm = 0; dimm < MAX_DIMMS_SUPPORTED; dimm++) {
				if (pDCTstat->dimm_valid & (1 << dimm)) {
					socket_allowed_voltages &= pDCTstat->DimmSupportedVoltages[dimm];
				}
			}
		}

		/* set voltage per socket after processing last contained node */
		if (pDCTstat->NodePresent && (node % 2)) {
			/* Set voltages */
			if (socket_allowed_voltages & 0x8) {
				set_voltage = 0x8;
				set_ddr3_voltage(socket, 3);
			} else if (socket_allowed_voltages & 0x4) {
				set_voltage = 0x4;
				set_ddr3_voltage(socket, 2);
			} else if (socket_allowed_voltages & 0x2) {
				set_voltage = 0x2;
				set_ddr3_voltage(socket, 1);
			} else {
				set_voltage = 0x1;
				set_ddr3_voltage(socket, 0);
			}

			/* Save final DIMM voltages for MCT and SMBIOS use */
			if (pDCTstat->NodePresent) {
				for (dimm = 0; dimm < MAX_DIMMS_SUPPORTED; dimm++) {
					pDCTstat->DimmConfiguredVoltage[dimm] = set_voltage;
				}
			}
			pDCTstat = pDCTstatA + (node - 1);
			if (pDCTstat->NodePresent) {
				for (dimm = 0; dimm < MAX_DIMMS_SUPPORTED; dimm++) {
					pDCTstat->DimmConfiguredVoltage[dimm] = set_voltage;
				}
			}
		}
	}

	/* Allow the DDR supply voltages to settle */
	udelay(100000);
}

void mainboard_early_init(int s3_resume)
{

	set_peripheral_control_lines();

	/* Configure SP5100 GPIOs to match vendor settings */
	pci_write_config16(PCI_DEV(0, 0x14, 0), 0x50, 0x0170);
	pci_write_config16(PCI_DEV(0, 0x14, 0), 0x54, 0x0707);
	pci_write_config16(PCI_DEV(0, 0x14, 0), 0x56, 0x0bb0);
	pci_write_config16(PCI_DEV(0, 0x14, 0), 0x5a, 0x0ff0);

	/* Set default DDR memory voltage
	 * This will be overridden later during RAM initialization
	 */
	set_lpc_sticky_ctl(1);	/* Retain LPC/IMC GPIO configuration during S3 sleep */
	if (!s3_resume) {	/* Avoid supply voltage glitches while the DIMMs are retaining data */
		set_ddr3_voltage(0, 0);	/* Node 0 */
		set_ddr3_voltage(1, 0);	/* Node 1 */
	}
}

static const u8 spd_addr_fam15[] = {
	// Socket 0 Node 0 ("Node 0")
	0, DIMM0, DIMM1, 0, 0, DIMM2, DIMM3, 0, 0,
	// Socket 0 Node 1 ("Node 1")
	0, DIMM4, DIMM5, 0, 0, DIMM6, DIMM7, 0, 0,
	// Socket 1 Node 0 ("Node 2")
	1, DIMM0, DIMM1, 0, 0, DIMM2, DIMM3, 0, 0,
	// Socket 1 Node 1 ("Node 3")
	1, DIMM4, DIMM5, 0, 0, DIMM6, DIMM7, 0, 0,
};

static const u8 spd_addr_fam10[] = {
	// Socket 0 Node 0 ("Node 0")
	0, DIMM0, DIMM1, 0, 0, DIMM2, DIMM3, 0, 0,
	// Socket 0 Node 1 ("Node 1")
	0, DIMM4, DIMM5, 0, 0, DIMM6, DIMM7, 0, 0,
	// Socket 1 Node 1 ("Node 2")
	1, DIMM4, DIMM5, 0, 0, DIMM6, DIMM7, 0, 0,
	// Socket 1 Node 0 ("Node 3")
	1, DIMM0, DIMM1, 0, 0, DIMM2, DIMM3, 0, 0,
};

void mainboard_spd_info(struct sys_info *sysinfo)
{
	/* It's the time to set ctrl in sysinfo now; */
	printk(BIOS_DEBUG, "fill_mem_ctrl() detected %d nodes\n", sysinfo->nodes);
	if (is_fam15h())
		fill_mem_ctrl(sysinfo->nodes, sysinfo->ctrl, spd_addr_fam15);
	else
		fill_mem_ctrl(sysinfo->nodes, sysinfo->ctrl, spd_addr_fam10);
	post_code(0x3D);
}

static void execute_memory_test(void)
{
	/* Test DRAM functionality */
	u32 i;
	u32 v;
	u32 w;
	u32 x;
	u32 y;
	u32 z;
	u32 *dataptr;
	u32 readback;
	u32 start = 0x300000;
	printk(BIOS_DEBUG, "Writing test pattern 1 to memory...\n");
	for (i = 0; i < 0x1000000; i = i + 8) {
		dataptr = (void *)(start + i);
		*dataptr = 0x55555555;
		dataptr = (void *)(start + i + 4);
		*dataptr = 0xaaaaaaaa;
	}
	printk(BIOS_DEBUG, "Done!\n");
	printk(BIOS_DEBUG, "Testing memory...\n");
	for (i = 0; i < 0x1000000; i = i + 8) {
		dataptr = (void *)(start + i);
		readback = *dataptr;
		if (readback != 0x55555555)
			printk(BIOS_DEBUG, "%p: INCORRECT VALUE %08x (should have been %08x)\n", dataptr, readback, 0x55555555);
		dataptr = (void *)(start + i + 4);
		readback = *dataptr;
		if (readback != 0xaaaaaaaa)
			printk(BIOS_DEBUG, "%p: INCORRECT VALUE %08x (should have been %08x)\n", dataptr, readback, 0xaaaaaaaa);
	}
	printk(BIOS_DEBUG, "Done!\n");
	printk(BIOS_DEBUG, "Writing test pattern 2 to memory...\n");
	/* Set up the PRNG seeds for initial write */
	w = 0x55555555;
	x = 0xaaaaaaaa;
	y = 0x12345678;
	z = 0x87654321;
	for (i = 0; i < 0x1000000; i = i + 4) {
		/* Use Xorshift as a PRNG to stress test the bus */
		v = x;
		v ^= v << 11;
		v ^= v >> 8;
		x = y;
		y = z;
		z = w;
		w ^= w >> 19;
		w ^= v;
		dataptr = (void *)(start + i);
		*dataptr = w;
	}
	printk(BIOS_DEBUG, "Done!\n");
	printk(BIOS_DEBUG, "Testing memory...\n");
	/* Reset the PRNG seeds for readback */
	w = 0x55555555;
	x = 0xaaaaaaaa;
	y = 0x12345678;
	z = 0x87654321;
	for (i = 0; i < 0x1000000; i = i + 4) {
		/* Use Xorshift as a PRNG to stress test the bus */
		v = x;
		v ^= v << 11;
		v ^= v >> 8;
		x = y;
		y = z;
		z = w;
		w ^= w >> 19;
		w ^= v;
		dataptr = (void *)(start + i);
		readback = *dataptr;
		if (readback != w)
			printk(BIOS_DEBUG, "%p: INCORRECT VALUE %08x (should have been %08x)\n", dataptr, readback, w);
	}
	printk(BIOS_DEBUG, "Done!\n");
}

void mainboard_after_raminit(struct sys_info *sysinfo)
{
	execute_memory_test();
	printk(BIOS_DEBUG, "disable_spd()\n");
	switch_spd_mux(1);
}

/**
 * BOOL AMD_CB_ManualBUIDSwapList(u8 Node, u8 Link, u8 **List)
 * Description:
 *	This routine is called every time a non-coherent chain is processed.
 *	BUID assignment may be controlled explicitly on a non-coherent chain. Provide a
 *	swap list. The first part of the list controls the BUID assignment and the
 *	second part of the list provides the device to device linking.  Device orientation
 *	can be detected automatically, or explicitly.  See documentation for more details.
 *
 *	Automatic non-coherent init assigns BUIDs starting at 1 and incrementing sequentially
 *	based on each device's unit count.
 *
 * Parameters:
 *	@param[in]  node   = The node on which this chain is located
 *	@param[in]  link   = The link on the host for this chain
 *	@param[out] List   = supply a pointer to a list
 */
BOOL AMD_CB_ManualBUIDSwapList(u8 node, u8 link, const u8 **list)
{
	/* Force BUID to 0 */
	static const u8 swaplist[] = {0, 0, 0xFF, 0, 0xFF};
	if ((is_fam15h() && (node == 0) && (link == 1)) || /* Family 15h BSP SB link */
	    (!is_fam15h() && (node == 0) && (link == 3))) { /* Family 10h BSP SB link */
		*list = swaplist;
		return 1;
	}

	return 0;
}
