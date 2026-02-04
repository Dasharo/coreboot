/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef SUPERIO_NUVOTON_NCT6796D_CHIP_H
#define SUPERIO_NUVOTON_NCT6796D_CHIP_H

#include <stdbool.h>
#include <stdint.h>
#include "nct6796d_hwm.h"

#define NCT6796D_MAX_FANS	7	/* SYSFAN, CPUFAN, AUXFAN0-4 */

/* HWM bank numbers for fan->bank (use in devicetree) */
#define NCT6796D_HWM_BANK_SYSFAN	1
#define NCT6796D_HWM_BANK_CPUFAN	2
#define NCT6796D_HWM_BANK_AUXFAN0	3
#define NCT6796D_HWM_BANK_AUXFAN1	8
#define NCT6796D_HWM_BANK_AUXFAN2	9
#define NCT6796D_HWM_BANK_AUXFAN3	10
#define NCT6796D_HWM_BANK_AUXFAN4	11

/* Fan tach wire mode: Index 31h per bank (0 = 4-wire, 1 = 3-wire) */
#define NCT6796D_FAN_4WIRE	0
#define NCT6796D_FAN_3WIRE	1

/* Fan channel configuration - Smart Fan IV curve */
struct nct6796d_fan_config {
	uint8_t bank;		/* HWM bank: NCT6796D_HWM_BANK_* */
	uint8_t temp_src;	/* Index 00h SOURCE[4:0]; use NCT6796D_FAN_TMP_SRC_* from nct6796d_hwm.h */
	bool stop_duty_en;	/* Index 00h bit7: use stop value vs zero when cooling */
	uint8_t wire_mode;	/* NCT6796D_FAN_4WIRE or NCT6796D_FAN_3WIRE (Index 31h) */
	uint8_t temp[4];	/* Temperature points (C) for curve */
	uint8_t duty[4];	/* PWM duty (0-255) at each temp point */
	uint8_t temp_tolerance;
	uint8_t step_up_time;	/* Units: 0.1s */
	uint8_t step_down_time;
	uint8_t stop_value;	/* Index 05h: PWM when stopped / below curve (0-255) */
	uint8_t startup_value;	/* Index 06h: PWM at fan spin-up (0-255) */
	uint8_t stop_time;	/* Index 07h: delay (0.1s) before stopping fan */
	uint8_t duty_per_step_up;
	uint8_t duty_per_step_down;
	uint8_t crit_temp;
	uint8_t crit_duty_en;
	uint8_t crit_duty;
	uint8_t crit_temp_tolerance;
};

struct superio_nuvoton_nct6796d_config {
	/* PECI enabled for Intel CPU temperature (required for fan control) */
	bool peci_enable;

	/* PECI Bank 7: Index 02h timing (default 0x32 per vendor), Index F8h/FAh calibration */
	uint8_t peci_timing;		/* default 0x32: TN_Extend, 750kHz */
	uint8_t peci_agent0_cal_0;	/* Bank 4 Index F8h, default 0x1c per vendor */
	uint8_t peci_agent0_cal_1;	/* Bank 4 Index FAh, default 0x23 per vendor */

	/* Fan configurations; unused fans have bank=0 */
	struct nct6796d_fan_config fans[NCT6796D_MAX_FANS];
};

#define FAN1	fans[0]
#define FAN2	fans[1]
#define FAN3	fans[2]
#define FAN4	fans[3]
#define FAN5	fans[4]
#define FAN6	fans[5]
#define FAN7	fans[6]

#endif /* SUPERIO_NUVOTON_NCT6796D_CHIP_H */
