/* SPDX-License-Identifier: GPL-2.0-only */

#include <soc/iomap.h>
#include <soc/irq.h>

Scope(\)
{
	/* Intel Legacy Block */
	OperationRegion(ILBS, SystemMemory, ILB_BASE_ADDRESS, ILB_BASE_SIZE)
	Field (ILBS, AnyAcc, NoLock, Preserve)
	{
		Offset (0x8),
		PRTA, 8,
		PRTB, 8,
		PRTC, 8,
		PRTD, 8,
		PRTE, 8,
		PRTF, 8,
		PRTG, 8,
		PRTH, 8,
	}
}

External (\TOLM, IntObj)

Name(_HID,EISAID("PNP0A08"))	/* PCIe */
Name(_CID,EISAID("PNP0A03"))	/* PCI */

Name(_BBN, 0)

Name (MCRS, ResourceTemplate()
{
	/* Bus Numbers */
	WordBusNumber (ResourceProducer, MinFixed, MaxFixed, PosDecode,
			0x0000, 0x0000, 0x00ff, 0x0000, 0x0100,,, PB00)

	/* IO Region 0 */
	DWordIO (ResourceProducer, MinFixed, MaxFixed, PosDecode, EntireRange,
			0x0000, 0x0000, 0x0cf7, 0x0000, 0x0cf8,,, PI00)

	/* PCI Config Space */
	Io (Decode16, 0x0cf8, 0x0cf8, 0x0001, 0x0008)

	/* IO Region 1 */
	DWordIO (ResourceProducer, MinFixed, MaxFixed, PosDecode, EntireRange,
			0x0000, 0x0d00, 0xffff, 0x0000, 0xf300,,, PI01)

	/* VGA memory (0xa0000-0xbffff) */
	DWordMemory (ResourceProducer, PosDecode, MinFixed, MaxFixed,
			Cacheable, ReadWrite,
			0x00000000, 0x000a0000, 0x000bffff, 0x00000000,
			0x00020000,,, ASEG)

	/* OPROM reserved (0xc0000-0xc3fff) */
	DWordMemory (ResourceProducer, PosDecode, MinFixed, MaxFixed,
			Cacheable, ReadWrite,
			0x00000000, 0x000c0000, 0x000c3fff, 0x00000000,
			0x00004000,,, OPR0)

	/* OPROM reserved (0xc4000-0xc7fff) */
	DWordMemory (ResourceProducer, PosDecode, MinFixed, MaxFixed,
			Cacheable, ReadWrite,
			0x00000000, 0x000c4000, 0x000c7fff, 0x00000000,
			0x00004000,,, OPR1)

	/* OPROM reserved (0xc8000-0xcbfff) */
	DWordMemory (ResourceProducer, PosDecode, MinFixed, MaxFixed,
			Cacheable, ReadWrite,
			0x00000000, 0x000c8000, 0x000cbfff, 0x00000000,
			0x00004000,,, OPR2)

	/* OPROM reserved (0xcc000-0xcffff) */
	DWordMemory (ResourceProducer, PosDecode, MinFixed, MaxFixed,
			Cacheable, ReadWrite,
			0x00000000, 0x000cc000, 0x000cffff, 0x00000000,
			0x00004000,,, OPR3)

	/* OPROM reserved (0xd0000-0xd3fff) */
	DWordMemory (ResourceProducer, PosDecode, MinFixed, MaxFixed,
			Cacheable, ReadWrite,
			0x00000000, 0x000d0000, 0x000d3fff, 0x00000000,
			0x00004000,,, OPR4)

	/* OPROM reserved (0xd4000-0xd7fff) */
	DWordMemory (ResourceProducer, PosDecode, MinFixed, MaxFixed,
			Cacheable, ReadWrite,
			0x00000000, 0x000d4000, 0x000d7fff, 0x00000000,
			0x00004000,,, OPR5)

	/* OPROM reserved (0xd8000-0xdbfff) */
	DWordMemory (ResourceProducer, PosDecode, MinFixed, MaxFixed,
			Cacheable, ReadWrite,
			0x00000000, 0x000d8000, 0x000dbfff, 0x00000000,
			0x00004000,,, OPR6)

	/* OPROM reserved (0xdc000-0xdffff) */
	DWordMemory (ResourceProducer, PosDecode, MinFixed, MaxFixed,
			Cacheable, ReadWrite,
			0x00000000, 0x000dc000, 0x000dffff, 0x00000000,
			0x00004000,,, OPR7)

	/* BIOS Extension (0xe0000-0xe3fff) */
	DWordMemory (ResourceProducer, PosDecode, MinFixed, MaxFixed,
			Cacheable, ReadWrite,
			0x00000000, 0x000e0000, 0x000e3fff, 0x00000000,
			0x00004000,,, ESG0)

	/* BIOS Extension (0xe4000-0xe7fff) */
	DWordMemory (ResourceProducer, PosDecode, MinFixed, MaxFixed,
			Cacheable, ReadWrite,
			0x00000000, 0x000e4000, 0x000e7fff, 0x00000000,
			0x00004000,,, ESG1)

	/* BIOS Extension (0xe8000-0xebfff) */
	DWordMemory (ResourceProducer, PosDecode, MinFixed, MaxFixed,
			Cacheable, ReadWrite,
			0x00000000, 0x000e8000, 0x000ebfff, 0x00000000,
			0x00004000,,, ESG2)

	/* BIOS Extension (0xec000-0xeffff) */
	DWordMemory (ResourceProducer, PosDecode, MinFixed, MaxFixed,
			Cacheable, ReadWrite,
			0x00000000, 0x000ec000, 0x000effff, 0x00000000,
			0x00004000,,, ESG3)

	/* System BIOS (0xf0000-0xfffff) */
	DWordMemory (ResourceProducer, PosDecode, MinFixed, MaxFixed,
			Cacheable, ReadWrite,
			0x00000000, 0x000f0000, 0x000fffff, 0x00000000,
			0x00010000,,, FSEG)

	/* LPEA Memory Region (0x20000000-0x201FFFFF) */
	DWordMemory (ResourceProducer, PosDecode, MinFixed, MaxFixed,
			Cacheable, ReadWrite,
			0x00000000, 0x00000000, 0x00000000, 0x00000000,
			0x00000000,,, LMEM)

	/* PCI Memory Region (Top of memory-CONFIG_ECAM_MMCONF_BASE_ADDRESS) */
	DWordMemory (ResourceProducer, PosDecode, MinFixed, MaxFixed,
			Cacheable, ReadWrite,
			0x00000000, 0x00000000, 0x00000000, 0x00000000,
                        0x00000000,,, PMEM)

	/* TPM Area (0xfed40000-0xfed44fff) */
	DWordMemory (ResourceProducer, PosDecode, MinFixed, MaxFixed,
			Cacheable, ReadWrite,
			0x00000000, 0xfed40000, 0xfed44fff, 0x00000000,
			0x00005000,,, TPMR)
})

Method (_CRS, 0, Serialized)
{

	/* Update LPEA resource area */
	CreateDWordField (MCRS, ^LMEM._MIN, LMIN)
	CreateDWordField (MCRS, ^LMEM._MAX, LMAX)
	CreateDWordField (MCRS, ^LMEM._LEN, LLEN)
	If (LPFW != 0 && LPEN == 1)
	{
		LMIN = LPFW
		LLEN = 0x00100000
		LMAX = LMIN + LLEN - 1
	}
	Else
	{
		LMIN = 0
		LMAX = 0
		LLEN = 0
	}

	/* Update PCI resource area */
	CreateDWordField (MCRS, ^PMEM._MIN, PMIN)
	CreateDWordField (MCRS, ^PMEM._MAX, PMAX)
	CreateDWordField (MCRS, ^PMEM._LEN, PLEN)

	/* TOLM is BMBOUND accessible from IOSF so is saved in NVS */
	PMIN = \TOLM
	PMAX = CONFIG_ECAM_MMCONF_BASE_ADDRESS - 1
	PLEN = PMAX - PMIN + 1

	Return (MCRS)
}

/* Device Resource Consumption */
Device (PDRC)
{
	Name (_HID, EISAID("PNP0C02"))
	Name (_UID, 1)

	Name (PDRS, ResourceTemplate() {
		Memory32Fixed(ReadWrite, ABORT_BASE_ADDRESS, ABORT_BASE_SIZE)
		Memory32Fixed(ReadWrite, PMC_BASE_ADDRESS, PMC_BASE_SIZE)
		Memory32Fixed(ReadWrite, ILB_BASE_ADDRESS, ILB_BASE_SIZE)
		Memory32Fixed(ReadWrite, SPI_BASE_ADDRESS, SPI_BASE_SIZE)
		Memory32Fixed(ReadWrite, MPHY_BASE_ADDRESS, MPHY_BASE_SIZE)
		Memory32Fixed(ReadWrite, PUNIT_BASE_ADDRESS, PUNIT_BASE_SIZE)
		Memory32Fixed(ReadWrite, RCBA_BASE_ADDRESS, RCBA_BASE_SIZE)
	})

	/* Current Resource Settings */
	Method (_CRS, 0, Serialized)
	{
		Return(PDRS)
	}
}

Method (_OSC, 4)
{
	/* Check for proper GUID */
	If (Arg0 == ToUUID("33DB4D5B-1FF7-401C-9657-7441C03DD766"))
	{
		/* Let OS control everything */
		Return (Arg3)
	}
	Else
	{
		/* Unrecognized UUID */
		CreateDWordField (Arg3, 0, CDW1)
		CDW1 |= 4
		Return (Arg3)
	}
}

/* IOSF MBI Interface for kernel access */
Device (IOSF)
{
	Name (_HID, "INT33BD")
	Name (_CID, "INT33BD")
	Name (_UID, 1)

	Name (RBUF, ResourceTemplate ()
	{
		/* MCR / MDR / MCRX */
		Memory32Fixed (ReadWrite, 0, 12, RBAR)
	})

	Method (_CRS)
	{
		CreateDwordField (^RBUF, ^RBAR._BAS, RBAS)
		RBAS = CONFIG_ECAM_MMCONF_BASE_ADDRESS + 0xD0
		Return (^RBUF)
	}

	Method (_STA)
	{
#if CONFIG(CHROMEOS)
		Return (0xF)
#else
		Return (0x0)
#endif
	}
}

/* LPC Bridge 0:1f.0 */
#include "lpc.asl"

/* SATA 0:13.0 */
#include "sata.asl"

/* USB XHCI 0:14.0 */
#include "xhci.asl"

/* USB OTG 0:16.0 */
#include "otg.asl"

/* TXE 0:1a.0 */
#include "txe.asl"

/* HD Audio 0:1b.0 */
#include "hda.asl"

/* IRQ routing for each PCI device */
#include "irqroute.asl"

/* PCI Express Ports 0:1c.x */
#include "pcie.asl"

Scope (\_SB)
{
	/* GPIO Devices */
	#include "gpio.asl"
}

Scope (\_SB.PCI0)
{
	/* LPSS Devices */
	#include "lpss.asl"

	/* SCC Devices */
	#include "scc.asl"

	/* LPE Device */
	#include "lpe.asl"
}

/* Integrated graphics 0:2.0 */
#include <drivers/intel/gma/acpi/gfx.asl>
