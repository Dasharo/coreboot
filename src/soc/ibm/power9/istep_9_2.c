/* SPDX-License-Identifier: GPL-2.0-only */

#include <cpu/power/istep_9.h>

#include <console/console.h>
#include <cpu/power/scom.h>
#include <delay.h>
#include <timer.h>

#include "xbus.h"

struct edip_data_t {
	uint32_t en_margin_pu;
	uint32_t en_margin_pd;
	uint32_t en_main;
	uint32_t sel_pre;
};

static void compute_margin_data(uint32_t margin, struct edip_data_t *d)
{
	/* ATTR_IO_XBUS_TX_FFE_PRECURSOR = 6 (default, talos.xml) */
	const uint8_t ffe_pre_coef = 6;

	/* Need to convert the 8R value to a 4R equivalent */

	const uint32_t val = margin >> 1;
	const uint32_t en_pre = 18;

	uint32_t val_4r = val - en_pre;

	d->en_margin_pu = 32;
	d->en_margin_pd = 32;
	d->en_main = 0;
	d->sel_pre = 0;

	if (val_4r < 64) {
		if (val_4r % 4 != 0) {
			d->en_main = 2;
			val_4r -= d->en_main;
		}
		d->en_margin_pd = val_4r / 2;
		d->en_margin_pu = val_4r - d->en_margin_pd;
	}

	d->en_main += val_4r - d->en_margin_pu - d->en_margin_pd;
	d->en_main = MIN(d->en_main, 50);
	d->sel_pre = (val * ffe_pre_coef) / 128;
	d->sel_pre = MIN(d->sel_pre, en_pre);
}

/* Converts a 4R decimal value to a 1R thermometer code */
static uint32_t convert_4r(uint32_t val_4r)
{
	/*
	 * 1. Add 2 for averaging since we will truncate the last 2 bits.
	 * 2. Divide by 4 to bring back to a 1r value.
	 * 3. Convert the decimal number to number of bits set by shifting a 0x1
	 *    over by the amount and subtracting 1.
	 */
	return (0x1 << ((val_4r + 2) / 4)) - 1;
}

static uint32_t convert_4r_with_2r(uint32_t val_4r, uint8_t width)
{
	/* Add 1 for rounding, then shift the 4r bit off. We now have a 2r equivalent */
	const uint32_t val_2r = (val_4r + 1) >> 1;

	/* If the LSB of the 2r equivalent is on, then we need to set the 2r bit (MSB) */
	const uint32_t on_2r = val_2r & 0x1;

	/* Shift the 2r equivalent to a 1r value and convert to a thermometer code */
	const uint32_t val_1r = (1 << (val_2r >> 0x1)) - 1;

	/* Combine 1r equivalent thermometer code + the 2r MSB value */
	return (on_2r << (width - 1)) | val_1r;
}

static void config_run_bus_group_mode(uint8_t chip, int group)
{
	enum {
		P9A_XBUS_TX_IMPCAL_PVAL_PB = 0x800F140006010C3F,
		P9A_XBUS_TX_IMPCAL_NVAL_PB = 0x800F0C0006010C3F,
	};

	/* ATTR_IO_XBUS_TX_MARGIN_RATIO = 0 (default) */
	const uint8_t margin_ratio = 0;

	const uint8_t PRE_WIDTH     = 5;
	/*             4R Total     = (1R * 4) + (2R * 2); */
	const uint32_t PRE_4R_TOTAL = ( 4 * 4) + ( 1 * 2);

	const uint64_t offset = group * XBUS_LINK_GROUP_OFFSET;

	/* Same registers are read for both groups */
	uint32_t pval = (get_scom(chip, P9A_XBUS_TX_IMPCAL_PVAL_PB) >> 7) & 0x1FF;
	uint32_t nval = (get_scom(chip, P9A_XBUS_TX_IMPCAL_NVAL_PB) >> 7) & 0x1FF;

	uint32_t sel_margin_pu;
	uint32_t sel_margin_pd;

	struct edip_data_t p;
	struct edip_data_t n;

	uint64_t val;

	compute_margin_data(pval, &p);
	compute_margin_data(nval, &n);

	sel_margin_pu = (pval * margin_ratio) / 256;
	sel_margin_pu = MIN(sel_margin_pu, MIN(p.en_margin_pu, n.en_margin_pu));

	sel_margin_pd = (nval * margin_ratio) / 256;
	sel_margin_pd = MIN(sel_margin_pd,
			    MIN(p.en_margin_pd, MIN(n.en_margin_pd, sel_margin_pu)));

	val = get_scom(chip, 0x800D340006010C3F + offset);

	/* EDIP_TX_PSEG_PRE_EN (pre bank pseg enable) */
	PPC_INSERT(val, convert_4r_with_2r(PRE_4R_TOTAL, PRE_WIDTH), 51, 5);
	/* EDIP_TX_PSEG_PRE_SEL (pre bank pseg mode selection) */
	PPC_INSERT(val, convert_4r_with_2r(p.sel_pre, PRE_WIDTH), 56, 5);

	put_scom(chip, 0x800D340006010C3F + offset, val);
	val = get_scom(chip, 0x800D3C0006010C3F + offset);

	/* EDIP_TX_NSEG_PRE_EN (pre bank nseg enable) */
	PPC_INSERT(val, convert_4r_with_2r(PRE_4R_TOTAL, PRE_WIDTH), 51, 5);
	/* EDIP_TX_NSEG_PRE_SEL (pre bank nseg mode selection) */
	PPC_INSERT(val, convert_4r_with_2r(n.sel_pre, PRE_WIDTH), 56, 5);

	put_scom(chip, 0x800D3C0006010C3F + offset, val);
	val = get_scom(chip, 0x800D440006010C3F + offset);

	/* EDIP_TX_PSEG_MARGINPD_EN (margin pull-down bank pseg enable) */
	PPC_INSERT(val, convert_4r(p.en_margin_pd), 56, 8);
	/* EDIP_TX_PSEG_MARGINPU_EN (margin pull-up bank pseg enable) */
	PPC_INSERT(val, convert_4r(p.en_margin_pu), 48, 8);

	put_scom(chip, 0x800D440006010C3F + offset, val);
	val = get_scom(chip, 0x800D4C0006010C3F + offset);

	/* EDIP_TX_NSEG_MARGINPD_EN (margin pull-down bank nseg enable) */
	PPC_INSERT(val, convert_4r(n.en_margin_pd), 56, 8);
	/* EDIP_TX_NSEG_MARGINPU_EN (margin pull-up bank nseg enable) */
	PPC_INSERT(val, convert_4r(n.en_margin_pu), 48, 8);

	put_scom(chip, 0x800D4C0006010C3F + offset, val);
	val = get_scom(chip, 0x800D540006010C3F + offset);

	/* EDIP_TX_MARGINPD_SEL (margin pull-down bank mode selection) */
	PPC_INSERT(val, convert_4r(sel_margin_pd), 56, 8);
	/* EDIP_TX_MARGINPU_SEL (margin pull-up bank mode selection) */
	PPC_INSERT(val, convert_4r(sel_margin_pu), 48, 8);

	put_scom(chip, 0x800D540006010C3F + offset, val);

	/* EDIP_TX_PSEG_MAIN_EN (main bank pseg enable) */
	val = get_scom(chip, 0x800D5C0006010C3F + offset);
	PPC_INSERT(val, convert_4r_with_2r(p.en_main, 13), 51, 13);
	put_scom(chip, 0x800D5C0006010C3F + offset, val);

	/* EDIP_TX_NSEG_MAIN_EN (main bank nseg enable) */
	val = get_scom(chip, 0x800D640006010C3F + offset);
	PPC_INSERT(val, convert_4r_with_2r(n.en_main, 13), 51, 13);
	put_scom(chip, 0x800D640006010C3F + offset, val);
}

static void config_run_bus_mode(uint8_t chip)
{
	enum {
		P9A_XBUS_TX_IMPCAL_PB = 0x800F040006010C3F,
		EDIP_TX_ZCAL_DONE = 50,
		EDIP_TX_ZCAL_ERROR = 51,
	};

	long time;

	/* Set EDIP_TX_ZCAL_REQ to start Tx Impedance Calibration */
	or_scom(chip, P9A_XBUS_TX_IMPCAL_PB, PPC_BIT(49));
	mdelay(20);

	time = wait_us(200 * 10, get_scom(chip, P9A_XBUS_TX_IMPCAL_PB) &
		       (PPC_BIT(EDIP_TX_ZCAL_DONE) | PPC_BIT(EDIP_TX_ZCAL_ERROR)));
	if (!time)
		die("Timed out waiting for I/O EDI+ Xbus Tx Z Calibration\n");

	if (get_scom(chip, P9A_XBUS_TX_IMPCAL_PB) & PPC_BIT(EDIP_TX_ZCAL_ERROR))
		die("I/O EDI+ Xbus Tx Z Calibration failed\n");

	config_run_bus_group_mode(chip, /*group=*/0);
	config_run_bus_group_mode(chip, /*group=*/1);
}

static void rx_dc_calibration_start(uint8_t chip, int group)
{
	const uint64_t offset = group * XBUS_LINK_GROUP_OFFSET;

	/* Must set lane invalid bit to 0 to run rx dccal, this enables us to
	 * run dccal on the specified lane. These bits are normally set by
	 * wiretest although we are not running that now. */
	for (int i = 0; i < XBUS_LANE_COUNT; i++) {
		uint64_t lane_offset = PPC_PLACE(i, 27, 5);
		/* EDIP_RX_LANE_INVALID */
		and_scom(chip, 0x8002400006010C3F | offset | lane_offset, ~PPC_BIT(50));
	}

	/* Start Cleanup Pll */

	/*
	 * EDIP_RX_CTL_CNTL4_E_PG
	 *  50 - EDIP_RX_WT_PLL_REFCLKSEL (0 - io clock, 1 - bist)
	 *  51 - EDIP_RX_PLL_REFCLKSEL_SCOM_EN (0 - pll controls selects refclk,
	 *                                      1 - and gcr register does it)
	 */
	or_scom(chip, 0x8009F80006010C3F + offset, PPC_BIT(50) | PPC_BIT(51));
	udelay(150);

	/*
	 * EDIP_RX_CTL_CNTL4_E_PG
	 *  48 - EDIP_RX_WT_CU_PLL_PGOOD (0 - places rx pll in reset,
	 *                                1 - sets pgood on rx pll for locking)
	 */
	or_scom(chip, 0x8009F80006010C3F + offset, PPC_BIT(48));
	udelay(5);

	/*
	 * EDIP_RX_DC_CALIBRATE_DONE
	 * (when this bit is read as a 1, the dc calibration steps have been completed)
	 */
	and_scom(chip, 0x800A380006010C3F + offset, ~PPC_BIT(53));

	/*
	 * EDIP_RX_START_DC_CALIBRATE
	 * (when this register is written to a 1 the training state machine will run the dc
	 *  calibrate substeps defined in eye optimizations)
	 */
	or_scom(chip, 0x8009F00006010C3F + offset, PPC_BIT(53));
}

static void rx_dc_calibration_poll(uint8_t chip, int group)
{
	const uint64_t offset = group * XBUS_LINK_GROUP_OFFSET;

	long time;

	/*
	 * EDIP_RX_DC_CALIBRATE_DONE
	 * (when this bit is read as a 1, the dc calibration steps have been completed)
	 */
	time = wait_ms(200 * 10, get_scom(chip, 0x800A380006010C3F + offset) & PPC_BIT(53));
	if (!time)
		die("Timed out waiting for Rx Dc Calibration\n");

	/*
	 * EDIP_RX_START_DC_CALIBRATE
	 * (when this register is written to a 1 the training state machine will run the dc
	 *  calibrate substeps defined in eye optimizations)
	 */
	and_scom(chip, 0x8009F00006010C3F + offset, ~PPC_BIT(53));

	/*
	 * EDIP_RX_CTL_CNTL4_E_PG
	 *  48 - EDIP_RX_WT_CU_PLL_PGOOD (0 - places rx pll in reset,
	 *                                1 - sets pgood on rx pll for locking)
	 *  50 - EDIP_RX_WT_PLL_REFCLKSEL (0 - io clock, 1 - bist)
	 *  51 - EDIP_RX_PLL_REFCLKSEL_SCOM_EN (0 - pll controls selects refclk,
	 *                                      1 - and gcr register does it)
	 */
	and_scom(chip, 0x8009F80006010C3F + offset, ~(PPC_BIT(48) | PPC_BIT(50) | PPC_BIT(51)));
	udelay(111);

	/* Restore the invalid bits, Wiretest will modify these as training is run */
	for (int i = 0; i < XBUS_LANE_COUNT; i++) {
		uint64_t lane_offset = PPC_PLACE(i, 27, 5);
		/* EDIP_RX_LANE_INVALID */
		or_scom(chip, 0x8002400006010C3F | offset | lane_offset, PPC_BIT(50));
	}
}

static void config_bus_mode(void)
{
	/* Initiate Dc calibration in parallel */
	rx_dc_calibration_start(/*chip=*/0, /*group=*/0);
	rx_dc_calibration_start(/*chip=*/1, /*group=*/0);
	rx_dc_calibration_start(/*chip=*/0, /*group=*/1);
	rx_dc_calibration_start(/*chip=*/1, /*group=*/1);

	/* HB does this delay inside rx_dc_calibration_poll(), but doing it
	 * once instead of four times should be enough */
	mdelay(100);

	/* Then wait for each combination of chip and group */
	rx_dc_calibration_poll(/*chip=*/0, /*group=*/0);
	rx_dc_calibration_poll(/*chip=*/1, /*group=*/0);
	rx_dc_calibration_poll(/*chip=*/0, /*group=*/1);
	rx_dc_calibration_poll(/*chip=*/1, /*group=*/1);
}

void istep_9_2(uint8_t chips)
{
	printk(BIOS_EMERG, "starting istep 9.2\n");
	report_istep(9,2);

	if (chips != 0x01) {
		config_run_bus_mode(/*chip=*/0);
		config_run_bus_mode(/*chip=*/1);

		config_bus_mode();
	}

	printk(BIOS_EMERG, "ending istep 9.2\n");
}
