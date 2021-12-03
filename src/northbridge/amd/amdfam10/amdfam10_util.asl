//
// This file is part of the coreboot project.
//
// Copyright (C) 2015 Timothy Pearson <tpearson@raptorengineeringinc.com>, Raptor Engineering
// Copyright (C) 2007 Advanced Micro Devices, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; version 2 of the License.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//

//AMD FAM10 util for BUSB and res range

Scope (\_SB)
{

	Name (OSTB, Ones)
	Method (OSVR, 0, NotSerialized)
	{
		If (LEqual (^OSTB, Ones))
		{
			Store (0x00, ^OSTB)
		}

		Return (^OSTB)
	}

	Method (SEQL, 2, Serialized)
	{
		Store (SizeOf (Arg0), Local0)
		Store (SizeOf (Arg1), Local1)
		If (LNotEqual (Local0, Local1)) { Return (Zero) }

		ToBuffer(Arg0, Local5)
		ToBuffer(Arg1, Local6)
		Store (Zero, Local2)
		While (LLess (Local2, Local0))
		{
			Store (DerefOf (Index (Local5, Local2)), Local3)
			Store (DerefOf (Index (Local6, Local2)), Local4)
			If (LNotEqual (Local3, Local4)) { Return (Zero) }

			Increment (Local2)
		}

		Return (One)
	}

	Method (GWBN, 2, Serialized)
	{
		Store (ResourceTemplate ()
		{
			WordBusNumber (ResourceProducer, MinFixed, MaxFixed, PosDecode,
				0x0000, // Address Space Granularity
				0x0000, // Address Range Minimum
				0x0000, // Address Range Maximum
				0x0000, // Address Translation Offset
				0x0001,,,)
		}, Local2)
		CreateWordField (Local2, 0x08, BMIN)
		CreateWordField (Local2, 0x0A, BMAX)
		CreateWordField (Local2, 0x0E, BLEN)
		Store (0x00, Local0)
		While (LLess (Local0, 0x20))
		{
			Store (DerefOf (Index (\_SB.PCI0.BUSN, Local0)), Local1)
			If (LEqual (And (Local1, 0x03), 0x03))
			{
				If (LEqual (Arg0, ShiftRight (And (Local1, 0xfc), 0x02)))
				{
					If (LOr (LEqual (Arg1, 0xFF), LEqual (Arg1, ShiftRight (And (Local1, 0x0700), 0x08))))
					{
						Store (ShiftRight (And (Local1, 0x000FF000), 0x0c), BMIN)
						Store (ShiftRight (Local1, 0x14), BMAX)
						Subtract (BMAX, BMIN, BLEN)
						Increment (BLEN)
						Return (RTAG (Local2))
					}
				}
			}

			Increment (Local0)
		}

		Return (RTAG (Local2))
	}

	Method (RTAG, 1, NotSerialized)
	{
		Store (Arg0, Local0)
		Store (SizeOf (Local0), Local1)
		Subtract (Local1, 0x02, Local1)
		Multiply (Local1, 0x08, Local1)
		CreateField (Local0, 0x00, Local1, RETB)
		Return (RETB)
	}
}
