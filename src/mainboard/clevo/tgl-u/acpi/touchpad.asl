External (\_SB.PCI0.I2C0.H015, DeviceObj)
External (\_SB.PCI0.I2C0.H015._STA, IntObj)
External (\_SB.PCI0.I2C0.H038, DeviceObj)
External (\_SB.PCI0.I2C0.H038._STA, IntObj)

Scope (PCI0.LPCB.EC0)
{
	Method (TPTG, 0, Serialized)
	{
		If (CondRefOf(\_SB.PCI0.I2C0.H015)) {
			If (\_SB.PCI0.I2C0.H015._STA == Zero) {
				\_SB.PCI0.I2C0.H015._STA = 0xF
			} Else {
				\_SB.PCI0.I2C0.H015._STA = 0x0
			}
			Notify (\_SB.PCI0.I2C0.H015, One)
		} ElseIf (CondRefOf(\_SB.PCI0.I2C0.H038)) {
			If (\_SB.PCI0.I2C0.H038._STA == Zero) {
				\_SB.PCI0.I2C0.H038._STA = 0xF
			} Else {
				\_SB.PCI0.I2C0.H038._STA = 0x0
			}
			Notify (\_SB.PCI0.I2C0.H038, One)
		}
	}
}

