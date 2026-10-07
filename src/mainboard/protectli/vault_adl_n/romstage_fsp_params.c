/* SPDX-License-Identifier: GPL-2.0-only */

#include <fsp/api.h>
#include <soc/romstage.h>
#include <soc/meminit.h>
#include <soc/gpio.h>
#include <variant/gpio.h>
#include <variant/pcie.h>

/* As in soc/intel/alderlake/romstage/fsp_params.c */
#define FSP_CLK_NOTUSED		0xff
#define FSP_CLK_FREE_RUNNING	0x80

static const struct mb_cfg ddr5_mem_config = {
	.type = MEM_TYPE_DDR5,
	.ect = true, /* Early Command Training */
	.UserBd = BOARD_TYPE_MOBILE,
	.LpDdrDqDqsReTraining = 1,
};

static const struct mem_spd dimm_module_spd_info = {
	.topo = MEM_TOPO_DIMM_MODULE,
	.smbus = {
		[0] = {
			.addr_dimm[0] = 0x50,
		},
	},
};

void mainboard_memory_init_params(FSPM_UPD *memupd)
{
	const struct pad_config *pads;
	size_t num;

	memcfg_init(memupd, &ddr5_mem_config, &dimm_module_spd_info, false);

	pads = board_gpio_table(&num);
	gpio_configure_pads(pads, num);

	// Set primary display to internal graphics
	memupd->FspmConfig.PrimaryDisplay = 0;
	memupd->FspmConfig.DmiMaxLinkSpeed = 4;
	memupd->FspmConfig.CpuPcieRpClockReqMsgEnable[0] = 0;
	memupd->FspmConfig.CpuPcieRpClockReqMsgEnable[1] = 0;
	memupd->FspmConfig.CpuPcieRpClockReqMsgEnable[2] = 0;

	/*
	 * The M.2 slot has one REFCLK (SRCCLK0) and one CLKREQ#. In 2x2 the
	 * slot's lanes 0-1 are on RP11 (lanes are reversed) and lanes 2-3 on
	 * RP9, so neither port owns the clock. Keep it running regardless of
	 * CLKREQ0#. A carrier with two devices has to buffer it.
	 */
	if (CONFIG(BOARD_PROTECTLI_V2210) && variant_pcie_is_2x2()) {
		memupd->FspmConfig.PcieClkSrcUsage[0] = FSP_CLK_FREE_RUNNING;
		memupd->FspmConfig.PcieClkSrcClkReq[0] = FSP_CLK_NOTUSED;
	}
}
