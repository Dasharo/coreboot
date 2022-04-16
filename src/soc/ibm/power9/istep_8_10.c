/* SPDX-License-Identifier: GPL-2.0-only */

#include <cpu/power/istep_8.h>

#include <console/console.h>
#include <cpu/power/scom.h>
#include <delay.h>

#include "xbus.h"

static void xbus_scom(uint8_t chip, uint8_t group)
{
	/* ATTR_IO_XBUS_CHAN_EQ is 0 by default and Hostboot logs seem to confirm this*/

	/* ATTR_IO_XBUS_MASTER_MODE */
	const bool xbus_master_mode = (chip == 0);
	/*
	 * Offset for group.
	 *
	 * Note that several registers are initialized for both groups and don't
	 * use the offset.  Some other writes are group-specific and don't need
	 * it either.
	 */
	const uint64_t offset = group * XBUS_LINK_GROUP_OFFSET;

	int i;

	/* *_RX_DATA_DAC_SPARE_MODE_PL */
	for (i = 0; i < 18; i++) {
		uint64_t addr = xbus_addr(0x8000000006010C3F + offset + 0x100000000 * i);
		// 53 - *_RX_DAC_REGS_RX_DAC_REGS_RX_PL_DATA_DAC_SPARE_MODE_5_OFF
		// 54 - *_RX_DAC_REGS_RX_DAC_REGS_RX_PL_DATA_DAC_SPARE_MODE_6_OFF
		// 55 - *_RX_DAC_REGS_RX_DAC_REGS_RX_PL_DATA_DAC_SPARE_MODE_7_OFF
		scom_and(chip, addr, ~PPC_BITMASK(53, 55));
	}

	/* *_RX_DAC_CNTL1_EO_PL */
	for (i = 0; i < 18; i++) {
		uint64_t addr = xbus_addr(0x8000080006010C3F + offset + 0x100000000 * i);
		// 54 - *_RX_DAC_REGS_RX_DAC_REGS_RX_LANE_ANA_PDWN_{OFF,ON}
		if (i < 17)
			scom_and(chip, addr, ~PPC_BIT(54));
		else
			scom_or(chip, addr, PPC_BIT(54));
	}

	/* *_RX_DAC_CNTL5_EO_PL */
	for (i = 0; i < 18; i++) {
		uint64_t addr = xbus_addr(0x8000280006010C3F + offset + 0x100000000 * i);
		scom_and(chip, addr,
			 ~(PPC_BITMASK(48, 51) | PPC_BITMASK(52, 56) | PPC_BITMASK(57, 61)));
	}

	/* *_RX_DAC_CNTL6_EO_PL */
	for (i = 0; i < 18; i++) {
		uint64_t addr = xbus_addr(0x8000300006010C3F + offset + 0x100000000 * i);
		scom_and_or(chip, addr,
			    ~(PPC_BITMASK(53, 56) | PPC_BITMASK(48, 52)),
			    PPC_PLACE(0x7, 53, 4) | PPC_PLACE(0x0C, 48, 5));
	}

	/* *_RX_DAC_CNTL9_E_PL */
	for (i = 0; i < 18; i++) {
		uint64_t addr = xbus_addr(0x8000C00006010C3F + offset + 0x100000000 * i);
		scom_and(chip, addr, ~(PPC_BITMASK(48, 54) | PPC_BITMASK(55, 60)));
	}

	/* *_RX_BIT_MODE1_EO_PL */
	for (i = 0; i < 18; i++) {
		uint64_t addr = xbus_addr(0x8002200006010C3F + offset + 0x100000000 * i);
		// 48 - *_RX_BIT_REGS_RX_LANE_DIG_PDWN_{OFF,ON}
		if (i < 17)
			scom_and(chip, addr, ~PPC_BIT(48));
		else
			scom_or(chip, addr, PPC_BIT(48));
	}

	/* *_RX_BIT_MODE1_E_PL */
	for (i = 0; i < 17; i++) {
		uint64_t addr = xbus_addr(0x8002C00006010C3F + offset + 0x100000000 * i);
		const uint16_t data[17] = {
			0x1000, 0xF03E, 0x07BC, 0x07C7, 0x03EF, 0x1F0F, 0x1800, 0x9C00,
			0x1000, 0x9C00, 0x1800, 0x1F0F, 0x03EF, 0x07C7, 0x07BC, 0xF03E,
			0x1000
		};
		scom_and_or(chip, addr, ~PPC_BITMASK(48, 63), PPC_PLACE(data[i], 48, 16));
	}

	/* *_RX_BIT_MODE2_E_PL */
	for (i = 0; i < 17; i++) {
		uint64_t addr = xbus_addr(0x8002C80006010C3F + offset + 0x100000000 * i);
		const uint8_t data[17] = {
			0x42, 0x3E, 0x00, 0x60, 0x40, 0x40, 0x03, 0x03,
			0x42, 0x03, 0x03, 0x40, 0x40, 0x60, 0x00, 0x3E,
			0x42
		};
		scom_and_or(chip, addr, ~PPC_BITMASK(48, 54), PPC_PLACE(data[i], 48, 7));
	}

	/* *_TX_MODE1_PL */
	for (i = 0; i < 17; i++) {
		uint64_t addr = xbus_addr(0x8004040006010C3F + offset + 0x100000000 * i);
		scom_and(chip, addr, ~PPC_BIT(48));
	}

	/* *_TX_MODE2_PL */
	for (i = 0; i < 17; i++) {
		uint64_t addr = xbus_addr(0x80040C0006010C3F + offset + 0x100000000 * i);
		scom_or(chip, addr, PPC_BIT(62));
	}

	/* *_TX_BIT_MODE1_E_PL */
	for (i = 0; i < 17; i++) {
		uint64_t addr = xbus_addr(0x80043C0006010C3F + offset + 0x100000000 * i);
		const uint16_t data[17] = {
			0x000, 0x000, 0x01E, 0x01F, 0x00F, 0x07C, 0xC63, 0xE73,
			0x000, 0xE73, 0xC63, 0x07C, 0x00F, 0x01F, 0x01E, 0x000,
			0x000,
		};
		scom_and_or(chip, addr, ~PPC_BITMASK(48, 63), PPC_PLACE(data[i], 48, 16));
	}

	/* *_TX_BIT_MODE2_E_PL */
	for (i = 0; i < 17; i++) {
		uint64_t addr = xbus_addr(0x8004440006010C3F + offset + 0x100000000 * i);
		const uint8_t data[17] = {
			0x01, 0x7C, 0x7B, 0x0C, 0x5E, 0x10, 0x0C, 0x4E,
			0x01, 0x4E, 0x0C, 0x10, 0x5E, 0x0C, 0x7B, 0x7C,
			0x01,
		};
		scom_and_or(chip, addr, ~PPC_BITMASK(48, 54), PPC_PLACE(data[i], 48, 7));
	}

	// P9A_XBUS_0_RX[01]_RX_SPARE_MODE_PG
	// 49 - IOF1_RX_RX0_RXCTL_CTL_REGS_RX_CTL_REGS_RX_PG_SPARE_MODE_1_ON
	scom_or(chip, xbus_addr(0x8008000006010C3F + offset), PPC_BIT(49));

	// P9A_XBUS_0_RX[01]_RX_ID1_PG
	scom_and_or(chip, xbus_addr(0x8008080006010C3F + offset),
		    ~PPC_BITMASK(48, 53),
		    PPC_PLACE((group == 0 ? 0x00 : 0x01), 48, 6));

	// P9A_XBUS_0_RX[01]_RX_CTL_MODE1_EO_PG
	// 48 - IOF1_RX_RX0_RXCTL_CTL_REGS_RX_CTL_REGS_RX_CLKDIST_PDWN_OFF
	scom_and(chip, xbus_addr(0x8008100006010C3F + offset), ~PPC_BIT(48));

	// P9A_XBUS_0_RX[01]_RX_CTL_MODE5_EO_PG
	// 51-53 - IOF1_RX_RX0_RXCTL_CTL_REGS_RX_CTL_REGS_RX_DYN_RECAL_INTERVAL_TIMEOUT_SEL_TAP5
	// 54-55 - IOF1_RX_RX0_RXCTL_CTL_REGS_RX_CTL_REGS_RX_DYN_RECAL_STATUS_RPT_TIMEOUT_SEL_TAP1
	scom_and_or(chip, xbus_addr(0x8008300006010C3F + offset),
		    ~(PPC_BITMASK(51, 53) | PPC_BITMASK(54, 55)),
		    PPC_PLACE(0x5, 51, 3) | PPC_PLACE(0x1, 54, 2));

	// P9A_XBUS_0_RX[01]_RX_CTL_MODE7_EO_PG
	scom_and_or(chip, xbus_addr(0x8008400006010C3F + offset),
		    ~PPC_BITMASK(60, 63),
		    PPC_PLACE(0xA, 60, 4));

	// P9A_XBUS_0_RX0_RX_CTL_MODE23_EO_PG (same address for both groups)
	// 55 - IOF1_RX_RX0_RXCTL_CTL_REGS_RX_CTL_REGS_RX_PEAK_TUNE_OFF
	// 56 - IOF1_RX_RX0_RXCTL_CTL_REGS_RX_CTL_REGS_RX_LTE_EN_ON
	// 59 - IOF1_RX_RX0_RXCTL_CTL_REGS_RX_CTL_REGS_RX_DFEHISPD_EN_ON
	// 60 - IOF1_RX_RX0_RXCTL_CTL_REGS_RX_CTL_REGS_RX_DFE12_EN_ON
	if (group == 0) {
		scom_and_or(chip, xbus_addr(0x8008C00006010C3F),
			~(PPC_BITMASK(48, 49) | PPC_BITMASK(55, 60)),
			PPC_PLACE(0x1, 48, 2) | PPC_BIT(56) | PPC_PLACE(0x3, 57, 2) |
			PPC_BIT(59) | PPC_BIT(60));
	} else {
		scom_and_or(chip, xbus_addr(0x8008C00006010C3F),
			    ~PPC_BITMASK(48, 49), PPC_PLACE(0x1, 48, 2));
	}

	// P9A_XBUS_0_RX0_RX_CTL_MODE23_EO_PG
	// 55 - IOF1_RX_RX1_RXCTL_CTL_REGS_RX_CTL_REGS_RX_PEAK_TUNE_OFF
	// 56 - IOF1_RX_RX1_RXCTL_CTL_REGS_RX_CTL_REGS_RX_LTE_EN_ON
	// 57 - 0b11
	// 59 - IOF1_RX_RX1_RXCTL_CTL_REGS_RX_CTL_REGS_RX_DFEHISPD_EN_ON
	// 60 - IOF1_RX_RX1_RXCTL_CTL_REGS_RX_CTL_REGS_RX_DFE12_EN_ON
	if (group == 1) {
		scom_and_or(chip, xbus_addr(0x8008C02006010C3F),
			    ~PPC_BITMASK(55, 60),
			    PPC_BIT(56) | PPC_PLACE(0x3, 57, 2) | PPC_BIT(59) | PPC_BIT(60));
	}

	// P9A_XBUS_0_RX0_RX_CTL_MODE29_EO_PG (identical for both groups)
	scom_and_or(chip, xbus_addr(0x8008D00006010C3F + offset),
		    ~(PPC_BITMASK(48, 55) | PPC_BITMASK(56, 63)),
		    PPC_PLACE(0x66, 48, 8) | PPC_PLACE(0x44, 56, 8));

	// P9A_XBUS_0_RX[01]_RX_CTL_MODE27_EO_PG
	// 48 - IOF1_RX_RX0_RXCTL_CTL_REGS_RX_CTL_REGS_RX_RC_ENABLE_CTLE_1ST_LATCH_OFFSET_CAL_ON
	scom_or(chip, xbus_addr(0x8009700006010C3F + offset), PPC_BIT(48));

	// P9A_XBUS_0_RX[01]_RX_ID2_PG
	scom_and_or(chip, xbus_addr(0x8009800006010C3F + offset),
		    ~(PPC_BITMASK(49, 55) | PPC_BITMASK(57, 63)),
		    PPC_PLACE(0x00, 49, 7) | PPC_PLACE(0x10, 57, 7));

	// P9A_XBUS_0_RX[01]_RX_CTL_MODE1_E_PG
	// 48 - IOF1_RX_RX0_RXCTL_CTL_REGS_RX_CTL_REGS_RX_MASTER_MODE_MASTER
	// 57 - IOF1_RX_RX0_RXCTL_CTL_REGS_RX_CTL_REGS_RX_FENCE_FENCED
	// 58 - IOF1_RX_RX0_RXCTL_CTL_REGS_RX_CTL_REGS_RX_PDWN_LITE_DISABLE_ON
	scom_or(chip, xbus_addr(0x8009900006010C3F + offset),
		(xbus_master_mode ? PPC_BIT(48) : 0) | PPC_BIT(57) | PPC_BIT(58));

	// P9A_XBUS_0_RX[01]_RX_CTL_MODE2_E_PG
	scom_and_or(chip, xbus_addr(0x8009980006010C3F + offset),
		    ~PPC_BITMASK(48, 52), PPC_PLACE(0x01, 48, 5));

	// P9A_XBUS_0_RX[01]_RX_CTL_MODE3_E_PG
	scom_and_or(chip, xbus_addr(0x8009A00006010C3F + offset),
		    ~PPC_BITMASK(48, 51), PPC_PLACE(0xB, 48, 4));

	// P9A_XBUS_0_RX[01]_RX_CTL_MODE5_E_PG
	scom_and_or(chip, xbus_addr(0x8009B00006010C3F + offset),
		    ~PPC_BITMASK(52, 55), PPC_PLACE(0x1, 52, 4));

	// P9A_XBUS_0_RX[01]_RX_CTL_MODE6_E_PG
	scom_and_or(chip, xbus_addr(0x8009B80006010C3F + offset),
		    ~(PPC_BITMASK(48, 54) | PPC_BITMASK(55, 61)),
		    PPC_PLACE(0x11, 48, 7) | PPC_PLACE(0x11, 55, 7));

	// P9A_XBUS_0_RX[01]_RX_CTL_MODE8_E_PG
	// 55-58 - IOF1_RX_RX0_RXCTL_CTL_REGS_RX_CTL_REGS_RX_DYN_RPR_ERR_CNTR1_DURATION_TAP5
	scom_and_or(chip, xbus_addr(0x8009C80006010C3F + offset),
		    ~(PPC_BITMASK(48, 54) | PPC_BITMASK(55, 58) | PPC_BITMASK(61, 63)),
		    PPC_PLACE(0xF, 48, 7) | PPC_PLACE(0x5, 55, 4) | PPC_PLACE(0x5, 61, 3));

	// P9A_XBUS_0_RX[01]_RX_CTL_MODE9_E_PG
	// 55-58 - IOF1_RX_RX0_RXCTL_CTL_REGS_RX_CTL_REGS_RX_DYN_RPR_ERR_CNTR2_DURATION_TAP5
	scom_and_or(chip, xbus_addr(0x8009D00006010C3F + offset),
		    ~(PPC_BITMASK(48, 54) | PPC_BITMASK(55, 58)),
		    PPC_PLACE(0x3F, 48, 7) | PPC_PLACE(0x5, 55, 4));

	// P9A_XBUS_0_RX[01]_RX_CTL_MODE11_E_PG
	scom_and(chip, xbus_addr(0x8009E00006010C3F + offset), ~PPC_BITMASK(48, 63));

	// P9A_XBUS_0_RX[01]_RX_CTL_MODE12_E_PG
	scom_and_or(chip, xbus_addr(0x8009E80006010C3F + offset),
		    ~PPC_BITMASK(48, 55), PPC_PLACE(0x7F, 48, 8));

	// P9A_XBUS_0_RX[01]_RX_GLBSM_SPARE_MODE_PG
	// 50 - IOF1_RX_RX0_RXCTL_GLBSM_REGS_RX_PG_GLBSM_SPARE_MODE_2_ON
	// 56 - IOF1_RX_RX0_RXCTL_GLBSM_REGS_RX_DESKEW_BUMP_AFTER_AFTER
	scom_or(chip, xbus_addr(0x800A800006010C3F + offset), PPC_BIT(50) | PPC_BIT(56));

	// P9A_XBUS_0_RX[01]_RX_GLBSM_CNTL3_EO_PG
	scom_and_or(chip, xbus_addr(0x800AE80006010C3F + offset),
		    ~PPC_BITMASK(56, 57), PPC_PLACE(0x2, 56, 2));

	// P9A_XBUS_0_RX[01]_RX_GLBSM_MODE1_EO_PG
	scom_and_or(chip, xbus_addr(0x800AF80006010C3F + offset),
		    ~(PPC_BITMASK(48, 51) | PPC_BITMASK(52, 55)),
		    PPC_PLACE(0xC, 48, 4) | PPC_PLACE(0xC, 52, 4));

	// P9A_XBUS_0_RX[01]_RX_DATASM_SPARE_MODE_PG
	// 60 - IOF1_RX_RX0_RXCTL_DATASM_DATASM_REGS_RX_CTL_DATASM_CLKDIST_PDWN_OFF
	scom_and(chip, xbus_addr(0x800B800006010C3F + offset), ~PPC_BIT(60));

	// P9A_XBUS_0_TX[01]_TX_SPARE_MODE_PG
	scom_and(chip, xbus_addr(0x800C040006010C3F + offset), ~PPC_BITMASK(56, 57));

	// P9A_XBUS_0_TX[01]_TX_ID1_PG
	scom_and_or(chip, xbus_addr(0x800C0C0006010C3F + offset),
		    ~PPC_BITMASK(48, 53),
		    PPC_PLACE((group == 0 ? 0x00 : 0x01), 48, 6));

	// P9A_XBUS_0_TX[01]_TX_CTL_MODE1_EO_PG
	// 48 - IOF1_TX_WRAP_TX0_TXCTL_CTL_REGS_TX_CTL_REGS_TX_CLKDIST_PDWN_OFF
	// 59 - IOF1_TX_WRAP_TX0_TXCTL_CTL_REGS_TX_CTL_REGS_TX_PDWN_LITE_DISABLE_ON
	scom_and_or(chip, xbus_addr(0x800C140006010C3F + offset),
		    ~(PPC_BIT(48) | PPC_BITMASK(53, 57) | PPC_BIT(59)),
		    PPC_PLACE(0x01, 53, 5) | PPC_BIT(59));

	// P9A_XBUS_0_TX[01]_TX_CTL_MODE2_EO_PG
	scom_and_or(chip, xbus_addr(0x800C1C0006010C3F + offset),
		    ~PPC_BITMASK(56, 62), PPC_PLACE(0x11, 56, 7));

	// P9A_XBUS_0_TX[01]_TX_CTL_CNTLG1_EO_PG
	// 48-49 - IOF1_TX_WRAP_TX0_TXCTL_CTL_REGS_TX_CTL_REGS_TX_DRV_CLK_PATTERN_GCRMSG_DRV_0S
	scom_and(chip, xbus_addr(0x800C240006010C3F + offset), ~PPC_BITMASK(48, 49));

	// P9A_XBUS_0_TX[01]_TX_ID2_PG
	scom_and_or(chip, xbus_addr(0x800C840006010C3F + offset),
		    ~(PPC_BITMASK(49, 55) | PPC_BITMASK(57, 63)),
		    PPC_PLACE(0x0, 49, 7) | PPC_PLACE(0x10, 57, 7));

	// P9A_XBUS_0_TX[01]_TX_CTL_MODE1_E_PG
	// 55-57 - IOF1_TX_WRAP_TX0_TXCTL_CTL_REGS_TX_CTL_REGS_TX_DYN_RECAL_INTERVAL_TIMEOUT_SEL_TAP5
	// 58-59 - IOF1_TX_WRAP_TX0_TXCTL_CTL_REGS_TX_CTL_REGS_TX_DYN_RECAL_STATUS_RPT_TIMEOUT_SEL_TAP1
	scom_and_or(chip, xbus_addr(0x800C8C0006010C3F + offset),
		    ~(PPC_BITMASK(55, 57) | PPC_BITMASK(58, 59)),
		    PPC_PLACE(0x5, 55, 3) | PPC_PLACE(0x1, 58, 2));

	// P9A_XBUS_0_TX[01]_TX_CTL_MODE2_E_PG
	scom_and(chip, xbus_addr(0x800CEC0006010C3F + offset), ~PPC_BITMASK(48, 63));

	// P9A_XBUS_0_TX[01]_TX_CTL_MODE3_E_PG
	scom_and_or(chip, xbus_addr(0x800CF40006010C3F + offset),
		    ~PPC_BITMASK(48, 55), PPC_PLACE(0x7F, 48, 8));

	// P9A_XBUS_0_TX[01]_TX_CTLSM_MODE1_EO_PG
	// 59 - IOF1_TX_WRAP_TX0_TXCTL_TX_CTL_SM_REGS_TX_FFE_BOOST_EN_ON
	scom_and(chip, xbus_addr(0x800D2C0006010C3F + offset), PPC_BIT(59));

	// P9A_XBUS_0_TX_IMPCAL_P_4X_PB (identical for both groups)
	scom_and_or(chip, xbus_addr(0x800F1C0006010C3F),
		    ~PPC_BITMASK(48, 54), PPC_PLACE(0x0E, 48, 5));
}

static void set_msb_swap(uint8_t chip, int group)
{
	enum {
		TX_CTL_MODE1_EO_PG = 0x800C140006010C3F,
		EDIP_TX_MSBSWAP = 58,
	};

	const uint64_t addr = xbus_addr(TX_CTL_MODE1_EO_PG + group * XBUS_LINK_GROUP_OFFSET);

	/* ATTR_EI_BUS_TX_MSBSWAP seems to be 0x80 which is GROUP_0_SWAP */
	if (group == 0)
		scom_or(chip, addr, PPC_BIT(EDIP_TX_MSBSWAP));
	else
		scom_and(chip, addr, ~PPC_BIT(EDIP_TX_MSBSWAP));
}

static void xbus_scominit(int group)
{
	enum {
		PU_PB_CENT_SM0_PB_CENT_FIR_REG = 0x05011C00,

		XBUS_PHY_FIR_ACTION0 = 0x0000000000000000ULL,
		XBUS_FIR_ACTION0_REG = 0x06010C06,
		XBUS_PHY_FIR_ACTION1 = 0x2068680000000000ULL,
		XBUS_FIR_ACTION1_REG = 0x06010C07,
		XBUS_PHY_FIR_MASK    = 0xDF9797FFFFFFC000ULL,
		XBUS_FIR_MASK_REG    = 0x06010C03,

		EDIP_RX_IORESET = 0x8009F80006010C3F,
		EDIP_TX_IORESET = 0x800C9C0006010C3F,
	};

	const uint64_t offset = group * XBUS_LINK_GROUP_OFFSET;

	/* Assert IO reset to power-up bus endpoint logic */
	scom_or(0, xbus_addr(EDIP_RX_IORESET + offset), PPC_BIT(52));
	scom_or(1, xbus_addr(EDIP_RX_IORESET + offset), PPC_BIT(52));
	udelay(50);
	scom_or(0, xbus_addr(EDIP_TX_IORESET + offset), PPC_BIT(48));
	scom_or(1, xbus_addr(EDIP_TX_IORESET + offset), PPC_BIT(48));
	udelay(50);

	set_msb_swap(/*chip=*/0, group);
	set_msb_swap(/*chip=*/1, group);

	xbus_scom(/*chip=*/0, group);
	xbus_scom(/*chip=*/1, group);

	/* PU_PB_CENT_SM0_PB_CENT_FIR_MASK_REG_SPARE_13 */
	if (!(read_scom(/*chip=*/0, PU_PB_CENT_SM0_PB_CENT_FIR_REG) & PPC_BIT(13))) {
		write_scom(/*chip=*/0, xbus_addr(XBUS_FIR_ACTION0_REG), XBUS_PHY_FIR_ACTION0);
		write_scom(/*chip=*/0, xbus_addr(XBUS_FIR_ACTION1_REG), XBUS_PHY_FIR_ACTION1);
		write_scom(/*chip=*/0, xbus_addr(XBUS_FIR_MASK_REG), XBUS_PHY_FIR_MASK);
	}
}

void istep_8_10(uint8_t chips)
{
	printk(BIOS_EMERG, "starting istep 8.10\n");
	report_istep(8,10);

	if (chips != 0x01) {
		xbus_scominit(/*group=*/0);
		xbus_scominit(/*group=*/1);
	}

	printk(BIOS_EMERG, "ending istep 8.10\n");
}
