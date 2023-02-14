/* SPDX-License-Identifier: GPL-2.0-only */

/*
 * This file is created based on Intel Tiger Lake Processor PCH Datasheet
 * Document number: 575857
 * Chapter number: 4
 */

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

static void pmc_lock_pmsync(void)
{
	uint8_t *pmcbase;
	uint32_t pmsyncreg;

	pmcbase = pmc_mmio_regs();

	pmsyncreg = read32(pmcbase + PMSYNC_TPR_CFG);
	pmsyncreg |= PCH2CPU_TPR_CFG_LOCK;
	write32(pmcbase + PMSYNC_TPR_CFG, pmsyncreg);
}

static void pmc_lock_abase(void)
{
	uint8_t *pmcbase;
	uint32_t reg32;

	pmcbase = pmc_mmio_regs();

	reg32 = read32(pmcbase + GEN_PMCON_B);
	reg32 |= (SLP_STR_POL_LOCK | ACPI_BASE_LOCK);
	write32(pmcbase + GEN_PMCON_B, reg32);
}

static void pmc_lock_smi(void)
{
	uint8_t *pmcbase;
	uint8_t reg8;

	pmcbase = pmc_mmio_regs();

	reg8 = read8(pmcbase + GEN_PMCON_B);
	reg8 |= SMI_LOCK;
	write8(pmcbase + GEN_PMCON_B, reg8);
}

static void pmc_lockdown_cfg(int chipset_lockdown)
{
	uint8_t *pmcbase = pmc_mmio_regs();

	/* PMSYNC */
	pmc_lock_pmsync();
	/* Lock down ABASE and sleep stretching policy */
	pmc_lock_abase();

	if (chipset_lockdown == CHIPSET_LOCKDOWN_COREBOOT)
		pmc_lock_smi();

	if (!CONFIG(USE_FSP_NOTIFY_PHASE_POST_PCI_ENUM)) {
		setbits32(pmcbase + ST_PG_FDIS1, ST_FDIS_LOCK);
		setbits32(pmcbase + SSML, SSML_SSL_EN);
		setbits32(pmcbase + PM_CFG, PM_CFG_DBG_MODE_LOCK |
					 PM_CFG_XRAM_READ_DISABLE);
	}

	/* Send PMC IPC to inform about PCI enumeration done */
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
}
