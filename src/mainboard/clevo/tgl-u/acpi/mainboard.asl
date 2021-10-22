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
}

Scope (_GPE) {
	#include <ec/clevo/it5570/acpi/gpe.asl>
}

External (\_SB.PCI0.RP11.PXSX.VDID, FieldUnitObj)
External (\_SB.PCI0.PMC.IPCS,MEthodObj)

/* Handle Bluetooth RFkill */
Scope (\_SB.PCI0.XHCI.RHUB.HS10)
{
	PowerResource (BTPR, 0x00, 0x0000)
	{
		Method (_STA, 0, NotSerialized)
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

		Method (_ON, 0, Serialized)
		{
			\_SB.PCI0.STXS (GPP_A13)
		}

		Method (_OFF, 0, Serialized)
		{
			\_SB.PCI0.CTXS (GPP_A13)
		}
	}
	Name (_S0W, 0x02)

	Method(GPR, 0, NotSerialized)
	{
		If(CondRefOf(\_SB.PCI0.CNVW))
		{
			Return (Package (0x01)
			{
				BTPR
			})
		}
		If(CondRefOf(\_SB.PCI0.RP11.PXSX))
		{
			Return (Package (0x01)
			{
				BTPR
			})
		}
		Return (Package (0x00){})
	}

	Method (_PR0, 0, NotSerialized)
	{
		Return (GPR())
	}

	Method (_PR3, 0, NotSerialized)
	{
		Return (GPR())
	}
}
