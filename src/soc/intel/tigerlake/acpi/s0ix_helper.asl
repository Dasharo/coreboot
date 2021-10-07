/* SPDX-License-Identifier: GPL-2.0-or-later */

Scope (\_SB)
{
	Name (HDAA, 1)
	Name (DISA, 1)
	Name (CIWF, 1)
	Name (CIBT, 1)
	Name (S23C, 0)
		
	Method(S023, 2, Serialized) {
			
		If (!S23C) {
			If (\_SB.PCI0.HDAS.VDID == 0xFFFFFFFF) {
				HDAA = 2
			}
			If (\_SB.PCI0.CNVW.VDID == 0xFFFFFFF) {
				CIWF = 2
				CIBT = 2
			}
			S23C = One
		}
			
		Switch(ToInteger(Arg0)) {
			Case(0)
			{
				If (HDAA != 2)
				{
					HDAA = Arg1
					Store (Arg1, HDAA)
				}
			}
			Case(1)
			{
				DISA = Arg1
			}
			Case(2)
			{
				If (CIWF != 2)
				{
					CIWF = 1
				}
			}
			Case(3)
			{
				If (CIBT != 2)
				{
					C1BT = 1 
				}
			}
			Default
			{
				Return
			}
		}
			
		If ((DISA != One) && (HDAA != One) &&
		    (CIWF != One) && (CIBT != One))
			XQSD = Zero
		} Else {
			XQSD = Zero
		}
	}
}