/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __SOC_INTEL_COMMON_BLOCK_PCIE_RTD3_CHIP_H__
#define __SOC_INTEL_COMMON_BLOCK_PCIE_RTD3_CHIP_H__

#include <acpi/acpi_device.h>

enum acpi_pcie_rp_pm_emit {
	ACPI_PCIE_RP_EMIT_NONE      = 0x00,   /* None            */
	ACPI_PCIE_RP_EMIT_L23       = 0x01,   /* L23             */
	ACPI_PCIE_RP_EMIT_PSD0      = 0x02,   /* PSD0            */
	ACPI_PCIE_RP_EMIT_SRCK      = 0x04,   /* SRCK            */
	ACPI_PCIE_RP_EMIT_L23_PSD0  = 0x03,   /* L23, PSD0       */
	ACPI_PCIE_RP_EMIT_L23_SRCK  = 0x05,   /* L23, SRCK       */
	ACPI_PCIE_RP_EMIT_PSD0_SRCK = 0x06,   /* PSD0, SRCK      */
	ACPI_PCIE_RP_EMIT_ALL       = 0x07    /* L23, PSD0, SRCK */
};

/* Device support at least one of enable/reset GPIO. */
struct soc_intel_common_block_pcie_rtd3_config {
	const char *desc;

	/* GPIO used to enable device. */
	struct acpi_gpio enable_gpio;
	/* Delay to be inserted after device is enabled. */
	unsigned int enable_delay_ms;
	/* Delay to be inserted after device is disabled. */
	unsigned int enable_off_delay_ms;

	/* GPIO used to take device out of reset or to put it into reset. */
	struct acpi_gpio reset_gpio;
	/* Delay to be inserted after device is taken out of reset. */
	unsigned int reset_delay_ms;
	/* Delay to be inserted after device is put into reset. */
	unsigned int reset_off_delay_ms;

	/*
	 * SRCCLK assigned to this root port which will be turned off via PMC IPC.
	 * If set to -1 then the clock will not be disabled in D3.
	 */
	int srcclk_pin;

	/*
	 * Add device property indicating the device provides an external PCI port
	 * for the OS to apply security restrictions.
	 */
	bool is_external;

	/*
	 * Allow a device to add the RuntimeD3Storage property even if the detected
	 * PCI device does not identify as storage class.
	 */
	bool is_storage;

	/*
	 * Disable the ACPI-driven L23 Ready-to-Detect transition for the root port.
	 */
	bool disable_l23;

	/*
	 * CPU PCIe port number has to be passed manually and handled differently
	 */
	unsigned int cpu_pcie_clk_usage;
};

#endif /* __SOC_INTEL_COMMON_BLOCK_PCIE_RTD3_CHIP_H__ */
