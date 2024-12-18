/* SPDX-License-Identifier: GPL-2.0-only */

OperationRegion (ERAM, EmbeddedControl, 0, 0xFF)
Field (ERAM, ByteAcc, Lock, Preserve)
{
	Offset (0x03),
	LSTE, 1,	// Lid is open
	, 1,
	LWKE, 1,	// Lid wake
	, 5,
	Offset (0x07),
	TMP1, 8,	// CPU temperature
	Offset (0x10),
	ADP, 1,		// AC adapter connected
	, 1,
	BAT0, 1,	// Battery connected
	, 5,
	WFNO, 8,	// Wake cause (not implemented)
	Offset (0x16),
	BDC0, 32,	// Battery design capacity
	BFC0, 32,	// Battery full capacity
	Offset (0x22),
	BDV0, 32,	// Battery design voltage
	BST0, 32,	// Battery status
	BPR0, 32,	// Battery current
	BRC0, 32,	// Battery remaining capacity
	BPV0, 32,	// Battery voltage
	Offset (0x3A),
	BCW0, 32,
	BCL0, 32,
	CYC0, 16,	// Battery cycle count
	Offset (0x68),
	ECOS, 8,	// Detected OS, 0 = no ACPI, 1 = ACPI but no driver, 2 = ACPI with driver
	Offset (0xC8),
	OEM1, 8,
	OEM2, 8,
	OEM3, 16,
	OEM4, 8,	// Extra SCI data
	Offset (0xCD),
	TMP2, 8,	// GPU temperature
	DUT1, 8,	// Fan 1 duty
	DUT2, 8,	// Fan 2 duty
	RPM1, 16,	// Fan 1 RPM
	RPM2, 16,	// Fan 2 RPM
	Offset (0xD4),
	GPUD, 8,	// GPU D-notify level
	APWR, 32,	// AC adapter power
	Offset (0xD9),
	AIRP, 8,	// Airplane mode LED
	WINF, 8,	// Enable ACPI brightness controls
	Offset (0xE0),
	S0XH, 1,	// S0ix hook
	DSPH, 1,	// Display hook
}
