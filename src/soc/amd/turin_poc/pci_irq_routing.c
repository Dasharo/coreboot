/* SPDX-License-Identifier: GPL-2.0-only */

#include <amdblocks/amd_pci_util.h>
#include <amdblocks/root_complex.h>
#include <amdblocks/smn.h>
#include <console/console.h>
#include <device/pci_def.h>
#include <soc/soc_chip.h>
#include <types.h>

/* GNB IO-APIC is located after the FCH IO-APIC */
#define FCH_IOAPIC_INTERRUPTS		24
#define GNB_GSI_BASE			FCH_IOAPIC_INTERRUPTS
#define GNB_IOAPIC_REDIR_ENTRIES	32

#define BUSES_PER_DOMAIN		0x20

/* Small IOHC has a RCEC device which has a sepearate routing register */
#define MAX_RCEC_PER_IOHC		1
#define MAX_SMALL_IOHC_BRIDGES		(9 + MAX_RCEC_PER_IOHC)
#define MAX_BIG_IOHC_BRIDGES		(22 + MAX_RCEC_PER_IOHC)

#define MAX_BRIDGES ((MAX_BIG_IOHC_BRIDGES + MAX_SMALL_IOHC_BRIDGES) * 4)

#define SMN_IOHC_MISC_DEVICE_REMAP_REG_OFFSET	0xb8

#define SMN_IOHUB0_PCI_INTERRUPT_ROUTING(rb_index)	\
	(0x14300040 + (((rb_index) & 0x3) << 20))
#define SMN_IOHUB1_PCI_INTERRUPT_ROUTING(rb_index)	\
	(0x1d800040 + (((rb_index) & 0x3) << 20))

union iohc_nb_prog_device_remap {
	struct {
		uint32_t fn_num		:  3; /* [ 2.. 0] */
		uint32_t dev_num	:  5; /* [ 7.. 3] */
		uint32_t		: 24; /* [31.. 8] */
	};
	uint32_t raw;
};

union ioapic_br_irq_routing {
	struct {
		uint32_t intr_grp	:  3; /* [ 2.. 0] */
		uint32_t		:  1; /* [ 3.. 3] */
		uint32_t intr_swz	:  2; /* [ 5.. 4] */
		uint32_t		: 10; /* [15.. 6] */
		uint32_t intr_map	:  5; /* [20..16] */
		uint32_t		: 11; /* [31..21] */
	};
	uint32_t raw;
};

unsigned int soc_get_gsi_base(const struct device *dev)
{
	/* FCH IOAPIC is always first*/
	unsigned int gsi_base = GNB_GSI_BASE;
	/* GNB IOAPIC GSI offset based on physical RB index */
	gsi_base += (domain_to_rb_index[dev_get_domain_id(dev)] * GNB_IOAPIC_REDIR_ENTRIES);

	return gsi_base;
}

const struct pci_routing_info *get_pci_routing_table(size_t *entries)
{
	static bool table_initialized;
	static struct pci_routing_info routing_table[MAX_BRIDGES];
	unsigned int pci_rb_index, pci_logical_rb;
	size_t max_rbs, idx;
	const struct domain_iohc_info *iohc;
	size_t iohc_count;
	uint32_t routing_reg_base, remap_reg_base;

	union iohc_nb_prog_device_remap dev_remap;
	union ioapic_br_irq_routing ioapic_routing;

	if (table_initialized) {
		*entries = MAX_BRIDGES;
		return routing_table;
	}

	iohc = get_iohc_info(&iohc_count);
	idx = 0;
	for (size_t d = 0; d < MAX_DOMAINS; d++) {
		pci_rb_index = domain_to_rb_index[d];
		pci_logical_rb = domain_to_logical_rb[d];

		if (pci_rb_index < 4) {
			max_rbs = MAX_BIG_IOHC_BRIDGES - MAX_RCEC_PER_IOHC;
			routing_reg_base = SMN_IOHUB0_PCI_INTERRUPT_ROUTING(pci_logical_rb);
		} else {
			max_rbs = MAX_SMALL_IOHC_BRIDGES - MAX_RCEC_PER_IOHC;
			routing_reg_base = SMN_IOHUB0_PCI_INTERRUPT_ROUTING(pci_logical_rb);
		}

		remap_reg_base = iohc[d].misc_smn_base + SMN_IOHC_MISC_DEVICE_REMAP_REG_OFFSET;

		/* Special case: RCEC. No remapping, routing reg at base - 4 */
		ioapic_routing.raw = 0;
		ioapic_routing.intr_map = smn_read32(routing_reg_base - sizeof(uint32_t)) & 0x1f;
		routing_table[idx].group = ioapic_routing.intr_grp;
		routing_table[idx].swizzle = ioapic_routing.intr_swz;
		routing_table[idx].bridge_irq = ioapic_routing.intr_map;
		routing_table[idx].pci_addr = PCI_DEVFN(0, 3);
		routing_table[idx].pci_addr |= ((d * BUSES_PER_DOMAIN) << 8);
		printk(BIOS_DEBUG, "%04x:%02x:%02x.%x: group: %u, swizzle: %u, irq: %u\n",
			routing_table[idx].pci_addr >> 16,
			(routing_table[idx].pci_addr >> 8) & 0xff,
			PCI_SLOT(routing_table[idx].pci_addr),
			PCI_FUNC(routing_table[idx].pci_addr),
			routing_table[idx].group,
			routing_table[idx].swizzle,
			routing_table[idx].bridge_irq);
		idx++;

		/*
		 * For Turin, the logical and physical bridge numbers map 1:1, which may not be
		 * the case for other SoCs.
		 */
		for (size_t i = 0; i < max_rbs; i++) {
			dev_remap.raw =
				smn_read32(remap_reg_base + sizeof(uint32_t) * i);
			ioapic_routing.raw =
				smn_read32(routing_reg_base + sizeof(uint32_t) * i);
			routing_table[idx + i].pci_addr = dev_remap.fn_num;
			routing_table[idx + i].pci_addr |= (dev_remap.dev_num << 3);

			/* Workaround empty remap for bridges 7.1 and 7.2 */
			if (i >= 20 && routing_table[idx + i].pci_addr == 0) {
				routing_table[idx + i].pci_addr = PCI_DEVFN(7, i - 19);
				dev_remap.fn_num = i - 19;
				dev_remap.dev_num = 7;
				smn_write32(remap_reg_base + sizeof(uint32_t) * i,
					    dev_remap.raw);
			}

			routing_table[idx + i].pci_addr |= ((d * BUSES_PER_DOMAIN) << 8);
			routing_table[idx + i].group = ioapic_routing.intr_grp;
			routing_table[idx + i].swizzle = ioapic_routing.intr_swz;
			routing_table[idx + i].bridge_irq = ioapic_routing.intr_map;
			printk(BIOS_DEBUG, "%04x:%02x:%02x.%x: group: %u, swizzle: %u, irq: %u\n",
			       routing_table[idx].pci_addr >> 16,
			       (routing_table[idx].pci_addr >> 8) & 0xff,
			       PCI_SLOT(routing_table[idx + i].pci_addr),
			       PCI_FUNC(routing_table[idx + i].pci_addr),
			       routing_table[idx + i].group,
			       routing_table[idx + i].swizzle,
			       routing_table[idx + i].bridge_irq);
		}

		idx += max_rbs;
	}
	table_initialized = true;

	*entries = MAX_BRIDGES;
	return routing_table;
}
