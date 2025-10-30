/* SPDX-License-Identifier: GPL-2.0-only */


#include <acpi/acpi.h>
#include <acpi/acpigen.h>
#include <acpi/acpigen_pci.h>
#include <acpi/acpi_device.h>
#include <amdblocks/amd_pci_util.h>
#include <amdblocks/ioapic.h>
#include <amdblocks/smn.h>
#include <amdblocks/root_complex.h>
#include <arch/ioapic.h>
#include <device/device.h>
#include <device/pci.h>
#include <device/pci_def.h>
#include <soc/soc_chip.h>
#include <types.h>

/*
 * The order of IOHCs here is not random. They are sorted so that:
 * 1. The First IOHC is the one with primary FCH. We want the LPC/SMBUS
 *    devices be on bus 0.
 * 2. The rest of IOHCs are listed in an order so that ECAM MMIO region is one
 *    continuous block for all domains.
 *
 * AGESA/OpenSIL sets up the PCI configuration decoding ranges in line with
 * this.
 */
static const struct domain_iohc_info iohc_info[] = {
	[0] = {
		.fabric_id = 0x24,
		.misc_smn_base = SMN_IOHC_MISC_BASE_13D1,
	},
	[1] = {
		.fabric_id = 0x25,
		.misc_smn_base = SMN_IOHC_MISC_BASE_1D61,
	},
	[2] = {
		.fabric_id = 0x26,
		.misc_smn_base = SMN_IOHC_MISC_BASE_13E1,
	},
	[3] = {
		.fabric_id = 0x27,
		.misc_smn_base = SMN_IOHC_MISC_BASE_1D51,
	},
	[4] = {
		.fabric_id = 0x23,
		.misc_smn_base = SMN_IOHC_MISC_BASE_1D41,
	},
	[5] = {
		.fabric_id = 0x22,
		.misc_smn_base = SMN_IOHC_MISC_BASE_13C1,
	},
	[6] = {
		.fabric_id = 0x21,
		.misc_smn_base = SMN_IOHC_MISC_BASE_1D71,
	},
	[7] = {
		.fabric_id = 0x20,
		.misc_smn_base = SMN_IOHC_MISC_BASE_13B1,
	},
};

const struct domain_iohc_info *get_iohc_info(size_t *count)
{
	*count = ARRAY_SIZE(iohc_info);
	return iohc_info;
}

static const struct non_pci_mmio_reg non_pci_mmio[] = {
	{ 0x2d8, 0xfffffff00000ull,   1 * MiB, NON_PCI_RES_IDX_AUTO },
	{ 0x2e0, 0xfffffff00000ull,   1 * MiB, NON_PCI_RES_IDX_AUTO },
	{ 0x2e8, 0xfffffff00000ull,   1 * MiB, NON_PCI_RES_IDX_AUTO },
	/*
	 * The hardware has a 256 byte alignment requirement for the IOAPIC
	 * MMIO base, but OpenSIL configures 64k-aligned base address and this
	 * is reported as 256 byte resource.
	 */
	{ 0x2f0, 0xffffffffff00ull,	  256, IOMMU_IOAPIC_IDX },
	{ 0x2f8, 0xfffffff00000ull,   1 * MiB, NON_PCI_RES_IDX_AUTO },
	{ 0x300, 0xfffffff00000ull,   1 * MiB, NON_PCI_RES_IDX_AUTO },
	{ 0x308, 0xfffffffff000ull,   4 * KiB, NON_PCI_RES_IDX_AUTO },
	{ 0x310, 0xfffffff00000ull,   1 * MiB, NON_PCI_RES_IDX_AUTO },
	{ 0x318, 0xfffffff80000ull, 512 * KiB, NON_PCI_RES_IDX_AUTO },
	{ 0x338, 0xfffffff00000ull,   1 * MiB, NON_PCI_RES_IDX_AUTO },
};

const struct non_pci_mmio_reg *get_iohc_non_pci_mmio_regs(size_t *count)
{
	*count = ARRAY_SIZE(non_pci_mmio);
	return non_pci_mmio;
}

#if ENV_RAMSTAGE

static const char *gnb_acpi_name(const struct device *dev)
{
	return "GNB";
}

struct pci_dev_int_routes {
	unsigned int devfn;
	unsigned int num_irqs;
	unsigned int irq;
};

static const struct pci_dev_int_routes iohc_devs[] = {
	{ .devfn = PCI_DEVFN(0, 0), .num_irqs = 1, .irq = 1 },
	{ .devfn = PCI_DEVFN(1, 0), .num_irqs = 4, .irq = 0 },
	{ .devfn = PCI_DEVFN(2, 0), .num_irqs = 2, .irq = 0 },
	{ .devfn = PCI_DEVFN(3, 0), .num_irqs = 4, .irq = 1 },
	{ .devfn = PCI_DEVFN(4, 0), .num_irqs = 2, .irq = 1 },
	{ .devfn = PCI_DEVFN(7, 1), .num_irqs = 1, .irq = 1 },
	{ .devfn = PCI_DEVFN(7, 2), .num_irqs = 1, .irq = 2 }
};

static const struct pci_dev_int_routes small_iohc_devs[] = {
	{ .devfn = PCI_DEVFN(0, 0), .num_irqs = 1, .irq = 1 },
	{ .devfn = PCI_DEVFN(1, 0), .num_irqs = 4, .irq = 0 },
	{ .devfn = PCI_DEVFN(2, 0), .num_irqs = 2, .irq = 0 }
};

static void acpigen_write_PRT_GSI(const struct device *rb)
{
	unsigned int irq;
	char *pkg_count;
	const struct device *dev;
	const struct pci_dev_int_routes *host_bridge_devs;
	unsigned int pci_rb_index = domain_to_rb_index[dev_get_domain_id(rb)];
	unsigned int num_devs;

	if (pci_rb_index < 4) {
		num_devs = ARRAY_SIZE(iohc_devs);
		host_bridge_devs = iohc_devs;
	} else {
		num_devs = ARRAY_SIZE(iohc_devs);
		host_bridge_devs = small_iohc_devs;
	}

	pkg_count = acpigen_write_package(0);/* Package - APIC Routing */

	for (unsigned int d = 0; d < num_devs; d++) {
		dev = pcidev_path_behind(rb->upstream, host_bridge_devs[d].devfn);
		if (!dev || !dev->enabled)
			continue;

		for (unsigned int i = 0; i < host_bridge_devs[d].num_irqs; ++i) {
			irq = host_bridge_devs[d].irq;
			(*pkg_count)++;
			acpigen_write_PRT_GSI_entry(
				PCI_SLOT(host_bridge_devs[d].devfn),
				i, /* pin */
				soc_get_gsi_base(dev) + irq);
		}
	}

	acpigen_pop_len(); /* Package - APIC Routing */
}

static void acpigen_write_PRT_PIC(const struct device *rb)
{
	unsigned int irq;
	char link_template[] = "\\_SB.INTX";
	char *pkg_count;
	struct device *dev;
	struct pci_dev_int_routes *host_bridge_devs;
	unsigned int pci_rb_index = domain_to_rb_index[dev_get_domain_id(rb)];
	unsigned int num_devs;

	if (pci_rb_index < 4) {
		num_devs = ARRAY_SIZE(iohc_devs);
		host_bridge_devs = (struct pci_dev_int_routes *)iohc_devs;
	} else {
		num_devs = ARRAY_SIZE(iohc_devs);
		host_bridge_devs = (struct pci_dev_int_routes *)small_iohc_devs;
	}

	pkg_count = acpigen_write_package(0); /* Package - PIC Routing */
	for (unsigned int d = 0; d < num_devs; d++) {
		dev = pcidev_path_behind(rb->upstream, host_bridge_devs[d].devfn);
		if (!dev || !dev->enabled)
			continue;

		for (unsigned int i = 0; i < host_bridge_devs[d].num_irqs; ++i) {
			irq = host_bridge_devs[d].irq;
			link_template[8] = 'A' + (irq % 8);
			(*pkg_count)++;
			acpigen_write_PRT_source_entry(
				PCI_SLOT(host_bridge_devs[d].devfn),
				i, /* pin */
				link_template /* Source */,
				0 /* Source Index */);
		}
	}

	acpigen_pop_len(); /* Package - PIC Routing */
}

static void acpigen_write_host_bridge_PRT(const struct device *dev)
{
	acpigen_write_method("_PRT", 0);

	/* If (PICM) */
	acpigen_write_if();
	acpigen_emit_namestring("PICM");

	/* Return (Package{...}) */
	acpigen_emit_byte(RETURN_OP);
	acpigen_write_PRT_GSI(dev);

	/* Else */
	acpigen_write_else();

	/* Return (Package{...}) */
	acpigen_emit_byte(RETURN_OP);
	acpigen_write_PRT_PIC(dev);

	acpigen_pop_len(); /* End Else */

	acpigen_pop_len(); /* Method */
}

static void acpigen_write_mpdma_device(const struct device *dev)
{
	struct resource *res;
	size_t reg_count, i;
	uint64_t bar, bar_offset, bar_size;
	uint32_t mpdma_redir_entry;
	struct device *domain = (struct device *)dev_get_domain(dev);
	struct acpi_irq mpdma_irq = ACPI_IRQ_EDGE_HIGH(0);
	const uint32_t iohc_misc_base = get_iohc_misc_smn_base(domain);
	const struct non_pci_mmio_reg *regs;

	if (dev_get_domain_id(dev) == 7) {
		printk(BIOS_DEBUG, "%s.DMA0\n", acpi_device_path(dev_get_domain(dev)));
		acpigen_write_device("DMA0");
		acpigen_write_name_string("_HID", "AMDI0096");
		acpigen_write_name_integer("_UID", 1);
		acpigen_pop_len(); /* Device */
	}

	if (dev_get_domain_id(dev) != 0)
		return;

	printk(BIOS_DEBUG, "%s.TMPM\n", acpi_device_path(dev_get_domain(dev)));
	acpigen_write_device("TMPM");
	acpigen_write_name_string("_HID", "AMDI0095");
	acpigen_write_name_integer("_UID", 0);

	regs = get_iohc_non_pci_mmio_regs(&reg_count);

	bar = 0;
	for (i = 0; i < reg_count; i++) {
		if (regs[i].res_idx != 0x300)
			continue;

		bar = smn_read64(iohc_misc_base + regs[i].iohc_misc_offset);
		break;
	}

	if (((bar & regs[i].mask) == 0) || !(bar & IOHC_MMIO_EN)) {
		acpigen_write_name_integer("_STA", 0);
		acpigen_pop_len(); /* Device */
		return;
	}

	bar &= regs[i].mask;
	bar_offset = 0x09510900 & (regs[i].size - 1);
	bar_size = regs[i].size - bar_offset;

	res = probe_resource(dev, IOMMU_IOAPIC_IDX);
	if (!res) {
		acpigen_write_name_integer("_STA", 0);
		acpigen_pop_len(); /* Device */
		return;
	}

	/* Calculate IOAPIC redirection entry offset based on RB index */
	mpdma_redir_entry = ioapic_get_max_vectors((uintptr_t)res->base);
	mpdma_redir_entry *= dev_get_domain_id(domain);
	/* Add offset of FCH IOAPIC redirection entries */
	mpdma_redir_entry += ioapic_get_max_vectors((uintptr_t)IO_APIC_ADDR);
	/* MPDMA has a fixed redirection entry of 28 */
	mpdma_redir_entry += 28;

	mpdma_irq.pin = mpdma_redir_entry;

	acpigen_write_name("_CRS");
	acpigen_write_resourcetemplate_header();
	acpigen_write_mem32fixed(1, bar + bar_offset, bar_size);
	acpi_device_write_interrupt(&mpdma_irq);
	acpigen_write_resourcetemplate_footer();

	acpigen_write_name_integer("_STA", 0xF);

	acpigen_pop_len(); /* Device */
}

static void gnb_fill_ssdt(const struct device *dev)
{
	const char *acpi_scope = acpi_device_path(dev_get_domain(dev));

	acpigen_write_scope(acpi_scope);

	printk(BIOS_DEBUG, "%s: writing _PRT\n", acpi_scope);
	acpigen_write_host_bridge_PRT(dev);

	acpigen_write_mpdma_device(dev);

	acpigen_pop_len(); /* Scope */
}

struct device_operations turin_root_complex_operations = {
	/* The root complex has no PCI BARs implemented, so there's no need to call
	   pci_dev_read_resources for it */
	.read_resources		= noop_read_resources,
	.set_resources		= noop_set_resources,
	.enable_resources	= pci_dev_enable_resources,
	.acpi_name		= gnb_acpi_name,
	.acpi_fill_ssdt		= gnb_fill_ssdt,
};

#endif /* ENV_RAMSTAGE */
