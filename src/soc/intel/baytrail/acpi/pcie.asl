/* SPDX-License-Identifier: GPL-2.0-only */

/* Intel SOC PCIe support */

Device (RP01)
{
	Name (_ADR, 0x001c0000)
	Name (_PRW, Package() {9, 3})

	OperationRegion (PXCS, PCI_Config, 0x40, 0xC0)
	Field (PXCS, AnyAcc, NoLock, Preserve)
	{
		Offset(0x12),
		,	13,
		LASX,	1,
		Offset(0x1A),
		ABPX,	1,
		,	2,
		PDCX,	1,
		,	2,
		PDSX,	1,
		,	1,
		Offset(0x20),
		,	16,
		PSPX,	1,
	}

	Device (PXSX)
	{
		Name (_ADR, 0x00000000)
		Name (_PRW, Package() {9,3})
	}

	Method (_PRT)
	{
		If (PICM) {
			Return (Package() {
				#undef PIC_MODE
				#include <soc/intel/baytrail/acpi/irq_helper.h>
				PCI_DEV_PIRQ_ROUTE(0x0, A, B, C, D)
			})
		} Else {
			Return (Package() {
				#define PIC_MODE
				#include <soc/intel/baytrail/acpi/irq_helper.h>
				PCI_DEV_PIRQ_ROUTE(0x0, A, B, C, D)
			})
		}
	}
}

Device (RP02)
{
	Name (_ADR, 0x001c0001)
	Name (_PRW, Package() {9, 3})

	OperationRegion (PXCS, PCI_Config, 0x40, 0xC0)
	Field (PXCS, AnyAcc, NoLock, Preserve)
	{
		Offset(0x12),
		,	13,
		LASX,	1,
		Offset(0x1A),
		ABPX,	1,
		,	2,
		PDCX,	1,
		,	2,
		PDSX,	1,
		,	1,
		Offset(0x20),
		,	16,
		PSPX,	1,
	}

	Device (PXSX)
	{
		Name (_ADR, 0x00000000)
		Name (_PRW, Package() {9,3})
	}

	Method (_PRT)
	{
		If (PICM) {
			Return (Package() {
				#undef PIC_MODE
				#include <soc/intel/baytrail/acpi/irq_helper.h>
				PCI_DEV_PIRQ_ROUTE(0x0, B, C, D, A)
			})
		} Else {
			Return (Package() {
				#define PIC_MODE
				#include <soc/intel/baytrail/acpi/irq_helper.h>
				PCI_DEV_PIRQ_ROUTE(0x0, B, C, D, A)
			})
		}
	}
}

Device (RP03)
{
	Name (_ADR, 0x001c0002)
	Name (_PRW, Package() {9, 3})

	OperationRegion (PXCS, PCI_Config, 0x40, 0xC0)
	Field (PXCS, AnyAcc, NoLock, Preserve)
	{
		Offset(0x12),
		,	13,
		LASX,	1,
		Offset(0x1A),
		ABPX,	1,
		,	2,
		PDCX,	1,
		,	2,
		PDSX,	1,
		,	1,
		Offset(0x20),
		,	16,
		PSPX,	1,
	}

	Device (PXSX)
	{
		Name (_ADR, 0x00000000)
		Name (_PRW, Package() {9,3})
	}

	Method (_PRT)
	{
		If (PICM) {
			Return (Package() {
				#undef PIC_MODE
				#include <soc/intel/baytrail/acpi/irq_helper.h>
				PCI_DEV_PIRQ_ROUTE(0x0, C, D, A, B)
			})
		} Else {
			Return (Package() {
				#define PIC_MODE
				#include <soc/intel/baytrail/acpi/irq_helper.h>
				PCI_DEV_PIRQ_ROUTE(0x0, C, D, A, B)
			})
		}
	}
}

Device (RP04)
{
	Name (_ADR, 0x001c0003)
	Name (_PRW, Package() {9, 3})

	OperationRegion (PXCS, PCI_Config, 0x40, 0xC0)
	Field (PXCS, AnyAcc, NoLock, Preserve)
	{
		Offset(0x12),
		,	13,
		LASX,	1,
		Offset(0x1A),
		ABPX,	1,
		,	2,
		PDCX,	1,
		,	2,
		PDSX,	1,
		,	1,
		Offset(0x20),
		,	16,
		PSPX,	1,
	}

	Device (PXSX)
	{
		Name (_ADR, 0x00000000)
		Name (_PRW, Package() {9,3})
	}

	Method (_PRT)
	{
		If (PICM) {
			Return (Package() {
				#undef PIC_MODE
				#include <soc/intel/baytrail/acpi/irq_helper.h>
				PCI_DEV_PIRQ_ROUTE(0x0, D, A, B, C)
			})
		} Else {
			Return (Package() {
				#define PIC_MODE
				#include <soc/intel/baytrail/acpi/irq_helper.h>
				PCI_DEV_PIRQ_ROUTE(0x0, D, A, B, C)
			})
		}
	}
}
