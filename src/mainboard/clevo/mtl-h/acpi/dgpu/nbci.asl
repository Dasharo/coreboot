/* SPDX-License-Identifier: GPL-2.0-or-later */

#define NBCI_FUNC_SUPPORT	0
#define NBCI_FUNC_PLATCAPS	1
#define NBCI_FUNC_GETOBJBYTYPE	16
#define NBCI_FUNC_GETCALLBACKS	19

/* 'DR' in ASCII, for DRiver Object */
#define NBCI_OBJTYPE_DR		0x4452

#define GPS_FUNC_GETCALLBACKS	0x13

Method (NBCI, 2, Serialized)
{
	Switch (ToInteger (Arg0))
	{
		Case (NBCI_FUNC_SUPPORT)
		{
			Return (ITOB(
				(1 << NBCI_FUNC_SUPPORT) |
				(1 << NBCI_FUNC_PLATCAPS) |
				(1 << NBCI_FUNC_GETOBJBYTYPE)))
		}
		Case (NBCI_FUNC_PLATCAPS)
		{
			Return (ITOB(
				(0 << 10) |	/* No 3D Hotkeys */
				(0 << 9)  |	/* Do not enumerate a dock */
				(0 << 7)  |	/* Does not have DISPLAYSTATUS */
				(0 << 5)  |	/* No LID support */
				(0 << 4)))	/* No Aux power state request */
		}
		Case (NBCI_FUNC_GETCALLBACKS)
		{
			/* Re-use the GPS subfunction's GETCALLBACKS Method */
			Return (GPS (GPS_FUNC_GETCALLBACKS, Arg1))
		}
		Case (NBCI_FUNC_GETOBJBYTYPE)
		{
			Return (NV_ERROR_UNSPECIFIED)
		}
	}

	Return (NV_ERROR_UNSUPPORTED)
}
