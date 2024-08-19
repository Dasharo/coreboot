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
				0x24, 0x05, 0x10, 0x1c, 0x01 }

			CreateWordField (Local0, 0x05, TGPA)
			CreateWordField (Local0, 0x07, TGPD)
			CreateWordField (Local0, 0x19, TPPA)
			CreateWordField (Local0, 0x1B, TPPD)
			CreateWordField (Local0, 0x1D, MAGA)
			CreateWordField (Local0, 0x1F, MAGD)
			CreateWordField (Local0, 0x21, MIGA)
			CreateWordField (Local0, 0x23, MIGD)
			CreateDWordField (Local0, 0x15, CEO0)

			/* Vendor firmware generates these in DXE or SMM, puts */
			/* them in NVS and then ACPI copies it here. */
			/* TODO: Dump vendor FW values for reference. */

			TGPA = 0x280	/* TGP on AC = 80W in 1/8-Watt increments */
			TGPD = 0x140	/* TGP on DC = 40W in 1/8-Watt increments */
			TPPA = 0x1E0 	/* Total Processor Power on AC = 60W */
			MAGA = 0x1E0	/* Max offset from TGP on AC = 60W */
			MIGA = 0xC8	/* Min offset from TGP on AC = 25W */

			CEO0 = 0x0000	/* [7:0] Controller index
					   [8:8] Disable controller on AC
					   [9:9] Disable controller on DC */
			Return (Local0)
		}
	}

	Return (NV_ERROR_UNSUPPORTED)
}
