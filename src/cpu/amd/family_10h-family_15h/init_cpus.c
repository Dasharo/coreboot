/* SPDX-License-Identifier: GPL-2.0-only */

#include <arch/cpu.h>
#include <arch/symbols.h>
#include <console/console.h>
#include <cpu/amd/common/common.h>
#include <cpu/amd/common/nums.h>
#include <cpu/amd/amddefs.h>
#include <cpu/amd/microcode.h>
#include <cpu/amd/msr.h>
#include <cpu/amd/mtrr.h>
#include <cpu/x86/mtrr.h>
#include <cpu/x86/lapic.h>
#include <device/pci_ops.h>
#include <drivers/amd/hypertransport/h3ncmn.h>
#include <drivers/amd/hypertransport/ht_wrapper.h>
#include <northbridge/amd/amdfam10/amdfam10.h>
#include <southbridge/amd/common/reset.h>
#include <southbridge/amd/sb700/sb700.h>
#include <delay.h>
#include <option.h>
#include <types.h>

#include "defaults.h"
#include "family_10h_15h.h"
#include "fidvid.h"

//core_range = 0 : all cores
//core range = 1 : core 0 only
//core range = 2 : cores other than core0
void for_each_ap(u32 bsp_apic_id, u32 core_range, s8 node,
		 process_ap_t process_ap, void *gp)
{
	// here assume the OS don't change our apic_id
	u32 ap_apicid;

	u32 nodes;
	u32 disable_siblings;
	u32 cores_found;
	int i, j;

	/* get_nodes define in ht_wrapper.c */
	nodes = get_nodes();

	if (!CONFIG(LOGICAL_CPUS) || get_uint_option("multi_core", 1) == 0) {
		disable_siblings = 1;
	} else {
		disable_siblings = 0;
	}

	for (i = 0; i < nodes; i++) {
		if ((node >= 0) && (i != node))
			continue;

		cores_found = get_core_num_in_bsp(i);

		u32 jstart, jend;

		if (core_range == 2) {
			jstart = 1;
		} else {
			jstart = 0;
		}

		if (disable_siblings || (core_range == 1)) {
			jend = 0;
		} else {
			jend = cores_found;
		}

		for (j = jstart; j <= jend; j++) {
			ap_apicid = get_boot_apic_id(i, j);

			if (CONFIG(ENABLE_APIC_EXT_ID) && (CONFIG_APIC_ID_OFFSET > 0)) {
				if ((i != 0) || (j != 0)) /* except bsp */
					ap_apicid += CONFIG_APIC_ID_OFFSET;
				else if (CONFIG(LIFT_BSP_APIC_ID) && ((i == 0) || (j == 0)))
					ap_apicid += CONFIG_APIC_ID_OFFSET;
			}

			if (ap_apicid == bsp_apic_id)
				continue;

			process_ap(ap_apicid, gp);

		}
	}
}

static __always_inline
void print_apicid_nodeid_coreid(u32 apic_id, struct node_core_id id,
				const char *str)
{
	printk(BIOS_DEBUG,
		"%s --- { APICID = %02x NODEID = %02x COREID = %02x} ---\n",
		str, apic_id, id.nodeid, id.coreid);
}

static u32 wait_cpu_state(u32 apic_id, u32 state, u32 state2)
{
	u32 readback = 0;
	u32 timeout = 1;
	int loop = 4000000;
	while (--loop > 0) {
		if (lapic_remote_read(apic_id, LAPIC_MSG_REG, &readback) != 0)
			continue;
		if ((readback & 0x3f) == state || (readback & 0x3f) == state2
				|| (readback & 0x3f) == F10_APSTATE_RESET) {
			timeout = 0;
			break;	//target CPU is in stage started
		}
	}
	if (timeout && readback) {
		timeout = readback;
	}

	return timeout;
}

static void wait_ap_started(u32 ap_apicid, void *gp)
{
	u32 timeout;
	timeout = wait_cpu_state(ap_apicid, F10_APSTATE_STARTED, F10_APSTATE_ASLEEP);
	printk(BIOS_DEBUG, "* AP %02x", ap_apicid);
	if (timeout) {
		printk(BIOS_DEBUG, " timed out:%08x\n", timeout);
	} else {
		printk(BIOS_DEBUG, "started\n");
	}
}

static void wait_all_other_cores_started(u32 bsp_apic_id)
{
	// all aps other than core0
	printk(BIOS_DEBUG, "started ap apic_id: ");
	for_each_ap(bsp_apic_id, 2, -1, wait_ap_started, (void *)0);
	printk(BIOS_DEBUG, "\n");
}

static void wait_ap_stopped(u32 ap_apicid, void *gp)
{
	u32 timeout;
	timeout = wait_cpu_state(ap_apicid, F10_APSTATE_ASLEEP, F10_APSTATE_ASLEEP);
	printk(BIOS_DEBUG, "* AP %02x", ap_apicid);
	if (timeout) {
		printk(BIOS_DEBUG, " timed out:%08x\n", timeout);
	} else {
		printk(BIOS_DEBUG, "stopped\n");
	}
}

void wait_all_other_cores_stopped(u32 bsp_apic_id)
{
	// all aps other than core0
	printk(BIOS_DEBUG, "stopped ap apic_id: ");
	for_each_ap(bsp_apic_id, 2, -1, wait_ap_stopped, (void *)0);
	printk(BIOS_DEBUG, "\n");
}

static void enable_apic_ext_id(u32 node)
{
	u32 val;

	val = pci_read_config32(NODE_PCI(node, 0), 0x68);
	val |= (HTTC_APIC_EXT_SPUR | HTTC_APIC_EXT_ID | HTTC_APIC_EXT_BRD_CST);
	pci_write_config32(NODE_PCI(node, 0), 0x68, val);
}

static __always_inline
void disable_cache_as_ram(u8 skip_sharedc_config)
{
	msr_t msr;
	u32 family;

	if (!skip_sharedc_config) {
		/* disable cache */
		write_cr0(read_cr0() | CR0_CacheDisable);

		msr.lo = 0;
		msr.hi = 0;
		wrmsr(MTRR_FIX_64K_00000, msr);


		/* disable fixed mtrr from now on,
		 * it will be enabled by ramstage again
		 */
		msr = rdmsr(SYSCFG_MSR);
		msr.lo &= ~(SYSCFG_MSR_MtrrFixDramEn
			| SYSCFG_MSR_MtrrFixDramModEn);
		wrmsr(SYSCFG_MSR, msr);

		/* Set the default memory type and
		 * disable fixed and enable variable MTRRs
		 */
		msr.hi = 0;
		msr.lo = MTRR_DEF_TYPE_EN;

		wrmsr(MTRR_DEF_TYPE_MSR, msr);

		enable_cache();
	}

	/* INVDWBINVD = 1 */
	msr = rdmsr(HWCR_MSR);
	msr.lo |= (0x1 << 4);
	wrmsr(HWCR_MSR, msr);

	family = get_cpu_family();

	if (CONFIG(CPU_AMD_MODEL_10XXX) && family >= 0x6f) {
		/* Family 15h or later */

		/* DisSS = 0 */
		msr = rdmsr(LS_CFG_MSR);
		msr.lo &= ~(0x1 << 28);
		wrmsr(LS_CFG_MSR, msr);

		if (!skip_sharedc_config) {
			/* DisSpecTlbRld = 0 */
			msr = rdmsr(IC_CFG_MSR);
			msr.lo &= ~(0x1 << 9);
			wrmsr(IC_CFG_MSR, msr);

			/* Erratum 714: SpecNbReqDis = 0 */
			msr = rdmsr(BU_CFG2_MSR);
			msr.lo &= ~(0x1 << 8);
			wrmsr(BU_CFG2_MSR, msr);
		}

		/* DisSpecTlbRld = 0 */
		/* DisHwPf = 0 */
		msr = rdmsr(DC_CFG_MSR);
		msr.lo &= ~(0x1 << 4);
		msr.lo &= ~(0x1 << 13);
		wrmsr(DC_CFG_MSR, msr);
	}
}


static void stop_car_and_cpu(u8 skip_sharedc_config, u32 apic_id)
{
	msr_t msr;
	u32 family;

	family = get_cpu_family();

	if (family < 0x6f) {
		/* Family 10h or earlier */

		/* Disable L2 IC to L3 connection (Only for CAR) */
		msr = rdmsr(BU_CFG2_MSR);
		msr.lo &= ~(1 << CL_LINES_TO_NB_DIS);
		wrmsr(BU_CFG2_MSR, msr);
	} else {
		/* Family 15h or later
		 * DRAM setup is delayed on Fam15 in order to prevent
		 * any DRAM access before ECC check bits are initialized.
		 * Each core also needs to have its initial DRAM map initialized
		 * before it is put to sleep, otherwise it will fail to wake
		 * in ramstage.  To meet both of these goals, delay DRAM map
		 * setup until the last possible moment, where speculative
		 * memory access is highly unlikely before core halt...
		 */
		if (!skip_sharedc_config) {
			/* Enable memory access for whole memory under 4G using top_mem.
			 * CONFIG_MMCONF_BASE_ADDRESS will likely be the boundary between DRAM
			 * and MMIO under 4G, if nto ramstage will set the MTRRs correclty.
			 */
			msr.hi = 0;
			msr.lo = CONFIG_MMCONF_BASE_ADDRESS;
			wrmsr(TOP_MEM, msr);
		}
	}

	disable_cache_as_ram(skip_sharedc_config);	// inline

	/* Mark the core as sleeping */
	lapic_write(LAPIC_MSG_REG, (apic_id << 24) | F10_APSTATE_ASLEEP);

	/* stop all cores except node0/core0 the bsp */
	stop_this_cpu();
}

static void configure_fidvid(u32 apic_id, struct node_core_id id)
{
	if (CONFIG(LOGICAL_CPUS) && CONFIG(SET_FIDVID_CORE0_ONLY)) {
		if (id.coreid != 0)	// only need set fid for core0
			return;
	}

	// Run on all AP for proper FID/VID setup.
	// check warm(bios) reset to call stage2 otherwise do stage1
	if (warm_reset_detect(id.nodeid)) {
		printk(BIOS_DEBUG,
			"init_fidvid_stage2 apic_id: %02x\n",
			apic_id);
		init_fidvid_stage2(apic_id, id.nodeid);
	} else {
		printk(BIOS_DEBUG,
			"init_fidvid_ap(stage1) apic_id: %02x\n",
			apic_id);
		init_fidvid_ap(apic_id, id.nodeid, id.coreid);
	}

}

static u32 is_core0_started(u32 nodeid)
{
	u32 htic;
	htic = pci_read_config32(NODE_PCI(nodeid, 0), HT_INIT_CONTROL);
	htic &= HTIC_COLDR_DETECT;
	return htic;
}

static void wait_all_core0_started(void)
{
	/* When core0 is started, it will distingush_cpu_resets
	 * So wait for that to finish */
	u32 i;
	u32 nodes = get_nodes();

	for (i = 1; i < nodes; i++) {	// skip bsp, because it is running on bsp
		while (!is_core0_started(i)) {
		}
		printk(BIOS_DEBUG, "core0 started, node %02x\n", i);
	}
}


static u32 init_cpus(struct sys_info *sysinfo)
{
	u32 bsp_apic_id = 0;
	u32 apic_id;
	u32 dword;
	u8 set_mtrrs;
	u8 node_count;
	u8 fam15_bsp_core1_apic_id;
	struct node_core_id id;

	/* That is from initial apic_id, we need nodeid and coreid
	 * later.
	 */
	id = get_node_core_id_x();

	/* NB_CFG MSR is shared between cores, so we need make sure
	 * core0 is done at first --- use wait_all_core0_started
	 */
	if (id.coreid == 0) {
		/* Set InitApicIdCpuIdLo / EnableCf8ExtCfg on core0 only */
		if (!is_fam15h())
			set_apicid_cpuid_lo();
		set_EnableCf8ExtCfg();
		if (CONFIG(ENABLE_APIC_EXT_ID))
			enable_apic_ext_id(id.nodeid);
	}

	enable_lapic();

	if (CONFIG(ENABLE_APIC_EXT_ID) && (CONFIG_APIC_ID_OFFSET > 0)) {
		u32 initial_apicid = get_initial_apicid();

		if (CONFIG(LIFT_BSP_APIC_ID)) {
			bsp_apic_id += CONFIG_APIC_ID_OFFSET;
		} else if (initial_apicid != 0) { // other than bsp
			/* use initial APIC id to lift it */
			dword = lapic_read(LAPIC_ID);
			dword &= ~(0xff << 24);
			dword |= (((initial_apicid + CONFIG_APIC_ID_OFFSET) & 0xff) << 24);
			lapic_write(LAPIC_ID, dword);
		}
	}

	/* get the apic_id, it may be lifted already */
	apic_id = lapicid();

	// show our apic_id, nodeid, and coreid
	if (id.coreid == 0) {
		if (id.nodeid != 0)	//all core0 except bsp
			print_apicid_nodeid_coreid(apic_id, id, " core0: ");
	} else {		//all other cores
		print_apicid_nodeid_coreid(apic_id, id, " corex: ");
	}

	printk(BIOS_DEBUG, "CPU INIT detect %08x\n", pci_read_config32(PCI_DEV(0,0,0), 0x80));

	if (pci_read_config32(PCI_DEV(0,0,0), 0x80) & (1 << apic_id)) {
		print_apicid_nodeid_coreid(apic_id, id,
					   "\n\n\nINIT detected from ");
		printk(BIOS_DEBUG, "\nIssuing SOFT_RESET...\n");
		soft_reset();
	}

	if (id.coreid == 0) {
		//FIXME: INIT is checked above but check for more resets?
		if (!(warm_reset_detect(id.nodeid))) {
			printk(BIOS_DEBUG, "Warm reset not detected, node %02d\n", id.nodeid);
			distinguish_cpu_resets(id.nodeid); // Also indicates we are started
		}
	}

	// Mark the core as started.
	printk(BIOS_DEBUG, "CPU APICID %02x start flag set\n", apic_id);
	lapic_write(LAPIC_MSG_REG, (apic_id << 24) | F10_APSTATE_STARTED);

	if (apic_id != bsp_apic_id) {
		/* Setup each AP's cores MSRs.
		 * This happens after HTinit.
		 * The BSP runs this code in it's own path.
		 */
		amd_update_microcode_from_cbfs();

		cpu_set_amd_msr((u8)id.nodeid);

		/* Set up HyperTransport probe filter support */
		if (is_gt_rev_d()) {
			dword = pci_read_config32(NODE_PCI(id.nodeid, 0), 0x60);
			node_count = ((dword >> 4) & 0x7) + 1;

			if (node_count > 1) {
				msr_t msr = rdmsr(BU_CFG2_MSR);
				msr.hi |= 1 << (42 - 32);
				wrmsr(BU_CFG2_MSR, msr);
			}
		}

		if (CONFIG(SET_FIDVID))
			configure_fidvid(apic_id, id);

		if (is_fam15h()) {
			/* core 1 on node 0 is special; to avoid corrupting the
			 * BSP do not alter MTRRs on that core */
			fam15_bsp_core1_apic_id = 1;
			if (CONFIG(ENABLE_APIC_EXT_ID) && (CONFIG_APIC_ID_OFFSET > 0))
				fam15_bsp_core1_apic_id += CONFIG_APIC_ID_OFFSET;

			if (apic_id == fam15_bsp_core1_apic_id)
				set_mtrrs = 0;
			else
				set_mtrrs = !!(apic_id & 0x1);
		} else {
			set_mtrrs = 1;
		}

		/* AP is ready, configure MTRRs to cache whole RAM under 4G and go to sleep */
		if (set_mtrrs)
			set_var_mtrr(0, 0x00000000, CONFIG_MMCONF_BASE_ADDRESS,
				     MTRR_TYPE_WRBACK);

		printk(BIOS_DEBUG, "Disabling CAR on AP %02x\n", apic_id);
		if (is_fam15h()) {
			/* Only modify the MSRs on the odd cores
			 * (the last cores to finish booting)
			 */
			stop_car_and_cpu(!set_mtrrs, apic_id);
		} else {
			/* Modify MSRs on all cores */
			stop_car_and_cpu(0, apic_id);
		}

		printk(BIOS_DEBUG, "\nAP %02x should be halted but you are reading this....\n",
		       apic_id);
	}

	return bsp_apic_id;
}

/**
 * void start_node(u32 node)
 *
 *  start the core0 in node, so it can generate HT packet to feature code.
 *
 * This function starts the AP nodes core0s. wait_all_core0_started() in
 * romstage.c waits for all the AP to be finished before continuing
 * system init.
 */
static void start_node(u8 node)
{
	u32 val;

	/* Enable routing table */
	printk(BIOS_DEBUG, "Start node %02x", node);

	if (CONFIG(NORTHBRIDGE_AMD_AMDFAM10)) {
		/* For FAM10 support, we need to set Dram base/limit for the new node */
		pci_write_config32(NODE_PCI(node, 1), 0x44, 0);
		pci_write_config32(NODE_PCI(node, 1), 0x40, 3);
	}

	/* Allow APs to make requests (ROM fetch) */
	val = pci_read_config32(NODE_PCI(node, 0), 0x6c);
	val &= ~(1 << 1);
	pci_write_config32(NODE_PCI(node, 0), 0x6c, val);

	printk(BIOS_DEBUG, " done.\n");
}

/**
 * Copy the BSP Address Map to each AP.
 */
static void setup_remote_node(u8 node)
{
	/* There registers can be used with F1x114_x Address Map at the
	   same time, So must set them even 32 node */
	static const u16 pci_reg[] = {
		/* DRAM Base/Limits Registers */
		0x44, 0x4c, 0x54, 0x5c, 0x64, 0x6c, 0x74, 0x7c,
		0x40, 0x48, 0x50, 0x58, 0x60, 0x68, 0x70, 0x78,
		0x144, 0x14c, 0x154, 0x15c, 0x164, 0x16c, 0x174, 0x17c,
		0x140, 0x148, 0x150, 0x158, 0x160, 0x168, 0x170, 0x178,
		/* MMIO Base/Limits Registers */
		0x84, 0x8c, 0x94, 0x9c, 0xa4, 0xac, 0xb4, 0xbc,
		0x80, 0x88, 0x90, 0x98, 0xa0, 0xa8, 0xb0, 0xb8,
		/* IO Base/Limits Registers */
		0xc4, 0xcc, 0xd4, 0xdc,
		0xc0, 0xc8, 0xd0, 0xd8,
		/* Configuration Map Registers */
		0xe0, 0xe4, 0xe8, 0xec,
	};
	u16 i;

	printk(BIOS_DEBUG, "setup_remote_node: %02x", node);

	/* copy the default resource map from node 0 */
	for (i = 0; i < ARRAY_SIZE(pci_reg); i++) {
		u32 value;
		u16 reg;
		reg = pci_reg[i];
		value = pci_read_config32(NODE_PCI(0, 1), reg);
		pci_write_config32(NODE_PCI(node, 1), reg, value);

	}
	printk(BIOS_DEBUG, " done\n");
}

static void real_start_other_core(u32 nodeid, u32 cores)
{
	ssize_t i;
	u32 dword;

	printk(BIOS_DEBUG,
		"Start other core - nodeid: %02x  cores: %02x\n", nodeid, cores);

	/* set PCI_DEV(0, 0x18+nodeid, 3), 0x44 bit 27 to redirect all MC4
	   accesses and error logging to core0 */
	dword = pci_read_config32(NODE_PCI(nodeid, 3), 0x44);
	dword |= 1 << 30;	/* SyncFloodOnDramAdrParErr=1 */
	dword |= 1 << 27;	/* NbMcaToMstCpuEn=1 */
	dword |= 1 << 21;	/* SyncFloodOnAnyUcErr=1 */
	dword |= 1 << 20;	/* SyncFloodOnWDT=1 */
	dword |= 1 << 2;	/* SyncFloodOnDramUcEcc=1 */
	pci_write_config32(NODE_PCI(nodeid, 3), 0x44, dword);
	if (is_fam15h()) {
		u32 core_activation_flags = 0;
		u32 active_cores = 0;

		/* Set PCI_DEV(0, 0x18+nodeid, 0),
		 * 0x1dc bits 7:1 to start cores
		 */
		for (i = 1; i < cores + 1; i++)
			core_activation_flags |= 1 << i;

		/* Each core shares a single set of MTRR registers with
		 * another core in the same compute unit, therefore, it
		 * is important that one core in each CU starts in advance
		 * of the other in order to avoid one core stomping all over
		 * the other core's settings.
		 */

		/* Start the first core of each compute unit sequentially */
		for (i = 2; i < cores + 1; i += 2) {
			dword = pci_read_config32(NODE_PCI(nodeid, 0), 0x1dc);
			active_cores |= (dword | (core_activation_flags & (1 << i)));
			pci_write_config32(NODE_PCI(nodeid, 0), 0x1dc, active_cores);
			/* Wait for the first core of each compute unit to start... */
			u32 ap_apicid =	get_boot_apic_id(nodeid, i);
			/* Timeout */
			wait_cpu_state(ap_apicid, F10_APSTATE_ASLEEP, F10_APSTATE_ASLEEP);
		}

		for (i = 1; i < cores + 1; i += 2) {
			/* Start the first core of each compute unit sequentially */
			dword = pci_read_config32(NODE_PCI(nodeid, 0), 0x1dc);
			active_cores |= (dword | (core_activation_flags & (1 << i)));
			pci_write_config32(NODE_PCI(nodeid, 0), 0x1dc, active_cores);
			/* Wait for the first core of each compute unit to start... */
			u32 ap_apicid =	get_boot_apic_id(nodeid, i);
			/* Timeout */
			wait_cpu_state(ap_apicid, F10_APSTATE_ASLEEP, F10_APSTATE_ASLEEP);
		}
	} else {
		// set PCI_DEV(0, 0x18+nodeid, 0), 0x68 bit 5 to start core1
		dword = pci_read_config32(NODE_PCI(nodeid, 0), 0x68);
		dword |= 1 << 5;
		pci_write_config32(NODE_PCI(nodeid, 0), 0x68, dword);

		if (cores > 1) {
			dword = pci_read_config32(NODE_PCI(nodeid, 0), 0x168);
			for (i = 0; i < cores - 1; i++)
				dword |= 1 << i;
			pci_write_config32(NODE_PCI(nodeid, 0), 0x168, dword);
		}
	}
}

//it is running on core0 of node0
static void start_other_cores(u32 bsp_apic_id)
{
	u32 nodes;
	u32 nodeid;

	if (get_uint_option("multi_core", 1) == 0)  {
		printk(BIOS_DEBUG, "Skip additional core init\n");
		return;
	}

	nodes = get_nodes();

	for (nodeid = 0; nodeid < nodes; nodeid++) {
		u32 cores = get_core_num_in_bsp(nodeid);
		printk(BIOS_DEBUG, "init node: %02x  cores: %02x pass 1\n", nodeid, cores);
		if (cores > 0)
			real_start_other_core(nodeid, cores);
	}
}

/**
 * finalize_node_setup()
 *
 * Do any additional post HT init
 *
 */
static void finalize_node_setup(struct sys_info *sysinfo)
{
	u8 i;
	u8 nodes = get_nodes();
	u32 reg;

	/* read Node0 F0_0x64 bit [8:10] to find out SbLink # */
	reg = pci_read_config32(NODE_PCI(0, 0), 0x64);
	sysinfo->sblk = (reg >> 8) & 7;
	sysinfo->sbbusn = 0;
	sysinfo->nodes = nodes;
	sysinfo->sbdn = get_sbdn(sysinfo->sbbusn);

	for (i = 0; i < nodes; i++) {
		cpu_set_amd_pci(i);
	}

	// Prep each node for FID/VID setup.
	if (CONFIG(SET_FIDVID))
		prep_fid_change();


	if (CONFIG_MAX_PHYSICAL_CPUS > 1) {
		/* Skip the BSP, start at node 1 */
		for (i = 1; i < nodes; i++) {
			setup_remote_node(i);
			start_node(i);
		}
	}
}

void setup_bsp(struct sys_info *sysinfo, u8 power_on_reset)
{
	initialize_mca(1, power_on_reset);
	amd_update_microcode_from_cbfs();

	post_code(0x33);

	cpu_set_amd_msr(0);
	post_code(0x34);

	amd_ht_init(sysinfo);
	amd_ht_fixup(sysinfo);
	post_code(0x35);

	/* Setup any mainboard PCI settings etc. */
	setup_mb_resource_map();

	/* Setup nodes PCI space and start core 0 AP init. */
	finalize_node_setup(sysinfo);

	/* Wait for all the APs core0 started by finalize_node_setup. */
	wait_all_core0_started();

	initialize_mca(0, power_on_reset);
	post_code(0x36);

}

u32 initialize_cores(struct sys_info *sysinfo)
{
	u32 bsp_apic_id = 0;

	post_code(0x30);

	bsp_apic_id = init_cpus(sysinfo);

	post_code(0x32);
	/* APs will not reach here*/

	return bsp_apic_id;
}

void early_cpu_finalize(struct sys_info *sysinfo, u32 bsp_apic_id)
{
	if (CONFIG(LOGICAL_CPUS)) {
		/* Core0 on each node is configured. Now setup any additional cores. */
		printk(BIOS_DEBUG, "start_other_cores()\n");
		start_other_cores(bsp_apic_id);
		post_code(0x37);
		wait_all_other_cores_started(bsp_apic_id);
	}

	if (CONFIG(SET_FIDVID)) {
		msr_t msr;
		msr = rdmsr(MSR_COFVID_STS);
		printk(BIOS_DEBUG, "\nBegin FIDVID MSR 0xc0010071 0x%08x 0x%08x\n",
			msr.hi, msr.lo);

		/* FIXME: The sb fid change may survive the warm reset
		 * and only need to be done once
		 */
		enable_fid_change_on_sb(sysinfo->sbbusn, sysinfo->sbdn);

		post_code(0x39);

		if (!warm_reset_detect(0)) {			// BSP is node 0
			init_fidvid_bsp(bsp_apic_id, sysinfo->nodes);
		} else {
			init_fidvid_stage2(bsp_apic_id, 0);	// BSP is node 0
		}

		post_code(0x3A);

		/* show final fid and vid */
		msr = rdmsr(MSR_COFVID_STS);
		printk(BIOS_DEBUG, "End FIDVIDMSR 0xc0010071 0x%08x 0x%08x\n", msr.hi, msr.lo);
	}
	post_code(0x38);

	init_timer();
}
