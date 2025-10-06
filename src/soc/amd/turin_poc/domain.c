/* SPDX-License-Identifier: GPL-2.0-only */

#include <acpi/acpigen_pci.h>
#include <amdblocks/ioapic.h>
#include <amdblocks/data_fabric.h>
#include <amdblocks/memmap.h>
#include <amdblocks/root_complex.h>
#include <amdblocks/smn.h>
#include <arch/ioapic.h>
#include <cbmem.h>
#include <console/console.h>
#include <cpu/amd/mtrr.h>
#include <cpu/cpu.h>
#include <device/device.h>
#include <drivers/amd/opensil/opensil.h>
#include <root_bridge_info.h>
#include <types.h>

/* EDK2 headers to construct proper RB attributes */
#include <Uefi/UefiBaseType.h>
#include <Uefi/UefiSpec.h>
#include <Protocol/DevicePath.h>
#include <Protocol/PciIo.h>

#define IOHC_IOAPIC_BASE_ADDR_LO 0x2f0

void read_soc_memmap_resources(struct device *domain, unsigned long *idx)
{
	read_lower_soc_memmap_resources(domain, idx);

	amd_opensil_add_memmap(domain, idx);
}

static void turin_domain_set_resources(struct device *domain)
{
	if (domain->downstream->bridge_ctrl & PCI_BRIDGE_CTL_VGA) {
		printk(BIOS_DEBUG, "Setting VGA decoding for domain 0x%x\n",
		       dev_get_domain_id(domain));
		const union df_vga_en vga_en = {
			.ve = 1,
			.dst_fabric_id = get_iohc_fabric_id(domain),
		};
		data_fabric_broadcast_write32(DF_VGA_EN, vga_en.raw);
	}

	pci_domain_set_resources(domain);

	/* Enable IOAPIC memory decoding */
	struct resource *res = probe_resource(domain, IOMMU_IOAPIC_IDX);
	if (res) {
		const uint32_t iohc_misc_base = get_iohc_misc_smn_base(domain);
		uint32_t ioapic_base = smn_read32(iohc_misc_base | IOHC_IOAPIC_BASE_ADDR_LO);
		ioapic_base |= (1 << 0);
		smn_write32(iohc_misc_base | IOHC_IOAPIC_BASE_ADDR_LO, ioapic_base);
	}
}

static const char *turin_domain_acpi_name(const struct device *domain)
{
	const unsigned int domain_id = dev_get_domain_id(domain);
	const char *domain_acpi_names[8] = {
		"S0B0",
		"S0B1",
		"S0B2",
		"S0B3",
		"S0B4",
		"S0B5",
		"S0B6",
		"S0B7",
	};

	if (domain_id < ARRAY_SIZE(domain_acpi_names))
		return domain_acpi_names[domain_id];

	return NULL;
}

static void fill_rb_attributes(struct device *dev, pci_root_bridge_t *rb)
{
	unsigned int domain_id = dev_get_domain_id(dev);

	/* Assume all attributes are supported */
	rb->supports = EFI_PCI_IO_ATTRIBUTE_MASK;

	/* Domain 0 should always decode ISA I/O */
	if (domain_id == 0) {
		rb->attributes |= (EFI_PCI_IO_ATTRIBUTE_ISA_MOTHERBOARD_IO |
				   EFI_PCI_IO_ATTRIBUTE_ISA_IO |
				   EFI_PCI_IO_ATTRIBUTE_ISA_IO_16);
	}

	if (dev->downstream->bridge_cmd & PCI_COMMAND_IO)
		rb->attributes |= EFI_PCI_IO_ATTRIBUTE_IO;

	if (dev->downstream->bridge_cmd & PCI_COMMAND_MEMORY)
		rb->attributes |= EFI_PCI_IO_ATTRIBUTE_MEMORY;

	if (dev->downstream->bridge_cmd & PCI_COMMAND_MASTER)
		rb->attributes |= EFI_PCI_IO_ATTRIBUTE_BUS_MASTER;

	if (dev->downstream->bridge_ctrl & PCI_BRIDGE_CTL_VGA) {
		rb->attributes |= (EFI_PCI_IO_ATTRIBUTE_VGA_MEMORY |
				   EFI_PCI_IO_ATTRIBUTE_VGA_IO);
		if (dev->downstream->bridge_cmd & PCI_COMMAND_VGA_PALETTE)
			rb->attributes |= EFI_PCI_IO_ATTRIBUTE_VGA_PALETTE_IO;
	}

	if (!dev->downstream->no_vga16) {
		if (dev->downstream->bridge_ctrl & PCI_BRIDGE_CTL_VGA16) {
			rb->attributes |= EFI_PCI_IO_ATTRIBUTE_VGA_IO_16;
			if (dev->downstream->bridge_cmd & PCI_COMMAND_VGA_PALETTE)
				rb->attributes |= EFI_PCI_IO_ATTRIBUTE_VGA_PALETTE_IO_16;
		}
	}

	printk(BIOS_DEBUG, "%s: attributes %llx\n", dev_path(dev), rb->attributes);
}

static void fill_rb_io_aperture(struct device *dev, pci_root_bridge_t *rb)
{
	const signed int iohc_dest_fabric_id = get_iohc_fabric_id(dev);
	union df_io_base base_reg;
	union df_io_limit limit_reg;
	resource_t io_base;
	resource_t io_limit;

	for (unsigned int i = 0; i < DF_IO_REG_COUNT; i++) {
		base_reg.raw = data_fabric_broadcast_read32(DF_IO_BASE(i));

		/* Relevant IO regions need to have both reads and writes enabled */
		if (!base_reg.we || !base_reg.re)
			continue;

		limit_reg.raw = data_fabric_broadcast_read32(DF_IO_LIMIT(i));

		/* Only look at IO regions that are decoded to the right PCI root */
		if (limit_reg.dst_fabric_id != iohc_dest_fabric_id)
			continue;

		io_base = base_reg.io_base << DF_IO_ADDR_SHIFT;
		io_limit = ((limit_reg.io_limit + 1) << DF_IO_ADDR_SHIFT) - 1;

		/* Beware that the lower 25 bits of io_base and io_limit can be non-zero
		   despite there only being 16 bits worth of IO port address space. */
		if (io_base > 0xffff) {
			printk(BIOS_WARNING, "DF IO base register %d value outside of valid "
					     "IO port address range.\n", i);
			continue;
		}
		/* If only the IO limit is outside of the valid 16 bit IO port range, report
		   the limit as 0xffff, so that the resource allcator won't put IO BARs outside
		   of the 16 bit IO port address range. */
		io_limit = MIN(io_limit, 0xffff);

		if (rb->io.base == 0 && rb->io.limit == 0) {
			rb->io.base = io_base;
			rb->io.limit = io_limit;
		} else {
			/* Ignore non contiguous ranges */
			if ((rb->io.limit + 1) != io_base)
				continue;

			/* If space is contiguous, update I/O limit */
			rb->io.limit = io_limit;
		}
	}

	/* If no aperture found, set base higher than limit */
	if (rb->io.base == 0 && rb->io.limit == 0)
		rb->io.base = 0xffff;

	printk(BIOS_DEBUG, "%s: I/O aperture: [%llx - %llx]\n", dev_path(dev), rb->io.base, rb->io.limit);
}


static bool is_mmio_region_valid(unsigned int reg, resource_t mmio_base, resource_t mmio_limit)
{
	if (mmio_base > mmio_limit)
		return false;

	if (mmio_base >= 4ULL * GiB) {
		/* MMIO region above 4GB needs to be above TOP_MEM2 MSR value */
		if (mmio_base < get_top_of_mem_above_4gb())
			return false;
	} else {
		/* MMIO region below 4GB needs to be above TOP_MEM MSR value */
		if (mmio_base < get_top_of_mem_below_4gb())
			return false;
		/* MMIO region below 4GB mustn't cross the 4GB boundary. */
		if (mmio_limit >= 4ULL * GiB)
			return false;
	}

	return true;
}

static void fill_rb_mmio_aperture(struct device *dev, pci_root_bridge_t *rb)
{
	const signed int iohc_dest_fabric_id = get_iohc_fabric_id(dev);
	union df_mmio_control ctrl;
	resource_t mmio_base;
	resource_t mmio_limit;

	/* The last 12GB of the usable address space are reserved and can't be used for MMIO */
	const resource_t reserved_upper_mmio_base =
		(1ULL << cpu_phys_address_size()) - DF_RESERVED_TOP_12GB_MMIO_SIZE;

	for (unsigned int i = 0; i < DF_MMIO_REG_SET_COUNT; i++) {
		ctrl.raw = data_fabric_broadcast_read32(DF_MMIO_CONTROL(i));

		/* Relevant MMIO regions need to have both reads and writes enabled */
		if (!ctrl.we || !ctrl.re)
			continue;

		/* Non-posted region contains fixed FCH MMIO devices */
		if (ctrl.np)
			continue;

		/* Only look at MMIO regions that are decoded to the right PCI root */
		if (ctrl.dst_fabric_id != iohc_dest_fabric_id)
			continue;

		data_fabric_get_mmio_base_size(i, &mmio_base, &mmio_limit);

		if (!is_mmio_region_valid(i, mmio_base, mmio_limit))
			continue;

		/* Make sure to not report a region overlapping with the fixed MMIO resources
		   below 4GB or the reserved MMIO range in the last 12GB of the addressable
		   address range. The code assumes that the fixed MMIO resources below 4GB
		   are between IO_APIC_ADDR and the 4GB boundary. */
		if (mmio_base < 4ULL * GiB) {
			if (mmio_base >= IO_APIC_ADDR)
				continue;
			if (mmio_limit >= IO_APIC_ADDR)
				mmio_limit = IO_APIC_ADDR - 1;

			if (rb->mem.base == 0 && rb->mem.limit == 0) {
				rb->mem.base = mmio_base;
				rb->mem.limit = mmio_limit;
			} else {
				/* Ignore non contiguous ranges */
				if ((rb->mem.limit + 1) != mmio_base)
					continue;

				/* If space is contiguous, update MMIO limit */
				rb->mem.limit = mmio_limit;
			}
		} else {
			if (mmio_base >= reserved_upper_mmio_base)
				continue;
			if (mmio_limit >= reserved_upper_mmio_base)
				mmio_limit = reserved_upper_mmio_base - 1;

			if (rb->mem_above4g.base == 0 && rb->mem_above4g.limit == 0) {
				rb->mem_above4g.base = mmio_base;
				rb->mem_above4g.limit = mmio_limit;
			} else {
				/* Ignore non contiguous ranges */
				if ((rb->mem_above4g.limit + 1) != mmio_base)
					continue;

				/* If space is contiguous, update I/O limit */
				rb->mem_above4g.limit = mmio_limit;
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

static void turin_domain_init(struct device *dev)
{
	pci_root_bridges_info_t *rb_info;
	pci_root_bridge_t *rb;
	uint16_t rb_info_size;
	unsigned int domain_id = dev_get_domain_id(dev);

	amd_pci_domain_init(dev);

	if (!CONFIG(PAYLOAD_EDK2))
		return;

	rb_info = cbmem_find(CBMEM_ID_RB_INFO);
	if (!rb_info) {
		/* Allocate RB info for 8 domain . */
		rb_info_size = sizeof(pci_root_bridges_info_t);
		rb_info_size += 8 * sizeof(pci_root_bridge_t);
		rb_info = cbmem_add(CBMEM_ID_RB_INFO, rb_info_size);
		if (!rb_info)
			return;

		memset(rb_info, 0, rb_info_size);
		rb_info->header.revision = 1;
		rb_info->header.length = sizeof(pci_root_bridges_info_t);
		rb_info->resource_assigned = true;
	}

	rb = &rb_info->root_bridge[domain_id];

	fill_rb_attributes(dev, rb);
	fill_rb_apertures(dev, rb);

	rb->hid = EISA_PNP_ID (0x0A03);
	rb->uid = domain_id;

	rb_info->count++;
	rb_info->header.length += sizeof(pci_root_bridge_t);
}

struct device_operations turin_pci_domain_ops = {
	.read_resources	= amd_pci_domain_read_resources,
	.set_resources	= turin_domain_set_resources,
	.scan_bus	= amd_pci_domain_scan_bus,
	.init		= turin_domain_init,
	.acpi_name	= turin_domain_acpi_name,
	.acpi_fill_ssdt	= pci_domain_fill_ssdt,
};
