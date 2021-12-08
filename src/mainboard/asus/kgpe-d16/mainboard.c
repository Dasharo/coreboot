/* SPDX-License-Identifier: GPL-2.0-only */

#include <console/console.h>
#include <cpu/amd/family_10h-family_15h/amdfam10_sysconf.h>
#include <device/device.h>
#include <device/pci_ops.h>
#include <northbridge/amd/amdfam10/amdfam10.h>
#include <southbridge/amd/sb700/sb700.h>
#include <southbridge/amd/sr5650/cmn.h>

void get_bus_conf(void)
{
	u8 router_bus;
	struct amdfam10_sysconf_t *sysconf = get_sysconf();
	get_default_pci1234(1);

	sysconf->sbdn = (sysconf->hcdn[0] & 0xff);
	router_bus = (sysconf->pci1234[0] >> 16) & 0xff;

	set_pirq_router_bus(router_bus);
}

void set_pcie_reset(void)
{
	struct device *pcie_core_dev;

	pcie_core_dev = pcidev_on_root(0, 0);
	set_htiu_enable_bits(pcie_core_dev, 0xA8, 0xFFFFFFFF, 0x28282828);
	set_htiu_enable_bits(pcie_core_dev, 0xA9, 0x000000FF, 0x00000028);
}

void set_pcie_dereset(void)
{
	struct device *pcie_core_dev;

	pcie_core_dev = pcidev_on_root(0, 0);
	set_htiu_enable_bits(pcie_core_dev, 0xA8, 0xFFFFFFFF, 0x6F6F6F6F);
	set_htiu_enable_bits(pcie_core_dev, 0xA9, 0x000000FF, 0x0000006F);
}


/* override the default SATA PHY setup */
void sb7xx_51xx_setup_sata_phys(struct device *dev)
{
	/* RPR7.6.1 Program the PHY Global Control to 0x2C00 */
	pci_write_config16(dev, 0x86, 0x2c00);

	/* RPR7.6.2 SATA GENI PHY ports setting */
	pci_write_config32(dev, 0x88, 0x01b48016);
	pci_write_config32(dev, 0x8c, 0x01b48016);
	pci_write_config32(dev, 0x90, 0x01b48016);
	pci_write_config32(dev, 0x94, 0x01b48016);
	pci_write_config32(dev, 0x98, 0x01b48016);
	pci_write_config32(dev, 0x9c, 0x01b48016);

	/* RPR7.6.3 SATA GEN II PHY port setting for port [0~5]. */
	pci_write_config16(dev, 0xa0, 0xa07a);
	pci_write_config16(dev, 0xa2, 0xa07a);
	pci_write_config16(dev, 0xa4, 0xa07a);
	pci_write_config16(dev, 0xa6, 0xa07a);
	pci_write_config16(dev, 0xa8, 0xa07a);
	pci_write_config16(dev, 0xaa, 0xa07a);
}

/* override the default SATA port setup */
void sb7xx_51xx_setup_sata_port_indication(void *sata_bar5)
{
	uint32_t dword;

	/* RPR7.9 Program Port Indication Registers */
	dword = read32(sata_bar5 + 0xf8);
	dword &= ~(0x3f << 12);	/* All ports are iSATA */
	dword &= ~0x3f;
	write32(sata_bar5 + 0xf8, dword);

	dword = read32(sata_bar5 + 0xfc);
	dword &= ~(0x1 << 20);	/* No eSATA ports are present */
	write32(sata_bar5 + 0xfc, dword);
}

static void mainboard_enable(struct device *dev)
{
	printk(BIOS_INFO, "Mainboard KGPE-D16 Enable. dev=0x%p\n", dev);
}

struct chip_operations mainboard_ops = {
	.enable_dev = mainboard_enable,
};
