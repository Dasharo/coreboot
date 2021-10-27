/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <console/console.h>
#include <southbridge/amd/sb700/sb700.h>
#include <northbridge/amd/amdfam10/amdfam10.h>
#include <northbridge/amd/amdfam10/raminit.h>
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

void mainboard_sysinfo_hook(struct sys_info *sysinfo)
{
	sysinfo->ht_link_cfg.ht_speed_limit = 2600;
}


static void set_peripheral_control_lines(void) {
	uint8_t byte;

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
static void set_ddr3_voltage(uint8_t node, uint8_t index) {
	uint8_t byte;
	uint8_t value = 0;

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

static const uint8_t spd_addr_fam15[] = {
	// Socket 0 Node 0 ("Node 0")
	0, DIMM0, DIMM1, 0, 0, DIMM2, DIMM3, 0, 0,
	// Socket 0 Node 1 ("Node 1")
	0, DIMM4, DIMM5, 0, 0, DIMM6, DIMM7, 0, 0,
	// Socket 1 Node 0 ("Node 2")
	1, DIMM0, DIMM1, 0, 0, DIMM2, DIMM3, 0, 0,
	// Socket 1 Node 1 ("Node 3")
	1, DIMM4, DIMM5, 0, 0, DIMM6, DIMM7, 0, 0,
};

static const uint8_t spd_addr_fam10[] = {
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

void mainboard_after_raminit(struct sys_info *sysinfo)
{
	//execute_memory_test();
	//printk(BIOS_DEBUG, "disable_spd()\n");
	//switch_spd_mux(0x1);
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
bool AMD_CB_ManualBUIDSwapList(u8 node, u8 link, const u8 **list)
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
