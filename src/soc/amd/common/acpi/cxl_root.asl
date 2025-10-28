/* SPDX-License-Identifier: GPL-2.0-only */

/*
 * CXL_ROOT_BRIDGE macro produces too long line for iASL to compile
 * SoC code has to define CXL_BRIDGE_NAME and CXL_BRIDGE_UID, then
 * include this file times the amount of CXL host bridges in the system.
 * Example usage:
 *
 * #define CXL_BRIDGE_NAME S0B0
 * #define CXL_BRIDGE_UID 0
 * #include <soc/amd/common/acpi/cxl_root.asl>
 * #undef CXL_BRIDGE_UID
 * #undef CXL_BRIDGE_NAME
 */
Device(CXL_BRIDGE_NAME) {
	Name(_HID, "ACPI0016")		/* CXL Host Bridge */
	Name(_CID, Package (0x02)
	{
		EISAID("PNP0A08"),	/* PCI Express Root Bridge */
		EISAID("PNP0A03")	/* PCI Root Bridge */
	})
	Name(_UID, CXL_BRIDGE_UID)

	Method (_OSC, 4, NotSerialized) {
		/* Check for proper PCI/PCIe UUID */
		If (Arg0 == ToUUID("33DB4D5B-1FF7-401C-9657-7441C03DD766"))
		{
			/* Let OS control everything */
			Return(Arg3)
		}
		/* Check for proper CXL UUID */
		ElseIf (Arg0 == ToUUID("68F2D50B-C469-4D8A-BD3D-941A103FD3FC"))
		{
			/* Let OS control everything */
			Return(Arg3)
		}
		Else {
			CreateDWordField(Arg3, 0, CDW1)
			CDW1 = CDW1 | 4	/* Unrecognized UUID, so set bit 2 to 1 */
			Return(Arg3)
		}
	}
}
