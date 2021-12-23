/* SPDX-License-Identifier: GPL-2.0-only */

#include <console/console.h>
#include <string.h>
#include <acpi/acpi.h>
#include <acpi/acpigen.h>
#include <device/pci.h>
#include <cpu/x86/msr.h>
#include <cpu/amd/mtrr.h>
#include <cpu/amd/family_10h-family_15h/amdfam10_sysconf.h>
#include "amdfam10.h"

//it seems some functions can be moved arch/x86/boot/acpi.c

unsigned long acpi_create_madt_lapic_nmis(unsigned long current, u16 flags, u8 lint)
{
	struct device *cpu;
	int cpu_index = 0;

	for (cpu = all_devices; cpu; cpu = cpu->next) {
		if ((cpu->path.type != DEVICE_PATH_APIC) ||
		   (cpu->bus->dev->path.type != DEVICE_PATH_CPU_CLUSTER)) {
			continue;
		}
		if (!cpu->enabled) {
			continue;
		}
		current += acpi_create_madt_lapic_nmi((acpi_madt_lapic_nmi_t *)current, cpu_index, flags, lint);
		cpu_index++;
	}
	return current;
}

unsigned long acpi_create_srat_lapics(unsigned long current)
{
	struct device *cpu;
	int cpu_index = 0;

	for (cpu = all_devices; cpu; cpu = cpu->next) {
		if ((cpu->path.type != DEVICE_PATH_APIC) ||
		   (cpu->bus->dev->path.type != DEVICE_PATH_CPU_CLUSTER)) {
			continue;
		}
		if (!cpu->enabled) {
			continue;
		}
		printk(BIOS_DEBUG, "SRAT: lapic cpu_index=%02x, node_id=%02x, apic_id=%02x\n",
		       cpu_index, cpu->path.apic.node_id, cpu->path.apic.apic_id);
		current += acpi_create_srat_lapic((acpi_srat_lapic_t *)current,
						  cpu->path.apic.node_id,
						  cpu->path.apic.apic_id);
		cpu_index++;
	}
	return current;
}

static unsigned long resk(uint64_t value)
{
	unsigned long resultk;
	if (value < (1ULL << 42)) {
		resultk = value >> 10;
	} else {
		resultk = 0xffffffff;
	}
	return resultk;
}

struct acpi_srat_mem_state {
	unsigned long current;
};

static void set_srat_mem(void *gp, struct device *dev, struct resource *res)
{
	struct acpi_srat_mem_state *state = gp;
	unsigned long basek, sizek;
	basek = resk(res->base);
	sizek = resk(res->size);

	printk(BIOS_DEBUG, "set_srat_mem: dev %s, res->index=%04lx "
	       "startk=%08lx, sizek=%08lx\n",
	       dev_path(dev), res->index, basek, sizek);

	if (res->index < 0xf)	/* Exclude MMIO resources */
		state->current +=
		    acpi_create_srat_mem((acpi_srat_mem_t *)state->current,
		                         0 /* TODO: add support for more compute units */,
		                         basek, sizek, 1);
}

static unsigned long acpi_fill_srat(unsigned long current)
{
	struct acpi_srat_mem_state srat_mem_state;

	/* create all subtables for processors */
	current = acpi_create_srat_lapics(current);

	/* create all subteble for memory range */

	/* 0-640K must be on node 0 - is this always true for current code? */

	srat_mem_state.current = current;
	search_global_resources(
		IORESOURCE_MEM | IORESOURCE_CACHEABLE | IORESOURCE_RESERVE,
		IORESOURCE_MEM | IORESOURCE_CACHEABLE,
		set_srat_mem, &srat_mem_state);

	current = srat_mem_state.current;
	return current;
}

static unsigned long acpi_fill_slit(unsigned long current)
{
	/* Implement SLIT algorithm in BKDG Rev. 3.62 Section 2.3.6.1
	 * Fill the first 8 bytes with the node number,
	 * then fill the next num*num byte with the distance,
	 * Distance entries vary with topology; the local node
	 * is always 10.
	 *
	 * Fully connected:
	 * Set all non-local nodes to 16
	 *
	 * Partially connected; with probe filter:
	 * Set all non-local nodes to 10+(num_hops*6)
	 *
	 * Partially connected; without probe filter:
	 * Set all non-local nodes to 13
	 *
	 * FIXME
	 * The partially connected cases are not implemented;
	 * once a means is found to detect partially connected
	 * topologies, implement the remaining cases.
	 */

	u8 *p = (u8 *)current;
	int nodes = get_sysconf()->nodes;
	int i,j;

	memset(p, 0, 8+nodes*nodes);
	*p = (u8) nodes;
	p += 8;

	for (i = 0; i < nodes; i++) {
		for (j = 0; j < nodes; j++) {
			if (i == j)
				p[i*nodes+j] = 10;
			else
				p[i*nodes+j] = 16;
		}
	}

	current += 8+nodes*nodes;
	return current;
}

void update_ssdtx(void *ssdtx, int i)
{
	u8 *PCI;
	u8 *HCIN;
	u8 *UID;

	PCI = ssdtx + 0x32;
	HCIN = ssdtx + 0x39;
	UID = ssdtx + 0x40;

	if (i < 7) {
		*PCI = (u8) ('4' + i - 1);
	} else {
		*PCI = (u8) ('A' + i - 1 - 6);
	}
	*HCIN = (u8) i;
	*UID = (u8) (i + 3);

	/* FIXME: need to update the GSI id in the ssdtx too */

}

void northbridge_acpi_write_vars(const struct device *device)
{
	/*
	 * If more than one physical CPU is installed, northbridge_acpi_write_vars()
	 * is called more than once and the resultant SSDT table is corrupted
	 * (duplicated entries).
	 * This prevents Linux from booting, with log messages like these:
	 * ACPI Error: [BUSN] Namespace lookup failure, AE_ALREADY_EXISTS (/dswload-353)
	 * ACPI Exception: AE_ALREADY_EXISTS, During name lookup/catalog (/psobject-222)
	 * followed by a slew of ACPI method failures and a hang when the invalid PCI
	 * resource entries are used.
	 * This routine prevents the SSDT table from being corrupted.
	 */
	static uint8_t ssdt_generated = 0;
	if (ssdt_generated)
		return;
	ssdt_generated = 1;

	char pscope[] = "\\_SB.PCI0";
	int i;

	struct device *f1_dev = NULL;

	acpigen_write_scope(pscope);

	acpigen_write_name("RSRC");
	acpigen_write_resourcetemplate_header();
	/* IO access to PCI configuration space */
	acpigen_write_io16(0xCF8, 0xCFF, 0x1, 0x8, 1);
	/* MMIO */
	for (i = 0x80; i <= 0xB8; i += 8) {
		struct resource *res = probe_resource(device, i);
		if (res)
			acpigen_resource_qword(0, 0xe, 1 /* R/W */, 0,
			                       res->base, res->limit, 0, res->size);
	}
	/* IO */
	for (i = 0xC0; i <= 0xD8; i += 8) {
		struct resource *res = probe_resource(device, i);
		if (res)
			acpigen_resource_word(1, 0xe, 3 /* ISA and non-ISA */, 0,
			                      res->base, res->limit, 0, res->size);
	}
	/* Buses */
	f1_dev = pcidev_path_behind(device->bus, device->path.pci.devfn + 1);
	for (i = 0xE0; i <= 0xEC; i += 4) {
		/* Shouldn't bus numbers be a resource type? */
		u32 dword = pci_read_config32(f1_dev, i);
		if (dword & 3) { // writes or reads enabled
			u8 bus_base = (dword & 0x00ff0000) >> 16;
			u8 bus_limit = (dword & 0xff000000) >> 24;
			acpigen_resource_word(2, 0xe, 0, 0, bus_base, bus_limit, 0,
			                      bus_limit - bus_base + 1);
		}
	}
	acpigen_write_resourcetemplate_footer();

	acpigen_write_scope_end();
}

unsigned long northbridge_write_acpi_tables(const struct device *device,
					    unsigned long current,
					    struct acpi_rsdp *rsdp)
{
	acpi_srat_t *srat;
	acpi_slit_t *slit;

	/* One set only, please */
	static int once;
	if (once)
		return current;
	once++;

	/* SRAT */
	current = ALIGN(current, 8);
	printk(BIOS_DEBUG, "ACPI:    * SRAT at %lx\n", current);
	srat = (acpi_srat_t *) current;
	acpi_create_srat(srat, acpi_fill_srat);
	current += srat->header.length;
	acpi_add_table(rsdp, srat);

	/* SLIT */
	current = ALIGN(current, 8);
	printk(BIOS_DEBUG, "ACPI:   * SLIT at %lx\n", current);
	slit = (acpi_slit_t *) current;
	acpi_create_slit(slit, acpi_fill_slit);
	current += slit->header.length;
	acpi_add_table(rsdp, slit);

	return current;
}
