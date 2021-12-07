/* SPDX-License-Identifier: GPL-2.0-only */

#include <acpi/acpi.h>
#include <arch/ioapic.h>
#include <console/console.h>
#include <cpu/amd/powernow.h>
#include <device/device.h>
#include <device/pci.h>
#include <device/pnp.h>
#include <device/pci_ids.h>
#include <device/pci_ops.h>
#include <pc80/mc146818rtc.h>
#include <pc80/i8254.h>
#include <pc80/i8259.h>
#include <pc80/isa-dma.h>

#include "sb700.h"

static void lpc_init(struct device *dev)
{
	u8 byte;
	u32 dword;
	struct device *sm_dev;

	printk(BIOS_SPEW, "%s\n", __func__);

	/* Enable the LPC Controller */
	sm_dev = pcidev_on_root(0x14, 0);
	dword = pci_read_config32(sm_dev, 0x64);
	dword |= 1 << 20;
	pci_write_config32(sm_dev, 0x64, dword);

	/* Initialize isa dma */
	if (CONFIG(SOUTHBRIDGE_AMD_SB700_SKIP_ISA_DMA_INIT))
		printk(BIOS_DEBUG, "Skipping isa_dma_init() to avoid getting stuck.\n");
	else
		isa_dma_init();


	if (!CONFIG(SOUTHBRIDGE_AMD_SB700_DISABLE_ISA_DMA)) {
		/* Enable DMA transaction on the LPC bus */
		byte = pci_read_config8(dev, 0x40);
		byte |= (1 << 2);
		pci_write_config8(dev, 0x40, byte);
	}

	/* Disable the timeout mechanism on LPC */
	byte = pci_read_config8(dev, 0x48);
	byte &= ~(1 << 7);
	pci_write_config8(dev, 0x48, byte);

	/* Disable LPC MSI Capability */
	byte = pci_read_config8(dev, 0x78);
	byte &= ~(1 << 1);
	if (CONFIG(SOUTHBRIDGE_AMD_SUBTYPE_SP5100)) {
		/* Disable FlowContrl, Always service the request from Host
		* whenever there is a request from Host pending
		*/
		byte &= ~(1 << 0);
	}
	pci_write_config8(dev, 0x78, byte);

	cmos_check_update_date();

	setup_i8259(); /* Initialize i8259 pic */
	setup_i8254(); /* Initialize i8254 timers */
}

static void sb700_lpc_read_resources(struct device *dev)
{
	struct resource *res;

	/* Get the normal pci resources of this device */
	pci_dev_read_resources(dev);	/* We got one for APIC, or one more for TRAP */

	/*
	 * Many IO ports in 0-0x1000 range are controlled by LPC ISA bridge.
	 * Instead of setting each one individually, add an extra subtractive
	 * resource for whole 0-0x1000 range.
	 *
	 * There are two ranges outside of 0-0x1000 controlled by PCI reg 0x4A:
	 * 0x4700-0x470B (bit 6) and 0xFD60-0xFD6F (bit 7). To keep allocation
	 * simpler, die() if any of those is enabled.
	 */
	if (pci_read_config8(dev, 0x4a) & ((1 << 6) | (1 << 7)))
		die("IO ports outside of 0-0x1000 range enabled in LPC ISA bridge\n");

	res = new_resource(dev, IOINDEX_SUBTRACTIVE(0, 0));
	res->base = 0;
	res->size = 0x1000;
	res->flags = IORESOURCE_IO | IORESOURCE_SUBTRACTIVE |
		     IORESOURCE_ASSIGNED | IORESOURCE_FIXED;

	/* Flash */
	res = new_resource(dev, IOINDEX_SUBTRACTIVE(1, 0));
	res->base = 4ull * GiB - CONFIG_ROM_SIZE;
	res->size = CONFIG_ROM_SIZE;
	res->flags = IORESOURCE_MEM | IORESOURCE_ASSIGNED | IORESOURCE_FIXED |
		     IORESOURCE_STORED | IORESOURCE_RESERVE;

	/* SPI BAR was set in bootblock_early_southbridge_init() */
	res = new_resource(dev, IOINDEX_SUBTRACTIVE(2, 0));
	res->base = (uintptr_t)SPI_BASE_ADDRESS;
	res->size = 0x1000;
	res->flags = IORESOURCE_MEM | IORESOURCE_ASSIGNED | IORESOURCE_FIXED |
		     IORESOURCE_STORED | IORESOURCE_RESERVE;

	compact_resources(dev);
}

/**
 * @brief Enable resources for children devices
 *
 * @param dev the device whose children's resources are to be enabled
 *
 */
static void sb700_lpc_enable_childrens_resources(struct device *dev)
{
	struct bus *link;
	u32 reg, reg_x;
	int var_num = 0;
	u16 reg_var[3] = {0x0, 0x0, 0x0};
	u8 wiosize = pci_read_config8(dev, 0x74);

	reg = pci_read_config32(dev, 0x44);
	reg_x = pci_read_config32(dev, 0x48);

	for (link = dev->link_list; link; link = link->next) {
		struct device *child;
		for (child = link->children; child;
		     child = child->sibling) {
			if (!(child->enabled
				&& (child->path.type == DEVICE_PATH_PNP)))
				continue;

			struct resource *res;
			for (res = child->resource_list; res; res = res->next) {
				u32 base, end;	/*  don't need long long */
				if (!(res->flags & IORESOURCE_IO))
					continue;
				base = res->base;
				end = resource_end(res);
				printk(BIOS_DEBUG, "sb700 lpc decode:%s,"
					" base=0x%08x, end=0x%08x\n",
					dev_path(child), base, end);
				if (end >= 0x1000)
					die("IO ports outside of 0-0x1000 range enabled in LPC ISA bridge\n");
				switch (base) {
				case 0x60:	/*  KB */
				case 0x64:	/*  MS */
					reg |= (1 << 29);
					break;
				case 0x3f8:	/*  COM1 */
					reg |= (1 << 6);
					break;
				case 0x2f8:	/*  COM2 */
					reg |= (1 << 7);
					break;
				case 0x378:	/*  Parallel 1 */
					reg |= (1 << 0);
					reg |= (1 << 1); /* + 0x778 for ECP */
					break;
				case 0x3f0:	/*  FD0 */
					reg |= (1 << 26);
					break;
				case 0x220:	/*  Audio 0 */
					reg |= (1 << 8);
					break;
				case 0x300:	/*  Midi 0 */
					reg |= (1 << 18);
					break;
				case 0x400:
					reg_x |= (1 << 16);
					break;
				case 0x480:
					reg_x |= (1 << 17);
					break;
				case 0x500:
					reg_x |= (1 << 18);
					break;
				case 0x580:
					reg_x |= (1 << 19);
					break;
				case 0x4700:
					/* unreachable */
					reg_x |= (1 << 22);
					break;
				case 0xfd60:
					/* unreachable */
					reg_x |= (1 << 23);
					break;
				default:
					/* only 3 var ; compact them ? */
					if (var_num >= 3)
						continue;
					switch (var_num) {
					case 0:
						reg_x |= (1 << 2);
						if ((end - base) < 16)
							wiosize |= (1 << 0);
						break;
					case 1:
						reg_x |= (1 << 24);
						if ((end - base) < 16)
							wiosize |= (1 << 2);
						break;
					case 2:
						reg_x |= (1 << 25);
						reg_x |= (1 << 24);
						if ((end - base) < 16)
							wiosize |= (1 << 3);
						break;
					}
					reg_var[var_num++] = base & 0xffff;
				}
			}
		}
	}
	pci_write_config32(dev, 0x44, reg);
	pci_write_config32(dev, 0x48, reg_x);
	/* Set WideIO for as many IOs found (fall through is on purpose) */
	switch (var_num) {
	case 3:
		pci_write_config16(dev, 0x90, reg_var[2]);
		/* fall through */
	case 2:
		pci_write_config16(dev, 0x66, reg_var[1]);
		/* fall through */
	case 1:
		pci_write_config16(dev, 0x64, reg_var[0]);
		break;
	}
	pci_write_config8(dev, 0x74, wiosize);
}

static void sb700_lpc_enable_resources(struct device *dev)
{
	pci_dev_enable_resources(dev);
	sb700_lpc_enable_childrens_resources(dev);
}

#if CONFIG(HAVE_ACPI_TABLES)

static void southbridge_acpi_fill_ssdt(const struct device *device)
{
	amd_generate_powernow(ACPI_CPU_CONTROL, 6, 1);
}

static const char *lpc_acpi_name(const struct device *dev)
{
	if (dev->path.type != DEVICE_PATH_PCI)
		return NULL;

	switch (dev->path.pci.devfn) {
		case PCI_DEVFN(0x14, 3):
			return "LPCB";
	}

	return NULL;
}
#endif


static struct pci_operations lops_pci = {
	.set_subsystem = pci_dev_set_subsystem,
};

static struct device_operations lpc_ops = {
	.read_resources = sb700_lpc_read_resources,
	.set_resources = pci_dev_set_resources,
	.enable_resources = sb700_lpc_enable_resources,
#if CONFIG(HAVE_ACPI_TABLES)
	.acpi_name = lpc_acpi_name,
	.write_acpi_tables = acpi_write_hpet,
	.acpi_fill_ssdt = southbridge_acpi_fill_ssdt,
#endif
	.init = lpc_init,
	.scan_bus = scan_static_bus,
	.ops_pci = &lops_pci,
};
static const struct pci_driver lpc_driver __pci_driver = {
	.ops = &lpc_ops,
	.vendor = PCI_VENDOR_ID_ATI,
	.device = PCI_DEVICE_ID_ATI_SB700_LPC,
};
