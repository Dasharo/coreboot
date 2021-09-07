/* SPDX-License-Identifier: GPL-2.0-only */

#include <device/device.h>
#include <drivers/ipmi/ipmi_bt.h>
#include <program_loading.h>
#include <cpu/power/istep_13.h>
#include <cpu/power/istep_18.h>

#include "istep_13_scom.h"
#include "chip.h"

#define SIZE_MASK	PPC_BITMASK(13,23)
#define SIZE_SHIFT	(63 - 23)
#define BASE_MASK	PPC_BITMASK(24,47)
#define BASE_SHIFT	(63 - 47)

/* Values in registers are in 4GB units, ram_resource_kb() expects kilobytes. */
#define CONVERT_4GB_TO_KB(x)	((x) << 22)

static inline unsigned long base_k(uint64_t reg)
{
	return CONVERT_4GB_TO_KB((reg & BASE_MASK) >> BASE_SHIFT);
}

static inline unsigned long size_k(uint64_t reg)
{
	return CONVERT_4GB_TO_KB(((reg & SIZE_MASK) >> SIZE_SHIFT) + 1);
}

static void enable_soc_dev(struct device *dev)
{
	int mcs_i, idx = 0;
	unsigned long reserved_size, top = 0;

	for (mcs_i = 0; mcs_i < MCS_PER_PROC; mcs_i++) {
		uint64_t reg;
		chiplet_id_t nest = mcs_to_nest[mcs_ids[mcs_i]];

		/* These registers are undocumented, see istep 14.5. */
		/* MCS_MCFGP */
		reg = read_scom_for_chiplet(nest, 0x0501080A);
		if (reg & PPC_BIT(0)) {
			ram_resource_kb(dev, idx++, base_k(reg), size_k(reg));
			if (base_k(reg) + size_k(reg) > top)
				top = base_k(reg) + size_k(reg);
		}

		/* MCS_MCFGPM */
		reg = read_scom_for_chiplet(nest, 0x0501080C);
		if (reg & PPC_BIT(0)) {
			ram_resource_kb(dev, idx++, base_k(reg), size_k(reg));
			if (base_k(reg) + size_k(reg) > top)
				top = base_k(reg) + size_k(reg);
		}
	}

	/*
	 * Reserve top 8M (OCC common area) + 4M (HOMER).
	 *
	 * TODO: 8M + (4M per CPU), hostboot reserves always 8M + 8 * 4M.
	 */
	reserved_size = 8*1024 + 4*1024 *8 /* * num_of_cpus */;
	top -= reserved_size;
	reserved_ram_resource_kb(dev, idx++, top, reserved_size);
	build_homer_image((void *)(top * 1024));

	istep_18_11();
	istep_18_12();
}

void platform_prog_run(struct prog *prog)
{
	/*
	 * TODO: do what 16.2 did now, when the payload and its interrupt
	 * vectors are already loaded
	 */

	/*
	 * Clear SMS_ATN aka EVT_ATN in BT_CTRL - Block Transfer IPMI protocol
	 *
	 * BMC sends event telling us that HIOMAP (access to flash, either real or
	 * emulated, through LPC) daemon has been started. This sets the mentioned bit.
	 * Skiboot enables interrupts, but because those are triggered on 0->1
	 * transition and bit is already set, they do not arrive.
	 *
	 * While we're at it, clear read and write pointers, in case circular buffer
	 * rolls over.
	 */
	if (ipmi_bt_clear(CONFIG_BMC_BT_BASE))
		die("ipmi_bt_clear() has failed.\n");
}

struct chip_operations soc_ibm_power9_ops = {
	CHIP_NAME("POWER9")
	.enable_dev = enable_soc_dev,
};
