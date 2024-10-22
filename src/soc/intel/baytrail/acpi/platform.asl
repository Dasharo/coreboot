/* SPDX-License-Identifier: GPL-2.0-only */

#include <soc/intel/baytrail/acpi/device_nvs.asl>
#include <southbridge/intel/common/acpi/platform.asl>

#include <soc/iomap.h>

External(\_SB.MPTS, MethodObj)
External(\_SB.MWAK, MethodObj)

/*
 * The _PTS method (Prepare To Sleep) is called before the OS is
 * entering a sleep state. The sleep state number is passed in Arg0
 */

Method(_PTS,1)
{
	If (CondRefOf (\_SB.MPTS))
	{
		\_SB.MPTS (Arg0)
	}
}

/* The _WAK method is called on system wakeup */

Method(_WAK,1)
{
	If (CondRefOf (\_SB.MWAK))
	{
		\_SB.MWAK (Arg0)
	}

	Return(Package(){0,0})
}

Method (_SWS)
{
	/* Index into PM1 for device that caused wake */
	Return (\PM1I)
}


OperationRegion (PMIO, SystemIo, ACPI_BASE_ADDRESS, 0x46)
Field (PMIO, ByteAcc, NoLock, Preserve)
{
	,	8,
	PWBS,	1,
	Offset(0x20),
	,	1,
	PHPS,	1,
	,	1,
	PWS0,	1,
	,	2,
	PWS1,	1,
	PWS2,	1,
	PWS3,	1,
	PEXS,	1,
	,	3,
	PMEB,	1,
	Offset(0x42),
	,	1,
	GPEC,	1
}
Field (PMIO, ByteAcc, NoLock, WriteAsZeros)
{
	Offset(0x20),
	,	4,
	PSCI,	1,
	SCIS,	1
}

Scope(\_GPE)
{

	/* Hot Plug SCI */
	Method (_L01)
	{
		If (\_SB.PCI0.RP01.PDCX)
		{
			\_SB.PCI0.RP01.PDCX = 1
			Notify (\_SB.PCI0.RP01, 0x00)
		}

		If (\_SB.PCI0.RP02.PDCX)
		{
			\_SB.PCI0.RP02.PDCX = 1
			Notify (\_SB.PCI0.RP02, 0x00)
		}

		If (\_SB.PCI0.RP03.PDCX)
		{
			\_SB.PCI0.RP03.PDCX = 1
			Notify (\_SB.PCI0.RP03, 0x00)
		}

		If (\_SB.PCI0.RP04.PDCX)
		{
			\_SB.PCI0.RP04.PDCX = 1
			Notify (\_SB.PCI0.RP04, 0x00)
		}

		PHPS = 1
	}

	Method (_L02)
	{
		GPEC = 0
	}

	Method (_L04)
	{
		/* Clear the PUNIT Status Bit. */
		PSCI = 1
	}

	/* PCI Express PME/SCI */
	Method (_L09)
	{
		If (\_SB.PCI0.RP01.PSPX)
		{
			\_SB.PCI0.RP01.PSPX = 1
			Notify (\_SB.PCI0.RP01, 0x02)
		}

		If (\_SB.PCI0.RP02.PSPX)
		{
			\_SB.PCI0.RP02.PSPX = 1
			Notify (\_SB.PCI0.RP02, 0x02)
		}

		If (\_SB.PCI0.RP03.PSPX)
		{
			\_SB.PCI0.RP03.PSPX = 1
			Notify (\_SB.PCI0.RP03, 0x02)
		}

		If (\_SB.PCI0.RP04.PSPX)
		{
			\_SB.PCI0.RP04.PSPX = 1
			Notify (\_SB.PCI0.RP04, 0x02)
		}

		PEXS = 1
	}

	/* PCI B0 PME */
	Method (_L0D, 0)
	{
		If (\_SB.PCI0.XHCI.PMES)
		{
			\_SB.PCI0.XHCI.PMES = 1
			Notify (\_SB.PCI0.XHCI, 0x02)
		}

		If (\_SB.PCI0.HDA.PMES)
		{
			\_SB.PCI0.HDA.PMES = 1
			Notify (\_SB.PCI0.HDA, 0x02)
		}

		PMEB = 1
	}
}
