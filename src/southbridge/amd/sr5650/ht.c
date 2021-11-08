/* SPDX-License-Identifier: GPL-2.0-only */

#include <arch/ioapic.h>
#include <console/console.h>
#include <device/device.h>
#include <device/pci.h>
#include <device/pci_ids.h>
#include <device/pci_ops.h>
#include "sr5650.h"
#include "cmn.h"

/* Table 6-6 Recommended Interrupt Routing Configuration */
typedef struct _apic_device_info {
	u8	group;
	u8	swizzle;
	u8	pin;
} apic_device_info;

#define ABCD		0
#define BCDA		1
#define CDAB		2
#define DABC		3

static const apic_device_info default_apic_device_info_t [] = {
	/* Group  Swizzling   Port Int Pin */
	[0] = {0,     0,          31},   /* HT */
	[1] = {0,     0,          31},   /* IOMMU */
	[2] = {0,     ABCD,       28},   /* Dev2  Grp0 [Int - 0..3] */
	[3] = {1,     ABCD,       28},   /* Dev3  Grp1 [Int - 4..7] */
	[4] = {5,     ABCD,       28},   /* Dev4  Grp5 [Int - 20..23] */
	[5] = {5,     CDAB,       28},   /* Dev5  Grp5 [Int - 20..23] */
	[6] = {6,     BCDA,       29},   /* Dev6  Grp6 [Int - 24..27] */
	[7] = {6,     CDAB,       29},   /* Dev7  Grp6 [Int - 24..27] */
	[8] = {0,     0,          0 },   /* Reserved */
	[9] = {6,     ABCD,       29},   /* Dev9  Grp6 [Int - 24..27] */
	[10] = {5,     BCDA,       30},   /* Dev10 Grp5 [Int - 20..23] */
	[11] = {2,     ABCD,       30},   /* Dev11 Grp2 [Int - 8..11] */
	[12] = {3,     ABCD,       30},   /* Dev12 Grp3 [Int - 12..15] */
	[13] = {4,     ABCD,       30}    /* Dev13 Grp4 [Int - 16..19] */
};

/* These define names are common, so undefine them to avoid potential issues in other code */
#undef ABCD
#undef BCDA
#undef CDAB
#undef DABC

/* Init APIC of sr5690 */
static void sr5690_apic_init(struct device *dev)
{
	u32 dword;
	u8 apicid_sr5650;
	const apic_device_info *entry = default_apic_device_info_t;

	/* rpr6.2.(2). Write to the IOAPIC Features Enable register */
	pci_write_config32(dev, 0xF8, 0x0);
	pci_write_config32(dev, 0xFC, 0x1F);
	/* rpr6.2.(3). Write to the remaining interrupt routing registers */

	/* IOAPICCMISCIND:0x3, group & swizzle of Dev 2,3,4,5 */
	dword = (entry[2].group | entry[2].swizzle << 4) << 0  |
		(entry[3].group | entry[3].swizzle << 4) << 8  |
		(entry[4].group | entry[4].swizzle << 4) << 16 |
		(entry[5].group | entry[5].swizzle << 4) << 24;
	pci_write_config32(dev, 0xF8, 0x3);
	pci_write_config32(dev, 0xFC, dword);

	/* IOAPICCMISCIND:0x4, group & swizzle of Dev 6,7,9,10 */
	dword = (entry[6].group | entry[6].swizzle << 4) << 0  |
		(entry[7].group | entry[7].swizzle << 4) << 8  |
		(entry[9].group | entry[9].swizzle << 4) << 16 |
		(entry[10].group | entry[10].swizzle << 4) << 24;
	pci_write_config32(dev, 0xF8, 0x4);
	pci_write_config32(dev, 0xFC, dword);

	/* IOAPICCMISCIND:0x5, group & swizzle of Dev 11,12,13 */
	dword = (entry[11].group | entry[11].swizzle << 4) << 0  |
		(entry[12].group | entry[12].swizzle << 4) << 8  |
		(entry[13].group | entry[13].swizzle << 4) << 16;
	pci_write_config32(dev, 0xF8, 0x5);
	pci_write_config32(dev, 0xFC, dword);

	/* IOAPICCMISCIND:0x6, pin map of dev 2,3,4,5 */
	dword = entry[2].pin |
		entry[3].pin << 8 |
		entry[4].pin << 16|
		entry[5].pin << 24;
	pci_write_config32(dev, 0xF8, 0x6);
	pci_write_config32(dev, 0xFC, dword);

	/* IOAPICCMISCIND:0x7, pin map of dev 6,7,8,9 */
	dword = entry[6].pin |
		entry[7].pin << 8 |
		entry[8].pin << 16|
		entry[9].pin << 24;
	pci_write_config32(dev, 0xF8, 0x7);
	pci_write_config32(dev, 0xFC, dword);

	/* IOAPICCMISCIND:0x8, pin map of dev 10,11,12,13 */
	dword = entry[10].pin |
		entry[11].pin << 8 |
		entry[12].pin << 16|
		entry[13].pin << 24;
	pci_write_config32(dev, 0xF8, 0x8);
	pci_write_config32(dev, 0xFC, dword);

	/* IOAPICCMISCIND:0x9, pin map of ht, iommu */
	dword = entry[0].pin | entry[1].pin << 8;
	pci_write_config32(dev, 0xF8, 0x9);
	pci_write_config32(dev, 0xFC, dword);

	pci_write_config32(dev, 0xF8, 0x1);
	dword = IO_APIC2_ADDR;
	dword = (1 << 3); /* Mem_IO_Map: 1 = MMIO, 0 = IO */
	pci_write_config32(dev, 0xFC, IO_APIC2_ADDR);

	/* Upper bits of IOAPIC base */
	pci_write_config32(dev, 0xF8, 0x2);
	pci_write_config32(dev, 0xFC, 0);
	/* On SR56x0/SP5100 board, the IOAPIC on SR56x0 is the
	 * 2nd one. We need to check if it also is on your board. */

	pci_write_config32(dev, 0xF8, 0x0);
	pci_read_config32(dev, 0xFC);

	printk(BIOS_DEBUG, "IOAPIC2 features: %08x\n", pci_read_config32(dev, 0xFC));

	if (CONFIG(ENABLE_APIC_EXT_ID) && (CONFIG_APIC_ID_OFFSET > 0))
		apicid_sr5650 = 0x1;
	else
		apicid_sr5650 = 0x21;

	setup_ioapic((void *)IO_APIC2_ADDR, apicid_sr5650);
}

static void pcie_init(struct device *dev)
{
	/* Enable pci error detecting */
	u32 dword;

	printk(BIOS_INFO, "pcie_init in sr5650_ht.c\n");

	/* System error enable */
	dword = pci_read_config32(dev, 0x04);
	dword |= (1 << 8);	/* System error enable */
	dword |= (1 << 30);	/* Clear possible errors */
	pci_write_config32(dev, 0x04, dword);

	/*
	 * 1 is APIC enable
	 * 18 is enable nb to accept A4 interrupt request from SB.
	 */
	dword = pci_read_config32(dev, 0x4C);
	dword |= (1 << 1) | (1 << 18);	/* Clear possible errors */
	pci_write_config32(dev, 0x4C, dword);

	sr5690_apic_init(dev);
}

static void sr5690_read_resource(struct device *dev)
{
	struct resource *res;

	if (CONFIG(MMCONF_SUPPORT)) {
		printk(BIOS_DEBUG,"%s: %s\n", __func__, dev_path(dev));
		set_nbmisc_enable_bits(dev, 0x0, 1 << 3, 1 << 3);	/* Hide BAR3 */
	}

	pci_dev_read_resources(dev);

	/* IOAPIC */
	res = new_resource(dev, 0xfc);
	res->base  = IO_APIC2_ADDR;
	res->size = 0x1000;
	res->flags = IORESOURCE_MEM | IORESOURCE_FIXED | IORESOURCE_ASSIGNED |
		     IORESOURCE_STORED;

	compact_resources(dev);
}

/* If IOAPIC's index changes, we should replace the pci_dev_set_resource(). */
static void sr5690_set_resources(struct device *dev)
{
	uint32_t reg;
	struct device *amd_ht_cfg_dev = NULL;
	struct device *amd_addr_map_dev = NULL;
	resource_t res_base;
	resource_t res_end;
	uint32_t base;
	uint32_t limit;
	struct resource *res;

	printk(BIOS_DEBUG,"%s %s\n", dev_path(dev), __func__);
	/* Set IOAPIC's index to 1 and make sure no one changes it */
	pci_write_config32(dev, 0xf8, 0x1);

	if (!CONFIG(MMCONF_SUPPORT)) {
		pci_dev_set_resources(dev);
		return;
	}

	amd_ht_cfg_dev = pcidev_on_root(0x18, 0);
	amd_addr_map_dev = pcidev_on_root(0x18, 1);

	if (!amd_ht_cfg_dev || !amd_addr_map_dev)
		die("Unable to locate CPU control devices");

	res = sr5650_retrieve_cpu_mmio_resource();

	if (res) {
		res_base = res->base;		/* Get the base address */
		res_end = resource_end(res);	/* Get the limit (rounded up) */
	} else {
		res_base = CONFIG_MMCONF_BASE_ADDRESS;
		res_end = CONFIG_MMCONF_BASE_ADDRESS + CONFIG_MMCONF_LENGTH;
	}


	/* Set up MMCONFIG bus range */
	/* Make BAR3 visible */
	set_nbmisc_enable_bits(dev, 0x0, 1 << 3, 0 << 3);
	/* Enables writes to the BAR3 register */
	set_nbcfg_enable_bits(dev, 0x7c, 1 << 30, 1 << 30);
	/* Program bus range = 255 busses */
	set_nbcfg_enable_bits(dev, 0x84, 7 << 16, 0 << 16);
	pci_write_config32(dev, 0x1c, res_base);

	/* Enable MMCONFIG decoding. */
	/* PCIEMiscInit */
	set_htiu_enable_bits(dev, 0x32, 1 << 28, 1 << 28);
	/* Disable writes to the BAR3 register */
	set_nbcfg_enable_bits(dev, 0x7c, 1 << 30, 0 << 30);
	/* Hide BAR3 */
	set_nbmisc_enable_bits(dev, 0x0, 1 << 3, 1 << 3);

	/* Set up nonposted resource in MMIO space */
	printk(BIOS_DEBUG, "%s: %s[0x1c] base = %0llx limit = %0llx\n", __func__,
			   dev_path(dev), res_base, res_end);

	/* Locate an unused MMIO resource */
	for (reg = 0xb8; reg >= 0x80; reg -= 8) {
		base = pci_read_config32(amd_addr_map_dev, reg);
		limit = pci_read_config32(amd_addr_map_dev, reg + 4);
		if (!(base & 0x3))
			break;	/* Unused resource found */
	}

	/* If an unused MMIO resource was available, set up the mapping */
	if (!(base & 0x3)) {
		uint32_t sblk;

		/* Get SBLink value (HyperTransport I/O Hub Link ID). */
		sblk = (pci_read_config32(amd_ht_cfg_dev, 0x64) >> 8) & 0x3;

		/* Calculate the MMIO mapping base */
		base &= 0x000000f0;
		base |= ((res_base >> 8) & 0xffffff00);
		base |= 3;

		/* Calculate the MMIO mapping limit */
		limit &= 0x00000048;
		limit |= ((res_end >> 8) & 0xffffff00);
		limit |= (sblk << 4);
		limit |= (1 << 7);

		/* Configure and enable MMIO mapping */
		printk(BIOS_INFO, "%s: %s <- index %x base %04x limit %04x\n", __func__, dev_path(amd_addr_map_dev), reg, base, limit);
		pci_write_config32(amd_addr_map_dev, reg + 4, limit);
		pci_write_config32(amd_addr_map_dev, reg, base);
	}
	else {
		printk(BIOS_WARNING, "%s: %s No free MMIO resources available\n", __func__, dev_path(dev));
	}

	pci_dev_set_resources(dev);
}

static struct pci_operations lops_pci = {
	.set_subsystem = pci_dev_set_subsystem,
};

static struct device_operations ht_ops = {
	.read_resources = sr5690_read_resource,
	.set_resources = sr5690_set_resources,
	.enable_resources = pci_dev_enable_resources,
	.init = pcie_init,
	.scan_bus = 0,
	.ops_pci = &lops_pci,
};

static const struct pci_driver ht_driver_sr5690 __pci_driver = {
	.ops = &ht_ops,
	.vendor = PCI_VENDOR_ID_ATI,
	.device = PCI_DEVICE_ID_AMD_SR5690_HT,
};

static const struct pci_driver ht_driver_sr5670 __pci_driver = {
	.ops = &ht_ops,
	.vendor = PCI_VENDOR_ID_ATI,
	.device = PCI_DEVICE_ID_AMD_SR5670_HT,
};

static const struct pci_driver ht_driver_sr5650 __pci_driver = {
	.ops = &ht_ops,
	.vendor = PCI_VENDOR_ID_ATI,
	.device = PCI_DEVICE_ID_AMD_SR5650_HT,
};
