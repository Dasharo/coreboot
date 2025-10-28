/* SPDX-License-Identifier: GPL-2.0-only */

#include "globalnvs.asl"

Scope(\_SB) {
	/* global utility methods expected within the \_SB scope */
	#include <arch/x86/acpi/globutil.asl>

	#include <soc/amd/common/acpi/gpio_bank_lib.asl>

	#include <soc/amd/common/acpi/osc.asl>

	/* PCI IRQ mapping for the Southbridge */
	#include "pci_int_defs.asl"

	/* Describe PCI INT[A-H] for the Southbridge */
	#include <soc/amd/common/acpi/pci_int.asl>

	#include "mmio.asl"

	#define CXL_BRIDGE_NAME S0B0
	#define CXL_BRIDGE_UID 0
	#include <soc/amd/common/acpi/cxl_root.asl>
	#undef CXL_BRIDGE_UID
	#undef CXL_BRIDGE_NAME

	#define CXL_BRIDGE_NAME S0B1
	#define CXL_BRIDGE_UID 1
	#include <soc/amd/common/acpi/cxl_root.asl>
	#undef CXL_BRIDGE_UID
	#undef CXL_BRIDGE_NAME

	#define CXL_BRIDGE_NAME S0B2
	#define CXL_BRIDGE_UID 2
	#include <soc/amd/common/acpi/cxl_root.asl>
	#undef CXL_BRIDGE_UID
	#undef CXL_BRIDGE_NAME

	#define CXL_BRIDGE_NAME S0B3
	#define CXL_BRIDGE_UID 3
	#include <soc/amd/common/acpi/cxl_root.asl>
	#undef CXL_BRIDGE_UID
	#undef CXL_BRIDGE_NAME

	#define CXL_BRIDGE_NAME S0B4
	#define CXL_BRIDGE_UID 4
	#include <soc/amd/common/acpi/cxl_root.asl>
	#undef CXL_BRIDGE_UID
	#undef CXL_BRIDGE_NAME

	#define CXL_BRIDGE_NAME S0B5
	#define CXL_BRIDGE_UID 5
	#include <soc/amd/common/acpi/cxl_root.asl>
	#undef CXL_BRIDGE_UID
	#undef CXL_BRIDGE_NAME

	#define CXL_BRIDGE_NAME S0B6
	#define CXL_BRIDGE_UID 6
	#include <soc/amd/common/acpi/cxl_root.asl>
	#undef CXL_BRIDGE_UID
	#undef CXL_BRIDGE_NAME

	#define CXL_BRIDGE_NAME S0B7
	#define CXL_BRIDGE_UID 7
	#include <soc/amd/common/acpi/cxl_root.asl>
	#undef CXL_BRIDGE_UID
	#undef CXL_BRIDGE_NAME

	#include "cxl_root_dev.asl"

	Scope(S0B0) {
		#include "resources.asl"
		#include <soc/amd/common/acpi/lpc.asl>
	} /* End S0B0 scope */

	#include "ioapic_routing.asl"
} /* End \_SB scope */

#include <soc/amd/common/acpi/alib.asl>

#include <soc/amd/common/acpi/platform.asl>

#include <soc/amd/common/acpi/sleepstates.asl>

/*
 * Platform Notify
 *
 * This is called by soc/amd/common/acpi/platform.asl.
 */
Method (PNOT)
{
}
