/* SPDX-License-Identifier: GPL-2.0-only */

#include <arch/romstage.h>
#include <cbmem.h>
#include <console/console.h>
#include <device/pci_ops.h>
#include <romstage_handoff.h>
#include <southbridge/intel/common/pmbase.h>
#include <southbridge/intel/common/pmclib.h>
#include <southbridge/intel/common/pmutil.h>
#include <southbridge/intel/i82801ix/i82801ix.h>

#include "q35.h"

void mainboard_romstage_entry(void)
{
	bool s3resume;

	i82801ix_early_init();

	if (!CONFIG(BOOTBLOCK_CONSOLE))
		mainboard_machine_check();

	/* Configure requested TSEG size */
	switch (CONFIG_SMM_TSEG_SIZE) {
	case 1 * MiB:
		pci_update_config8(HOST_BRIDGE, ESMRAMC, ~TSEG_SZ_MASK, 0 << 1);
		break;
	case 2 * MiB:
		pci_update_config8(HOST_BRIDGE, ESMRAMC, ~TSEG_SZ_MASK, 1 << 1);
		break;
	case 8 * MiB:
		pci_update_config8(HOST_BRIDGE, ESMRAMC, ~TSEG_SZ_MASK, 2 << 1);
		break;
	default:
		printk(BIOS_WARNING, "%s: Unsupported TSEG size: 0x%x\n", __func__, CONFIG_SMM_TSEG_SIZE);
	}

	/*
	 * QEMU clears SLP_TYP on wake and leaves WAK_STS set. Detect that
	 * here so ramstage can recover CBMEM and jump to the FACS vector.
	 */
	s3resume = southbridge_detect_s3_resume();
	/*
	 * Dasharo qemu-q35 UEFI images default to loglevel 0 (EMERG only).
	 * Print resume at BIOS_EMERG so serial evidence is visible on stock
	 * images; cold boot stays at BIOS_INFO for the high-loglevel smoke
	 * config.
	 */
	printk(s3resume ? BIOS_EMERG : BIOS_INFO,
	       "Q35 S3: PM1_STS=0x%04x PM1_CNT=0x%08x s3resume=%d\n",
	       read_pmbase16(PM1_STS), read_pmbase32(PM1_CNT), s3resume);

	if (cbmem_recovery(s3resume)) {
		printk(BIOS_ERR, "S3 resume: CBMEM recovery failed, cold boot\n");
		s3resume = false;
		cbmem_initialize_empty();
	}

	romstage_handoff_init(s3resume);
}
