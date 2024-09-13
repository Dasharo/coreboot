/* SPDX-License-Identifier: GPL-2.0-or-later */

#define NVPCF_FUNC_SUPPORT			0
#define NVPCF_FUNC_GET_STATIC_CONFIG_TABLES	1
#define NVPCF_FUNC_UPDATE_DYNAMIC_PARAMS	2

Method (NPCF, 2, Serialized)
{
	Switch (ToInteger (Arg0))
	{
		Case (NVPCF_FUNC_SUPPORT)
		{
			Return (ITOB(
				(1 << NVPCF_FUNC_SUPPORT) |
				(1 << NVPCF_FUNC_GET_STATIC_CONFIG_TABLES) |
				(1 << NVPCF_FUNC_UPDATE_DYNAMIC_PARAMS)))
		}
		Case (NVPCF_FUNC_GET_STATIC_CONFIG_TABLES)
		{
			Return (Buffer () {
				/* System Device Table Header (v2.0) */
				0x20, 0x03, 0x01,

				/* System Device Table Entries */
				0x00,			/* [3:0] CPU type (0=Intel, 1=AMD),
							   [7:4] GPU type (0=Nvidia) */

				/* System Controller Table Header (v2.2), 1 controller entry */
				0x22, 0x04, 0x05, 0x01,

				/* Controller #1 Flags */
				0x01,			/* [3:0] Controller class
							      0=Disabled, 1=Dynamic Boost,
							      2=CTGP-only.
							   [7:4] Reserved. Set to 0. */
				/* Controller #1 Params */
							/* Class = Dynamic Boost
							   [0:0] DC support
							      0=Not supported, 1=Supported
							   [31:1] Reserved. Set to 0. */
				0x00, 0x00, 0x00, 0x00,

				/* Twos-complement checksum */
				0xaf
			})
		}
		Case (NVPCF_FUNC_UPDATE_DYNAMIC_PARAMS)
		{
			Local0 = Buffer (0x31) {
				/* Dynamic Params Table Header (1 controller entry, 0x1c bytes) */
				0x22, 0x05, 0x10, 0x1c, 0x01 }

			CreateWordField (Local0, 0x1d, MAGA)
			CreateWordField (Local0, 0x19, TPPA)
			CreateDWordField (Local0, 0x15, CEO0)

			MAGA = 0x348	/* TGP on AC = 105W in 1/8-Watt increments */
			TPPA = 0x348	/* TPPA = 105W in 1/8-Watt increments */
			CEO0 = 0x200	/* [7:0] Controller index
					   [8:8] Disable controller on AC
					   [9:9] Disable controller on DC */
			Return (Local0)
		}
	}

	Return (NV_ERROR_UNSUPPORTED)
}
