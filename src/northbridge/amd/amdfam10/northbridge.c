/* SPDX-License-Identifier: GPL-2.0-only */

#include <console/console.h>
#include <cpu/amd/mtrr.h>
#include <cpu/amd/family_10h-family_15h/amdfam10_sysconf.h>
#include <cpu/amd/family_10h-family_15h/ram_calc.h>
#include <device/device.h>
#include <device/pci.h>

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

static void amdfam10_nb_init(void *chip_info)
{
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

static struct device_operations pci_domain_ops = {
	.read_resources	  = pci_domain_read_resources,
	.set_resources	  = pci_domain_set_resources,
	.scan_bus	  = pci_domain_scan_bus,
#if CONFIG(HAVE_ACPI_TABLES)
	.acpi_name	  = amdfam10_domain_acpi_name,
#endif
#if CONFIG(GENERATE_SMBIOS_TABLES)
	.get_smbios_data  = amdfam10_get_smbios_data,
#endif
};

static void cpu_bus_scan(struct device *dev)
{

}

static void cpu_bus_init(struct device *dev)
{

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

static void root_complex_finalize(void *chip_info)
{

}

struct chip_operations northbridge_amd_amdfam10_root_complex_ops = {
	CHIP_NAME("AMD Family 10h/15h Root Complex")
	.enable_dev = root_complex_enable_dev,
	.final = root_complex_finalize,
};
