/* SPDX-License-Identifier: GPL-2.0-only */

/*
 * This file is created based on Intel Alder Lake Processor PCH Datasheet
 * Document number: 621483
 * Chapter number: 4
 */

#include <cpu/x86/msr.h>
#include <device/mmio.h>
#include <intelblocks/cfg.h>
#include <intelblocks/pcr.h>
#include <intelblocks/pmclib.h>
#include <intelpch/lockdown.h>
#include <soc/pcr_ids.h>
#include <soc/pm.h>
#include <stdint.h>

/* PCR PSTH Control Register */
#define PCR_PSTH_CTRLREG		0x1d00
#define   PSTH_CTRLREG_IOSFPTCGE	(1 << 2)

#define MSR_IA32_DEBUG_INTERFACE		0xc80
#define   MSR_IA32_DEBUG_INTERFACE_EN		(1 << 0)
#define   MSR_IA32_DEBUG_INTERFACE_LOCK		(1 << 30)

static void cpu_lockdown_cfg(void)
{
	msr_t msr = rdmsr(MSR_IA32_DEBUG_INTERFACE);

	if (!(msr.lo & MSR_IA32_DEBUG_INTERFACE_LOCK)) {
		if (CONFIG(INTEL_TXT))
			msr.lo &= ~MSR_IA32_DEBUG_INTERFACE_EN;

		msr.lo |= MSR_IA32_DEBUG_INTERFACE_LOCK;
		wrmsr(MSR_IA32_DEBUG_INTERFACE, msr);
	}
}

static void pmc_lockdown_cfg(int chipset_lockdown)
{
	uint8_t *pmcbase = pmc_mmio_regs();

	/* PMSYNC */
	setbits32(pmcbase + PMSYNC_TPR_CFG, PCH2CPU_TPR_CFG_LOCK);
	/* Lock down ABASE and sleep stretching policy */
	setbits32(pmcbase + GEN_PMCON_B, SLP_STR_POL_LOCK | ACPI_BASE_LOCK);

	if (chipset_lockdown == CHIPSET_LOCKDOWN_COREBOOT)
		setbits32(pmcbase + GEN_PMCON_B, SMI_LOCK);

	if (!CONFIG(USE_FSP_NOTIFY_PHASE_POST_PCI_ENUM)) {
		setbits32(pmcbase + ST_PG_FDIS1, ST_FDIS_LOCK);
		setbits32(pmcbase + SSML, SSML_SSL_EN);
		setbits32(pmcbase + PM_CFG, PM_CFG_DBG_MODE_LOCK |
					 PM_CFG_XRAM_READ_DISABLE);
	}

	/* Send PMC IPC to inform about both BIOS Reset and PCI enumeration done */
	pmc_send_bios_reset_pci_enum_done();
}

static void pch_lockdown_cfg(void)
{
	if (CONFIG(USE_FSP_NOTIFY_PHASE_POST_PCI_ENUM))
		return;

	/* Enable IOSF Primary Trunk Clock Gating */
	pcr_rmw32(PID_PSTH, PCR_PSTH_CTRLREG, ~0, PSTH_CTRLREG_IOSFPTCGE);
}

void soc_lockdown_config(int chipset_lockdown)
{
	/* PMC lock down configuration */
	pmc_lockdown_cfg(chipset_lockdown);
	/* PCH lock down configuration */
	pch_lockdown_cfg();
	cpu_lockdown_cfg();
}
