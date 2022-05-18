/* SPDX-License-Identifier: GPL-2.0-only */

OperationRegion (CMS1, SystemIO, 0x72, 0x2)
Field (CMS1, ByteAcc, NoLock, Preserve)
{
	IND1, 8,
	DAT1, 8,
}

IndexField (IND1, DAT1, ByteAcc, NoLock, Preserve)
{
	KBBR, 8,		// Keyboard Backlight Brightness
	KBEN, 8,		// Keyboard Backlight State
	KBCR, 8,		// Keyboard Backlight Color Red
	KBCG, 8,		// Keyboard Backlight Color Green
	KBCB, 8,		// Keyboard Backlight Color Blue
	KBPS, 8,		// Keyboard Backlight Preset
}
