/* SPDX-License-Identifier: GPL-2.0-only */

#include <soc/gpio_soc_defs.h>

#define EC_GPE_SLPB 0x6B // PME_STS
#define EC_GPE_LID 0x6B // PME_STS
#define EC_GPE_SCI 0x6E // ESPI_SCI
#define EC_GPE_PWRB 0x43 // GPD9 Power Button
#define EC_GPE_RTC 0x72 // RTC Alarm Status
// echo '\_SB.SLPB._PRW' | sudo tee /proc/acpi/call; sudo cat /proc/acpi/call
// echo '\_SB.PC00.LPCB.EC._GPE' | sudo tee /proc/acpi/call; sudo cat /proc/acpi/call

Scope (\_SB) {
	#include "sleep.asl"

	#include <ec/clevo/it5570/acpi/ac.asl>
	#include <ec/clevo/it5570/acpi/battery.asl>
	#include <ec/clevo/it5570/acpi/buttons.asl>
	#include <ec/clevo/it5570/acpi/lid.asl>

	Scope (PCI0.LPCB) {
		#include <ec/clevo/it5570/acpi/ec.asl>
	}

	#include "hid.asl"
}

Scope (_GPE) {
	#include <ec/clevo/it5570/acpi/gpe.asl>
}

/* Handle Bluetooth RFkill */
Scope (\_SB.PCI0.XHCI.RHUB.HS10)
{
	PowerResource (BTPR, 0x00, 0x0000)
	{
		Method (_STA, 0, NotSerialized)  // _STA: Status
		{
			If ((\_SB.PCI0.GTXS(GPP_A13) == One))
			{
				Return (One)
			}
			Else
			{
				Return (Zero)
			}
		}

		Method (_ON, 0, Serialized)  // _ON_: Power On
		{
			\_SB.PCI0.STXS (One)
		}

		Method (_OFF, 0, Serialized)  // _OFF: Power Off
		{
			\_SB.PCI0.CTXS (GPP_A13)
		}
	}
	Name (_S0W, 0x02)  // _S0W: S0 Device Wake State

	Method(GPR, 0, NotSerialized)
	{
		If(CondRefOf(\_SB.PCI0.CNVW))
		{
			If (\_SB.PCI0.CNVW.VDID != 0xFFFFFFFF)
			{
				Return (Package (0x01)
				{
					BTPR
				})
			}
		}
		If(CondRefOf(\_SB.PC00.RP11.PXSX))
		{
			If (\_SB.PC00.RP11.PXSX.VDID != 0xFFFFFFFF)
			{
				Return (Package (0x01)
				{
					BTPR
				})
			}
		}
		Return (Package (0x00){})
	}

	Method (_PR0, 0, NotSerialized)
	{
		Return (GPR())
	}

	Method (_PR2, 0, NotSerialized)
	{
		Return (GPR())
	}

	Method (_PR3, 0, NotSerialized)
	{
		Return (GPR())
	}
}

/* Workaround lack of reset and power GPIOs for SD CARD reader */
Scope (\_SB.PCI0.RP09)
{
	OperationRegion (PXCS, PCI_Config, Zero, 0xFF)
	Field (PXCS, AnyAcc, NoLock, Preserve)
	{
		VDID, 32
		Offset (0x52),
		, 13,
		LASX, 1,
		Offset (0xE0),
		, 7,
		NCB7, 1,
		Offset (0xE2),
		, 2,
		L23E, 1,
		L23R, 1
	}
	Name (_S0W, 0x04)
	Name (_PR0, Package (0x01)
	{
		RTD3
	})
	Name (_PR3, Package (0x01)
	{
		RTD3
	})
	PowerResource (RTD3, 0x00, 0x0000)
	{
		Name (_STA, One)
		Method (_ON, 0, Serialized)
		{
			\_SB.PCI0.PMC.IPCS (0xAC, Zero, 0x10, 0x00000008, 0x00000008, 0x00000100, 0x00000100)
			If ((NCB7 == One))
			{
				L23R = One
				Local7 = 0x14
				While ((Local7 > Zero))
				{
					If ((L23R == Zero))
					{
					Break
					}

					Sleep (0x10)
					Local7--
				}

				NCB7 = Zero
				Local7 = 0x08
				While ((Local7 > Zero))
				{
					If ((LASX == One))
					{
					Break
					}

					Sleep (0x10)
					Local7--
				}
			}
		}

		Method (_OFF, 0, Serialized)
		{
			L23E = One
			Local7 = 0x08
			While ((Local7 > Zero))
			{
				If ((L23E == Zero))
				{
					Break
				}

				Sleep (0x10)
				Local7--
			}

			NCB7 = One
			\_SB.PCI0.PMC.IPCS (0xAC, Zero, 0x10, 0x00000008, 0x00000000, 0x00000100, 0x00000000)
		}
	}

	Name (_DSD, Package (0x02)  // _DSD: Device-Specific Data
	{
	ToUUID ("6211e2c0-58a3-4af3-90e1-927a4e0c55a4"), 
	Package (0x01)
	{
		Package (0x02)
		{
		"HotPlugSupportInD3", 
		One
		}
	}
	})
}
