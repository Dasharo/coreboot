/* SPDX-License-Identifier: GPL-2.0-only */

/* PCIe Ports */

Device (RP01)
{
	Name (_ADR, 0x00140000)
	Name (_DDN, "PCIe-B 0")

//	#include "pcie_port.asl"
}

Device (RP03)
{
	Name (_ADR, 0x00130000)
	Name (_DDN, "PCIe-A 0")

//	#include "pcie_port.asl"
}
