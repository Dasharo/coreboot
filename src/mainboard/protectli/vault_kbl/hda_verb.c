/* SPDX-License-Identifier: GPL-2.0-only */

/* TODO: Convert to macros */

#include <device/azalia_device.h>

const u32 cim_verb_data[] = {
	/* --- KabyLake HDA --- */
	0x80862809,	/* Codec Vendor / Device ID: Intel Skylake HDMI */
	0x80860101,	/* Subsystem ID */
	5,		/* Number of jacks (NID entries) */

	/* Enable the third converter and pin first */
	0x20878101,
	0x20878101,
	0x20878101,
	0x20878101,

	/* Pin Widget Verb Table */
	AZALIA_PIN_CFG(2, 0x05, 0x18560010),
	AZALIA_PIN_CFG(2, 0x06, 0x18560020),
	AZALIA_PIN_CFG(2, 0x07, 0x18560030),

	/* Disable the third converter and third pin */
	0x20878100,
	0x20878100,
	0x20878100,
	0x20878100,
};

const u32 pc_beep_verbs[] = {};

AZALIA_ARRAY_SIZES;
