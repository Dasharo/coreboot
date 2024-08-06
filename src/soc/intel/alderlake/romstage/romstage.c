/* SPDX-License-Identifier: GPL-2.0-only */

#include <arch/romstage.h>
#include <cbmem.h>
#include <cf9_reset.h>
#include <console/console.h>
#include <elog.h>
#include <fsp/util.h>
#include <intelblocks/cfg.h>
#include <intelblocks/cse.h>
#include <intelblocks/early_graphics.h>
#include <intelblocks/oc_wdt.h>
#include <intelblocks/pcr.h>
#include <intelblocks/pmclib.h>
#include <intelblocks/smbus.h>
#include <intelblocks/thermal.h>
#include <intelblocks/vtd.h>
#include <intelbasecode/debug_feature.h>
#include <memory_info.h>
#include <soc/intel/common/smbios.h>
#include <soc/iomap.h>
#include <soc/pm.h>
#include <soc/romstage.h>
#include <soc/soc_chip.h>
#include <cpu/intel/cpu_ids.h>
#include <timestamp.h>
#include <string.h>
#include <security/intel/txt/txt.h>
#include <soc/pcr_ids.h>

#define PSF_UFS0_BASE_ADDRESS  0x280
#define PSF_UFS1_BASE_ADDRESS  0x300
#define PCR_PSFX_T0_SHDW_PCIEN 0x1C
#define PCR_PSFX_T0_SHDW_PCIEN_FUNDIS  (1 << 8)

static void disable_ufs(void)
{
	/* disable USF0 */
	pcr_or32(PID_PSF2, PSF_UFS0_BASE_ADDRESS + PCR_PSFX_T0_SHDW_PCIEN,
			 PCR_PSFX_T0_SHDW_PCIEN_FUNDIS);

	/* disable USF1 */
	pcr_or32(PID_PSF2, PSF_UFS1_BASE_ADDRESS + PCR_PSFX_T0_SHDW_PCIEN,
			 PCR_PSFX_T0_SHDW_PCIEN_FUNDIS);
}

#include "ux.h"

#define FSP_SMBIOS_MEMORY_INFO_GUID	\
{	\
	0xd4, 0x71, 0x20, 0x9b, 0x54, 0xb0, 0x0c, 0x4e,	\
	0x8d, 0x09, 0x11, 0xcf, 0x8b, 0x9f, 0x03, 0x23	\
}

bool skip_cse_sub_part_update(void)
{
	return cpu_get_cpuid() != CPUID_ALDERLAKE_K0;
}

/* Save the DIMM information for SMBIOS table 17 */
static void save_dimm_info(void)
{
	int node, channel, dimm, dimm_max, index;
	size_t hob_size;
	const CONTROLLER_INFO *ctrlr_info;
	const CHANNEL_INFO *channel_info;
	const DIMM_INFO *src_dimm;
	struct dimm_info *dest_dimm;
	struct memory_info *mem_info;
	const MEMORY_INFO_DATA_HOB *meminfo_hob;
	const uint8_t smbios_memory_info_guid[sizeof(EFI_GUID)] = FSP_SMBIOS_MEMORY_INFO_GUID;
	const uint8_t *serial_num;
	const char *dram_part_num = NULL;
	size_t dram_part_num_len = 0;

	/* Locate the memory info HOB, presence validated by raminit */
	meminfo_hob = fsp_find_extension_hob_by_guid(
						smbios_memory_info_guid,
						&hob_size);
	if (meminfo_hob == NULL || hob_size == 0) {
		printk(BIOS_ERR, "SMBIOS MEMORY_INFO_DATA_HOB not found\n");
		return;
	}

	/*
	 * Allocate CBMEM area for DIMM information used to populate SMBIOS
	 * table 17
	 */
	mem_info = cbmem_add(CBMEM_ID_MEMINFO, sizeof(*mem_info));
	if (mem_info == NULL) {
		printk(BIOS_ERR, "CBMEM entry for DIMM info missing\n");
		return;
	}
	memset(mem_info, 0, sizeof(*mem_info));

	/* Allow mainboard to override DRAM part number. */
	dram_part_num = mainboard_get_dram_part_num();
	if (dram_part_num)
		dram_part_num_len = strlen(dram_part_num);

	/* Save available DIMM information */
	index = 0;
	dimm_max = ARRAY_SIZE(mem_info->dimm);
	for (node = 0; node < MAX_NODE; node++) {
		ctrlr_info = &meminfo_hob->Controller[node];
		for (channel = 0; channel < MAX_CH && index < dimm_max; channel++) {
			channel_info = &ctrlr_info->ChannelInfo[channel];
			if (channel_info->Status != CHANNEL_PRESENT)
				continue;

			for (dimm = 0; dimm < MAX_DIMM && index < dimm_max; dimm++) {
				src_dimm = &channel_info->DimmInfo[dimm];
				dest_dimm = &mem_info->dimm[index];
				if (src_dimm->Status != DIMM_PRESENT)
					continue;

				/* If there is no DRAM part number overridden by
				 * mainboard then use original one. */
				if (!dram_part_num) {
					dram_part_num_len = sizeof(src_dimm->ModulePartNum);
					dram_part_num = (const char *)
								&src_dimm->ModulePartNum[0];
				}

				uint8_t memProfNum = meminfo_hob->MemoryProfile;
				serial_num = src_dimm->SpdSave + SPD_SAVE_OFFSET_SERIAL;

				/* Populate the DIMM information */
				dimm_info_fill(dest_dimm,
					src_dimm->DimmCapacity,
					meminfo_hob->MemoryType,
					meminfo_hob->ConfiguredMemoryClockSpeed,
					src_dimm->RankInDimm,
					channel_info->ChannelId,
					src_dimm->DimmId,
					dram_part_num,
					dram_part_num_len,
					serial_num,
					meminfo_hob->DataWidth,
					meminfo_hob->VddVoltage[memProfNum],
					meminfo_hob->EccSupport,
					src_dimm->MfgId,
					src_dimm->SpdModuleType,
					node,
					meminfo_hob->MaximumMemoryClockSpeed);
				index++;
			}
		}
	}
	mem_info->dimm_cnt = index;
	printk(BIOS_DEBUG, "%d DIMMs found\n", mem_info->dimm_cnt);
}

void cse_fw_update_misc_oper(void)
{
	if (ux_inform_user_of_update_operation("CSE update"))
		elog_add_event_byte(ELOG_TYPE_FW_EARLY_SOL, ELOG_FW_EARLY_SOL_CSE_SYNC);
}

void cse_board_reset(void)
{
	early_graphics_stop();
}

void mainboard_romstage_entry(void)
{
	struct chipset_power_state *ps = pmc_get_power_state();
	bool s3wake = pmc_fill_power_state(ps) == ACPI_S3;

	setup_oc_wdt();

	/* Initialize HECI interface */
	cse_init(HECI1_BASE_ADDRESS);

	if (CONFIG(SOC_INTEL_COMMON_BASECODE_DEBUG_FEATURE))
		dbg_feature_cntrl_init();

	/*
	 * Disable Intel TXT if `CPU is unsupported` or `SoC haven't selected the config`.
	 *
	 * It would help to access VGA framebuffer prior calling into CSE
	 * firmware update or FSP-M.
	 */
	if (!CONFIG(INTEL_TXT))
		disable_intel_txt();
	else
		intel_txt_romstage_init();

	if (CONFIG(SOC_INTEL_CSE_LITE_SYNC_IN_ROMSTAGE) && !s3wake)
		cse_fw_sync();

	/* Program to Disable UFS Controllers */
	if (!is_devfn_enabled(PCH_DEVFN_UFS) &&
			 (CONFIG(USE_UNIFIED_AP_FIRMWARE_FOR_UFS_AND_NON_UFS))) {
		printk(BIOS_INFO, "Disabling UFS controllers\n");
		disable_ufs();
		if (ps->prev_sleep_state == ACPI_S5) {
			printk(BIOS_INFO, "Warm Reset after disabling UFS controllers\n");
			system_reset();
		}
	}

	/* Program MCHBAR, DMIBAR, GDXBAR and EDRAMBAR */
	systemagent_early_init();
	/* Program SMBus base address and enable it */
	smbus_common_init();


	/* Update coreboot timestamp table with CSE timestamps */
	if (CONFIG(SOC_INTEL_CSE_PRE_CPU_RESET_TELEMETRY))
		cse_get_telemetry_data();

	/*
	 * Set low maximum temp threshold value used for dynamic thermal sensor
	 * shutdown consideration.
	 *
	 * If Dynamic Thermal Shutdown is enabled then PMC logic shuts down the
	 * thermal sensor when CPU is in a C-state and LTT >= DTS Temp.
	 */
	pch_thermal_configuration();

	fsp_memory_init(s3wake);
	pmc_set_disb();
	if (!s3wake)
		save_dimm_info();

	/*
	 * Turn-off early graphics configuration with two purposes:
	 * - Clear any potentially still on-screen message
	 * - Allow PEIM graphics driver to smoothly execute in ramstage if
	 *   RUN_FSP_GOP is selected
	 */
	early_graphics_stop();

	if (CONFIG(ENABLE_EARLY_DMA_PROTECTION))
		vtd_enable_dma_protection();
}
