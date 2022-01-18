/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef CPU_AMD_COMMON_H
#define CPU_AMD_COMMON_H

#include <arch/cpu.h>
#include <device/device.h>
#include <device/pci_ops.h>
#include <cpu/x86/lapic.h>
#include <types.h>

#include "nums.h"

#define LAPIC_MSG_REG 0x380
#define F10_APSTATE_STARTED 0x13  // start of AP execution
#define F10_APSTATE_ASLEEP  0x14  // AP sleeping
#define F10_APSTATE_STOPPED 0x15  // allow AP to stop
#define F10_APSTATE_RESET   0x01  // waiting for warm reset

#define MAX_CORES_SUPPORTED 128

struct node_core_id {
	u32 nodeid;
	u32 coreid;
};

u32 read_nb_cfg_54(void);
u64 get_logical_cpuid(u32 node);
u32 get_initial_apicid(void);
u32 get_core_num_in_bsp(u32 nodeid);
u8 set_apicid_cpuid_lo(void);
u8 get_processor_package_type(void);
struct node_core_id get_node_core_id(u32 nb_cfg_54);
struct node_core_id get_node_core_id_x(void);
u32 get_boot_apic_id(u8 node, u32 core);

void set_EnableCf8ExtCfg(void);

u32 get_platform_type(void);

static __always_inline u8 is_fam15h(void)
{
	u8 fam15h = 0;
	u32 family;

	family = cpuid_eax(0x80000001);
	family = ((family & 0xf00000) >> 16) | ((family & 0xf00) >> 8);

	if (family >= 0x6f)
		/* Family 15h or later */
		fam15h = 1;

	return fam15h;
}


static __always_inline u32 get_cpu_family(void)
{
	u32 family;

	family = cpuid_eax(0x80000001);
	family = ((family & 0xf00000) >> 16) | ((family & 0xf00) >> 8);

	return family;
}

static inline u8 is_gt_rev_d(void)
{
	u8 rev_gte_d = 0;
	u32 model;

	model = cpuid_eax(0x80000001);
	model = ((model & 0xf0000) >> 12) | ((model & 0xf0) >> 4);

	if ((model >= 0x8) || is_fam15h()) {
		/* Revision D or later */
		rev_gte_d = 1;
	}
	return rev_gte_d;
}

static __always_inline u8 is_dual_node(u8 node)
{
	/* Check for dual node capability */
	return !!(pci_read_config32(NODE_PCI(node, 3), 0xe8) & 0x20000000);
}

static __always_inline void lapic_wait_icr_idle(void)
{
	do { } while (lapic_read(LAPIC_ICR) & LAPIC_ICR_BUSY);
}


static inline int lapic_remote_read(int apicid, int reg, u32 *pvalue)
{
	int timeout;
	u32 status;
	int result;
	lapic_wait_icr_idle();
	lapic_write(LAPIC_ICR2, SET_LAPIC_DEST_FIELD(apicid));
	lapic_write(LAPIC_ICR, LAPIC_DM_REMRD | (reg >> 4));

	/* Extra busy check compared to lapic.h */
	timeout = 0;
	do {
		status = lapic_read(LAPIC_ICR) & LAPIC_ICR_BUSY;
	} while (status == LAPIC_ICR_BUSY && timeout++ < 1000);

	timeout = 0;
	do {
		status = lapic_read(LAPIC_ICR) & LAPIC_ICR_RR_MASK;
	} while (status == LAPIC_ICR_RR_INPROG && timeout++ < 1000);

	result = -1;

	if (status == LAPIC_ICR_RR_VALID) {
		*pvalue = lapic_read(LAPIC_RRR);
		result = 0;
	}
	return result;
}

#endif // CPU_AMD_COMMON_H
