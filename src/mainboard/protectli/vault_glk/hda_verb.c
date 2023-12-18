/* SPDX-License-Identifier: GPL-2.0-only */

#include <device/azalia_device.h>

const u32 cim_verb_data[] = {
	/* GeminiLake HDMI */
	0x8086280d, /* Vendor ID */
	0x80860101, /* Subsystem ID */
	4, /* Number of entries */
	AZALIA_SUBVENDOR(2, 0x80860101),
	AZALIA_PIN_CFG(2, 0x05, 0x18560010),
	AZALIA_PIN_CFG(2, 0x06, 0x18560010),
	AZALIA_PIN_CFG(2, 0x07, 0x18560010)
};

const u32 pc_beep_verbs[] = {};

AZALIA_ARRAY_SIZES;
