/* SPDX-License-Identifier: GPL-2.0-only */

OperationRegion (CMS1, SystemIO, 0x72, 0x2)
Field (CMS1, ByteAcc, NoLock, Preserve)
{
	IND1, 8,
	DAT1, 8,
}

IndexField (IND1, DAT1, ByteAcc, NoLock, Preserve)
{
	KBDL, 8,		// Keyboard Backlight Brightness
	KBDC, 32,		// Keyboard Backlight Color RGB
}
