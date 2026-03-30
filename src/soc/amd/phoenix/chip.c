/* SPDX-License-Identifier: GPL-2.0-only */

#include <acpi/acpigen_pci.h>
#include <amdblocks/acpi.h>
#include <amdblocks/data_fabric.h>
#include <amdblocks/fsp.h>
#include <amdblocks/root_complex.h>
#include <arch/ioapic.h>
#include <cbmem.h>
#include <console/console.h>
#include <device/device.h>
#include <device/pci.h>
#include <drivers/amd/opensil/opensil.h>
#include <root_bridge_info.h>
#include <soc/cpu.h>
#include <soc/pci_devs.h>
#include <soc/southbridge.h>
#include <types.h>

#include "chip.h"

/* EDK2 headers to construct proper RB attributes */
#include <Uefi/UefiBaseType.h>
#include <Uefi/UefiSpec.h>
#include <Protocol/DevicePath.h>
#include <Protocol/PciHostBridgeResourceAllocation.h>
#include <Protocol/PciIo.h>

#define ROOT_BRIDGE_SUPPORTS_DEFAULT	(EFI_PCI_IO_ATTRIBUTE_VGA_IO_16 | \
					 EFI_PCI_IO_ATTRIBUTE_VGA_PALETTE_IO_16 | \
					 EFI_PCI_IO_ATTRIBUTE_ISA_IO_16 | \
					 EFI_PCI_IO_ATTRIBUTE_IDE_PRIMARY_IO | \
					 EFI_PCI_IO_ATTRIBUTE_IDE_SECONDARY_IO | \
					 EFI_PCI_IO_ATTRIBUTE_VGA_IO | \
					 EFI_PCI_IO_ATTRIBUTE_VGA_MEMORY | \
					 EFI_PCI_IO_ATTRIBUTE_VGA_PALETTE_IO | \
					 EFI_PCI_IO_ATTRIBUTE_ISA_IO | \
					 EFI_PCI_IO_ATTRIBUTE_ISA_MOTHERBOARD_IO | \
					 EFI_PCI_IO_ATTRIBUTE_IO | \
					 EFI_PCI_IO_ATTRIBUTE_MEMORY | \
					 EFI_PCI_IO_ATTRIBUTE_BUS_MASTER)

static void fill_rb_attributes(struct device *dev, pci_root_bridge_t *rb)
{
	rb->dma_above4g = true;
	/* Assume all attributes are supported */
	rb->supports = ROOT_BRIDGE_SUPPORTS_DEFAULT;
	/* Actual attributes will be filled by the EDK2 PCI enumerator */
	rb->attributes = 0;
	rb->allocation_attributes = EFI_PCI_HOST_BRIDGE_COMBINE_MEM_PMEM |
				    EFI_PCI_HOST_BRIDGE_MEM64_DECODE;

	/* Only domain 0 should decode ISA I/O */
	if (!is_domain0(dev)) {
		rb->supports &= ~(EFI_PCI_IO_ATTRIBUTE_ISA_MOTHERBOARD_IO |
				  EFI_PCI_IO_ATTRIBUTE_ISA_IO |
				  EFI_PCI_IO_ATTRIBUTE_ISA_IO_16);
	}
}

static void fill_rb_io_aperture(struct device *dev, pci_root_bridge_t *rb)
{
	const unsigned long type = IORESOURCE_IO | IORESOURCE_ASSIGNED;
	struct resource *res;

	for (res = dev->resource_list; res != NULL; res = res->next) {
		if (res->flags != type)
			continue;
		if (res->limit <= res->base)
			continue;

		if (rb->io.base == 0 && rb->io.limit == 0) {
			rb->io.base = res->base;
			rb->io.limit = res->limit;
		} else {
			/* Ignore non contiguous ranges */
			if ((rb->io.limit + 1) != res->base)
				continue;

			/* If space is contiguous, update I/O limit */
			rb->io.limit = res->limit;
		}
	}

	/* If no aperture found, set base higher than limit */
	if (rb->io.base == 0 && rb->io.limit == 0)
		rb->io.base = 0xffff;

	printk(BIOS_DEBUG, "%s: I/O aperture: [%llx - %llx]\n",
	       dev_path(dev), rb->io.base, rb->io.limit);
}

static void fill_rb_mmio_aperture(struct device *dev, pci_root_bridge_t *rb)
{
	const unsigned long type = IORESOURCE_MEM | IORESOURCE_ASSIGNED;
	struct resource *res;

	for (res = dev->resource_list; res != NULL; res = res->next) {
		if (res->flags != type)
			continue;
		if (res->limit <= res->base)
			continue;
		/*
		 * Make sure to not report a region overlapping with the fixed MMIO resources
		 * below 4GB or the reserved MMIO range in the last 12GB of the addressable
		 * address range. The code assumes that the fixed MMIO resources below 4GB
		 * are between IO_APIC_ADDR and the 4GB boundary.
		 */
		if (res->base < 4ULL * GiB) {
			if (res->base >= IO_APIC_ADDR)
				continue;
			if (res->limit >= IO_APIC_ADDR)
				res->limit = IO_APIC_ADDR - 1;

			if (rb->mem.base == 0 && rb->mem.limit == 0) {
				rb->mem.base = res->base;
				rb->mem.limit = res->limit;
			} else {
				/* Ignore non contiguous ranges */
				if ((rb->mem.limit + 1) != res->base)
					continue;

				/* If space is contiguous, update MMIO limit */
				rb->mem.limit = res->limit;
			}
		} else {
			if (rb->mem_above4g.base == 0 && rb->mem_above4g.limit == 0) {
				rb->mem_above4g.base = res->base;
				rb->mem_above4g.limit = res->limit;
			} else {
				/* Ignore non contiguous ranges */
				if ((rb->mem_above4g.limit + 1) != res->base)
					continue;

				/* If space is contiguous, update I/O limit */
				rb->mem_above4g.limit = res->limit;
			}
		}
	}

	/* If no aperture found, set base higher than limit */
	if (rb->mem.base == 0 && rb->mem.limit == 0)
		rb->mem.base = UINT32_MAX;
	if (rb->mem_above4g.base == 0 && rb->mem_above4g.limit == 0)
		rb->mem_above4g.base = UINT64_MAX;
	else
		rb->dma_above4g = true;

	/* Make prefetchable memory invalid. We put everything into memory aperture. */
	rb->pmem.base = UINT32_MAX;
	rb->pmem_above4g.base = UINT64_MAX;

	printk(BIOS_DEBUG, "%s: MEM aperture: [%llx - %llx]\n",
	       dev_path(dev), rb->mem.base, rb->mem.limit);
	printk(BIOS_DEBUG, "%s: MEM above 4G aperture: [%llx - %llx]\n",
	       dev_path(dev), rb->mem_above4g.base, rb->mem_above4g.limit);
	printk(BIOS_DEBUG, "%s: PMEM aperture: [%llx - %llx]\n",
	       dev_path(dev), rb->pmem.base, rb->pmem.limit);
	printk(BIOS_DEBUG, "%s: PMEM above 4G aperture: [%llx - %llx]\n",
	       dev_path(dev), rb->pmem_above4g.base, rb->pmem_above4g.limit);
}

static void fill_rb_pci_bus_aperture(struct device *dev, pci_root_bridge_t *rb)
{
	rb->segment = dev->downstream->segment_group;
	rb->bus.base = dev->downstream->secondary;
	rb->bus.limit = dev->downstream->max_subordinate;
}

static void fill_rb_apertures(struct device *dev, pci_root_bridge_t *rb)
{
	/* Get apertures from DF registers */
	fill_rb_io_aperture(dev, rb);
	fill_rb_mmio_aperture(dev, rb);
	fill_rb_pci_bus_aperture(dev, rb);
}

static void phoenix_domain_init(struct device *dev)
{
	pci_root_bridges_info_t *rb_info;
	pci_root_bridge_t *rb;
	uint16_t rb_info_size;

	amd_pci_domain_init(dev);

	if (!CONFIG(PAYLOAD_EDK2))
		return;

	rb_info = cbmem_find(CBMEM_ID_RB_INFO);
	if (!rb_info) {
		/* Allocate RB info for 1 domain . */
		rb_info_size = sizeof(pci_root_bridges_info_t);
		rb_info_size += sizeof(pci_root_bridge_t);
		rb_info = cbmem_add(CBMEM_ID_RB_INFO, rb_info_size);
		if (!rb_info)
			return;

		memset(rb_info, 0, rb_info_size);
		rb_info->header.revision = 1;
		rb_info->header.length = sizeof(pci_root_bridges_info_t);
		rb_info->resource_assigned = true;
	}

	rb = &rb_info->root_bridge[0];

	fill_rb_attributes(dev, rb);
	fill_rb_apertures(dev, rb);

	rb->hid = EISA_PNP_ID(0x0A03);
	rb->uid = 0;

	rb_info->count++;
	rb_info->header.length += sizeof(pci_root_bridge_t);
}

static const char *soc_acpi_name(const struct device *dev)
{
	if (dev->path.type == DEVICE_PATH_DOMAIN)
		return "PCI0";

	if (dev->path.type != DEVICE_PATH_PCI)
		return NULL;

	printk(BIOS_WARNING, "Unknown PCI device: dev: %d, fn: %d\n",
	       PCI_SLOT(dev->path.pci.devfn), PCI_FUNC(dev->path.pci.devfn));
	return NULL;
};

struct device_operations phoenix_pci_domain_ops = {
	.read_resources	= amd_pci_domain_read_resources,
	.set_resources	= pci_domain_set_resources,
	.scan_bus	= amd_pci_domain_scan_bus,
	.init		= phoenix_domain_init,
	.acpi_name	= soc_acpi_name,
	.acpi_fill_ssdt	= pci_domain_fill_ssdt,
};

static void soc_init(void *chip_info)
{
	default_dev_ops_root.write_acpi_tables = soc_acpi_write_tables;

	if (CONFIG(PLATFORM_USES_FSP2_0)) {
		amd_fsp_silicon_init();
	} else {
		amd_opensil_silicon_init();
	}

	data_fabric_print_mmio_conf();

	fch_init(chip_info);
}

static void soc_final(void *chip_info)
{
	fch_final(chip_info);
}

struct chip_operations soc_amd_phoenix_ops = {
	.name = "AMD Phoenix SoC",
	.init = soc_init,
	.final = soc_final
};
