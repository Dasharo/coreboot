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
#include <drivers/amd/hypertransport/ht_wrapper.h>
#include <northbridge/amd/amdfam10/amdfam10.h>
#include <southbridge/amd/common/reset.h>
#include <southbridge/amd/sb700/sb700.h>
#include <option.h>
#include <types.h>

#include "defaults.h"
#include "family_10h_15h.h"
#include "fidvid.h"

//core_range = 0 : all cores
//core range = 1 : core 0 only
//core range = 2 : cores other than core0
void for_each_ap(uint32_t bsp_apicid, uint32_t core_range, int8_t node,
		 process_ap_t process_ap, void *gp)
{
	// here assume the OS don't change our apicid
	u32 ap_apicid;

	u32 nodes;
	u32 disable_siblings;
	u32 cores_found;
	int i, j;

	/* get_nodes define in ht_wrapper.c */
	nodes = get_nodes();

	if (!CONFIG(LOGICAL_CPUS) || get_uint_option("multi_core", 1) != 0) {
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

			if (ap_apicid == bsp_apicid)
				continue;

			process_ap(ap_apicid, gp);

		}
	}
}

static __always_inline
void print_apicid_nodeid_coreid(u32 apicid, struct node_core_id id,
				const char *str)
{
	printk(BIOS_DEBUG,
	       "%s --- { APICID = %02x NODEID = %02x COREID = %02x} ---\n", str,
	       apicid, id.nodeid, id.coreid);
}

static uint32_t wait_cpu_state(uint32_t apicid, uint32_t state, uint32_t state2)
{
	u32 readback = 0;
	u32 timeout = 1;
	int loop = 4000000;
	while (--loop > 0) {
		if (lapic_remote_read(apicid, LAPIC_MSG_REG, &readback) != 0)
			continue;
		if ((readback & 0x3f) == state || (readback & 0x3f) == state2 || (readback & 0x3f) == F10_APSTATE_RESET) {
			timeout = 0;
			break;	//target CPU is in stage started
		}
	}
	if (timeout) {
		if (readback) {
			timeout = readback;
		}
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

static void wait_all_other_cores_started(u32 bsp_apicid)
{
	// all aps other than core0
	printk(BIOS_DEBUG, "started ap apicid: ");
	for_each_ap(bsp_apicid, 2, -1, wait_ap_started, (void *)0);
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

void wait_all_other_cores_stopped(u32 bsp_apicid)
{
	// all aps other than core0
	printk(BIOS_DEBUG, "stopped ap apicid: ");
	for_each_ap(bsp_apicid, 2, -1, wait_ap_stopped, (void *)0);
	printk(BIOS_DEBUG, "\n");
}

static void start_other_core(uint32_t nodeid, uint32_t cores)
{
	ssize_t i;
	uint32_t dword;

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
		uint32_t core_activation_flags = 0;
		uint32_t active_cores = 0;

		/* Set PCI_DEV(0, 0x18+nodeid, 0),
		 * 0x1dc bits 7:1 to start cores
		 */
		dword = pci_read_config32(NODE_PCI(nodeid, 0), 0x1dc);
		for (i = 1; i < cores + 1; i++)
			core_activation_flags |= 1 << i;
		/* Start the first core of each compute unit */
		active_cores |= core_activation_flags & 0x55;
		pci_write_config32(NODE_PCI(nodeid, 0), 0x1dc, dword
				  | active_cores);

		/* Each core shares a single set of MTRR registers with
		 * another core in the same compute unit, therefore, it
		 * is important that one core in each CU starts in advance
		 * of the other in order to avoid one core stomping all over
		 * the other core's settings.
		 */

		/* Wait for the first core of each compute unit to start... */
		for (i = 1; i < cores + 1; i++) {
			if (!(i & 0x1)) {
				uint32_t ap_apicid =
				get_boot_apic_id(nodeid, i);
				/* Timeout */
				wait_cpu_state(ap_apicid, F10_APSTATE_ASLEEP,
						F10_APSTATE_ASLEEP);
			}
		}

		/* Start the second core of each compute unit */
		active_cores |= core_activation_flags & 0xaa;
		pci_write_config32(NODE_PCI(nodeid, 0), 0x1dc, dword | active_cores);
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

static void enable_apic_ext_id(u32 node)
{
	u32 val;

	val = pci_read_config32(NODE_PCI(node, 0), 0x68);
	val |= (HTTC_APIC_EXT_SPUR | HTTC_APIC_EXT_ID | HTTC_APIC_EXT_BRD_CST);
	pci_write_config32(NODE_PCI(node, 0), 0x68, val);
}

static __always_inline
void disable_cache_as_ram(uint8_t skip_sharedc_config)
{
	msr_t msr;
	uint32_t family;

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
		msr.lo = (1 << 11);

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


static void stop_car_and_cpu(uint8_t skip_sharedc_config, uint32_t apicid)
{
	msr_t msr;
	uint32_t family;

	family = get_cpu_family();

	if (family < 0x6f) {
		/* Family 10h or earlier */

		/* Disable L2 IC to L3 connection (Only for CAR) */
		msr = rdmsr(BU_CFG2_MSR);
		msr.lo &= ~(1 << ClLinesToNbDis);
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
			/* Enable memory access for first MBs using top_mem */
			msr.hi = 0;
			msr.lo = (CONFIG_RAMTOP + TOP_MEM_MASK) & (~TOP_MEM_MASK);
			wrmsr(TOP_MEM, msr);
		}
	}

	disable_cache_as_ram(skip_sharedc_config);	// inline

	/* Mark the core as sleeping */
	lapic_write(LAPIC_MSG_REG, (apicid << 24) | F10_APSTATE_ASLEEP);

	/* stop all cores except node0/core0 the bsp */
	stop_this_cpu();
}

static void configure_fidvid(uint32_t apicid, struct node_core_id id)
{
	if (CONFIG(LOGICAL_CPUS) && CONFIG(SET_FIDVID_CORE0_ONLY)) {
		if (id.coreid != 0)	// only need set fid for core0
			return;
	}

	// Run on all AP for proper FID/VID setup.
	// check warm(bios) reset to call stage2 otherwise do stage1
	if (warm_reset_detect(id.nodeid)) {
		printk(BIOS_DEBUG,
			"init_fidvid_stage2 apicid: %02x\n",
			apicid);
		init_fidvid_stage2(apicid, id.nodeid);
	} else {
		printk(BIOS_DEBUG,
			"init_fidvid_ap(stage1) apicid: %02x\n",
			apicid);
		init_fidvid_ap(apicid, id.nodeid, id.coreid);
	}

}

static void enable_memory_speed_boost(uint8_t node_id)
{
	msr_t msr;
	uint32_t f3x1fc = pci_read_config32(NODE_PCI(node_id, 3), 0x1fc);
	msr = rdmsr(FP_CFG_MSR);
	msr.hi &= ~(0x7 << (42-32));			/* DiDtCfg4 */
	msr.hi |= (((f3x1fc >> 17) & 0x7) << (42-32));
	msr.hi &= ~(0x1 << (41-32));			/* DiDtCfg5 */
	msr.hi |= (((f3x1fc >> 22) & 0x1) << (41-32));
	msr.hi &= ~(0x1 << (40-32));			/* DiDtCfg3 */
	msr.hi |= (((f3x1fc >> 16) & 0x1) << (40-32));
	msr.hi &= ~(0x7 << (32-32));			/* DiDtCfg1 (1) */
	msr.hi |= (((f3x1fc >> 11) & 0x7) << (32-32));
	msr.lo &= ~(0x1f << 27);			/* DiDtCfg1 (2) */
	msr.lo |= (((f3x1fc >> 6) & 0x1f) << 27);
	msr.lo &= ~(0x3 << 25);				/* DiDtCfg2 */
	msr.lo |= (((f3x1fc >> 14) & 0x3) << 25);
	msr.lo &= ~(0x1f << 18);			/* DiDtCfg0 */
	msr.lo |= (((f3x1fc >> 1) & 0x1f) << 18);
	msr.lo &= ~(0x1 << 16);				/* DiDtMode */
	msr.lo |= ((f3x1fc & 0x1) << 16);
	wrmsr(FP_CFG_MSR, msr);

	if (get_uint_option("experimental_memory_speed_boost", 0)) {
		msr = rdmsr(BU_CFG3_MSR);
		msr.lo |= (0x3 << 20);			/* PfcStrideMul = 0x3 */
		wrmsr(BU_CFG3_MSR, msr);
	}
}

static void enable_message_triggered_c1e(uint64_t revision)
{
	msr_t msr;
	/* Set up message triggered C1E */
	msr = rdmsr(MSR_INTPEND);
	msr.lo &= ~0xffff;		/* IOMsgAddr = ACPI_PM_EVT_BLK */
	msr.lo |= ACPI_PM_EVT_BLK & 0xffff;
	msr.lo |= (0x1 << 29);		/* BmStsClrOnHltEn = 1 */
	if (revision & AMD_DR_GT_D0) {
		msr.lo &= ~(0x1 << 28);	/* C1eOnCmpHalt = 0 */
		msr.lo &= ~(0x1 << 27);	/* SmiOnCmpHalt = 0 */
	}
	wrmsr(MSR_INTPEND, msr);

	msr = rdmsr(HWCR_MSR);
	msr.lo |= (0x1 << 12);		/* HltXSpCycEn = 1 */
	wrmsr(HWCR_MSR, msr);
}

static void enable_cpu_c_states(void)
{
	if (!CONFIG(HAVE_ACPI_TABLES))
		return;

	if (get_uint_option("cpu_c_states", 0)) {
		/* Set up the C-state base address */
		msr_t c_state_addr_msr;
		c_state_addr_msr = rdmsr(MSR_CSTATE_ADDRESS);
		c_state_addr_msr.lo = ACPI_CPU_P_LVL2;
		wrmsr(MSR_CSTATE_ADDRESS, c_state_addr_msr);
	}
}

static void cpb_enable_disable(void)
{
	if (!get_uint_option("cpu_core_boost", 1)) {
		msr_t msr;
		/* Disable Core Performance Boost */
		msr = rdmsr(HWCR_MSR);
		msr.lo |= (0x1 << 25);		/* CpbDis = 1 */
		wrmsr(HWCR_MSR, msr);
	}
}

static void AMD_Errata298(void)
{
	/* Workaround for L2 Eviction May Occur during operation to
	 * set Accessed or dirty bit.
	 */

	msr_t msr;
	u8 i;
	u8 affectedRev = 0;
	u8 nodes = get_nodes();

	/* For each core we need to check for a "broken" node */
	for (i = 0; i < nodes; i++) {
		if (get_logical_CPUID(i) & (AMD_DR_B0 | AMD_DR_B1 | AMD_DR_B2)) {
			affectedRev = 1;
			break;
		}
	}

	if (affectedRev) {
		msr = rdmsr(HWCR_MSR);
		msr.lo |= 0x08;	/* Set TlbCacheDis bit[3] */
		wrmsr(HWCR_MSR, msr);

		msr = rdmsr(BU_CFG_MSR);
		msr.lo |= 0x02;	/* Set TlbForceMemTypeUc bit[1] */
		wrmsr(BU_CFG_MSR, msr);

		msr = rdmsr(OSVW_ID_Length);
		msr.lo |= 0x01;	/* OS Visible Workaround - MSR */
		wrmsr(OSVW_ID_Length, msr);

		msr = rdmsr(OSVW_Status);
		msr.lo |= 0x01;	/* OS Visible Workaround - MSR */
		wrmsr(OSVW_Status, msr);
	}

	if (!affectedRev && (get_logical_CPUID(0xFF) & AMD_DR_B3)) {
		msr = rdmsr(OSVW_ID_Length);
		msr.lo |= 0x01;	/* OS Visible Workaround - MSR */
		wrmsr(OSVW_ID_Length, msr);

	}
}

static void cpuSetAMDMSR(uint8_t node_id)
{
	/* This routine loads the CPU with default settings in fam10_msr_default
	 * table . It must be run after Cache-As-RAM has been enabled, and
	 * Hypertransport initialization has taken place.  Also note
	 * that it is run on the current processor only, and only for the current
	 * processor core.
	 */
	msr_t msr;
	u8 i;
	u32 platform;
	uint64_t revision;

	printk(BIOS_DEBUG, "cpuSetAMDMSR ");

	revision = get_logical_CPUID(0xFF);
	platform = get_platform_type();

	for (i = 0; i < ARRAY_SIZE(fam10_msr_default); i++) {
		if ((fam10_msr_default[i].revision & revision) &&
		    (fam10_msr_default[i].platform & platform)) {
			msr = rdmsr(fam10_msr_default[i].msr);
			msr.hi &= ~fam10_msr_default[i].mask_hi;
			msr.hi |= fam10_msr_default[i].data_hi;
			msr.lo &= ~fam10_msr_default[i].mask_lo;
			msr.lo |= fam10_msr_default[i].data_lo;
			wrmsr(fam10_msr_default[i].msr, msr);
		}
	}
	AMD_Errata298();

	/* Revision C0 and above */
	if (revision & AMD_OR_C0)
		enable_memory_speed_boost(node_id);

	if (CONFIG(SOUTHBRIDGE_AMD_SB700) || CONFIG(SOUTHBRIDGE_AMD_SB800)) {
		if (revision & (AMD_DR_GT_D0 | AMD_FAM15_ALL))
			enable_message_triggered_c1e(revision);

		if (revision & (AMD_DR_Ex | AMD_FAM15_ALL))
			enable_cpu_c_states();
	}

	if (revision & AMD_FAM15_ALL)
		cpb_enable_disable();

	printk(BIOS_DEBUG, " done\n");
}

static u32 is_core0_started(u32 nodeid)
{
	u32 htic;
	htic = pci_read_config32(NODE_PCI(nodeid, 0), HT_INIT_CONTROL);
	htic &= HTIC_ColdR_Detect;
	return htic;
}

static void wait_all_core0_started(void)
{
	/* When core0 is started, it will distingush_cpu_resets
	 * So wait for that to finish */
	u32 i;
	u32 nodes = get_nodes();

	printk(BIOS_DEBUG, "core0 started: ");
	for (i = 1; i < nodes; i++) {	// skip bsp, because it is running on bsp
		while (!is_core0_started(i)) {
		}
		printk(BIOS_DEBUG, " %02x", i);
	}
	printk(BIOS_DEBUG, "\n");
}


static u32 init_cpus(struct sys_info *sysinfo)
{
	uint32_t bsp_apicid = 0;
	uint32_t apicid;
	uint32_t dword;
	uint8_t set_mtrrs;
	uint8_t node_count;
	uint8_t fam15_bsp_core1_apicid;
	struct node_core_id id;

	/* Please refer to the calculations and explanation in cache_as_ram.S
	 * before modifying these values
	 */
	uint32_t max_ap_stack_region_size = CONFIG_MAX_CPUS * CONFIG_DCACHE_AP_STACK_SIZE;
	uint32_t max_bsp_stack_region_size = 0x4000 + CONFIG_DCACHE_BSP_TOP_STACK_SLUSH;
	uint32_t bsp_stack_region_lower_boundary = (uint32_t)_ecar_stack - max_bsp_stack_region_size;

	void *lower_stack_region_boundary = (void *)(bsp_stack_region_lower_boundary -
						max_ap_stack_region_size);

	if (((void*)(sysinfo + 1)) > lower_stack_region_boundary)
		printk(BIOS_WARNING,
			"sysinfo extends into stack region (sysinfo range: [%p,%p] lower stack region boundary: %p)\n",
			sysinfo, sysinfo + 1, lower_stack_region_boundary);


	/* that is from initial apicid, we need nodeid and coreid
	   later */
	id = get_node_core_id_x();

	/* NB_CFG MSR is shared between cores, so we need make sure
	   core0 is done at first --- use wait_all_core0_started  */
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
			bsp_apicid += CONFIG_APIC_ID_OFFSET;
		} else if (initial_apicid != 0) { // other than bsp
			/* use initial APIC id to lift it */
			dword = lapic_read(LAPIC_ID);
			dword &= ~(0xff << 24);
			dword |= (((initial_apicid + CONFIG_APIC_ID_OFFSET) & 0xff) << 24);
			lapic_write(LAPIC_ID, dword);
		}
	}

	/* get the apicid, it may be lifted already */
	apicid = lapicid();

	// show our apicid, nodeid, and coreid
	if (id.coreid == 0) {
		if (id.nodeid != 0)	//all core0 except bsp
			print_apicid_nodeid_coreid(apicid, id, " core0: ");
	} else {		//all other cores
		print_apicid_nodeid_coreid(apicid, id, " corex: ");
	}

	if (pci_read_config32(PCI_DEV(0,0,0), 0x80) & (1 << apicid)) {
		print_apicid_nodeid_coreid(apicid, id,
					   "\n\n\nINIT detected from ");
		printk(BIOS_DEBUG, "\nIssuing SOFT_RESET...\n");
		soft_reset();
	}

	if (id.coreid == 0) {
		//FIXME: INIT is checked above but check for more resets?
		if (!(warm_reset_detect(id.nodeid)))	
			distinguish_cpu_resets(id.nodeid); // Also indicates we are started
	}

	// Mark the core as started.
	lapic_write(LAPIC_MSG_REG, (apicid << 24) | F10_APSTATE_STARTED);
	printk(BIOS_DEBUG, "CPU APICID %02x start flag set\n", apicid);

	if (apicid != bsp_apicid) {
		/* Setup each AP's cores MSRs.
		 * This happens after HTinit.
		 * The BSP runs this code in it's own path.
		 */
		update_microcode(cpuid_eax(1));

		cpuSetAMDMSR(id.nodeid);

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
			configure_fidvid(apicid, id);

		if (is_fam15h()) {
			/* core 1 on node 0 is special; to avoid corrupting the
			 * BSP do not alter MTRRs on that core */
			fam15_bsp_core1_apicid = 1;
			if (CONFIG(ENABLE_APIC_EXT_ID) && (CONFIG_APIC_ID_OFFSET > 0))
				fam15_bsp_core1_apicid += CONFIG_APIC_ID_OFFSET;

			if (apicid == fam15_bsp_core1_apicid)
				set_mtrrs = 0;
			else
				set_mtrrs = !!(apicid & 0x1);
		} else {
			set_mtrrs = 1;
		}

		/* AP is ready, configure MTRRs and go to sleep */
		if (set_mtrrs)
			set_var_mtrr(0, 0x00000000, 0x01000000, MTRR_TYPE_WRBACK);

		printk(BIOS_DEBUG, "Disabling CAR on AP %02x\n", apicid);
		if (is_fam15h()) {
			/* Only modify the MSRs on the odd cores (the last cores to finish booting) */
			stop_car_and_cpu(!set_mtrrs, apicid);
		} else {
			/* Modify MSRs on all cores */
			stop_car_and_cpu(0, apicid);
		}

		printk(BIOS_DEBUG, "\nAP %02x should be halted but you are reading this....\n",
		       apicid);
	}

	return bsp_apicid;
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
		cpuSetAMDPCI(i);
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
	update_microcode(cpuid_eax(1));

	post_code(0x33);

	cpuSetAMDMSR(0);
	post_code(0x34);

	amd_ht_init(sysinfo);
	amd_ht_fixup(sysinfo);
	post_code(0x35);

	/* Setup nodes PCI space and start core 0 AP init. */
	finalize_node_setup(sysinfo);

	/* Setup any mainboard PCI settings etc. */
	setup_mb_resource_map();
	initialize_mca(0, power_on_reset);
	post_code(0x36);

	/* Wait for all the APs core0 started by finalize_node_setup. */
	wait_all_core0_started();
}

u32 initialize_aps(struct sys_info *sysinfo)
{
	u32 bsp_apicid = 0;

	post_code(0x30);

	bsp_apicid = init_cpus(sysinfo);

	post_code(0x32);
	/* APs will not reach here*/

	return bsp_apicid;
}

void early_cpu_finalize(struct sys_info *sysinfo, u32 bsp_apicid)
{
	if (CONFIG(LOGICAL_CPUS)) {
		/* Core0 on each node is configured. Now setup any additional cores. */
		printk(BIOS_DEBUG, "start_other_cores()\n");
		start_other_cores(bsp_apicid);
		post_code(0x37);
		wait_all_other_cores_started(bsp_apicid);
	}

	if (CONFIG(SET_FIDVID)) {
		msr_t msr;
		msr = rdmsr(MSR_COFVID_STS);
		printk(BIOS_DEBUG, "\nBegin FIDVID MSR 0xc0010071 0x%08x 0x%08x\n",
			msr.hi, msr.lo);

		/* FIXME: The sb fid change may survive the warm reset and only need to be done once */
		enable_fid_change_on_sb(sysinfo->sbbusn, sysinfo->sbdn);

		post_code(0x39);

		if (!warm_reset_detect(0)) {			// BSP is node 0
			init_fidvid_bsp(bsp_apicid, sysinfo->nodes);
		} else {
			init_fidvid_stage2(bsp_apicid, 0);	// BSP is node 0
		}

		post_code(0x3A);

		/* show final fid and vid */
		msr = rdmsr(MSR_COFVID_STS);
		printk(BIOS_DEBUG, "End FIDVIDMSR 0xc0010071 0x%08x 0x%08x\n", msr.hi, msr.lo);
	}
	post_code(0x38);

	init_timer(); // Need to use TMICT to synconize FID/VID
}