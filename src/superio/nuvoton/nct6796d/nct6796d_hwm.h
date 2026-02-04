/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef SUPERIO_NUVOTON_NCT6796D_HWM_H
#define SUPERIO_NUVOTON_NCT6796D_HWM_H

#include <types.h>

/*
 * NCT6796D Hardware Monitor register definitions.
 * Bank-based index/data access via HWM base (index at base+5, data at base+6).
 * Bank select: write bank to index 0x4e in Bank 0.
 *
 * Banks: 0=global, 1=SYSFAN, 2=CPUFAN, 3=AUXFAN0, 4=PECI/cal, 7=PECI setup,
 * 8=AUXFAN1, 9=AUXFAN2, 10=AUXFAN3, 11=AUXFAN4
 */

/* Bank numbers */
#define NCT6796D_HWM_BANK_GLOBAL	0
#define NCT6796D_HWM_BANK_SYSFAN	1
#define NCT6796D_HWM_BANK_CPUFAN	2
#define NCT6796D_HWM_BANK_AUXFAN0	3
#define NCT6796D_HWM_BANK_PECI_CAL	4
#define NCT6796D_HWM_BANK_PECI_SETUP	7
#define NCT6796D_HWM_BANK_AUXFAN1	8
#define NCT6796D_HWM_BANK_AUXFAN2	9
#define NCT6796D_HWM_BANK_AUXFAN3	10
#define NCT6796D_HWM_BANK_AUXFAN4	11

/* Bank 0 - Global config (index 0x4e = bank select) */
#define NCT6796D_HWM_BANK_SELECT	0x4e

/* Bank 0 - PECI temperature reading enable for Smart Fan control */
#define NCT6796D_PECI_TEMP_ENABLE	0xae

/* Bank 4 - PECI Agent 0 calibration */
#define NCT6796D_PECI_AGENT0_CAL_0	0xf8
#define NCT6796D_PECI_AGENT0_CAL_1	0xfa

/* Bank 7 - PECI interface setup */
#define NCT6796D_PECI_CTRL_0		0x01
#define NCT6796D_PECI_CTRL_1		0x02
#define NCT6796D_PECI_CTRL_2		0x03
#define NCT6796D_PECI_CTRL_3		0x04
#define NCT6796D_PECI_CTRL_9		0x09

/* CTRL_0 (Index 01h) - PECI Enable */
#define NCT6796D_PECI_CTRL_0_PECI_EN		BIT(7)
#define NCT6796D_PECI_CTRL_0_IS_PECI30		BIT(2)
#define NCT6796D_PECI_CTRL_0_ROUTINE_EN		BIT(0)

/* CTRL_1 (Index 02h) - Timing: rate, TN_Extend, Adj, PECI_DC (750kHz typical) */
#define NCT6796D_PECI_TIMING_DEFAULT		0x32

/* CTRL_2 (Index 03h) - Agent enable: bits 5-4 En_Agt (01=Agent0, 11=both) */
#define NCT6796D_PECI_CTRL_2_AGT_AGENT0		BIT(4)

/* CTRL_3 (Index 04h) - Temp config: Virtual_En, Clamp, RtDmn_Agt, RtHigher (all default) */
#define NCT6796D_PECI_CTRL_3_DEFAULT		0x00

/* CTRL_9 (Index 09h) - Tbase0 offset for Agent0 (counts) */
#define NCT6796D_TBASE0_DEFAULT			0x64

/* Bank 4 - PECI Agent 0 calibration defaults */
#define NCT6796D_PECI_AGENT0_CAL_0_DEFAULT	0x1c	/* F8h: update times, adjust unit */
#define NCT6796D_PECI_AGENT0_CAL_1_DEFAULT	0x23	/* FAh: enable, interval */

/* Smart Fan IV registers (per fan bank) */
#define NCT6796D_FAN_TEMP_SOURCE		0x00
#  define NCT6796D_FAN_TMP_SRC_STOPDUTY_EN	BIT(7)	/* use stop value vs zero when cooling */
#  define NCT6796D_FAN_TMP_SRC_PECI0		0x10
#  define NCT6796D_FAN_TMP_SRC_PECI0_CAL	0x1c
#  define NCT6796D_FAN_TMP_SRC_SYSTIN		0x01
#  define NCT6796D_FAN_TMP_SRC_CPUTIN		0x02
#  define NCT6796D_FAN_TMP_SRC_AUXTIN0		0x03
#  define NCT6796D_FAN_TMP_SRC_AUXTIN1		0x04
#  define NCT6796D_FAN_TMP_SRC_AUXTIN2		0x05
#  define NCT6796D_FAN_TMP_SRC_AUXTIN3		0x06
#  define NCT6796D_FAN_TMP_SRC_AUXTIN4		0x07

/* Index 31h: 3-wire fan enable (0 = 4-wire, 1 = 3-wire) */
#define NCT6796D_FAN_3WIRE_ENABLE	0x31

#define NCT6796D_FAN_MODE_TOLERANCE	0x02
#  define NCT6796D_FAN_MODE_SFIV	4

#define NCT6796D_FAN_STEP_UP_TIME	0x03
#define NCT6796D_FAN_STEP_DOWN_TIME	0x04
#define NCT6796D_FAN_STOP_VALUE		0x05
#define NCT6796D_FAN_STARTUP_VALUE	0x06
#define NCT6796D_FAN_STOP_TIME		0x07

#define NCT6796D_FAN_TEMP(i)		(0x21 + (i))
#define NCT6796D_FAN_DUTY(i)		(0x27 + (i))

#define NCT6796D_FAN_CRIT_TEMP		0x35
#define NCT6796D_FAN_CRIT_DUTY_EN	0x36
#define NCT6796D_FAN_CRIT_DUTY		0x37
#define NCT6796D_FAN_CRIT_TEMP_TOL	0x38
#define NCT6796D_FAN_DUTY_PER_STEP	0x66

struct superio_nuvoton_nct6796d_config;

void nct6796d_hwm_init(uint16_t hwm_base,
		       const struct superio_nuvoton_nct6796d_config *conf);

#endif /* SUPERIO_NUVOTON_NCT6796D_HWM_H */
