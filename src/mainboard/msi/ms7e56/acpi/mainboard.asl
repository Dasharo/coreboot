/* SPDX-License-Identifier: GPL-2.0-or-later */

External (\_SB.PCI0.GP09, DeviceObj)
External (\_SB.PCI0.GP0A, DeviceObj)
External (\_SB.PCI0.GP0B, DeviceObj)
External (\_SB.PCI0.GP0C, DeviceObj)
External (\_SB.PCI0.GP11, DeviceObj)
External (\_SB.PCI0.GP12, DeviceObj)
External (\_SB.PCI0.GP11, DeviceObj)
External (\_SB.PCI0.GP41.XHC1, DeviceObj)
External (\_SB.PCI0.GP41.XHC2, DeviceObj)
External (\_SB.PCI0.GP41.AZAL, DeviceObj)
External (\_SB.PCI0.GP41.ACPD, DeviceObj)

Scope (\_SB.PCI0) {
	/* This device triggers automatic drivers and MSI utilities installation on Windows */
	Device (MSIV) {
		Name (_HID, "MBAD0002")
		Name (_UID, 1)
		Method (_STA, 0, NotSerialized){
			Return (1)
		}
	}
}

Scope (\_SB)
{
	Device (PWRB)
	{
		Name (_HID, EisaId ("PNP0C0C"))
		Name (_UID, 0xAA)
		Name (_STA, 0x0B)
	}

	Name (S0IX, 0)

	/*
	 * Read dword from memory
	 * Arg0 - Base Address
	 * Arg1 - Offset
	 */
	Method (M04B, 2, Serialized) {
		Local0 = 0
		If (Arg0 != 0) {
			Local0 = Arg0 + Arg1
			OperationRegion (VARM, SystemMemory, Local0, 0x4)
			Field (VARM, DWordAcc, NoLock, Preserve) {
				VARR, 32,
			}
			Local0 = VARR
		}
		Return (Local0)
	}

	/*
	 * Write dword to memory
	 * Arg0 - Base Address
	 * Arg1 - Offset
	 * Arg2 - Dword of data
	 */
	Method (M04E, 3, Serialized) {
		If (Arg0 != 0) {
			Local0 = Arg0 + Arg1
			OperationRegion (VARM, SystemMemory, Local0, 0x4)
			Field (VARM, DWordAcc, NoLock, Preserve) {
				VARR, 32,
			}
			VARR = Arg2
		}
	}

	/*
	 * Write Memory
	 * Arg0 - Base Address
	 * Arg1 - Offset
	 * Arg2 - Start Bit
	 * Arg3 - Bit Width
	 * Arg4 - Value
	 */
	Method (M014, 5, Serialized)
	{
		Local1 = M04B (Arg0, Arg1)
		Local5 = 0x7FFFFFFF
		Local5 |= 0x80000000
		Local2 = (Arg2 + Arg3)
		Local2 = (32 - Local2)
		Local2 = (((Local5 << Local2) & Local5) >> Local2)
		Local2 = ((Local2 >> Arg2) << Arg2)
		Local3 = (Arg4 << Arg2)
		Local4 = ((Local1 & (Local5 ^ Local2)) | Local3)
		M04E (Arg0, Arg1, Local4)
	}
}

Scope (\_GPE)
{
	Method (_L02, 0, Serialized)
	{
		Notify (\_SB.PCI0.GP11, 0x00)
		Notify (\_SB.PCI0.GP11, 0x02)
	}

	Method (_L08, 0, NotSerialized)
	{
		Notify (\_SB.PCI0.GP09, 0x02)
		Notify (\_SB.PCI0.GP0A, 0x02)
	}

	Method (_L0E, 0, NotSerialized)
	{
		Notify (\_SB.PCI0.GP0C, 0x02)
		Notify (\_SB.PWRB, 0x02)
	}

	Method (_L0F, 0, NotSerialized)
	{
		Notify (\_SB.PCI0.GP0B, 0x02)
		Notify (\_SB.PWRB, 0x02)
	}

	Method (_L16, 0, NotSerialized)
	{
		Notify (\_SB.PCI0.GP12, Zero)
		Notify (\_SB.PCI0.GP12, 0x02)
	}
}

Scope (\_SB.GPIO)
{

	Method (_AEI, 0, Serialized)
	{
		Name (BUFF, ResourceTemplate ()
		{
			GpioInt (Level, ActiveLow, ExclusiveAndWake, PullDefault, 0x01F4,
				"\\_SB.GPIO", 0x00, ResourceConsumer, , ) {
				0x000A
			}
			GpioInt (Edge, ActiveLow, ExclusiveAndWake, PullNone, 0x0000,
				"\\_SB.GPIO", 0x00, ResourceConsumer, , ) {
				0x0003
			}
			GpioInt (Edge, ActiveLow, ExclusiveAndWake, PullNone, 0x0000,
				"\\_SB.GPIO", 0x00, ResourceConsumer, , ) {
				0x0020
			}
		})
		Name (BUNP, ResourceTemplate ()
		{
			GpioInt (Edge, ActiveHigh, ExclusiveAndWake, PullDefault, 0x1388,
				"\\_SB.GPIO", 0x00, ResourceConsumer, , ) {
				0x0000
			}
			GpioInt (Level, ActiveHigh, ExclusiveAndWake, PullNone, 0x0000,
				"\\_SB.GPIO", 0x00, ResourceConsumer, , ) {
				0x003D
			}
			GpioInt (Level, ActiveHigh, ExclusiveAndWake, PullNone, 0x0000,
				"\\_SB.GPIO", 0x00, ResourceConsumer, , ) {
				0x003E
			}
			GpioInt (Level, ActiveHigh, ExclusiveAndWake, PullNone, 0x0000,
				"\\_SB.GPIO", 0x00, ResourceConsumer, , ) {
				0x003A
			}
			GpioInt (Level, ActiveHigh, ExclusiveAndWake, PullNone, 0x0000,
				"\\_SB.GPIO", 0x00, ResourceConsumer, , ) {
				0x003B
			}
			GpioInt (Edge, ActiveLow, ExclusiveAndWake, PullNone, 0x0000,
				"\\_SB.GPIO", 0x00, ResourceConsumer, , ) {
				0x0002
			}
			GpioInt (Edge, ActiveLow, ExclusiveAndWake, PullNone, 0x0000,
				"\\_SB.GPIO", 0x00, ResourceConsumer, , ) {
				0x0003
			}
			GpioInt (Edge, ActiveLow, ExclusiveAndWake, PullNone, 0x0000,
				"\\_SB.GPIO", 0x00, ResourceConsumer, , ) {
				0x0009
			}
			GpioInt (Level, ActiveLow, Exclusive, PullDefault, 0x01F4,
				"\\_SB.GPIO", 0x00, ResourceConsumer, , ) {
				0x000A
			}
		})
		If (S0IX) {
			Return (BUNP)
		} Else {
			Return (BUFF)
		}
	}

	Method (_EVT, 1, Serialized)
	{
		Switch (ToInteger (Arg0))
		{
			Case (Zero) { Notify (\_SB.PWRB, 0x80) }
			Case (0x3A) {
				If (CondRefOf (\_SB.PCI0.GP41.XHC1))
				{
					Notify (\_SB.PCI0.GP41.XHC1, 0x02)
				}
			}
			Case (0x3B) {
				If (CondRefOf (\_SB.PCI0.GP41.XHC2))
				{
					Notify (\_SB.PCI0.GP41.XHC2, 0x02)
				}
			}
			Case (0x3D) {
				If (CondRefOf (\_SB.PCI0.GP41.AZAL))
				{
					Notify (\_SB.PCI0.GP41.AZAL, 0x02)
				}
			}
			Case (0x3E) {
				If (CondRefOf (\_SB.PCI0.GP41.ACPD))
				{
					Notify (\_SB.PCI0.GP41.ACPD, 0x02)
				}
			}
			Case (0x02)
			{
				\_SB.M014 (0xFED80200, 0, 0, 32, 0x0100)
				If (CondRefOf (\_GPE._L08))
				{
					\_GPE._L08 ()
				}
			}
			Case (0x03)
			{

				\_SB.M014 (0xFED80200, 0, 0, 32, 0x04)
				If (CondRefOf (\_GPE._L02))
				{
					\_GPE._L02 ()
				}
			}
			Case (0x20)
			{
				\_SB.M014 (0xFED80200, 0, 0, 32, 0x00800000)
				If (CondRefOf (\_GPE._L02))
				{
					\_GPE._L02 ()
				}
			}
			Case (0x09)
			{
				If (CondRefOf (\_GPE._L16))
				{
					\_GPE._L16 ()
				}
			}
		}
	}
}
