/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <intelblocks/pcr.h>
#include <soc/gpe.h>

/* PCIE device */
#include "pcie.asl"

/* LPSS device */
#include "lpss.asl"

/* PCI IRQ assignment */
#include "pci_irqs.asl"

/* GPIO controller */
#include "gpio.asl"

#include "xhci.asl"

/* LPC */
#include <soc/intel/common/block/acpi/acpi/lpc.asl>

/* SMBUS */
#include "smbus.asl"

/* eMMC */
#include "scs.asl"

/* PMC IPC controller */
#include "pmc_ipc.asl"

/* PCI _OSC */
#include <soc/intel/common/acpi/pci_osc.asl>

/* SGX */
#include <soc/intel/common/acpi/sgx.asl>
