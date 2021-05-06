/* SPDX-License-Identifier: GPL-2.0-only */

#include <cbmem.h>
#include <console/console.h>
#include <fsp/util.h>
#include <memory_info.h>
#include <soc/intel/common/smbios.h>
#include <soc/meminit.h>
#include <string.h>
#include <smbios.h>

#include <FspmUpd.h>

#define FSP_SMBIOS_MEMORY_INFO_GUID	\
{	\
	0x8c, 0x10, 0xa1, 0x01, 0xee, 0x9d, 0x84, 0x49,	\
	0x88, 0xc3, 0xee, 0xe8, 0xc4, 0x9e, 0xfb, 0x89	\
}

enum {
	DIMM_ENABLED,
	DIMM_DISABLED,
	DIMM_PRESENT,
	DIMM_NOT_PRESENT
};

enum {
	CHANNEL_NOT_PRESENT,
	CHANNEL_DISABLED,
	CHANNEL_PRESENT
};

void save_lpddr4_dimm_info_part_num(const char *dram_part_num)
{
	int channel, dimm, dimm_max, index, node;
	size_t hob_size;
	const DIMM_INFO *src_dimm;
	struct dimm_info *dest_dimm;
	struct memory_info *mem_info;
	const CHANNEL_INFO *channel_info;
	const FSP_SMBIOS_MEMORY_INFO *memory_info_hob;
	const CONTROLLER_INFO *ctrl_info;
	const uint8_t smbios_memory_info_guid[16] =
			FSP_SMBIOS_MEMORY_INFO_GUID;

	if (!dram_part_num)
		dram_part_num = "Unknown";

	/* Locate the memory info HOB */
	memory_info_hob = fsp_find_extension_hob_by_guid(
				smbios_memory_info_guid,
				&hob_size);

	if (memory_info_hob == NULL || hob_size == 0) {
		printk(BIOS_ERR, "SMBIOS memory info HOB is missing\n");
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

	/* Describe the first N DIMMs in the system */
	index = 0;
	dimm_max = ARRAY_SIZE(mem_info->dimm);

	for (node = 0; node < MAX_NODE_NUM; node++) {
		ctrl_info = &memory_info_hob->Controller[node];
		for (channel = 0; channel < ctrl_info->ChannelCount;
			channel++) {
			if (index >= dimm_max)
				break;

			channel_info = &ctrl_info->ChannelInfo[channel];

			for (dimm = 0; dimm < channel_info->DimmCount; dimm++) {
				if (index >= dimm_max)
					break;
				src_dimm = &channel_info->DimmInfo[dimm];
				dest_dimm = &mem_info->dimm[index];

				if (!src_dimm->DimmCapacity)
					continue;

				/* Populate the DIMM information */
				dimm_info_fill(dest_dimm,
				    src_dimm->DimmCapacity,
				    memory_info_hob->MemoryType,
				    memory_info_hob->ConfiguredMemoryClockSpeed,
				    src_dimm->RankInDimm,
				    channel_info->ChannelId,
				    src_dimm->DimmId,
				    dram_part_num,
				    strlen(dram_part_num),
				    src_dimm->SpdSave + SPD_SAVE_OFFSET_SERIAL,
				    memory_info_hob->DataWidth,
				    0,
				    0,
				    src_dimm->MfgId,
				    src_dimm->SpdModuleType);
				index++;
			}
		}
	}
	mem_info->dimm_cnt = index;
	printk(BIOS_DEBUG, "%d DIMMs found\n", mem_info->dimm_cnt);
}

void save_lpddr4_dimm_info(const struct lpddr4_cfg *lp4cfg, size_t mem_sku)
{
	const char *part_num = NULL;

	if (mem_sku >= lp4cfg->num_skus) {
		printk(BIOS_ERR, "Too few LPDDR4 SKUs: 0x%zx/0x%zx\n",
			mem_sku, lp4cfg->num_skus);
	} else {
		part_num = lp4cfg->skus[mem_sku].part_num;
	}

	save_lpddr4_dimm_info_part_num(part_num);
}

/* Save the DIMM information for SMBIOS table 17 */
void save_ddr4_dimm_info(void)
{
	int channel, dimm, dimm_max, index, node;
	size_t hob_size;
	uint8_t ddr_type;
	const CONTROLLER_INFO *ctrlr_info;
	const CHANNEL_INFO *channel_info;
	const DIMM_INFO *src_dimm;
	struct dimm_info *dest_dimm;
	struct memory_info *mem_info;
	const FSP_SMBIOS_MEMORY_INFO *memory_info_hob;
	const uint8_t smbios_memory_info_guid[16] =
			FSP_SMBIOS_MEMORY_INFO_GUID;

	/* Locate the memory info HOB, presence validated by raminit */
	memory_info_hob =
		fsp_find_extension_hob_by_guid(smbios_memory_info_guid,
						&hob_size);
	if (memory_info_hob == NULL || hob_size == 0) {
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

	/* Describe the first N DIMMs in the system */
	index = 0;
	dimm_max = ARRAY_SIZE(mem_info->dimm);
	for (node = 0; node < MAX_NODE_NUM; node++) {
		ctrlr_info = &memory_info_hob->Controller[node];
		for (channel = 0; channel < MAX_CHANNELS_NUM && index < dimm_max; channel++) {
			channel_info = &ctrlr_info->ChannelInfo[channel];
			if (channel_info->Status != CHANNEL_PRESENT)
				continue;
			for (dimm = 0; dimm < MAX_DIMMS_NUM && index < dimm_max; dimm++) {
				src_dimm = &channel_info->DimmInfo[dimm];
				dest_dimm = &mem_info->dimm[index];

				if (src_dimm->Status != DIMM_PRESENT)
					continue;

				switch (memory_info_hob->MemoryType) {
				case MRC_DDR_TYPE_DDR4:
					ddr_type = MEMORY_TYPE_DDR4;
					break;
				case MRC_DDR_TYPE_LPDDR4:
					ddr_type = MEMORY_TYPE_LPDDR4;
					break;
				default:
					ddr_type = MEMORY_TYPE_UNKNOWN;
					break;
				}
				u8 memProfNum = memory_info_hob->MemoryProfile;

				/* Populate the DIMM information */
				dimm_info_fill(dest_dimm,
					src_dimm->DimmCapacity,
					ddr_type,
					memory_info_hob->ConfiguredMemoryClockSpeed,
					src_dimm->RankInDimm,
					channel_info->ChannelId,
					src_dimm->DimmId,
					(const char *)src_dimm->ModulePartNum,
					sizeof(src_dimm->ModulePartNum),
					src_dimm->SpdSave + SPD_SAVE_OFFSET_SERIAL,
					memory_info_hob->DataWidth,
					memory_info_hob->VddVoltage[memProfNum],
					memory_info_hob->EccSupport,
					src_dimm->MfgId,
					src_dimm->SpdModuleType);
				index++;
			} // for dimm
		} // for channel
	} // for node
	mem_info->dimm_cnt = index;
	printk(BIOS_DEBUG, "%d DIMMs found\n", mem_info->dimm_cnt);
}

