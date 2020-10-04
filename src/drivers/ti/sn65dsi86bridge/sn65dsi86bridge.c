/* SPDX-License-Identifier: GPL-2.0-only */

#include <console/console.h>
#include <delay.h>
#include <endian.h>
#include <device/i2c_simple.h>
#include <edid.h>
#include <timer.h>
#include <types.h>
#include <soc/addressmap.h>
#include "sn65dsi86bridge.h"

#define BRIDGE_GETHIGHERBYTE(x)		(uint8_t)((x & 0xff00) >> 8)
#define BRIDGE_GETLOWERBYTE(x)		(uint8_t)((x & 0x00ff))

/* fudge factor required to account for 8b/10b encoding */
#define DP_CLK_FUDGE_NUM 10
#define DP_CLK_FUDGE_DEN 8

/* DPCD */
#define DP_BRIDGE_DPCD_REV		0x700
#define DP_BRIDGE_11			0x00
#define DP_BRIDGE_12			0x01
#define DP_BRIDGE_13			0x02
#define DP_BRIDGE_14			0x03
#define DP_BRIDGE_CONFIGURATION_SET	0x10a
#define DP_MAX_LINK_RATE		0x001
#define DP_MAX_LANE_COUNT		0x002
#define DP_SUPPORTED_LINK_RATES		0x010 /* eDP 1.4 */
#define DP_MAX_LINK_RATE		0x001
#define DP_MAX_SUPPORTED_RATES		8 /* 16-bit little-endian */
#define DP_LANE_COUNT_MASK		0xf

/* link configuration */
#define DP_LINK_BW_SET		0x100
#define DP_LINK_BW_1_62		0x06
#define DP_LINK_BW_2_7		0x0a
#define DP_LINK_BW_5_4		0x14

#define AUX_CMD_SEND		0x1
#define MIN_DSI_CLK_FREQ_MHZ	40
#define MAX_DSI_CLK_FREQ_MHZ    750

enum bridge_regs {
	SN_DPPLL_SRC_REG = 0x0A,
	SN_PLL_ENABLE_REG = 0x0D,
	SN_DSI_LANES_REG = 0x10,
	SN_DSIA_CLK_FREQ_REG = 0x12,
	SN_CHA_ACTIVE_LINE_LENGTH_LOW_REG = 0x20,
	SN_CHA_ACTIVE_LINE_LENGTH_HIGH_REG = 0x21,
	SN_CHA_VERTICAL_DISPLAY_SIZE_LOW_REG = 0x24,
	SN_CHA_VERTICAL_DISPLAY_SIZE_HIGH_REG = 0x25,
	SN_CHA_HSYNC_PULSE_WIDTH_LOW_REG = 0x2C,
	SN_CHA_HSYNC_PULSE_WIDTH_HIGH_REG = 0x2D,
	SN_CHA_VSYNC_PULSE_WIDTH_LOW_REG = 0x30,
	SN_CHA_VSYNC_PULSE_WIDTH_HIGH_REG = 0x31,
	SN_CHA_HORIZONTAL_BACK_PORCH_REG = 0x34,
	SN_CHA_VERTICAL_BACK_PORCH_REG = 0x36,
	SN_CHA_HORIZONTAL_FRONT_PORCH_REG = 0x38,
	SN_CHA_VERTICAL_FRONT_PORCH_REG = 0x3A,
	SN_COLOR_BAR_REG = 0x3C,
	SN_ENH_FRAME_REG = 0x5A,
	SN_DATA_FORMAT_REG = 0x5B,
	SN_HPD_DISABLE_REG = 0x5C,
	SN_I2C_CLAIM_ADDR_EN1 = 0x60,
	SN_AUX_WDATA_REG_0 = 0x64,
	SN_AUX_WDATA_REG_1 = 0x65,
	SN_AUX_WDATA_REG_2 = 0x66,
	SN_AUX_WDATA_REG_3 = 0x67,
	SN_AUX_WDATA_REG_4 = 0x68,
	SN_AUX_WDATA_REG_5 = 0x69,
	SN_AUX_WDATA_REG_6 = 0x6A,
	SN_AUX_WDATA_REG_7 = 0x6B,
	SN_AUX_WDATA_REG_8 = 0x6C,
	SN_AUX_WDATA_REG_9 = 0x6D,
	SN_AUX_WDATA_REG_10 = 0x6E,
	SN_AUX_WDATA_REG_11 = 0x6F,
	SN_AUX_WDATA_REG_12 = 0x70,
	SN_AUX_WDATA_REG_13 = 0x71,
	SN_AUX_WDATA_REG_14 = 0x72,
	SN_AUX_WDATA_REG_15 = 0x73,
	SN_AUX_ADDR_19_16_REG = 0x74,
	SN_AUX_ADDR_15_8_REG = 0x75,
	SN_AUX_ADDR_7_0_REG = 0x76,
	SN_AUX_LENGTH_REG = 0x77,
	SN_AUX_CMD_REG = 0x78,
	SN_AUX_RDATA_REG_0 = 0x79,
	SN_AUX_RDATA_REG_1 = 0x7A,
	SN_AUX_RDATA_REG_2 = 0x7B,
	SN_AUX_RDATA_REG_3 = 0x7C,
	SN_AUX_RDATA_REG_4 = 0x7D,
	SN_AUX_RDATA_REG_5 = 0x7E,
	SN_AUX_RDATA_REG_6 = 0x7F,
	SN_AUX_RDATA_REG_7 = 0x80,
	SN_AUX_RDATA_REG_8 = 0x81,
	SN_AUX_RDATA_REG_9 = 0x82,
	SN_AUX_RDATA_REG_10 = 0x83,
	SN_AUX_RDATA_REG_11 = 0x84,
	SN_AUX_RDATA_REG_12 = 0x85,
	SN_AUX_RDATA_REG_13 = 0x86,
	SN_AUX_RDATA_REG_14 = 0x87,
	SN_AUX_RDATA_REG_15 = 0x88,
	SN_SSC_CONFIG_REG = 0x93,
	SN_DATARATE_CONFIG_REG = 0x94,
	SN_ML_TX_MODE_REG = 0x96,
	SN_AUX_CMD_STATUS_REG = 0xF4,
};

enum {
	HPD_ENABLE = 0x0,
	HPD_DISABLE = 0x1,
};

enum {
	SOT_ERR_TOL_DSI = 0x0,
	CHB_DSI_LANES = 0x1,
	CHA_DSI_LANES = 0x2,
	DSI_CHANNEL_MODE = 0x3,
	LEFT_RIGHT_PIXELS = 0x4,
};

enum vstream_config {
	VSTREAM_DISABLE = 0,
	VSTREAM_ENABLE = 1,
};

enum i2c_over_aux {
	I2C_OVER_AUX_WRITE_MOT_0 = 0x0,
	I2C_OVER_AUX_READ_MOT_0 = 0x1,
	I2C_OVER_AUX_WRITE_STATUS_UPDATE_0 = 0x4,
	I2C_OVER_AUX_WRITE_MOT_1 = 0x5,
	I2C_OVER_AUX_READ_MOT_1 = 0x6,
	I2C_OVER_AUX_WRITE_STATUS_UPDATE_1 = 0x7,
	NATIVE_AUX_WRITE = 0x8,
	NATIVE_AUX_READ = 0x9,
};

enum ml_tx_mode {
	MAIN_LINK_OFF = 0x0,
	NORMAL_MODE = 0x1,
	TPS1 = 0x2,
	TPS2 = 0x3,
	TPS3 = 0x4,
	PRBS7 = 0x5,
	HBR2_COMPLIANCE_EYE_PATTERN = 0x6,
	SYMBOL_ERR_RATE_MEASUREMENT_PATTERN = 0x7,
	CUTSOM_PATTERN = 0x8,
	FAST_LINK_TRAINING = 0x9,
	SEMI_AUTO_LINK_TRAINING = 0xa,
	REDRIVER_SEMI_AUTO_LINK_TRAINING = 0xb,
};

enum dpcd_request {
	DPCD_READ = 0x0,
	DPCD_WRITE = 0x1,
};

enum {
	EDID_LENGTH = 128,
	EDID_I2C_ADDR = 0x50,
	EDID_EXTENSION_FLAG = 0x7e,
};

/*
 * LUT index corresponds to register value and LUT values corresponds
 * to dp data rate supported by the bridge in Mbps unit.
 */
static const unsigned int sn65dsi86_bridge_dp_rate_lut[] = {
	0, 1620, 2160, 2430, 2700, 3240, 4320, 5400
};

enum cb_err sn65dsi86_bridge_read_edid(uint8_t bus, uint8_t chip, struct edid *out)
{
	int ret;
	u8 edid[EDID_LENGTH * 2];
	int edid_size = EDID_LENGTH;

	/* Send I2C command to claim EDID I2c slave */
	i2c_writeb(bus, chip, SN_I2C_CLAIM_ADDR_EN1, (EDID_I2C_ADDR << 1) | 0x1);

	/* read EDID */
	ret = i2c_read_bytes(bus, EDID_I2C_ADDR, 0x0, edid, EDID_LENGTH);
	if (ret != 0) {
		printk(BIOS_ERR, "ERROR: Failed to read EDID.\n");
		return CB_ERR;
	}

	if (edid[EDID_EXTENSION_FLAG]) {
		edid_size += EDID_LENGTH;
		ret = i2c_read_bytes(bus, EDID_I2C_ADDR, EDID_LENGTH,
					&edid[EDID_LENGTH], EDID_LENGTH);
		if (ret != 0) {
			printk(BIOS_ERR, "Failed to read EDID ext block.\n");
			return CB_ERR;
		}
	}

	if (decode_edid(edid, edid_size, out) != EDID_CONFORMANT) {
		printk(BIOS_ERR, "ERROR: Failed to decode EDID.\n");
		return CB_ERR;
	}

	return CB_SUCCESS;
}

static void sn65dsi86_bridge_dpcd_request(uint8_t bus,
					  uint8_t chip,
					  unsigned int dpcd_reg,
					  unsigned int len,
					  enum dpcd_request request,
					  uint8_t *data)
{
	int i;
	uint32_t length;
	uint8_t buf;
	uint8_t reg;

	while (len) {
		length = MIN(len, 16);

		i2c_writeb(bus, chip, SN_AUX_ADDR_19_16_REG, (dpcd_reg >> 16) & 0xF);
		i2c_writeb(bus, chip, SN_AUX_ADDR_15_8_REG, (dpcd_reg >> 8) & 0xFF);
		i2c_writeb(bus, chip, SN_AUX_ADDR_7_0_REG, (dpcd_reg) & 0xFF);
		i2c_writeb(bus, chip, SN_AUX_LENGTH_REG, length); /* size of 1 Byte data */
		if (request == DPCD_WRITE) {
			reg = SN_AUX_WDATA_REG_0;
			for (i = 0; i < length; i++)
				i2c_writeb(bus, chip, reg++, *data++);

			i2c_writeb(bus, chip,
				   SN_AUX_CMD_REG, AUX_CMD_SEND | (NATIVE_AUX_WRITE << 4));
		} else {
			i2c_writeb(bus, chip,
				   SN_AUX_CMD_REG, AUX_CMD_SEND | (NATIVE_AUX_READ << 4));
			if (!wait_ms(100,
			    !i2c_readb(bus, chip, SN_AUX_CMD_REG,
			    &buf) && !(buf & AUX_CMD_SEND))) {
				printk(BIOS_ERR, "ERROR: aux command send failed\n");
			}

			reg = SN_AUX_RDATA_REG_0;
			for (i = 0; i < length; i++) {
				i2c_readb(bus, chip, reg++, &buf);
				*data++ = buf;
			}
		}

		len -= length;
	}
}

static void sn65dsi86_bridge_valid_dp_rates(uint8_t bus, uint8_t chip, bool rate_valid[])
{
	unsigned int rate_per_200khz;
	uint8_t dpcd_val;
	int i, j;

	sn65dsi86_bridge_dpcd_request(bus, chip,
					DP_BRIDGE_DPCD_REV, 1, DPCD_READ, &dpcd_val);
	if (dpcd_val >= DP_BRIDGE_14) {
		/* eDP 1.4 devices must provide a custom table */
		uint8_t sink_rates[DP_MAX_SUPPORTED_RATES * 2];

		sn65dsi86_bridge_dpcd_request(bus, chip, DP_SUPPORTED_LINK_RATES,
					      sizeof(sink_rates),
					      DPCD_READ, sink_rates);
		for (i = 0; i < ARRAY_SIZE(sink_rates); i++) {
			rate_per_200khz = le16_to_cpu(sink_rates[i]);

			if (!rate_per_200khz)
				break;

			for (j = 0;
			     j < ARRAY_SIZE(sn65dsi86_bridge_dp_rate_lut);
			     j++) {
				if (sn65dsi86_bridge_dp_rate_lut[j] * (MHz / KHz) ==
				    rate_per_200khz * 200)
					rate_valid[j] = true;
			}
		}

		for (i = 0; i < ARRAY_SIZE(sn65dsi86_bridge_dp_rate_lut); i++) {
			if (rate_valid[i])
				return;
		}

		printk(BIOS_ERR, "No matching eDP rates in table; falling back\n");
	}

	/* On older versions best we can do is use DP_MAX_LINK_RATE */
	 sn65dsi86_bridge_dpcd_request(bus, chip,
					DP_MAX_LINK_RATE, 1, DPCD_READ, &dpcd_val);

	switch (dpcd_val) {
	default:
		printk(BIOS_ERR,
			      "Unexpected max rate (%#x); assuming 5.4 GHz\n",
			      (int)dpcd_val);
		/* fall through */
	case DP_LINK_BW_5_4:
		rate_valid[7] = 1;
		/* fall through */
	case DP_LINK_BW_2_7:
		rate_valid[4] = 1;
		/* fall through */
	case DP_LINK_BW_1_62:
		rate_valid[1] = 1;
		break;
	}
}

static void sn65dsi86_bridge_set_dsi_clock_range(uint8_t bus, uint8_t chip,
						 struct edid *edid,
						 uint32_t num_of_lanes, uint32_t bpp)
{
	uint64_t pixel_clk_hz;
	uint64_t stream_bit_rate_mhz;
	uint64_t min_req_dsi_clk;

	pixel_clk_hz = edid->mode.pixel_clock * KHz;
	stream_bit_rate_mhz = (pixel_clk_hz * bpp) / MHz;

	/* For TI the clock frequencies are half the bit rates */
	min_req_dsi_clk = stream_bit_rate_mhz / (num_of_lanes * 2);

	/* for each increment in val, frequency increases by 5MHz */
	min_req_dsi_clk = MAX(MIN_DSI_CLK_FREQ_MHZ,
			  MIN(MAX_DSI_CLK_FREQ_MHZ, min_req_dsi_clk)) / 5;
	i2c_writeb(bus, chip, SN_DSIA_CLK_FREQ_REG, min_req_dsi_clk);
}

static void sn65dsi86_bridge_set_dp_clock_range(uint8_t bus, uint8_t chip,
						struct edid *edid, uint32_t num_of_lanes)
{
	uint64_t stream_bit_rate_khz;
	bool rate_valid[ARRAY_SIZE(sn65dsi86_bridge_dp_rate_lut)] = { };
	uint64_t dp_rate_mhz;
	int dp_rate_idx, i;

	stream_bit_rate_khz = edid->mode.pixel_clock * 18;

	/* Calculate minimum DP data rate, taking 80% as per DP spec */
	dp_rate_mhz = DIV_ROUND_UP(stream_bit_rate_khz * DP_CLK_FUDGE_NUM,
				   KHz * num_of_lanes * DP_CLK_FUDGE_DEN);

	for (i = 0; i < ARRAY_SIZE(sn65dsi86_bridge_dp_rate_lut) - 1; i++)
		if (sn65dsi86_bridge_dp_rate_lut[i] > dp_rate_mhz)
			break;

	sn65dsi86_bridge_valid_dp_rates(bus, chip, rate_valid);

	/* Train until we run out of rates */
	for (dp_rate_idx = i;
	     dp_rate_idx < ARRAY_SIZE(sn65dsi86_bridge_dp_rate_lut);
	     dp_rate_idx++)
		if (rate_valid[dp_rate_idx])
			break;

	if (dp_rate_idx < ARRAY_SIZE(sn65dsi86_bridge_dp_rate_lut))
		i2c_write_field(bus, chip, SN_DATARATE_CONFIG_REG, dp_rate_idx, 8, 5);
	else
		printk(BIOS_ERR, "ERROR: valid dp rate not found");
}

static void sn65dsi86_bridge_set_bridge_active_timing(uint8_t bus,
						      uint8_t chip,
						      struct edid *edid)
{
	i2c_writeb(bus, chip, SN_CHA_ACTIVE_LINE_LENGTH_LOW_REG,
			   BRIDGE_GETLOWERBYTE(edid->mode.ha));
	i2c_writeb(bus, chip, SN_CHA_ACTIVE_LINE_LENGTH_HIGH_REG,
			   BRIDGE_GETHIGHERBYTE(edid->mode.ha));
	i2c_writeb(bus, chip, SN_CHA_VERTICAL_DISPLAY_SIZE_LOW_REG,
			   BRIDGE_GETLOWERBYTE(edid->mode.va));
	i2c_writeb(bus, chip, SN_CHA_VERTICAL_DISPLAY_SIZE_HIGH_REG,
			   BRIDGE_GETHIGHERBYTE(edid->mode.va));
	i2c_writeb(bus, chip, SN_CHA_HSYNC_PULSE_WIDTH_LOW_REG,
			   BRIDGE_GETLOWERBYTE(edid->mode.hspw));
	i2c_writeb(bus, chip, SN_CHA_HSYNC_PULSE_WIDTH_HIGH_REG,
			   BRIDGE_GETHIGHERBYTE(edid->mode.hspw));
	i2c_writeb(bus, chip, SN_CHA_VSYNC_PULSE_WIDTH_LOW_REG,
			   BRIDGE_GETLOWERBYTE(edid->mode.vspw));
	i2c_writeb(bus, chip, SN_CHA_VSYNC_PULSE_WIDTH_HIGH_REG,
			   BRIDGE_GETHIGHERBYTE(edid->mode.vspw));
	i2c_writeb(bus, chip, SN_CHA_HORIZONTAL_BACK_PORCH_REG,
			   edid->mode.hbl - edid->mode.hso - edid->mode.hspw);
	i2c_writeb(bus, chip, SN_CHA_VERTICAL_BACK_PORCH_REG,
			   edid->mode.vbl - edid->mode.vso - edid->mode.vspw);
	i2c_writeb(bus, chip, SN_CHA_HORIZONTAL_FRONT_PORCH_REG,
			   edid->mode.hso);
	i2c_writeb(bus, chip, SN_CHA_VERTICAL_FRONT_PORCH_REG,
			   edid->mode.vso);
}

static void sn65dsi86_bridge_link_training(uint8_t bus, uint8_t chip)
{
	uint8_t buf;

	/* enable pll lock */
	i2c_writeb(bus, chip, SN_PLL_ENABLE_REG, 0x1);

	if (!wait_ms(500,
	    !(i2c_readb(bus, chip, SN_DPPLL_SRC_REG, &buf)) &&
	    (buf & BIT(7)))) {
		printk(BIOS_ERR, "ERROR: PLL lock failure\n");
	}

	/*
	 * The SN65DSI86 only supports ASSR Display Authentication method and
	 * this method is enabled by default. An eDP panel must support this
	 * authentication method. We need to enable this method in the eDP panel
	 * at DisplayPort address 0x0010A prior to link training.
	 */
	buf = 0x1;
	sn65dsi86_bridge_dpcd_request(bus, chip,
				      DP_BRIDGE_CONFIGURATION_SET, 1, DPCD_WRITE, &buf);

	int i;	/* Kernel driver suggests to retry this up to 10 times if it fails. */
	for (i = 0; i < 10; i++) {
		i2c_writeb(bus, chip, SN_ML_TX_MODE_REG, SEMI_AUTO_LINK_TRAINING);

		if (!wait_ms(500, !(i2c_readb(bus, chip, SN_ML_TX_MODE_REG, &buf)) &&
				  (buf == NORMAL_MODE || buf == MAIN_LINK_OFF))) {
			printk(BIOS_ERR, "ERROR: unexpected link training state: %#x\n", buf);
			return;
		}
		if (buf == NORMAL_MODE)
			return;
	}

	printk(BIOS_ERR, "ERROR: Link training failed 10 times\n");
}

static enum cb_err sn65dsi86_bridge_get_plug_in_status(uint8_t bus, uint8_t chip)
{
	int val;
	uint8_t buf;

	val = i2c_readb(bus, chip, SN_HPD_DISABLE_REG, &buf);
	if (val == 0 && (buf & HPD_DISABLE))
		return CB_SUCCESS;

	return CB_ERR;
}

/*
 * support bridge HPD function some hardware versions do not support bridge hdp,
 * we use 360ms to try to get the hpd single now, if we can not get bridge hpd single,
 * it will delay 360ms, also meet the bridge power timing request, to be compatible
 * all of the hardware versions
 */
static void sn65dsi86_bridge_wait_hpd(uint8_t bus, uint8_t chip)
{
	if (wait_ms(400, sn65dsi86_bridge_get_plug_in_status(bus, chip)))
		return;

	printk(BIOS_WARNING, "HPD detection failed, force hpd\n");

	/* Force HPD */
	i2c_write_field(bus, chip, SN_HPD_DISABLE_REG, HPD_DISABLE, 1, 0);
}

static void sn65dsi86_bridge_assr_config(uint8_t bus, uint8_t chip, int enable)
{
	if (enable)
		i2c_write_field(bus, chip, SN_ENH_FRAME_REG, VSTREAM_ENABLE, 1, 3);
	else
		i2c_write_field(bus, chip, SN_ENH_FRAME_REG, VSTREAM_DISABLE, 1, 3);
}

static int sn65dsi86_bridge_dp_lane_config(uint8_t bus, uint8_t chip)
{
	uint8_t lane_count;

	sn65dsi86_bridge_dpcd_request(bus, chip, DP_MAX_LANE_COUNT, 1, DPCD_READ, &lane_count);
	lane_count &= DP_LANE_COUNT_MASK;
	i2c_write_field(bus, chip, SN_SSC_CONFIG_REG, MIN(lane_count, 3), 3, 4);

	return lane_count;
}

void sn65dsi86_bridge_init(uint8_t bus, uint8_t chip, enum dp_pll_clk_src ref_clk)
{
	sn65dsi86_bridge_wait_hpd(bus, chip);

	/* set refclk to 19.2 MHZ */
	i2c_write_field(bus, chip, SN_DPPLL_SRC_REG, ref_clk, 7, 1);
}

void sn65dsi86_bridge_configure(uint8_t bus, uint8_t chip,
				struct edid *edid, uint32_t num_of_lanes,
				uint32_t dsi_bpp)
{
	int dp_lanes;

	/* DSI Lanes config */
	i2c_write_field(bus, chip, SN_DSI_LANES_REG, (4 - num_of_lanes), 3, 3);

	/* DP Lane config */
	dp_lanes = sn65dsi86_bridge_dp_lane_config(bus, chip);

	sn65dsi86_bridge_set_dsi_clock_range(bus, chip, edid, num_of_lanes, dsi_bpp);

	sn65dsi86_bridge_set_dp_clock_range(bus, chip, edid, dp_lanes);

	/* Disable vstream */
	sn65dsi86_bridge_assr_config(bus, chip, 0);
	sn65dsi86_bridge_link_training(bus, chip);
	sn65dsi86_bridge_set_bridge_active_timing(bus, chip, edid);

	/* DP BPP config */
	i2c_writeb(bus, chip, SN_DATA_FORMAT_REG, 0x1);

	/* color bar disabled */
	i2c_writeb(bus, chip, SN_COLOR_BAR_REG, 0x5);

	/* Enable vstream */
	sn65dsi86_bridge_assr_config(bus, chip, 1);
}
