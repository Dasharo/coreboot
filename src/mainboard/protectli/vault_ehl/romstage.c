/* SPDX-License-Identifier: GPL-2.0-only */

#include <string.h>
#include <types.h>
#include <soc/meminit.h>
#include <soc/romstage.h>
#include <spd_bin.h>

static void mainboard_fill_dq_map_data(void *dq_map_ch0, void *dq_map_ch1)
{
	const uint8_t dq_map[2][12] = {
		{ 0x0F, 0xF0, 0x00, 0xF0, 0x0F, 0xF0,
		  0x0F, 0x00, 0xFF, 0x00, 0xFF, 0x00 },
		{ 0x33, 0xCC, 0x00, 0xCC, 0x33, 0xCC,
		  0x33, 0x00, 0xFF, 0x00, 0xFF, 0x00 } };
	memcpy(dq_map_ch0, dq_map[0], sizeof(dq_map[0]));
	memcpy(dq_map_ch1, dq_map[1], sizeof(dq_map[1]));
}


static void mainboard_fill_dqs_map_data(void *dqs_map_ch0, void *dqs_map_ch1)
{
	const uint8_t dqs_map[2][8] = {
		{ 0, 1, 2, 3, 4, 5, 6, 7 },
		{ 0, 1, 2, 3, 4, 5, 6, 7 } };
	memcpy(dqs_map_ch0, dqs_map[0], sizeof(dqs_map[0]));
	memcpy(dqs_map_ch1, dqs_map[1], sizeof(dqs_map[1]));
}


static void fill_ddr4_params(FSP_M_CONFIG *cfg)
{

	struct spd_block blk = {
		.addr_map = { 0x50, 0x52, },
	};

	cfg->DqPinsInterleaved = 0x01;
	cfg->CaVrefConfig = 0x02;

	get_spd_smbus(&blk);
	dump_spd_info(&blk);

	cfg->MemorySpdDataLen = blk.len;
	cfg->MemorySpdPtr00 = (uintptr_t)blk.spd_array[0];
	cfg->MemorySpdPtr10 = (uintptr_t)blk.spd_array[1];


	mainboard_fill_dq_map_data(&cfg->DqByteMapCh0,
					&cfg->DqByteMapCh1);
	mainboard_fill_dqs_map_data(&cfg->DqsMapCpu2DramCh0,
					&cfg->DqsMapCpu2DramCh1);

}

void mainboard_memory_init_params(FSPM_UPD *memupd)
{
	FSP_M_CONFIG *cfg = &memupd->FspmConfig;
	fill_ddr4_params(cfg);
}
