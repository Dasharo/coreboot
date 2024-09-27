/* SPDX-License-Identifier: GPL-2.0-or-later */

#define NVPCF_FUNC_SUPPORT			0
#define NVPCF_FUNC_GET_STATIC_CONFIG_TABLES	1
#define NVPCF_FUNC_UPDATE_DYNAMIC_PARAMS	2

Name(_HID, "NVDA0820")
Name(_UID, "NPCF")

Method(_DSM, 4, Serialized) {
	If (Arg0 == ToUUID (UUID_NVPCF))
	{
		Return (NPCF (Arg2, Arg3))
	}

	Return (NV_ERROR_UNSUPPORTED)
}

Method (NPCF, 2, Serialized)
{
	Printf("NPCF NPCF(): Func %o", Arg0)
	Switch (ToInteger (Arg0))
	{
		Case (NVPCF_FUNC_SUPPORT)
		{
			Printf("NVPCF: Get supported subfunctions")
			Return (ITOB(
				(1 << NVPCF_FUNC_SUPPORT) |
				(1 << NVPCF_FUNC_GET_STATIC_CONFIG_TABLES) |
				(1 << NVPCF_FUNC_UPDATE_DYNAMIC_PARAMS)))
		}
		Case (NVPCF_FUNC_GET_STATIC_CONFIG_TABLES)
		{
			Printf("NVPCF: Get static config tables")
			Return (Buffer () {
				/* System Device Table Header (v2.0) */
				0x20, 0x03, 0x01,

				/* System Device Table Entries */
				0x00,			/* [3:0] CPU type (0=Intel, 1=AMD),
							   [7:4] GPU type (0=Nvidia) */

				/* System Controller Table Header (v2.2), 1 controller entry */
				0x24, 0x04, 0x05, 0x01,

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
				0x01, 0x00, 0x00, 0x00,

				/* Twos-complement checksum */
				0xac
			})
		}
		Case (NVPCF_FUNC_UPDATE_DYNAMIC_PARAMS)
		{
			Printf("NVPCF: Update dynamic params")

			CreateField(Arg1, 0x28, 2, ICMD)
			Printf("NVPCF: ICMD = %o", ICMD)

			Local0 = Buffer (0x31) {
				/* Dynamic Params Table Header (1 controller entry, 0x1c bytes) */
				0x24, 0x05, 0x10, 0x1c, 0x01 }

			CreateByteField (Local0, 0x15, PC01)
			CreateByteField (Local0, 0x15, PC02)
			CreateWordField (Local0, 0x1D, MAGA)
			CreateWordField (Local0, 0x21, MIGA)

			Switch(ToInteger(ICMD)) {
				Case(0) {
					Printf("Get Controller Params")

					/* Values obtained by poking Clevo ACPI implementation */
					PC02 = 0x03
					MAGA = 0x50
					MIGA = 0xffd8

					Return (Local0)
				}
				Case(1) {
					Printf("Set Controller Status")
					Return (Local0)
				}
			}

			Return (Local0)
		}
	}

	Return (NV_ERROR_UNSUPPORTED)
}
