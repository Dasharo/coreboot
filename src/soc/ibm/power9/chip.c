/* SPDX-License-Identifier: GPL-2.0-only */

#include <device/device.h>
#include <drivers/ipmi/ipmi_bt.h>
#include <program_loading.h>
#include <cpu/power/istep_13.h>
#include <cpu/power/istep_18.h>

#include "homer.h"
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

static void activate_slave_cores(void)
{
	enum { DOORBELL_MSG_TYPE = 0x0000000028000000 };

	uint8_t i;

	/* Read OCC CCSR written by the code earlier */
	const uint64_t functional_cores = read_scom(0x0006C090);

	/* Find and process the first core in a separate loop to slightly
	 * simplify processing of all the other cores by removing a conditional */
	for (i = 0; i < MAX_CORES_PER_CHIP; ++i) {
		uint8_t thread;
		uint64_t core_msg;

		if (!IS_EC_FUNCTIONAL(i, functional_cores))
			continue;

		/* Message value for thread 0 of the current core */
		core_msg = DOORBELL_MSG_TYPE | (i << 2);

		/* Skip sending doorbell to the current thread of the current core */
		for (thread = 1; thread < 4; ++thread) {
			register uint64_t msg = core_msg | thread;
			asm volatile("msgsnd %0" :: "r" (msg));
		}

		break;
	}

	for (++i; i < MAX_CORES_PER_CHIP; ++i) {
		uint8_t thread;
		uint64_t core_msg;

		if (!IS_EC_FUNCTIONAL(i, functional_cores))
			continue;

		/* Message value for thread 0 of the i-th core */
		core_msg = DOORBELL_MSG_TYPE | (i << 2);

		for (thread = 0; thread < 4; ++thread) {
			register uint64_t msg = core_msg | thread;
			asm volatile("msgsnd %0" :: "r" (msg));
		}
	}
}

void platform_prog_run(struct prog *prog)
{
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

	/*
	 * Now that the payload and its interrupt vectors are already loaded
	 * perform 16.2.
	 *
	 * This MUST be done as late as possible so that none of the newly
	 * activated threads start execution before current thread jumps into
	 * the payload.
	 */
	activate_slave_cores();
}

struct chip_operations soc_ibm_power9_ops = {
	CHIP_NAME("POWER9")
	.enable_dev = enable_soc_dev,
};
