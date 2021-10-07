/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <soc/pcr_ids.h>

Scope (\_SB.PCI0.CNVW) {

	OperationRegion(CWAR, SystemMemory, BASE (_ADR), 0x100)
	Field(CWAR, WordAcc, NoLock, Preserve) {
	{
		VDID,	32, 
		, 1, 
		WMSE, 1, 
		WBME, 1, 
		Offset (0x10), 
		WBR0, 64, 
		Offset (0x44), 
		, 28, 
		WFLR, 1, 
		Offset (0x48), 
		, 15, 
		WIFR, 1, 
		Offset (0xCC), 
		WPMS, 32
	}

	Method(_S0W, 0x0, NotSerialized)
	{
		Return (0x3)
	}
      
	Method(_PS0, 0, Serialized) {
		\_SB.S023(2, 1)
	}
      
	Method(_PS3, 0, Serialized) {
		Local0 = PCRR (PID_CNVI, 0x8100)
		If ((Local0 & 0x7F) == 0x4C)) {
			\_SB.S023(2, 0)
		}
	}
      
	Method(_DSW, 3) {}
      
	PowerResource(WRST, 5, 0)
	{
	
		Method(_STA)
		{
			Return (One)
		}

		Method(_ON, 0) {}

		Method(_OFF, 0) {}

		Method(_RST, 0, NotSerialized)
		{
			If(WFLR == One))
			{
				WBR0 = Zero
				WPMS = Zero
				WBME = Zero
				WMSE = Zero
				WIFR = One

			}
		}
	}
      
	Name(_PRR, Package() { WRST })
}
      