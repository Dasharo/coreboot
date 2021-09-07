/* SPDX-License-Identifier: GPL-2.0-only */
/* This file is part of the coreboot project. */

Device (HIDD)
{
	Name (_HID, "INTC1051")  // _HID: Hardware ID
	Name (HBSY, Zero)
	Name (HIDX, Zero)
	Name (HMDE, Zero)
	Name (HRDY, Zero)
	Name (BTLD, Zero)
	Name (BTS1, Zero)
	Name (HEB1, 0x02)
	Name (HEB2, Zero)
	Name (BIST, Zero)

	Name (DPKG, Package (0x04)
	{
		0x11111111,
		0x22222222,
		0x33333333,
		0x44444444
	})

	Method (_STA, 0, Serialized)  // _STA: Status
	{
		Return (0x0F)
	}

	Method (_DSM, 4, Serialized)  // _DSM: Device-Specific Method
	{
		If ((Arg0 == ToUUID ("eeec56b3-4442-408f-a792-4edd4d758054")))
		{
			If ((One == ToInteger (Arg1)))
			{
				BIST = One
				Switch (ToInteger (Arg2))
				{
					Case (Zero)
					{
						Return (Buffer (0x02)
						{
							0xFF, 0x01
						})
					}
					Case (One)
					{
						BTNL ()
					}
					Case (0x02)
					{
						Return (HDMM ())
					}
					Case (0x03)
					{
						HDSM (DerefOf (Arg3 [Zero]))
					}
					Case (0x04)
					{
						Return (HDEM ())
					}
					Case (0x05)
					{
						Return (BTNS ())
					}
					Case (0x06)
					{
						BTNE (DerefOf (Arg3 [Zero]))
					}
					Case (0x07)
					{
						Return (HEBC ())
					}
					Case (0x08)
					{
					}

				}
			}
			Else
			{
				BIST = Zero
			}
		}

		Return (Buffer (One) { 0x00 })
	}

	Method (HDDM, 0, Serialized)
	{
		Return (DPKG)
	}

	Method (HDEM, 0, Serialized)
	{
		HBSY = Zero
		If ((HMDE == Zero))
		{
			Return (HIDX)
		}

		Return (HMDE)
	}

	Method (HDMM, 0, Serialized)
	{
		Return (HMDE)
	}

	Method (HDSM, 1, Serialized)
	{
		HRDY = Arg0
	}

	Method (HPEM, 1, Serialized)
	{
		HBSY = One
		If ((HMDE == Zero))
		{
			HIDX = Arg0
		}
		Else
		{
			HIDX = Arg0
		}

		Notify (HIDD, 0xC0)
		Local0 = Zero

		While (((Local0 < 0xFA) && HBSY))
		{
			Sleep (0x04)
			Local0++
		}

		If ((HBSY == One))
		{
			HBSY = Zero
			HIDX = Zero
			Return (One)
		}
		Else
		{
			Return (Zero)
		}
	}

	Method (BTNL, 0, Serialized)
	{
		BTS1 = Zero
	}

	Method (BTNE, 1, Serialized)
	{
		BTS1 = ((Arg0 & 0x1E) | One)
	}

	Method (BTNS, 0, Serialized)
	{
		Return (BTS1)
	}

	Method (BTNC, 0, Serialized)
	{
		Return (Zero)
	}

	Method (HEBC, 0, Serialized)
	{
		Return (HEB1)
	}

	Method (HEEC, 0, Serialized)
	{
		Return (HEB2)
	}
}
