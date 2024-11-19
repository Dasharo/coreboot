/* SPDX-License-Identifier: GPL-2.0-or-later */

#include "gpu_defines.h"

Scope (\_SB.PCI0.RP12)
{
	#include "peg.asl"

	Device (PXSX)
	{
		Name (_ADR, 0x0)
		OperationRegion (PCIC, PCI_Config, 0x00, 0x100)
		Field (PCIC, DWordAcc, NoLock, Preserve)
		{
			NVID,	16,
			NDID,	16,
			Offset (0x40),
			NVSS,	32
		}

		#include "utility.asl"
		#include "power.asl"
		#include "nvop.asl"
		#include "nvjt.asl"
		#include "nbci.asl"
		#include "gps.asl"
		#include "gpu_ec.asl"

		/* Convert D Notify from EC to GPU */
		Method (CNVD, 1, NotSerialized)
		{
			Switch (ToInteger(Arg0)) {
				Case (D1_EC) { Return (D1_GPU) }
				Case (D2_EC) { Return (D2_GPU) }
				Case (D3_EC) { Return (D3_GPU) }
				Case (D4_EC) { Return (D4_GPU) }
				Case (D5_EC) { Return (D5_GPU) }
				Default { Return (D5_GPU) }
			}
		}

		/* Current D Notify Value, defaults to D1
		* Arg0 == Shared value
		* Arg1 == force notification if no change (0 or 1)
		*/
		Name (CDNV, D1_EC)
		Method (DNOT, 2, Serialized)
		{
			Printf ("EC: GPU D-Notify, %o", Arg0)
			If ((Arg0 != CDNV) || (Arg1 == 1))
			{
				CDNV = Arg0
				Local0 = CNVD (Arg0)
				Notify (\_SB.PCI0.RP12.PXSX, Local0)
			}
		}

		Method (_DSM, 4, Serialized)
		{
			If (Arg0 == ToUUID (UUID_NVOP))
			{
				If (ToInteger(Arg1) >= REVISION_MIN_NVOP)
				{
					Return (NVOP (Arg2, Arg3))
				}
			}
			ElseIf (Arg0 == ToUUID (UUID_NVJT))
			{
				If (ToInteger (Arg1) >= REVISION_MIN_NVJT)
				{
					Return (NVJT (Arg2, Arg3))
				}
			}
			ElseIf (Arg0 == ToUUID (UUID_NBCI))
			{
				If (ToInteger (Arg1) >= REVISION_MIN_NBCI)
				{
					Return (NBCI (Arg2, Arg3))
				}
			}
			ElseIf (Arg0 == ToUUID (UUID_GPS))
			{
				If (ToInteger (Arg1) >= REVISION_MIN_GPS)
				{
					Return (GPS (Arg2, Arg3))
				}
			}

			Return (NV_ERROR_UNSUPPORTED)
		}
	}
}
