/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <acpi/acpigen.h>
#include <arch/ioapic.h>
#include <arch/smp/mpspec.h>
#include <bootstate.h>
#include <cbmem.h>
#include <cf9_reset.h>
#include <acpi/acpi_gnvs.h>
#include <console/console.h>
#include <cpu/intel/turbo.h>
#include <cpu/x86/smm.h>
#include <intelblocks/acpi.h>
#include <intelblocks/msr.h>
#include <intelblocks/pmclib.h>
#include <intelblocks/uart.h>
#include <soc/gpio.h>
#include <soc/iomap.h>
#include <soc/nvs.h>
#include <soc/pm.h>
#include <string.h>

__attribute__((weak)) unsigned long acpi_fill_mcfg(unsigned long current)
{
	/* PCI Segment Group 0, Start Bus Number 0, End Bus Number is 255 */
	current += acpi_create_mcfg_mmconfig((void *)current,
					     CONFIG_MMCONF_BASE_ADDRESS, 0, 0,
					     (CONFIG_SA_PCIEX_LENGTH >> 20) - 1);
	return current;
}

static int acpi_sci_irq(void)
{
	int sci_irq = 9;
	uint32_t scis;

	scis = soc_read_sci_irq_select();
	scis &= SCI_IRQ_SEL;
	scis >>= SCI_IRQ_ADJUST;

	/* Determine how SCI is routed. */
	switch (scis) {
	case SCIS_IRQ9:
	case SCIS_IRQ10:
	case SCIS_IRQ11:
		sci_irq = scis - SCIS_IRQ9 + 9;
		break;
	case SCIS_IRQ20:
	case SCIS_IRQ21:
	case SCIS_IRQ22:
	case SCIS_IRQ23:
		sci_irq = scis - SCIS_IRQ20 + 20;
		break;
	default:
		printk(BIOS_DEBUG, "Invalid SCI route! Defaulting to IRQ9.\n");
		sci_irq = 9;
		break;
	}

	printk(BIOS_DEBUG, "SCI is IRQ%d\n", sci_irq);
	return sci_irq;
}

static unsigned long acpi_madt_irq_overrides(unsigned long current)
{
	int sci = acpi_sci_irq();
	uint16_t flags = MP_IRQ_TRIGGER_LEVEL;

	/* INT_SRC_OVR */
	current += acpi_create_madt_irqoverride((void *)current, 0, 0, 2, 0);

	flags |= soc_madt_sci_irq_polarity(sci);

	/* SCI */
	current +=
	    acpi_create_madt_irqoverride((void *)current, 0, sci, sci, flags);

	return current;
}

unsigned long acpi_fill_madt(unsigned long current)
{
	/* Local APICs */
	current = acpi_create_madt_lapics(current);

	/* IOAPIC */
	current += acpi_create_madt_ioapic((void *)current, 2, IO_APIC_ADDR, 0);

	return acpi_madt_irq_overrides(current);
}

void acpi_fill_fadt(acpi_fadt_t *fadt)
{
	const uint16_t pmbase = ACPI_BASE_ADDRESS;

	fadt->header.revision = get_acpi_table_revision(FADT);

	fadt->sci_int = acpi_sci_irq();

	if (permanent_smi_handler()) {
		fadt->smi_cmd = APM_CNT;
		fadt->acpi_enable = APM_CNT_ACPI_ENABLE;
		fadt->acpi_disable = APM_CNT_ACPI_DISABLE;
	}

	fadt->pm1a_evt_blk = pmbase + PM1_STS;
	fadt->pm1a_cnt_blk = pmbase + PM1_CNT;

	fadt->gpe0_blk = pmbase + GPE0_STS(0);

	fadt->pm1_evt_len = 4;
	fadt->pm1_cnt_len = 2;

	/* GPE0 STS/EN pairs each 32 bits wide. */
	fadt->gpe0_blk_len = 2 * GPE0_REG_MAX * sizeof(uint32_t);

	fadt->duty_offset = 1;
	fadt->day_alrm = 0xd;

	fadt->flags |= ACPI_FADT_WBINVD | ACPI_FADT_C1_SUPPORTED |
			ACPI_FADT_C2_MP_SUPPORTED | ACPI_FADT_SLEEP_BUTTON |
			ACPI_FADT_SEALED_CASE | ACPI_FADT_S4_RTC_WAKE |
			ACPI_FADT_PLATFORM_CLOCK;

	fadt->x_pm1a_evt_blk.space_id = ACPI_ADDRESS_SPACE_IO;
	fadt->x_pm1a_evt_blk.bit_width = fadt->pm1_evt_len * 8;
	fadt->x_pm1a_evt_blk.addrl = pmbase + PM1_STS;
	fadt->x_pm1a_evt_blk.access_size = ACPI_ACCESS_SIZE_WORD_ACCESS;

	fadt->x_pm1a_cnt_blk.space_id = ACPI_ADDRESS_SPACE_IO;
	fadt->x_pm1a_cnt_blk.bit_width = fadt->pm1_cnt_len * 8;
	fadt->x_pm1a_cnt_blk.addrl = pmbase + PM1_CNT;
	fadt->x_pm1a_cnt_blk.access_size = ACPI_ACCESS_SIZE_WORD_ACCESS;

	/*
	 * Windows 10 requires x_gpe0_blk to be set starting with FADT revision 5.
	 * The bit_width field intentionally overflows here.
	 * The OSPM can instead use the values in `fadt->gpe0_blk{,_len}`, which
	 * seems to work fine on Linux 5.0 and Windows 10.
	 */
	fadt->x_gpe0_blk.space_id = ACPI_ADDRESS_SPACE_IO;
	fadt->x_gpe0_blk.bit_width = fadt->gpe0_blk_len * 8;
	fadt->x_gpe0_blk.bit_offset = 0;
	fadt->x_gpe0_blk.access_size = ACPI_ACCESS_SIZE_BYTE_ACCESS;
	fadt->x_gpe0_blk.addrl = fadt->gpe0_blk;
	fadt->x_gpe0_blk.addrh = 0;
}

unsigned long southbridge_write_acpi_tables(const struct device *device,
					    unsigned long current,
					    struct acpi_rsdp *rsdp)
{
	current = acpi_write_dbg2_pci_uart(rsdp, current,
					   uart_get_device(),
					   ACPI_ACCESS_SIZE_DWORD_ACCESS);
	return acpi_write_hpet(device, current, rsdp);
}

__weak
uint32_t acpi_fill_soc_wake(uint32_t generic_pm1_en,
			    const struct chipset_power_state *ps)
{
	return generic_pm1_en;
}

#if CONFIG(SOC_INTEL_COMMON_ACPI_WAKE_SOURCE)
/*
 * Save wake source information for calculating ACPI _SWS values
 *
 * @pm1:  PM1_STS register with only enabled events set
 * @gpe0: GPE0_STS registers with only enabled events set
 *
 * return the number of registers in the gpe0 array or -1 if nothing
 * is provided by this function.
 */

static int acpi_fill_wake(uint32_t *pm1, uint32_t **gpe0)
{
	struct chipset_power_state *ps;
	static uint32_t gpe0_sts[GPE0_REG_MAX];
	uint32_t pm1_en;
	int i;

	ps = cbmem_find(CBMEM_ID_POWER_STATE);
	if (ps == NULL)
		return -1;

	/*
	 * PM1_EN to check the basic wake events which can happen through
	 * powerbtn or any other wake source like lidopen, key board press etc.
	 */
	pm1_en = ps->pm1_en;

	pm1_en = acpi_fill_soc_wake(pm1_en, ps);

	*pm1 = ps->pm1_sts & pm1_en;

	/* Mask off GPE0 status bits that are not enabled */
	*gpe0 = &gpe0_sts[0];
	for (i = 0; i < GPE0_REG_MAX; i++)
		gpe0_sts[i] = ps->gpe0_sts[i] & ps->gpe0_en[i];

	return GPE0_REG_MAX;
}
#endif

__weak void acpi_create_gnvs(struct global_nvs *gnvs)
{
}

void southbridge_inject_dsdt(const struct device *device)
{
	struct global_nvs *gnvs;

	gnvs = cbmem_find(CBMEM_ID_ACPI_GNVS);
	if (!gnvs) {
		gnvs = cbmem_add(CBMEM_ID_ACPI_GNVS, sizeof(*gnvs));
		if (gnvs)
			memset(gnvs, 0, sizeof(*gnvs));
	}

	if (gnvs) {
		acpi_create_gnvs(gnvs);
		/* And tell SMI about it */
		apm_control(APM_CNT_GNVS_UPDATE);

		/* Add it to DSDT.  */
		acpigen_write_scope("\\");
		acpigen_write_name_dword("NVSA", (uintptr_t) gnvs);
		acpigen_pop_len();
	}
}

static int calculate_power(int tdp, int p1_ratio, int ratio)
{
	u32 m;
	u32 power;

	/*
	 * M = ((1.1 - ((p1_ratio - ratio) * 0.00625)) / 1.1) ^ 2
	 *
	 * Power = (ratio / p1_ratio) * m * tdp
	 */

	m = (110000 - ((p1_ratio - ratio) * 625)) / 11;
	m = (m * m) / 1000;

	power = ((ratio * 100000 / p1_ratio) / 100);
	power *= (m / 100) * (tdp / 1000);
	power /= 1000;

	return power;
}

static int get_cores_per_package(void)
{
	struct cpuinfo_x86 c;
	struct cpuid_result result;
	int cores = 1;

	get_fms(&c, cpuid_eax(1));
	if (c.x86 != 6)
		return 1;

	result = cpuid_ext(0xb, 1);
	cores = result.ebx & 0xff;

	return cores;
}

static void generate_c_state_entries(void)
{
	acpi_cstate_t *c_state_map;
	size_t entries;

	c_state_map = soc_get_cstate_map(&entries);

	/* Generate C-state tables */
	acpigen_write_CST_package(c_state_map, entries);
}

void generate_p_state_entries(int core, int cores_per_package)
{
	int ratio_min, ratio_max, ratio_turbo, ratio_step;
	int coord_type, power_max, num_entries;
	int ratio, power, clock, clock_max;
	bool turbo;

	coord_type = cpu_get_coord_type();
	ratio_min = cpu_get_min_ratio();
	ratio_max = cpu_get_max_ratio();
	clock_max = (ratio_max * cpu_get_bus_clock()) / KHz;
	turbo = (get_turbo_state() == TURBO_ENABLED);

	/* Calculate CPU TDP in mW */
	power_max = cpu_get_power_max();

	/* Write _PCT indicating use of FFixedHW */
	acpigen_write_empty_PCT();

	/* Write _PPC with no limit on supported P-state */
	acpigen_write_PPC_NVS();
	/* Write PSD indicating configured coordination type */
	acpigen_write_PSD_package(core, 1, coord_type);

	/* Add P-state entries in _PSS table */
	acpigen_write_name("_PSS");

	/* Determine ratio points */
	ratio_step = PSS_RATIO_STEP;
	do {
		num_entries = ((ratio_max - ratio_min) / ratio_step) + 1;
		if (((ratio_max - ratio_min) % ratio_step) > 0)
			num_entries += 1;
		if (turbo)
			num_entries += 1;
		if (num_entries > PSS_MAX_ENTRIES)
			ratio_step += 1;
	} while (num_entries > PSS_MAX_ENTRIES);

	/* _PSS package count depends on Turbo */
	acpigen_write_package(num_entries);

	/* P[T] is Turbo state if enabled */
	if (turbo) {
		ratio_turbo = cpu_get_max_turbo_ratio();

		/* Add entry for Turbo ratio */
		acpigen_write_PSS_package(clock_max + 1,	/* MHz */
					  power_max,		/* mW */
					  PSS_LATENCY_TRANSITION,/* lat1 */
					  PSS_LATENCY_BUSMASTER,/* lat2 */
					  ratio_turbo << 8,	/* control */
					  ratio_turbo << 8);	/* status */
		num_entries -= 1;
	}

	/* First regular entry is max non-turbo ratio */
	acpigen_write_PSS_package(clock_max,		/* MHz */
				  power_max,		/* mW */
				  PSS_LATENCY_TRANSITION,/* lat1 */
				  PSS_LATENCY_BUSMASTER,/* lat2 */
				  ratio_max << 8,	/* control */
				  ratio_max << 8);	/* status */
	num_entries -= 1;

	/* Generate the remaining entries */
	for (ratio = ratio_min + ((num_entries - 1) * ratio_step);
	     ratio >= ratio_min; ratio -= ratio_step) {

		/* Calculate power at this ratio */
		power = calculate_power(power_max, ratio_max, ratio);
		clock = (ratio * cpu_get_bus_clock()) / KHz;

		acpigen_write_PSS_package(clock,		/* MHz */
					  power,		/* mW */
					  PSS_LATENCY_TRANSITION,/* lat1 */
					  PSS_LATENCY_BUSMASTER,/* lat2 */
					  ratio << 8,		/* control */
					  ratio << 8);		/* status */
	}
	/* Fix package length */
	acpigen_pop_len();
}

__attribute__ ((weak)) acpi_tstate_t *soc_get_tss_table(int *entries)
{
	*entries = 0;
	return NULL;
}

void generate_t_state_entries(int core, int cores_per_package)
{
	acpi_tstate_t *soc_tss_table;
	int entries;

	soc_tss_table = soc_get_tss_table(&entries);
	if (entries == 0)
		return;

	/* Indicate SW_ALL coordination for T-states */
	acpigen_write_TSD_package(core, cores_per_package, SW_ALL);

	/* Indicate FixedHW so OS will use MSR */
	acpigen_write_empty_PTC();

	/* Set NVS controlled T-state limit */
	acpigen_write_TPC("\\TLVL");

	/* Write TSS table for MSR access */
	acpigen_write_TSS_package(entries, soc_tss_table);
}

__weak void soc_power_states_generation(int core_id,
						int cores_per_package)
{
}

void generate_cpu_entries(const struct device *device)
{
	int core_id, cpu_id, pcontrol_blk = ACPI_BASE_ADDRESS;
	int plen = 6;
	int totalcores = dev_count_cpu();
	int cores_per_package = get_cores_per_package();
	int numcpus = totalcores / cores_per_package;

	printk(BIOS_DEBUG, "Found %d CPU(s) with %d core(s) each.\n",
	       numcpus, cores_per_package);

	for (cpu_id = 0; cpu_id < numcpus; cpu_id++) {
		for (core_id = 0; core_id < cores_per_package; core_id++) {
			if (core_id > 0) {
				pcontrol_blk = 0;
				plen = 0;
			}

			/* Generate processor \_SB.CPUx */
			acpigen_write_processor((cpu_id) * cores_per_package +
						core_id, pcontrol_blk, plen);

			/* Generate C-state tables */
			generate_c_state_entries();

			/* Soc specific power states generation */
			soc_power_states_generation(core_id, cores_per_package);

			acpigen_pop_len();
		}
	}
	/* PPKG is usually used for thermal management
	   of the first and only package. */
	acpigen_write_processor_package("PPKG", 0, cores_per_package);

	/* Add a method to notify processor nodes */
	acpigen_write_processor_cnot(cores_per_package);
}

#if CONFIG(SOC_INTEL_COMMON_ACPI_WAKE_SOURCE)
/* Save wake source data for ACPI _SWS methods in NVS */
static void acpi_save_wake_source(void *unused)
{
	struct global_nvs *gnvs = acpi_get_gnvs();
	uint32_t pm1, *gpe0;
	int gpe_reg, gpe_reg_count;
	int reg_size = sizeof(uint32_t) * 8;

	if (!gnvs)
		return;

	gnvs->pm1i = -1;
	gnvs->gpei = -1;

	gpe_reg_count = acpi_fill_wake(&pm1, &gpe0);
	if (gpe_reg_count < 0)
		return;

	/* Scan for first set bit in PM1 */
	for (gnvs->pm1i = 0; gnvs->pm1i < reg_size; gnvs->pm1i++) {
		if (pm1 & 1)
			break;
		pm1 >>= 1;
	}

	/* If unable to determine then return -1 */
	if (gnvs->pm1i >= 16)
		gnvs->pm1i = -1;

	/* Scan for first set bit in GPE registers */
	for (gpe_reg = 0; gpe_reg < gpe_reg_count; gpe_reg++) {
		uint32_t gpe = gpe0[gpe_reg];
		int start = gpe_reg * reg_size;
		int end = start + reg_size;

		if (gpe == 0) {
			if (!gnvs->gpei)
				gnvs->gpei = end;
			continue;
		}

		for (gnvs->gpei = start; gnvs->gpei < end; gnvs->gpei++) {
			if (gpe & 1)
				break;
			gpe >>= 1;
		}
	}

	/* If unable to determine then return -1 */
	if (gnvs->gpei >= gpe_reg_count * reg_size)
		gnvs->gpei = -1;

	printk(BIOS_DEBUG, "ACPI _SWS is PM1 Index %lld GPE Index %lld\n",
	       (long long)gnvs->pm1i, (long long)gnvs->gpei);
}

BOOT_STATE_INIT_ENTRY(BS_OS_RESUME, BS_ON_ENTRY, acpi_save_wake_source, NULL);

#endif
