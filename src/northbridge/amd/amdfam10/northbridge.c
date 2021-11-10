/* SPDX-License-Identifier: GPL-2.0-only */

#include <console/console.h>
#include <cpu/cpu.h>
#include <cpu/amd/common/common.h>
#include <cpu/amd/common/nums.h>
#include <cpu/amd/msr.h>
#include <cpu/amd/mtrr.h>
#include <cpu/amd/family_10h-family_15h/amdfam10_sysconf.h>
#include <cpu/amd/family_10h-family_15h/ram_calc.h>
#include <cpu/x86/cache.h>
#include <cpu/x86/lapic.h>
#include <cpu/x86/mtrr.h>
#include <device/device.h>
#include <device/hypertransport.h>
#include <device/pci.h>
#include <device/pci_ids.h>
#include <delay.h>
#include <lib.h>
#include <option.h>

#include "amdfam10.h"

struct amdfam10_sysconf_t sysconf;
u8 pirq_router_bus;

struct amdfam10_sysconf_t *get_sysconf(void)
{
	return &sysconf;
}

void set_pirq_router_bus(u8 bus)
{
	pirq_router_bus = bus;
}

u8 get_pirq_router_bus(void)
{
	return pirq_router_bus;
}

static struct device *__f0_dev[NODE_NUMS];
static struct device *__f1_dev[NODE_NUMS];
static struct device *__f2_dev[NODE_NUMS];
static struct device *__f4_dev[NODE_NUMS];
static unsigned int fx_devs = 0;

static DEVTREE_CONST struct device *dev_find_slot(unsigned int bus, unsigned int devfn)
{
	DEVTREE_CONST struct device *dev, *result;

	result = 0;
	for (dev = all_devices; dev; dev = dev->next) {
		if ((dev->path.type == DEVICE_PATH_PCI) &&
		    (dev->bus->secondary == bus) &&
		    (dev->path.pci.devfn == devfn)) {
			result = dev;
			break;
		}
	}
	return result;
}

static u32 amdfam10_nodeid(struct device *dev)
{
#if NODE_NUMS == 64
	unsigned int busn;
	busn = dev->bus->secondary;
	if (busn != CONFIG_CBB) {
		return (dev->path.pci.devfn >> 3) - CONFIG_CDB + 32;
	} else {
		return (dev->path.pci.devfn >> 3) - CONFIG_CDB;
	}

#else
	return (dev->path.pci.devfn >> 3) - CONFIG_CDB;
#endif
}

/*
 * Assumptions: CONFIG_CBB == 0
 * dev_find_slot() was made static, also wouldn't work before PCI enumeration
 */
static void get_fx_devs(void)
{
	if (fx_devs)
		return;

	int i;
	for (i = 0; i < NODE_NUMS; i++) {
		__f0_dev[i] = pcidev_on_root(CONFIG_CDB + i, 0);
		__f1_dev[i] = pcidev_on_root(CONFIG_CDB + i, 1);
		__f2_dev[i] = pcidev_on_root(CONFIG_CDB + i, 2);
		__f4_dev[i] = pcidev_on_root(CONFIG_CDB + i, 4);
		if (__f0_dev[i] != NULL && __f1_dev[i] != NULL)
			fx_devs = i+1;
	}
	if (__f1_dev[0] == NULL || __f0_dev[0] == NULL || fx_devs == 0) {
		die("Cannot find 0:0x18.[0|1]\n");
	}
}

static u32 f1_read_config32(unsigned int reg)
{
	if (fx_devs == 0)
		get_fx_devs();
	return pci_read_config32(__f1_dev[0], reg);
}

static void f1_write_config32(unsigned int reg, u32 value)
{
	int i;
	if (fx_devs == 0)
		get_fx_devs();
	for (i = 0; i < fx_devs; i++) {
		struct device *dev;
		dev = __f1_dev[i];
		if (dev && dev->enabled) {
			pci_write_config32(dev, reg, value);
		}
	}
}

static void set_vga_enable_reg(u32 nodeid, u32 linkn)
{
	u32 val;

	val =  1 | (nodeid<<4) | (linkn<<12);
	/* it will route (1)mmio  0xa0000:0xbffff (2) io 0x3b0:0x3bb, 0x3c0:0x3df */
	f1_write_config32(0xf4, val);

}

static u32 ht_c_key(struct bus *link)
{
	u32 nodeid = amdfam10_nodeid(link->dev);
	u32 linkn = link->link_num & 0x0f;
	u32 val = (linkn << 8) | ((nodeid & 0x3f) << 2) | 3;
	return val;
}

static u32 get_ht_c_index_by_key(u32 key)
{
	u32 ht_c_index = 0;

	for (ht_c_index = 0; ht_c_index < 32; ht_c_index++) {
		if ((sysconf.ht_c_conf_bus[ht_c_index] & 0xfff) == key) {
			return ht_c_index;
		}
	}

	for (ht_c_index = 0; ht_c_index < 32; ht_c_index++) {
		if (sysconf.ht_c_conf_bus[ht_c_index] == 0) {
			 return ht_c_index;
		}
	}

	return	-1;
}

static u32 get_ht_c_index(struct bus *link)
{
	u32 val = ht_c_key(link);
	return get_ht_c_index_by_key(val);
}

static void set_config_map_reg(struct bus *link)
{
	u32 tempreg;
	u32 i;
	u32 ht_c_index = get_ht_c_index(link);
	u32 linkn = link->link_num & 0x0f;
	u32 busn_min = (link->secondary >> sysconf.segbit) & 0xff;
	u32 busn_max = (link->subordinate >> sysconf.segbit) & 0xff;
	u32 nodeid = amdfam10_nodeid(link->dev);

	tempreg = ((nodeid & 0x30) << (12-4)) | ((nodeid & 0xf) << 4) | 3;
	tempreg |= (busn_max << 24)|(busn_min << 16)|(linkn << 8);

	for (i = 0; i < sysconf.nodes; i++) {
		struct device *dev = __f1_dev[i];
		pci_write_config32(dev, 0xe0 + ht_c_index * 4, tempreg);
	}
}

static void clear_config_map_reg(struct bus *link)
{
	u32 i;
	u32 ht_c_index = get_ht_c_index(link);

	for (i = 0; i < sysconf.nodes; i++) {
		struct device *dev = __f1_dev[i];
		pci_write_config32(dev, 0xe0 + ht_c_index * 4, 0);
	}
}

static void store_ht_c_conf_bus(struct bus *link)
{
	u32 val = ht_c_key(link);
	u32 ht_c_index = get_ht_c_index_by_key(val);

	u32 segn = (link->subordinate >> 8) & 0x0f;
	u32 busn_min = link->secondary & 0xff;
	u32 busn_max = link->subordinate & 0xff;

	val |= (segn << 28) | (busn_max << 20) | (busn_min << 12);

	sysconf.ht_c_conf_bus[ht_c_index] = val;
	sysconf.hcdn_reg[ht_c_index] = link->hcdn_reg;
	sysconf.ht_c_num++;
}

typedef enum {
	HT_ROUTE_CLOSE,
	HT_ROUTE_SCAN,
	HT_ROUTE_FINAL,
} scan_state;

static void ht_route_link(struct bus *link, scan_state mode)
{
	struct bus *parent = link->dev->bus;
	u32 busses;

	printk(BIOS_SPEW, "%s\n", __func__);

	if (mode == HT_ROUTE_SCAN) {
		if (parent->subordinate == 0)
			link->secondary = 0;
		else
			link->secondary = parent->subordinate + 1;

		link->subordinate = link->secondary;
	}

	/* Configure the bus numbers for this bridge: the configuration
	 * transactions will not be propagated by the bridge if it is
	 * not correctly configured
	 */
	busses = pci_read_config32(link->dev, link->cap + 0x14);
	busses &= ~(0xff << 8);
	busses |= parent->secondary & 0xff;
	if (mode == HT_ROUTE_CLOSE)
		busses |= 0xff << 8;
	else if (mode == HT_ROUTE_SCAN)
		busses |= ((u32) link->secondary & 0xff) << 8;
	else if (mode == HT_ROUTE_FINAL)
		busses |= ((u32) link->secondary & 0xff) << 8;
	pci_write_config32(link->dev, link->cap + 0x14, busses);

	if (mode == HT_ROUTE_FINAL) {
		if (CONFIG(HT_CHAIN_DISTRIBUTE))
			parent->subordinate = ALIGN_UP(link->subordinate, 8) - 1;
		else
			parent->subordinate = link->subordinate;
	}
}

static void amd_g34_fixup(struct bus *link, struct device *dev)
{
	uint32_t nodeid = amdfam10_nodeid(dev);
	uint32_t f3xe8;

	printk(BIOS_SPEW, "%s\n", __func__);

	if (is_gt_rev_d() || is_fam15h()) {

		if (is_dual_node((u8)nodeid)) {
			/* Each G34 processor contains a defective HT link.
			 * See the BKDG Rev 3.62 section 2.7.1.5 for details.
			 */
			f3xe8 = pci_read_config32(NODE_PCI(nodeid, 3), 0xe8);
			uint8_t internal_node_number = ((f3xe8 & 0xc0000000) >> 30);
			if (internal_node_number == 0) {
				/* Node 0 */
				if (link->link_num == 6)	/* Link 2 Sublink 1 */
					printk(BIOS_DEBUG, "amdfam10_scan_chain(): node %d (internal node ID %d): skipping defective HT link\n", nodeid, internal_node_number);
			} else {
				/* Node 1 */
				if (link->link_num == 5)	/* Link 1 Sublink 1 */
					printk(BIOS_DEBUG, "amdfam10_scan_chain(): node %d (internal node ID %d): skipping defective HT link\n", nodeid, internal_node_number);
			}
		}
	}
}

static void amdfam10_scan_chain(struct bus *link)
{
	unsigned int next_unitid;

	printk(BIOS_SPEW, "%s\n", __func__);

	/* See if there is an available configuration space mapping
		* register in function 1.
		*/
	if (get_ht_c_index(link) >= 4)
		return;

	/* Set up the primary, secondary and subordinate bus numbers.
		* We have no idea how many busses are behind this bridge yet,
		* so we set the subordinate bus number to 0xff for the moment.
		*/

	ht_route_link(link, HT_ROUTE_SCAN);

	/* set the config map space */
	set_config_map_reg(link);

	/* Now we can scan all of the subordinate busses i.e. the
	 * chain on the hypertranport link
	 */

	next_unitid = hypertransport_scan_chain(link);

	/* Now that nothing is overlapping it is safe to scan the children. */
	pci_scan_bus(link, 0x00, ((next_unitid - 1) << 3) | 7);

	ht_route_link(link, HT_ROUTE_FINAL);

	/* We know the number of busses behind this bridge.  Set the
	 * subordinate bus number to it's real value
	 */
	if (0) {
		/* Clear the extend reg. */
		clear_config_map_reg(link);
	}

	set_config_map_reg(link);

	store_ht_c_conf_bus(link);
}

/* Do sb ht chain at first, in case s2885 put sb chain
 * (8131/8111) on link2, but put 8151 on link0.
 */
static void relocate_sb_ht_chain(void)
{
	struct device *dev;
	struct bus *link, *prev = NULL;
	u8 sblink;

	printk(BIOS_SPEW, "%s\n", __func__);

	dev = dev_find_slot(CONFIG_CBB, PCI_DEVFN(CONFIG_CDB, 0));
	sblink = (pci_read_config32(dev, 0x64)>>8) & 7;
	link = dev->link_list;

	while (link) {
		if (link->link_num == sblink) {
			if (!prev)
				return;
			prev->next = link->next;
			link->next = dev->link_list;
			dev->link_list = link;
			return;
		}
		prev = link;
		link = link->next;
	}
}

static void trim_ht_chain(struct device *dev)
{
	struct bus *link;

	printk(BIOS_SPEW, "%s\n", __func__);

	/* Check for connected link. */
	for (link = dev->link_list; link; link = link->next) {
		link->cap = 0x80 + (link->link_num * 0x20);
		link->ht_link_up = ht_is_non_coherent_link(link);
	}
}

static void amdfam10_scan_chains(struct device *dev)
{
	struct bus *link;

	printk(BIOS_SPEW, "%s\n", __func__);

	if (is_fam15h() && CONFIG(CPU_AMD_SOCKET_G34_NON_AGESA)) {
		uint8_t current_link_number = 0;

		for (link = dev->link_list; link; link = link->next) {
			/* The following links have changed position in Fam15h G34 processors:
			 * Fam10  Fam15
			 * Node 0
			 * L3 --> L1
			 * L0 --> L3
			 * L1 --> L2
			 * L2 --> L0
			 * Node 1
			 * L0 --> L0
			 * L1 --> L3
			 * L2 --> L1
			 * L3 --> L2
			 */
			if (link->link_num == 0)
				link->link_num = 3;
			else if (link->link_num == 1)
				link->link_num = 2;
			else if (link->link_num == 2)
				link->link_num = 0;
			else if (link->link_num == 3)
				link->link_num = 1;
			else if (link->link_num == 5)
				link->link_num = 7;
			else if (link->link_num == 6)
				link->link_num = 5;
			else if (link->link_num == 7)
				link->link_num = 6;

			current_link_number++;
			if (current_link_number > 3)
				current_link_number = 0;
		}
	}

	/* Do sb ht chain at first, in case s2885 put sb chain (8131/8111) on link2, but put 8151 on link0 */
	trim_ht_chain(dev);

	for (link = dev->link_list; link; link = link->next) {
		if (link->ht_link_up) {
			if (CONFIG(CPU_AMD_MODEL_10XXX))
				amd_g34_fixup(link, dev);
			amdfam10_scan_chain(link);
		}
	}
}

static u32 get_io_addr_index(u32 nodeid, u32 linkn)
{
	u32 index;

	for (index = 0; index < 256; index++) {

		if (index + 4 >= ARRAY_SIZE(sysconf.conf_io_addrx))
			die("Error! Out of bounds read in %s:%s\n", __FILE__, __func__);

		if (sysconf.conf_io_addrx[index+4] == 0) {
			sysconf.conf_io_addr[index+4] =  (nodeid & 0x3f);
			sysconf.conf_io_addrx[index+4] = 1 | ((linkn & 0x7)<<4);
			return index;
		 }
	 }

	 return	 0;
}

static u32 get_mmio_addr_index(u32 nodeid, u32 linkn)
{
	u32 index;

	for (index = 0; index < 64; index++) {

		if (index + 8 >= ARRAY_SIZE(sysconf.conf_mmio_addrx))
			die("Error! Out of bounds read in %s:%s\n", __FILE__, __func__);

		if (sysconf.conf_mmio_addrx[index+8] == 0) {
			sysconf.conf_mmio_addr[index+8] = (nodeid & 0x3f);
			sysconf.conf_mmio_addrx[index+8] = 1 | ((linkn & 0x7)<<4);
			return index;
		}
	}

	return 0;
}

static void store_conf_io_addr(u32 nodeid, u32 linkn, u32 reg, u32 index,
				u32 io_min, u32 io_max)
{
	u32 val;

	/* io range allocation */
	index = (reg-0xc0)>>3;

	val = (nodeid & 0x3f); // 6 bits used
	sysconf.conf_io_addr[index] = val | ((io_max<<8) & 0xfffff000); //limit : with nodeid
	val = 3 | ((linkn & 0x7)<<4); // 8 bits used
	sysconf.conf_io_addrx[index] = val | ((io_min<<8) & 0xfffff000); // base : with enable bit

	if (sysconf.io_addr_num < (index+1))
		sysconf.io_addr_num = index+1;
}


static void store_conf_mmio_addr(u32 nodeid, u32 linkn, u32 reg, u32 index,
					u32 mmio_min, u32 mmio_max)
{
	u32 val;

	/* io range allocation */
	index = (reg-0x80)>>3;

	val = (nodeid & 0x3f); // 6 bits used
	sysconf.conf_mmio_addr[index] = val | (mmio_max & 0xffffff00); //limit : with nodeid and linkn
	val = 3 | ((linkn & 0x7)<<4); // 8 bits used
	sysconf.conf_mmio_addrx[index] = val | (mmio_min & 0xffffff00); // base : with enable bit

	if (sysconf.mmio_addr_num<(index+1))
		sysconf.mmio_addr_num = index+1;
}


static void set_io_addr_reg(struct device *dev, u32 nodeid, u32 linkn, u32 reg,
		     u32 io_min, u32 io_max)
{
	u32 i;
	u32 tempreg;

	/* io range allocation */
	tempreg = (nodeid&0xf) | ((nodeid & 0x30)<<(8-4)) | (linkn<<4) |  ((io_max&0xf0)<<(12-4)); //limit
	for (i = 0; i < sysconf.nodes; i++)
		pci_write_config32(__f1_dev[i], reg+4, tempreg);

	tempreg = 3 /*| (3<<4)*/ | ((io_min&0xf0)<<(12-4));	      //base :ISA and VGA ?
	for (i = 0; i < sysconf.nodes; i++)
		pci_write_config32(__f1_dev[i], reg, tempreg);
}

static void set_mmio_addr_reg(u32 nodeid, u32 linkn, u32 reg, u32 index, u32 mmio_min, u32 mmio_max, u32 nodes)
{
	u32 i;
	u32 tempreg;

	/* io range allocation */
	tempreg = (nodeid&0xf) | (linkn<<4) |	 (mmio_max&0xffffff00); //limit
	for (i = 0; i < nodes; i++)
		pci_write_config32(__f1_dev[i], reg+4, tempreg);
	tempreg = 3 | (nodeid & 0x30) | (mmio_min&0xffffff00);
	for (i = 0; i < sysconf.nodes; i++)
		pci_write_config32(__f1_dev[i], reg, tempreg);
}

static int reg_useable(unsigned int reg, struct device *goal_dev, unsigned int goal_nodeid,
			unsigned int goal_link)
{
	struct resource *res;
	unsigned int nodeid, link = 0;
	int result;
	res = 0;
	for (nodeid = 0; !res && (nodeid < fx_devs); nodeid++) {
		struct device *dev;
		dev = __f0_dev[nodeid];
		if (!dev)
			continue;
		for (link = 0; !res && (link < 8); link++) {
			res = probe_resource(dev, IOINDEX(0x1000 + reg, link));
		}
	}
	result = 2;
	if (res) {
		result = 0;
		if (	(goal_link == (link - 1)) &&
			(goal_nodeid == (nodeid - 1)) &&
			(res->flags <= 1)) {
			result = 1;
		}
	}
	return result;
}

static struct resource *amdfam10_find_iopair(struct device *dev, unsigned int nodeid, unsigned int link)
{
	struct resource *resource;
	u32 free_reg, reg;
	resource = 0;
	free_reg = 0;
	for (reg = 0xc0; reg <= 0xd8; reg += 0x8) {
		int result;
		result = reg_useable(reg, dev, nodeid, link);
		if (result == 1) {
			/* I have been allocated this one */
			break;
		} else if (result > 1) {
			/* I have a free register pair */
			free_reg = reg;
		}
	}
	if (reg > 0xd8) {
		reg = free_reg; // if no free, the free_reg still be 0
	}

	//Ext conf space
	if (!reg) {
		//because of Extend conf space, we will never run out of reg,
		// but we need one index to differ them. so same node and
		// same link can have multi range
		u32 index = get_io_addr_index(nodeid, link);
		reg = 0x110 + (index<<24) + (4<<20); // index could be 0, 255
	}

		resource = new_resource(dev, IOINDEX(0x1000 + reg, link));

	return resource;
}

static struct resource *amdfam10_find_mempair(struct device *dev, u32 nodeid, u32 link)
{
	struct resource *resource;
	u32 free_reg, reg;
	resource = 0;
	free_reg = 0;
	for (reg = 0x80; reg <= 0xb8; reg += 0x8) {
		int result;
		result = reg_useable(reg, dev, nodeid, link);
		if (result == 1) {
			/* I have been allocated this one */
			break;
		} else if (result > 1) {
			/* I have a free register pair */
			free_reg = reg;
		}
	}
	if (reg > 0xb8) {
		reg = free_reg;
	}

	//Ext conf space
	if (!reg) {
		//because of Extend conf space, we will never run out of reg,
		// but we need one index to differ them. so same node and
		// same link can have multi range
		u32 index = get_mmio_addr_index(nodeid, link);
		reg = 0x110 + (index<<24) + (6<<20); // index could be 0, 63

	}
	resource = new_resource(dev, IOINDEX(0x1000 + reg, link));
	return resource;
}

static void amdfam10_link_read_bases(struct device *dev, u32 nodeid, u32 link)
{
	struct resource *resource;

	/* Initialize the io space constraints on the current bus */
	resource = amdfam10_find_iopair(dev, nodeid, link);
	if (resource) {
		u32 align;
		align = log2(HT_IO_HOST_ALIGN);
		resource->base	= 0;
		resource->size	= 0;
		resource->align = align;
		resource->gran	= align;
		resource->limit = 0xffffUL;
		resource->flags = IORESOURCE_IO | IORESOURCE_BRIDGE;
	}

	/* Initialize the prefetchable memory constraints on the current bus */
	resource = amdfam10_find_mempair(dev, nodeid, link);
	if (resource) {
		resource->base = 0;
		resource->size = 0;
		resource->align = log2(HT_MEM_HOST_ALIGN);
		resource->gran = log2(HT_MEM_HOST_ALIGN);
		resource->limit = 0xffffffffffULL;
		resource->flags = IORESOURCE_MEM | IORESOURCE_PREFETCH;
		resource->flags |= IORESOURCE_BRIDGE;
	}

	/* Initialize the memory constraints on the current bus */
	resource = amdfam10_find_mempair(dev, nodeid, link);
	if (resource) {
		resource->base = 0;
		resource->size = 0;
		resource->align = log2(HT_MEM_HOST_ALIGN);
		resource->gran = log2(HT_MEM_HOST_ALIGN);
		resource->limit = 0xffffffffffULL;
		resource->flags = IORESOURCE_MEM | IORESOURCE_BRIDGE;
	}
}


static void amdfam10_create_vga_resource(struct device *dev, unsigned int nodeid)
{
	struct bus *link;

	/* find out which link the VGA card is connected,
	 * we only deal with the 'first' vga card */
	for (link = dev->link_list; link; link = link->next) {
		if (link->bridge_ctrl & PCI_BRIDGE_CTL_VGA) {
#if CONFIG(MULTIPLE_VGA_ADAPTERS)
			extern struct device *vga_pri; // the primary vga device, defined in device.c
			printk(BIOS_DEBUG, "VGA: vga_pri bus num = %d bus range [%d,%d]\n", vga_pri->bus->secondary,
				link->secondary,link->subordinate);
			/* We need to make sure the vga_pri is under the link */
			if ((vga_pri->bus->secondary >= link->secondary) &&
			    (vga_pri->bus->secondary <= link->subordinate))
#endif
			break;
		}
	}

	/* no VGA card installed */
	if (link == NULL)
		return;

	printk(BIOS_DEBUG, "VGA: %s (aka node %d) link %d has VGA device\n", dev_path(dev), nodeid, link->link_num);
	set_vga_enable_reg(nodeid, link->link_num);
}

static void amdfam10_read_resources(struct device *dev)
{
	get_fx_devs();

	u32 nodeid;
	struct bus *link;

	nodeid = amdfam10_nodeid(dev);

	amdfam10_create_vga_resource(dev, nodeid);

	for (link = dev->link_list; link; link = link->next) {
		if (link->children) {
			amdfam10_link_read_bases(dev, nodeid, link->link_num);
		}
	}
}

static void amdfam10_set_resource(struct device *dev, struct resource *resource,
				u32 nodeid)
{
	resource_t rbase, rend;
	unsigned int reg, link_num;
	char buf[50];

	/* Make certain the resource has actually been set */
	if (!(resource->flags & IORESOURCE_ASSIGNED)) {
		return;
	}

	/* If I have already stored this resource don't worry about it */
	if (resource->flags & IORESOURCE_STORED) {
		return;
	}

	/* Only handle PCI memory and IO resources */
	if (!(resource->flags & (IORESOURCE_MEM | IORESOURCE_IO)))
		return;

	/* Ensure I am actually looking at a resource of function 1 */
	if ((resource->index & 0xffff) < 0x1000) {
		return;
	}
	/* Get the base address */
	rbase = resource->base;

	/* Get the limit (rounded up) */
	rend  = resource_end(resource);

	/* Get the register and link */
	reg  = resource->index & 0xfff; // 4k
	link_num = IOINDEX_LINK(resource->index);

	if (resource->flags & IORESOURCE_IO) {
		set_io_addr_reg(dev, nodeid, link_num, reg, rbase>>8, rend>>8);
		store_conf_io_addr(nodeid, link_num, reg, (resource->index >> 24), rbase>>8, rend>>8);
	} else if (resource->flags & IORESOURCE_MEM) {
		set_mmio_addr_reg(nodeid, link_num, reg, (resource->index >>24), rbase>>8, rend>>8, sysconf.nodes); // [39:8]
		store_conf_mmio_addr(nodeid, link_num, reg, (resource->index >>24), rbase>>8, rend>>8);
	}
	resource->flags |= IORESOURCE_STORED;
	snprintf(buf, sizeof(buf), " <node %x link %x>",
		 nodeid, link_num);
	report_resource_stored(dev, resource, buf);
}

static void amdfam10_set_resources(struct device *dev)
{
	unsigned int nodeid;
	struct bus *bus;
	struct resource *res;

	/* Find the nodeid */
	nodeid = amdfam10_nodeid(dev);

	/* Set each resource we have found */
	for (res = dev->resource_list; res; res = res->next) {
		amdfam10_set_resource(dev, res, nodeid);
	}

	for (bus = dev->link_list; bus; bus = bus->next) {
		if (bus->children) {
			assign_resources(bus);
		}
	}
}

static void mcf0_control_init(struct device *dev)
{
}

static struct device_operations northbridge_operations = {
	.read_resources	  = amdfam10_read_resources,
	.set_resources	  = amdfam10_set_resources,
	.enable_resources = pci_dev_enable_resources,
	.init		  = mcf0_control_init,
	.scan_bus	  = amdfam10_scan_chains,
	.enable		  = 0,
	.ops_pci	  = 0,
};

static const struct pci_driver mcf0_driver __pci_driver = {
	.ops	= &northbridge_operations,
	.vendor = PCI_VENDOR_ID_AMD,
	.device = 0x1200,
};

static const struct pci_driver mcf0_driver_fam15_model10 __pci_driver = {
	.ops	= &northbridge_operations,
	.vendor = PCI_VENDOR_ID_AMD,
	.device = 0x1400,
};

static const struct pci_driver mcf0_driver_fam15 __pci_driver = {
	.ops	= &northbridge_operations,
	.vendor = PCI_VENDOR_ID_AMD,
	.device = 0x1600,
};

static void amdfam10_nb_init(void *chip_info)
{
	relocate_sb_ht_chain();
}

struct chip_operations northbridge_amd_amdfam10_ops = {
	CHIP_NAME("AMD Family 10h/15h Northbridge")
	.enable_dev = 0,
	.init = amdfam10_nb_init,
};

#if CONFIG(GENERATE_SMBIOS_TABLES)
static int amdfam10_get_smbios_data(struct device *dev, int *handle, unsigned long *current)
{
	return 0;
}
#endif
#if CONFIG(HAVE_ACPI_TABLES)
static const char *amdfam10_domain_acpi_name(const struct device *dev)
{
	if (dev->path.type == DEVICE_PATH_DOMAIN)
		return "PCI0";

	return NULL;
}
#endif

static void reserve_cpu_cc6_storage_area(struct device *dev, int idx)
{
	if (!(is_fam15h() && get_uint_option("cpu_cc6_state", 1)))
		return;

	uint8_t node;
	uint8_t interleaved;
	int8_t range;
	uint8_t max_node;
	uint64_t max_range_limit;
	uint32_t dword;
	uint32_t dword2;
	uint64_t qword;
	uint8_t num_nodes;

	/* Find highest DRAM range (DramLimitAddr) */
	num_nodes = 0;
	max_node = 0;
	interleaved = 0;
	max_range_limit = 0;
	struct device *node_dev;
	for (node = 0; node < NODE_NUMS; node++) {
		node_dev = pcidev_on_root(CONFIG_CDB + node, 0);
		/* Test for node presence */
		if ((!node_dev) || (pci_read_config32(node_dev, PCI_VENDOR_ID) == 0xffffffff))
			continue;

		num_nodes++;
		for (range = 0; range < 8; range++) {
			dword = pci_read_config32(pcidev_on_root(CONFIG_CDB + node, 1),
							0x40 + (range * 0x8));
			if (!(dword & 0x3))
				continue;

			if ((dword >> 8) & 0x7)
				interleaved = 1;

			dword = pci_read_config32(pcidev_on_root(CONFIG_CDB + node, 1),
							0x44 + (range * 0x8));
			dword2 = pci_read_config32(pcidev_on_root(CONFIG_CDB + node, 1),
							0x144 + (range * 0x8));
			qword = 0xffffff;
			qword |= ((((uint64_t)dword) >> 16) & 0xffff) << 24;
			qword |= (((uint64_t)dword2) & 0xff) << 40;

			if (qword > max_range_limit) {
				max_range_limit = qword;
				max_node = dword & 0x7;
			}
		}
	}

	/* Calculate CC6 storage area size */
	if (interleaved)
		qword = (uint64_t)0x1000000 * num_nodes;
	else
		qword = 0x1000000;

	/* FIXME
		* The BKDG appears to be incorrect as to the location of the CC6 save region
		* lower boundary on non-interleaved systems, causing lockups on attempted write
		* to the CC6 save region.
		*
		* For now, work around by allocating the maximum possible CC6 save region size.
		*
		* Determine if this is a BKDG error or a setup problem and remove this warning!
		*/
	qword = (0x1 << 27);
	max_range_limit = (((uint64_t)(pci_read_config32(pcidev_on_root(CONFIG_CDB + max_node, 1),
								0x124) & 0x1fffff)) << 27) - 1;

	printk(BIOS_INFO, "Reserving CC6 save segment base: %08llx size: %08llx\n", (max_range_limit + 1), qword);

	/* Reserve the CC6 save segment */
	reserved_ram_resource(dev, idx++, (max_range_limit + 1) >> 10, qword >> 10);
}

static void amdfam10_domain_read_resources(struct device *dev)
{
	unsigned int reg;
	msr_t tom2;
	int idx = 7;	// value from original code

	/* Find the already assigned resource pairs */
	get_fx_devs();
	for (reg = 0x80; reg <= 0xd8; reg+= 0x08) {
		u32 base, limit;
		base  = f1_read_config32(reg);
		limit = f1_read_config32(reg + 0x04);
		/* Is this register allocated? */
		if ((base & 3) != 0) {
			unsigned int nodeid, reg_link;
			struct device *reg_dev;
			if (reg < 0xc0) { // mmio
				nodeid = (limit & 0xf) + (base&0x30);
			} else { // io
				nodeid =  (limit & 0xf) + ((base>>4)&0x30);
			}
			reg_link = (limit >> 4) & 7;
			reg_dev = __f0_dev[nodeid];
			if (reg_dev) {
				/* Reserve the resource  */
				struct resource *res;
				res = new_resource(reg_dev, IOINDEX(0x1000 + reg, reg_link));
				if (res) {
					res->flags = 1;
				}
			}
		}
	}
	/* FIXME: do we need to check extend conf space?
	   I don't believe that much preset value */

	pci_domain_read_resources(dev);

	/* We have MMCONF_SUPPORT, create the resource window. */
	mmconf_resource(dev, MMIO_CONF_BASE);

	ram_resource(dev, idx++, 0, 0xa0000 >> 10);

	/* Reserve everything between A segment and 1MB:
	 *
	 * 0xa0000 - 0xbffff: legacy VGA
	 * 0xc0000 - 0xfffff: option ROMs and SeaBIOS (if used)
	 */
	mmio_resource(dev, idx++, 0xa0000 >> 10, (0xc0000 - 0xa0000) >> 10);
	reserved_ram_resource(dev, idx++, 0xc0000 >> 10, (0x100000 - 0xc0000) >> 10);

	/* The rest up to TOP_MEM and TOP_MEM2 is RAM */
	ram_resource(dev, idx++, 0x100000, rdmsr(TOP_MEM).lo >> 10);
	tom2 = rdmsr(TOP_MEM2);
	printk(BIOS_INFO, "TOM2: %08x%08x\n", tom2.hi, tom2.lo);
	if (tom2.hi)
		ram_resource(dev, idx++, 4 * (GiB/KiB),
		             ((tom2.lo >> 10) | (tom2.hi << (32 - 10))) - 4 * (GiB/KiB));

	reserve_cpu_cc6_storage_area(dev, idx);
}

static void amdfam10_domain_scan_bus(struct device *dev)
{
	u32 reg;
	int i;
	struct bus *link;

	/* Unmap all of the HT chains */
	for (reg = 0xe0; reg <= 0xec; reg += 4) {
		f1_write_config32(reg, 0);
	}

	for (link = dev->link_list; link; link = link->next) {
		link->secondary = dev->bus->subordinate;
		pci_scan_bus(link, PCI_DEVFN(CONFIG_CDB, 0), 0xff);
		dev->bus->subordinate = link->subordinate;
	}

	/* Tune the hypertransport transaction for best performance.
	 * Including enabling relaxed ordering if it is safe.
	 */
	get_fx_devs();
	for (i = 0; i < fx_devs; i++) {
		struct device *f0_dev;
		f0_dev = __f0_dev[i];
		if (f0_dev && f0_dev->enabled) {
			u32 httc;
			httc = pci_read_config32(f0_dev, HT_TRANSACTION_CONTROL);
			httc &= ~HTTC_RSP_PASS_PW;
			if (!dev->link_list->disable_relaxed_ordering) {
				httc |= HTTC_RSP_PASS_PW;
			}
			printk(BIOS_SPEW, "%s passpw: %s\n",
				dev_path(dev),
				(!dev->link_list->disable_relaxed_ordering)?
				"enabled":"disabled");
			pci_write_config32(f0_dev, HT_TRANSACTION_CONTROL, httc);
		}
	}
}

static struct device_operations pci_domain_ops = {
	.read_resources	  = amdfam10_domain_read_resources,
	.set_resources	  = pci_domain_set_resources,
	.enable_resources = NULL,
	.init		  = NULL,
	.scan_bus	  = amdfam10_domain_scan_bus,
#if CONFIG(HAVE_ACPI_TABLES)
	.acpi_name	  = amdfam10_domain_acpi_name,
#endif
#if CONFIG(GENERATE_SMBIOS_TABLES)
	.get_smbios_data  = amdfam10_get_smbios_data,
#endif
};

static void remap_nodes_0_to_31_to_bus_255(struct device *pci_domain)
{
	struct device *dev_mc;

	dev_mc = pcidev_on_root(CONFIG_CDB, 0); //0x00
	if (dev_mc && dev_mc->bus) {
		printk(BIOS_DEBUG, "%s found", dev_path(dev_mc));
		pci_domain = dev_mc->bus->dev;
		if (pci_domain && (pci_domain->path.type == DEVICE_PATH_DOMAIN)) {
			printk(BIOS_DEBUG, "\n%s move to ",dev_path(dev_mc));
			dev_mc->bus->secondary = CONFIG_CBB; // move to 0xff
			printk(BIOS_DEBUG, "%s",dev_path(dev_mc));

		} else {
			printk(BIOS_DEBUG, " but it is not under pci_domain directly ");
		}
		printk(BIOS_DEBUG, "\n");
	}
	dev_mc = dev_find_slot(CONFIG_CBB, PCI_DEVFN(CONFIG_CDB, 0));
	dev_mc = dev_mc ? dev_mc : pcidev_on_root(0x18, 0);

	if (dev_mc && dev_mc->bus) {
		printk(BIOS_DEBUG, "%s found\n", dev_path(dev_mc));
		pci_domain = dev_mc->bus->dev;
		if (pci_domain && (pci_domain->path.type == DEVICE_PATH_DOMAIN)) {
			if ((pci_domain->link_list) &&
			    (pci_domain->link_list->children == dev_mc)) {
				printk(BIOS_DEBUG, "%s move to ",dev_path(dev_mc));
				dev_mc->bus->secondary = CONFIG_CBB; // move to 0xff
				printk(BIOS_DEBUG, "%s\n",dev_path(dev_mc));
				while (dev_mc) {
					printk(BIOS_DEBUG, "%s move to ",dev_path(dev_mc));
					dev_mc->path.pci.devfn -= PCI_DEVFN(0x18,0);
					printk(BIOS_DEBUG, "%s\n",dev_path(dev_mc));
					dev_mc = dev_mc->sibling;
				}
			}
		}
	}
}

static void remap_bsp_lapic(struct bus *cpu_bus)
{
	struct device_path cpu_path;
	struct device *cpu;
	u32 bsp_lapic_id = lapicid();

	if (bsp_lapic_id) {
		cpu_path.type = DEVICE_PATH_APIC;
		cpu_path.apic.apic_id = 0;
		cpu = find_dev_path(cpu_bus, &cpu_path);
		if (cpu)
			cpu->path.apic.apic_id = bsp_lapic_id;
	}
}

static unsigned int get_num_siblings(void)
{
	unsigned int siblings;
	unsigned int ApicIdCoreIdSize = (cpuid_ecx(0x80000008)>>12 & 0xf);

	if (ApicIdCoreIdSize) {
		siblings = (1 << ApicIdCoreIdSize) - 1;
	} else {
		siblings = 3; //quad core
	}

	return siblings;
}

static void setup_cdb_links(unsigned int busn, unsigned int devn, struct bus *pbus)
{
	struct device *cdb_dev;
	/* Find the cpu's pci device */
	cdb_dev = dev_find_slot(busn, PCI_DEVFN(devn, 0));
	if (!cdb_dev) {
		/* If I am probing things in a weird order
			* ensure all of the cpu's pci devices are found.
			*/
		int fn;
		for (fn = 0; fn <= 5; fn++) { //FBDIMM?
			cdb_dev = pci_probe_dev(NULL, pbus,
				PCI_DEVFN(devn, fn));
		}
	}


	/* Ok, We need to set the links for that device.
		* otherwise the device under it will not be scanned
		*/
	cdb_dev = dev_find_slot(busn, PCI_DEVFN(devn, 0));
	if (cdb_dev)
		add_more_links(cdb_dev, 4);

	cdb_dev = dev_find_slot(busn, PCI_DEVFN(devn, 4));
	if (cdb_dev)
		add_more_links(cdb_dev, 4);
}

static int get_num_cores(unsigned int busn, unsigned int devn, int *cores_found,
			 int *enable_node)
{
	int j;
	struct device *cdb_dev;
	unsigned int siblings = get_num_siblings();

	*cores_found = 0; // one core
	if (is_fam15h())
		cdb_dev = dev_find_slot(busn, PCI_DEVFN(devn, 5));
	else
		cdb_dev = dev_find_slot(busn, PCI_DEVFN(devn, 3));

	*enable_node = cdb_dev && cdb_dev->enabled;
	if (*enable_node) {
		if (is_fam15h()) {
			*cores_found = pci_read_config32(cdb_dev, 0x84) & 0xff;
		} else {
			j = pci_read_config32(cdb_dev, 0xe8);
			*cores_found = (j >> 12) & 3; // dev is func 3
			if (siblings > 3)
				*cores_found |= (j >> 13) & 4;
		}
		printk(BIOS_DEBUG, "  %s siblings=%d\n", dev_path(cdb_dev), *cores_found);
	}

	if (siblings > *cores_found)
		siblings = *cores_found;

	return j;
}

static uint8_t check_dual_node_cap(u8 node)
{
	uint8_t dual_node = 0;
	uint32_t model;

	model = cpuid_eax(0x80000001);
	model = ((model & 0xf0000) >> 12) | ((model & 0xf0) >> 4);

	if ((model >= 0x8) || is_fam15h()) {
		/* Check for dual node capability */
		if (is_dual_node(node))
			dual_node = 1;
	}

	return dual_node;
}

static u32 get_apic_id(int i, int j)
{
	u32 apic_id;
	uint8_t dual_node = check_dual_node_cap((u8)i);
	uint8_t fam15h = is_fam15h();
	unsigned int siblings = get_num_siblings();
	// How can I get the nb_cfg_54 of every node's nb_cfg_54 in bsp???
	unsigned int nb_cfg_54 = read_nb_cfg_54();

	if (dual_node) {
		apic_id = 0;
		if (fam15h) {
			apic_id |= ((i >> 1) & 0x3) << 5;			/* Node ID */
			apic_id |= ((i & 0x1) * (siblings + 1)) + j;		/* Core ID */
		} else {
			if (nb_cfg_54) {
				apic_id |= ((i >> 1) & 0x3) << 4;			/* Node ID */
				apic_id |= ((i & 0x1) * (siblings + 1)) + j;		/* Core ID */
			} else {
				apic_id |= i & 0x3;					/* Node ID */
				apic_id |= (((i & 0x1) * (siblings + 1)) + j) << 4;	/* Core ID */
			}
		}
	} else {
		if (fam15h) {
			apic_id = 0;
			apic_id |= (i & 0x7) << 4;	/* Node ID */
			apic_id |= j & 0xf;		/* Core ID */
		} else {
			apic_id = i * (nb_cfg_54?(siblings+1):1) + j * (nb_cfg_54?1:64); // ?
		}
	}

	if (CONFIG(ENABLE_APIC_EXT_ID) && (CONFIG_APIC_ID_OFFSET > 0)) {
		if (sysconf.enabled_apic_ext_id) {
			if (apic_id != 0 || sysconf.lift_bsp_apicid) {
				apic_id += sysconf.apicid_offset;
			}
		}
	}

	return apic_id;
}

static void sysconf_init(struct device *dev) // first node
{
	sysconf.sblk = (pci_read_config32(dev, 0x64) >> 8) & 7; // don't forget sublink1
	sysconf.segbit = 0;
	sysconf.ht_c_num = 0;
	unsigned int ht_c_index;

	for (ht_c_index = 0; ht_c_index < 32; ht_c_index++) {
		sysconf.ht_c_conf_bus[ht_c_index] = 0;
	}

	sysconf.nodes = ((pci_read_config32(dev, 0x60)>>4) & 7) + 1;
	if (CONFIG_MAX_PHYSICAL_CPUS > 8)
		sysconf.nodes += (((pci_read_config32(dev, 0x160)>>4) & 7)<<3);


	sysconf.enabled_apic_ext_id = 0;
	sysconf.lift_bsp_apicid = 0;

	/* Find the bootstrap processors apicid */
	sysconf.bsp_apicid = lapicid();
	sysconf.apicid_offset = sysconf.bsp_apicid;

	if (CONFIG(ENABLE_APIC_EXT_ID)) {
		if (pci_read_config32(dev, 0x68) & (HTTC_APIC_EXT_ID | HTTC_APIC_EXT_BRD_CST))
		{
			sysconf.enabled_apic_ext_id = 1;
		}
		if (sysconf.enabled_apic_ext_id && CONFIG_APIC_ID_OFFSET > 0) {
			if (sysconf.bsp_apicid == 0) {
				/* bsp apic id is not changed */
				sysconf.apicid_offset = CONFIG_APIC_ID_OFFSET;
			} else {
				sysconf.lift_bsp_apicid = 1;
			}
		}
	}
}

static void cpu_bus_scan(struct device *dev)
{
	struct bus *cpu_bus;
	struct device *dev_mc;
	struct device *pci_domain;
	int i,j;
	int cores_found;
	int disable_siblings = !get_uint_option("multi_core", CONFIG(LOGICAL_CPUS));
	uint8_t disable_cu_siblings = !get_uint_option("compute_unit_siblings", 1);
	int enable_node;

	if (CONFIG_CBB)
		remap_nodes_0_to_31_to_bus_255(pci_domain);

	dev_mc = dev_find_slot(CONFIG_CBB, PCI_DEVFN(CONFIG_CDB, 0));
	if (!dev_mc) {
		printk(BIOS_ERR, "%02x:%02x.0 not found", CONFIG_CBB, CONFIG_CDB);
		die("");
	}

	sysconf_init(dev_mc);

	/* Find which cpus are present */
	cpu_bus = dev->link_list;
	/* Always use the devicetree node with lapic_id 0 for BSP. */
	remap_bsp_lapic(cpu_bus);

	if (disable_cu_siblings)
		printk(BIOS_DEBUG, "Disabling siblings on each compute unit as requested\n");

	for (i = 0; i < sysconf.nodes; i++) {
		unsigned int busn, devn;
		struct bus *pbus;
		u32 jj;

		busn = CONFIG_CBB;
		devn = CONFIG_CDB + i;
		pbus = dev_mc->bus;
		if (CONFIG_CBB && (NODE_NUMS > 32)) {
			if (i >= 32) {
				busn--;
				devn -= 32;
				pbus = pci_domain->link_list->next;
			}
		}

		setup_cdb_links(busn, devn, pbus);
		j = get_num_cores(busn, devn, &cores_found, &enable_node);

		if (disable_siblings)
			jj = 0;
		else
			jj = cores_found;

		for (j = 0; j <=jj; j++) {

			if (disable_cu_siblings && (j & 0x1))
				continue;

			struct device *cpu = add_cpu_device(cpu_bus, get_apic_id(i, j),
							    enable_node);
			if (cpu)
				amd_cpu_topology(cpu, i, j);
		}
	}
}

static uint8_t probe_filter_prepare(uint32_t *f3x58, uint32_t *f3x5c)
{
	uint8_t i;
	uint8_t pfmode = 0x0;
	uint32_t dword;

	/* Disable L3 and DRAM scrubbers and configure system for probe filter support */
	for (i = 0; i < sysconf.nodes; i++) {
		struct device *f2x_dev = pcidev_on_root(0x18 + i, 2);
		struct device *f3x_dev = pcidev_on_root(0x18 + i, 3);

		f3x58[i] = pci_read_config32(f3x_dev, 0x58);
		f3x5c[i] = pci_read_config32(f3x_dev, 0x5c);
		pci_write_config32(f3x_dev, 0x58, f3x58[i] & ~((0x1f << 24) | 0x1f));
		pci_write_config32(f3x_dev, 0x5c, f3x5c[i] & ~0x1);

		dword = pci_read_config32(f2x_dev, 0x1b0);
		dword &= ~(0x7 << 8);	/* CohPrefPrbLmt = 0x0 */
		pci_write_config32(f2x_dev, 0x1b0, dword);

		msr_t msr = rdmsr_amd(BU_CFG2_MSR);
		msr.hi |= 1 << (42 - 32);
		wrmsr_amd(BU_CFG2_MSR, msr);

		if (is_fam15h()) {
			uint8_t subcache_size = 0x0;
			uint8_t pref_so_repl = 0x0;
			uint32_t f3x1c4 = pci_read_config32(f3x_dev, 0x1c4);
			if ((f3x1c4 & 0xffff) == 0xcccc) {
				subcache_size = 0x1;
				pref_so_repl = 0x2;
				pfmode = 0x3;
			} else {
				pfmode = 0x2;
			}

			dword = pci_read_config32(f3x_dev, 0x1d4);
			dword |= 0x1 << 29;	/* PFLoIndexHashEn = 0x1 */
			dword &= ~(0x3 << 20);	/* PFPreferredSORepl = pref_so_repl */
			dword |= (pref_so_repl & 0x3) << 20;
			dword |= 0x1 << 17;	/* PFWayHashEn = 0x1 */
			dword |= 0xf << 12;	/* PFSubCacheEn = 0xf */
			dword &= ~(0x3 << 10);	/* PFSubCacheSize3 = subcache_size */
			dword |= (subcache_size & 0x3) << 10;
			dword &= ~(0x3 << 8);	/* PFSubCacheSize2 = subcache_size */
			dword |= (subcache_size & 0x3) << 8;
			dword &= ~(0x3 << 6);	/* PFSubCacheSize1 = subcache_size */
			dword |= (subcache_size & 0x3) << 6;
			dword &= ~(0x3 << 4);	/* PFSubCacheSize0 = subcache_size */
			dword |= (subcache_size & 0x3) << 4;
			dword &= ~(0x3 << 2);	/* PFWayNum = 0x2 */
			dword |= 0x2 << 2;
			pci_write_config32(f3x_dev, 0x1d4, dword);
		} else {
			pfmode = 0x2;

			dword = pci_read_config32(f3x_dev, 0x1d4);
			dword |= 0x1 << 29;	/* PFLoIndexHashEn = 0x1 */
			dword &= ~(0x3 << 20);	/* PFPreferredSORepl = 0x2 */
			dword |= 0x2 << 20;
			dword |= 0xf << 12;	/* PFSubCacheEn = 0xf */
			dword &= ~(0x3 << 10);	/* PFSubCacheSize3 = 0x0 */
			dword &= ~(0x3 << 8);	/* PFSubCacheSize2 = 0x0 */
			dword &= ~(0x3 << 6);	/* PFSubCacheSize1 = 0x0 */
			dword &= ~(0x3 << 4);	/* PFSubCacheSize0 = 0x0 */
			dword &= ~(0x3 << 2);	/* PFWayNum = 0x2 */
			dword |= 0x2 << 2;
			pci_write_config32(f3x_dev, 0x1d4, dword);
		}
	}

	return pfmode;
}

static void probe_filter_enable(uint8_t pfmode)
{
	uint8_t i;
	uint32_t dword;

	/* Enable probe filter */
	for (i = 0; i < sysconf.nodes; i++) {
		struct device *f3x_dev = pcidev_on_root(0x18 + i, 3);

		dword = pci_read_config32(f3x_dev, 0x1c4);
		dword |= (0x1 << 31);	/* L3TagInit = 1 */
		pci_write_config32(f3x_dev, 0x1c4, dword);
		do {
		} while (pci_read_config32(f3x_dev, 0x1c4) & (0x1 << 31));

		dword = pci_read_config32(f3x_dev, 0x1d4);
		dword &= ~0x3;		/* PFMode = pfmode */
		dword |= pfmode & 0x3;
		pci_write_config32(f3x_dev, 0x1d4, dword);
		do {
		} while (!(pci_read_config32(f3x_dev, 0x1d4) & (0x1 << 19)));
	}
}

static void enable_atm_mode(void)
{
	uint32_t dword;
	uint8_t i;

	printk(BIOS_DEBUG, "Enabling ATM mode\n");

	/* Enable ATM mode */
	for (i = 0; i < sysconf.nodes; i++) {
		struct device *f0x_dev = pcidev_on_root(0x18 + i, 0);
		struct device *f3x_dev = pcidev_on_root(0x18 + i, 3);

		dword = pci_read_config32(f0x_dev, 0x68);
		dword |= (0x1 << 12);	/* ATMModeEn = 1 */
		pci_write_config32(f0x_dev, 0x68, dword);

		dword = pci_read_config32(f3x_dev, 0x1b8);
		dword |= (0x1 << 27);	/* L3ATMModeEn = 1 */
		pci_write_config32(f3x_dev, 0x1b8, dword);
	}
}

static void detect_and_enable_probe_filter(struct device *dev)
{
	uint8_t rev_gte_d = is_gt_rev_d();
	uint8_t pfmode = 0;
	uint8_t i;

	/* Check to see if the probe filter is allowed */
	if (!get_uint_option("probe_filter", 1))
		return;

	if (rev_gte_d && (sysconf.nodes > 1)) {
		/* Enable the probe filter */
		uint32_t f3x58[MAX_NODES_SUPPORTED];
		uint32_t f3x5c[MAX_NODES_SUPPORTED];

		printk(BIOS_DEBUG, "Enabling probe filter\n");
		pfmode = probe_filter_prepare(f3x58, f3x5c);

		udelay(40);
		disable_cache();
		wbinvd();

		probe_filter_enable(pfmode);

		if (is_fam15h()) {
			enable_atm_mode();
		}

		enable_cache();

		/* Reenable L3 and DRAM scrubbers */
		for (i = 0; i < sysconf.nodes; i++) {
			struct device *f3x_dev = pcidev_on_root(0x18 + i, 3);

			pci_write_config32(f3x_dev, 0x58, f3x58[i]);
			pci_write_config32(f3x_dev, 0x5c, f3x5c[i]);
		}
	}
}

static void detect_and_enable_cache_partitioning(struct device *dev)
{
	uint8_t i;
	uint32_t dword;

	if (!get_uint_option("l3_cache_partitioning", 0))
		return;

	if (is_fam15h()) {
		printk(BIOS_DEBUG, "Enabling L3 cache partitioning\n");

		uint32_t f5x80;
		uint8_t cu_enabled;
		uint8_t compute_unit_count = 0;

		for (i = 0; i < sysconf.nodes; i++) {
			struct device *f3x_dev = pcidev_on_root(0x18 + i, 3);
			struct device *f4x_dev = pcidev_on_root(0x18 + i, 4);
			struct device *f5x_dev = pcidev_on_root(0x18 + i, 5);

			/* Determine the number of active compute units on this node */
			f5x80 = pci_read_config32(f5x_dev, 0x80);
			cu_enabled = f5x80 & 0xf;
			if (cu_enabled == 0x1)
				compute_unit_count = 1;
			if (cu_enabled == 0x3)
				compute_unit_count = 2;
			if (cu_enabled == 0x7)
				compute_unit_count = 3;
			if (cu_enabled == 0xf)
				compute_unit_count = 4;

			/* Disable BAN mode */
			dword = pci_read_config32(f3x_dev, 0x1b8);
			dword &= ~(0x7 << 19);	/* L3BanMode = 0x0 */
			pci_write_config32(f3x_dev, 0x1b8, dword);

			/* Set up cache mapping */
			dword = pci_read_config32(f4x_dev, 0x1d4);
			if (compute_unit_count == 1) {
				dword |= 0xf;		/* ComputeUnit0SubCacheEn = 0xf */
			}
			if (compute_unit_count == 2) {
				dword &= ~(0xf << 4);	/* ComputeUnit1SubCacheEn = 0xc */
				dword |= (0xc << 4);
				dword &= ~0xf;		/* ComputeUnit0SubCacheEn = 0x3 */
				dword |= 0x3;
			}
			if (compute_unit_count == 3) {
				dword &= ~(0xf << 8);	/* ComputeUnit2SubCacheEn = 0x8 */
				dword |= (0x8 << 8);
				dword &= ~(0xf << 4);	/* ComputeUnit1SubCacheEn = 0x4 */
				dword |= (0x4 << 4);
				dword &= ~0xf;		/* ComputeUnit0SubCacheEn = 0x3 */
				dword |= 0x3;
			}
			if (compute_unit_count == 4) {
				dword &= ~(0xf << 12);	/* ComputeUnit3SubCacheEn = 0x8 */
				dword |= (0x8 << 12);
				dword &= ~(0xf << 8);	/* ComputeUnit2SubCacheEn = 0x4 */
				dword |= (0x4 << 8);
				dword &= ~(0xf << 4);	/* ComputeUnit1SubCacheEn = 0x2 */
				dword |= (0x2 << 4);
				dword &= ~0xf;		/* ComputeUnit0SubCacheEn = 0x1 */
				dword |= 0x1;
			}
			pci_write_config32(f4x_dev, 0x1d4, dword);

			/* Enable cache partitioning */
			pci_write_config32(f4x_dev, 0x1d4, dword);
			if (compute_unit_count == 1) {
				dword &= ~(0xf << 26);	/* MaskUpdateForComputeUnit = 0x1 */
				dword |= (0x1 << 26);
			} else if (compute_unit_count == 2) {
				dword &= ~(0xf << 26);	/* MaskUpdateForComputeUnit = 0x3 */
				dword |= (0x3 << 26);
			} else if (compute_unit_count == 3) {
				dword &= ~(0xf << 26);	/* MaskUpdateForComputeUnit = 0x7 */
				dword |= (0x7 << 26);
			} else if (compute_unit_count == 4) {
				dword |= (0xf << 26);	/* MaskUpdateForComputeUnit = 0xf */
			}
			pci_write_config32(f4x_dev, 0x1d4, dword);
		}
	}
}

static void cpu_bus_init(struct device *dev)
{
	detect_and_enable_probe_filter(dev);
	detect_and_enable_cache_partitioning(dev);
	initialize_cpus(dev->link_list);
}

static struct device_operations cpu_bus_ops = {
	.read_resources	  = noop_read_resources,
	.set_resources	  = noop_set_resources,
	.init		  = cpu_bus_init,
	.scan_bus	  = cpu_bus_scan,
};

static void setup_uma_memory(void)
{
#if CONFIG(GFXUMA)
	uint32_t topmem = (uint32_t) bsp_topmem();

	uma_memory_size = get_uma_memory_size(topmem);
	uma_memory_base = topmem - uma_memory_size;	/* TOP_MEM1 */

	printk(BIOS_INFO, "%s: uma size 0x%08llx, memory start 0x%08llx\n",
			 __func__, uma_memory_size, uma_memory_base);
#endif
}


static void root_complex_enable_dev(struct device *dev)
{
	static int done = 0;

	/* Do not delay UMA setup, as a device on the PCI bus may evaluate
	   the global uma_memory variables already in its enable function. */
	if (!done) {
		setup_bsp_ramtop();
		setup_uma_memory();
		done = 1;
	}

	/* Set the operations if it is a special bus type */
	if (dev->path.type == DEVICE_PATH_DOMAIN) {
		dev->ops = &pci_domain_ops;
	} else if (dev->path.type == DEVICE_PATH_CPU_CLUSTER) {
		dev->ops = &cpu_bus_ops;
	}
}

static const char *interleave_strings[] = {
	[0] = "No",
	[1] = "on A[12] (2 nodes)",
	[2] = "Reserved",
	[3] = "on A[12] and A[13] (4 nodes)",
	[4] = "Reserved",
	[5] = "Reserved",
	[6] = "Reserved",
	[7] = "on A[12], A[13], and A[14] (8 nodes)",
};

static void decode_dram_flags(u32 flags)
{
	if (flags & 1)
		printk(BIOS_DEBUG, "RE, ");
	if (flags & 2)
		printk(BIOS_DEBUG, "WE, ");
	printk(BIOS_DEBUG, "Interleave %s, ", interleave_strings[(flags >> 8) & 7]);
	printk(BIOS_DEBUG, "Destination Node %d, ", (flags >> 16) & 7);
	printk(BIOS_DEBUG, "Inerleave select %x\n", (flags >> 24) & 7);
}

static void dump_dram_mapping(u8 node)
{
	u64 base, limit;
	u16 reg;
	u32 flags;

	for (reg = 0x40; reg <= 0x78; reg += 4) {
		base = pci_read_config32(__f1_dev[node], reg);
		flags = base & 0xffff;
		base &= 0xffff0000;
		base <<= 8;
		base |= ((u64)pci_read_config8(__f1_dev[node], reg + 0x100) << 40);

		reg += 4;

		limit = pci_read_config32(__f1_dev[node], reg);
		flags |= ((limit & 0xffff) << 16);
		limit &= 0xffff0000;
		limit <<= 8;
		limit |= ((u64)pci_read_config8(__f1_dev[node], reg + 0x100) << 40);

		printk(BIOS_DEBUG, "DRAM [0x%016llx - 0x%016llx] - flags: ", base, limit);
		decode_dram_flags(flags);
	}
}

static void decode_mmio_flags(u32 flags)
{
	if (flags & 1)
		printk(BIOS_DEBUG, "RE, ");
	if (flags & 2)
		printk(BIOS_DEBUG, "WE, ");
	if (flags & 8)
		printk(BIOS_DEBUG, "L, ");
	if (flags & (1 << 23))
		printk(BIOS_DEBUG, "NP, ");
	printk(BIOS_DEBUG, "Destination Node %d, ", (flags >> 16) & 7);
	printk(BIOS_DEBUG, "Destination Link %d sublink %d\n", (flags >> 20) & 3, (flags >> 22) & 1);
}

static void dump_mmio_mapping(u8 node)
{
	u64 base, limit;
	u16 reg;
	u32 flags;
	u32 hi_addr = 0x180;

	for (reg = 0x80; reg <= 0xb8; reg += 4) {
		base = pci_read_config32(__f1_dev[node], reg);
		flags = base & 0xffff;
		base &= 0xffff0000;
		base <<= 8;
		base |= ((u64)pci_read_config8(__f1_dev[node], hi_addr) << 40);

		reg += 4;

		limit = pci_read_config32(__f1_dev[node], reg);
		flags |= ((limit & 0xffff) << 16);
		limit &= 0xffff0000;
		limit <<= 8;
		limit |= ((u64)pci_read_config8(__f1_dev[node], hi_addr) << 40);

		hi_addr += 4;

		printk(BIOS_DEBUG, "MMIO [0x%016llx - 0x%016llx] - flags: ", base, limit);
		decode_mmio_flags(flags);
	}

	hi_addr = 0x1c0;
	for (reg = 0x1a0; reg <= 0x1b8; reg += 4) {
		base = pci_read_config32(__f1_dev[node], reg);
		flags = base & 0xf;
		base &= 0xfffffff0;
		base <<= 8;
		base |= ((u64)(pci_read_config8(__f1_dev[node], hi_addr) & 0xff) << 40);

		reg += 4;

		limit = pci_read_config32(__f1_dev[node], reg);
		flags |= ((limit & 0xff) << 16);
		limit &= 0xfffffff0;
		limit <<= 8;
		limit |= ((u64)(pci_read_config8(__f1_dev[node], hi_addr) & 0xff0000) << 24);

		hi_addr += 4;

		printk(BIOS_DEBUG, "MMIO [0x%016llx - 0x%016llx] - flags: ", base, limit);
		decode_mmio_flags(flags);
	}
}

static void decode_io_flags(u32 flags)
{
	if (flags & 1)
		printk(BIOS_DEBUG, "RE, ");
	if (flags & 2)
		printk(BIOS_DEBUG, "WE, ");
	if (flags & (1 << 4))
		printk(BIOS_DEBUG, "VE, ");
	if (flags & (1 << 5))
		printk(BIOS_DEBUG, "IE, ");
	printk(BIOS_DEBUG, "Destination Node %d, ", (flags >> 16) & 7);
	printk(BIOS_DEBUG, "Destination Link %d sublink %d\n", (flags >> 20) & 3, (flags >> 22) & 1);
}

static void dump_io_mapping(u8 node)
{
	u32 base, limit;
	u16 reg;
	u32 flags;

	for (reg = 0xc0; reg <= 0xd8; reg += 4) {
		base = pci_read_config32(__f1_dev[node], reg);
		flags = base & 0xff;
		base &= 0x00fff000;

		reg += 4;

		limit = pci_read_config32(__f1_dev[node], reg);
		flags |= ((limit & 0xff) << 16);
		limit &= 0x00fff000;

		printk(BIOS_DEBUG, "IO [0x%08x - 0x%08x] - flags: ", base, limit);
		decode_io_flags(flags);
	}
}


static void decode_bus_flags(u16 flags)
{
	if (flags & 1)
		printk(BIOS_DEBUG, "RE, ");
	if (flags & 2)
		printk(BIOS_DEBUG, "WE, ");
	if (flags & 4)
		printk(BIOS_DEBUG, "DCE, ");

	printk(BIOS_DEBUG, "Destination Node %d, ", (flags >> 4) & 7);
	printk(BIOS_DEBUG, "Destination Link %d sublink %d\n", (flags >> 8) & 3, (flags >> 10) & 1);
}

static void dump_config_mapping(u8 node)
{
	u32 bus_base, bus_limit;
	u32 dword;
	u16 reg;
	u16 flags;

	for (reg = 0xe0; reg <= 0xec; reg += 4) {
		dword = pci_read_config32(__f1_dev[node], reg);
		flags = dword & 0xffff;
		bus_base = (dword & 0x00ff0000) >> 16;
		bus_limit = (dword & 0xff000000) >> 24;

		printk(BIOS_DEBUG, "Bus [0x%02x - 0x%02x] - flags: ", bus_base, bus_limit);
		decode_bus_flags(flags);
	}
}

static void decode_system_dram_flags(u8 flags)
{
	printk(BIOS_DEBUG, "Interleave %s, ", interleave_strings[(flags >> 4) & 7]);
	printk(BIOS_DEBUG, "Inerleave select %x\n", flags & 7);
}

static void dump_system_dram_mapping(u8 node)
{
	u64 base, limit;
	u8 flags;

	base = pci_read_config32(__f1_dev[node], 0x120);
	flags = (base >> 21) & 0x7;
	base &= 0x001fffff;
	base <<= 27;

	limit = pci_read_config32(__f1_dev[node], 0x124);
	flags |= (((base >> 21) & 0x7) << 4);
	limit &= 0x001fffff;
	limit <<= 27;

	printk(BIOS_DEBUG, "System Address DRAM [0x%016llx - 0x%016llx] - flags: ", base, limit);
	decode_system_dram_flags(flags);

}

void dump_memory_mapping(void)
{
	u8 i;

	/* Refresh fx devs because these could be remapped */
	fx_devs = 0;
	get_fx_devs();

	if (fx_devs < sysconf.nodes) {
		printk(BIOS_WARNING, "WARNING: Less fx devs than nodes\n");
	}

	/* Enable probe filter */
	for (i = 0; i < sysconf.nodes; i++) {
		printk(BIOS_DEBUG, "Memory and bus map for node %d:\n", i);
		dump_dram_mapping(i);
		dump_mmio_mapping(i);
		dump_io_mapping(i);
		dump_config_mapping(i);
		dump_system_dram_mapping(i);
	}
}


static void root_complex_finalize(void *chip_info)
{
	dump_memory_mapping();
	display_mtrrs();
}

struct chip_operations northbridge_amd_amdfam10_root_complex_ops = {
	CHIP_NAME("AMD Family 10h/15h Root Complex")
	.enable_dev = root_complex_enable_dev,
	.final = root_complex_finalize,
};
