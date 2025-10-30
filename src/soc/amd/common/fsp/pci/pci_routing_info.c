/* SPDX-License-Identifier: GPL-2.0-only */

#include <amdblocks/amd_pci_util.h>
#include <console/console.h>
#include <device/pci_def.h>
#include <fsp/util.h>
#include <FspGuids.h>
#include <string.h>
#include <types.h>

const struct pci_routing_info *get_pci_routing_table(size_t *entries)
{
	static struct pci_routing_info *routing_table = NULL;
	const struct fsp_pci_routing_info *fsp_routing_table = NULL;
	static size_t routing_table_entries;
	size_t hob_size = 0;
	size_t table_size = 0;
	const struct {
		uint32_t num_of_entries;
		struct fsp_pci_routing_info routing_table[];
	} __packed *routing_hob;

	if (routing_table) {
		*entries = routing_table_entries;
		return routing_table;
	}

	routing_hob = fsp_find_extension_hob_by_guid(AMD_FSP_PCIE_DEVFUNC_REMAP_HOB_GUID.b,
						       &hob_size);

	if (routing_hob == NULL || hob_size == 0 || routing_hob->num_of_entries == 0) {
		printk(BIOS_ERR, "Couldn't find valid PCIe interrupt routing HOB.\n");
		return NULL;
	}

	fsp_routing_table = routing_hob->routing_table;
	routing_table_entries = routing_hob->num_of_entries;
	table_size = routing_table_entries * sizeof(struct pci_routing_info);
	routing_table =(struct pci_routing_info *)malloc(table_size);

	if (routing_table == NULL) {
		printk(BIOS_ERR, "Couldn't allocate memory for PCIe interrupt routing table.\n");
		return NULL;
	}

	memset((void *)routing_table, 0, table_size);

	for (size_t i = 0; i < routing_table_entries; ++i) {
		routing_table[i].pci_addr =  fsp_routing_table[i].devfn;
		routing_table[i].group =  fsp_routing_table[i].group;
		routing_table[i].swizzle =  fsp_routing_table[i].swizzle;
		routing_table[i].bridge_irq =  fsp_routing_table[i].bridge_irq;

		printk(BIOS_DEBUG, "%02x.%x: group: %u, swizzle: %u, bridge irq: %u\n",
		       PCI_SLOT(fsp_routing_table[i].devfn), PCI_FUNC(fsp_routing_table[i].devfn),
		       fsp_routing_table[i].group, fsp_routing_table[i].swizzle, fsp_routing_table[i].bridge_irq);
	}

	*entries = routing_table_entries;

	return routing_table;
}
