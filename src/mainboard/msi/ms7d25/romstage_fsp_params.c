/* SPDX-License-Identifier: GPL-2.0-only */

#include <assert.h>
#include <console/console.h>
#include <dasharo/options.h>
#include <delay.h>
#include <device/dram/ddr4.h>
#include <drivers/nuvoton/nct3933/nct3933.h>
#include <fsp/api.h>
#include <soc/romstage.h>
#include <soc/meminit.h>
#include <superio/nuvoton/nct6687d/nct6687d_smbus.h>
#include <spd_bin.h>
#include <string.h>

#include "gpio.h"

#define EC_IO_BASE 			0xa20

#define NCT3933_DRAM_OV_ADDR		0x20

#define FSP_CLK_NOTUSED			0xFF
#define FSP_CLK_FREE_RUNNING		0x80

static const struct mb_cfg ddr4_mem_config = {
	.type = MEM_TYPE_DDR4,
	/* According to DOC #573387 rcomp values no longer have to be provided */
	/* DDR DIMM configuration does not need to set DQ/DQS maps */
	.UserBd = BOARD_TYPE_DESKTOP_2DPC,

	.ddr_config = {
		.dq_pins_interleaved = true,
	},
};

static const struct mb_cfg ddr5_mem_config = {
	.type = MEM_TYPE_DDR5,

	.ect = true, /* Early Command Training */

	/* According to DOC #573387 rcomp values no longer have to be provided */
	/* DDR DIMM configuration does not need to set DQ/DQS maps */
	.UserBd = BOARD_TYPE_DESKTOP_2DPC,

	.LpDdrDqDqsReTraining = 1,

	.ddr_config = {
		.dq_pins_interleaved = true,
	},
};

static const struct mem_spd dimm_module_spd_info = {
	.topo = MEM_TOPO_DIMM_MODULE,
	.smbus = {
		[0] = {
			.addr_dimm[0] = 0x50,
			.addr_dimm[1] = 0x51,
		},
		[1] = {
			.addr_dimm[0] = 0x52,
			.addr_dimm[1] = 0x53,
		},
	},
};

static void disable_pcie_clock_requests(FSP_M_CONFIG *m_cfg)
{
	memset(m_cfg->PcieClkSrcUsage, FSP_CLK_NOTUSED, sizeof(m_cfg->PcieClkSrcUsage));
	memset(m_cfg->PcieClkSrcClkReq, FSP_CLK_NOTUSED, sizeof(m_cfg->PcieClkSrcClkReq));

	m_cfg->PcieClkSrcUsage[0]  = FSP_CLK_FREE_RUNNING;
	m_cfg->PcieClkSrcUsage[8]  = FSP_CLK_FREE_RUNNING;
	m_cfg->PcieClkSrcUsage[9]  = FSP_CLK_FREE_RUNNING;
	m_cfg->PcieClkSrcUsage[10] = FSP_CLK_FREE_RUNNING;
	m_cfg->PcieClkSrcUsage[12] = FSP_CLK_FREE_RUNNING;
	m_cfg->PcieClkSrcUsage[13] = FSP_CLK_FREE_RUNNING;
	m_cfg->PcieClkSrcUsage[14] = FSP_CLK_FREE_RUNNING;
	m_cfg->PcieClkSrcUsage[15] = FSP_CLK_FREE_RUNNING;
	m_cfg->PcieClkSrcUsage[17] = FSP_CLK_FREE_RUNNING;

	gpio_configure_pads(clkreq_disabled_table, ARRAY_SIZE(clkreq_disabled_table));
}

static void set_dram_voltage(const struct nct3933_smbus_ops *ops, uint8_t addr, uint8_t reg,
			     uint16_t requested_mv, uint16_t default_mv)
{
	uint16_t current_mv;
	uint8_t dac_val, direction;
	int steps;
	static const char * const voltages[] = { "", "VDD", "VTT", "VPP" };

	if (nct3933_get_current_dac(ops, addr, reg, &dac_val)) {
		printk(BIOS_ERR, "Failed to read current DRAM %s from NCT3933\n", voltages[reg]);
		return;
	}

	if ((dac_val == 0) && (requested_mv == default_mv)) {
		printk(BIOS_ERR, "DRAM %s at standard default value\n", voltages[reg]);
		return;
	}

	if (dac_val & NCT3933_CURRENT_SOURCE)
		current_mv = default_mv + (dac_val & NCT3933_CURRENT_VAL_MASK) * 10;
	else
		current_mv = default_mv - (dac_val & NCT3933_CURRENT_VAL_MASK) * 10;

	printk(BIOS_DEBUG, "Current DRAM %s: %u mV\n", voltages[reg], current_mv);

	/* Round up to 10mV */
	if (requested_mv % 10 >= 5)
		requested_mv += 10 - (requested_mv % 10);
	else
		requested_mv -= (requested_mv % 10);

	if (requested_mv == current_mv)
		return;

	if (current_mv > requested_mv) {
		steps = (current_mv - requested_mv) / 10;
		direction = 0; // decrementing
	} else {
		steps = (requested_mv - current_mv) / 10;
		direction = 1; // incrementing
	}

	for (int i = 0; i < steps; i++) {
		if (direction) {
			if ((dac_val & NCT3933_CURRENT_VAL_MASK) == NCT3933_CURRENT_VAL_MASK)
				break;

			dac_val = (dac_val & NCT3933_CURRENT_VAL_MASK) + 1;
		} else {
			if ((dac_val & NCT3933_CURRENT_VAL_MASK) == 0)
				break;

			dac_val = (dac_val & NCT3933_CURRENT_VAL_MASK) - 1;
		}

		/* We are always ramping the voltage above default 1.2V or down to 1.2V */
		dac_val |= NCT3933_CURRENT_SOURCE;

		if (nct3933_set_current_dac(ops, addr, reg, dac_val))
			printk(BIOS_ERR, "Failed to set DRAM %s to %02x via NCT3933\n",
			       voltages[reg], dac_val);

		mdelay(1);
	}

	if (!nct3933_get_current_dac(ops, addr, reg, &dac_val)) {
		if (dac_val & NCT3933_CURRENT_SOURCE)
			current_mv = default_mv + (dac_val & NCT3933_CURRENT_VAL_MASK) * 10;
		else
			current_mv = default_mv - (dac_val & NCT3933_CURRENT_VAL_MASK) * 10;

		printk(BIOS_DEBUG, "DRAM %s set to %u mV\n", voltages[reg], current_mv);
	}
}

static void boost_dram_voltage(uint16_t requested_voltage)
{
	static const struct nct3933_smbus_ops ops = {
		.read_byte = nct6687d_smbus_read_byte,
		.write_byte = nct6687d_smbus_write_byte
	};

	nct6687d_smbus_init(EC_IO_BASE);

	if (nct3933_probe(&ops, NCT3933_DRAM_OV_ADDR)) {
		printk(BIOS_ERR, "NCT3933 for DRAM OV not found\n"
				 "Will not overvolt the memory\n");
		return;
	}

	/* OUT1 is used for DRAM VDD OV */
	set_dram_voltage(&ops, NCT3933_DRAM_OV_ADDR, NCT3933_OUT_DAC_REG(1),
			 requested_voltage, 1200);

	/* OUT2 is used for DRAM VTT OV */
	set_dram_voltage(&ops, NCT3933_DRAM_OV_ADDR, NCT3933_OUT_DAC_REG(2),
			 requested_voltage / 2, 1200 / 2);

	/* OUT3 is used for DRAM VPP OV */
	// set_dram_voltage(&ops, NCT3933_DRAM_OV_ADDR, NCT3933_OUT_DAC_REG(3), 2500, 2500);
}

static void check_ddr4_xmp_valid(FSPM_UPD *memupd)
{
	int ret = 0;
	uint16_t requested_voltage = 1200; // DDR4 default 1.2V
	enum ddr4_xmp_profile profile;
	struct dimm_attr_ddr4_st dimm[4];
	struct spd_block blk = {
		.addr_map = { 0x50, 0x51, 0x52, 0x53, },
	};

	if (memupd->FspmConfig.SpdProfileSelected == 2) { // XMP1
		profile = DDR4_XMP_PROFILE_1;
	}
	else if (memupd->FspmConfig.SpdProfileSelected == 3) { // XMP2
		profile = DDR4_XMP_PROFILE_2;
	} else {
		boost_dram_voltage(requested_voltage);
		return;
	}

	get_spd_smbus(&blk);

	if (blk.spd_array[0] == NULL && blk.spd_array[1] == NULL &&
	    blk.spd_array[2] == NULL && blk.spd_array[3] == NULL)
		die("DRAM SPD not found!\n");

	for (int i = 0; i < 4; i++) {
		if (blk.spd_array[i] == NULL)
			continue;
		
		ret = spd_xmp_decode_ddr4(&dimm[i], blk.spd_array[i], profile);
		if (ret)
			break;

		/* Take the maximum of all DIMM voltages */
		requested_voltage = MAX(requested_voltage, dimm[i].vdd_voltage);
	}

	/* If any error occurred when parsing, revert to JEDEC standard profile */
	if (ret != 0) {
		printk(BIOS_ERR, "An error occurred when parsing XMP profile,"
				 " reverting to standard JEDEC profile\n");
		memupd->FspmConfig.SpdProfileSelected = 0;
		requested_voltage = 1200;
	} else {
		/* Do not exceed 1.5V */
		requested_voltage = MIN(requested_voltage, 1500);

		memupd->FspmConfig.RefClk = 1; // Use 100Mhz Memory Reference Clock
		// /* Give SA additional voltage for high frequency XMP1 profile */
		// if (profile == DDR4_XMP_PROFILE_1) {
		// 	memupd->FspmConfig.SaOcSupport = 1;
		// 	memupd->FspmConfig.SaVoltageMode = 1;
		// 	memupd->FspmConfig.SaVoltageOverride = requested_voltage + 50;
		// }
	}

	boost_dram_voltage(requested_voltage);
}

void mainboard_memory_init_params(FSPM_UPD *memupd)
{
	memupd->FspmConfig.CpuPcieRpClockReqMsgEnable[0] = CONFIG(PCIEXP_CLK_PM);
	memupd->FspmConfig.CpuPcieRpClockReqMsgEnable[1] = CONFIG(PCIEXP_CLK_PM);
	memupd->FspmConfig.CpuPcieRpClockReqMsgEnable[2] = CONFIG(PCIEXP_CLK_PM);
	memupd->FspmConfig.DmiMaxLinkSpeed = 4; // Gen4 speed, undocumented
	memupd->FspmConfig.DmiAspm = 0;
	memupd->FspmConfig.DmiAspmCtrl = 0;
	memupd->FspmConfig.SkipExtGfxScan = 0;

	memupd->FspmConfig.PchHdaAudioLinkHdaEnable = 1;
	memupd->FspmConfig.PchHdaSdiEnable[0] = 1;

	memupd->FspmConfig.MmioSize = 0xb00; /* 2.75GB in MB */

	memupd->FspmConfig.OcLock = 0;
	// memupd->FspmConfig.OcSupport = 1;
	// memupd->FspmConfig.UnderVoltProtection = 0;
	// memupd->FspmConfig.IaCepEnable = 0;

	memupd->FspmConfig.SpdProfileSelected = dasharo_get_memory_profile();

	if (CONFIG(BOARD_MSI_Z690_A_PRO_WIFI_DDR4)) {
		check_ddr4_xmp_valid(memupd);
		memcfg_init(memupd, &ddr4_mem_config, &dimm_module_spd_info, false);
	}
	if (CONFIG(BOARD_MSI_Z690_A_PRO_WIFI_DDR5))
		memcfg_init(memupd, &ddr5_mem_config, &dimm_module_spd_info, false);

	gpio_configure_pads(gpio_table, ARRAY_SIZE(gpio_table));

	if (!CONFIG(PCIEXP_CLK_PM))
		disable_pcie_clock_requests(&memupd->FspmConfig);
}
