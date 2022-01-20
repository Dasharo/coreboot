/* SPDX-License-Identifier: GPL-2.0-only */

/* Description: Main memory controller system configuration for DDR 2 */


/* KNOWN ISSUES - ERRATA
 *
 * trtp is not calculated correctly when the controller is in 64-bit mode, it
 * is 1 busclock off.	No fix planned.	 The controller is not ordinarily in
 * 64-bit mode.
 *
 * 32 Byte burst not supported. No fix planned. The controller is not
 * ordinarily in 64-bit mode.
 *
 * trc precision does not use extra Jedec defined fractional component.
 * Instead trc (course) is rounded up to nearest 1 ns.
 *
 * Mini and Micro dimm not supported. Only RDIMM, UDIMM, SO-dimm defined types
 * supported.
 */

#include <string.h>
#include <console/console.h>
#include <cpu/amd/common/common.h>
#include <cpu/amd/msr.h>
#include <device/pci_ops.h>

#include <drivers/amd/amdmct/wrappers/mcti.h>

static u8 reconfigure_dimm_spare_d(struct MCTStatStruc *p_mct_stat,
					struct DCTStatStruc *p_dct_stat_a);
static void rqs_timing_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat_a);
static void load_dqs_sig_tmg_regs_d(struct MCTStatStruc *p_mct_stat,
					struct DCTStatStruc *p_dct_stat_a);
static void ht_mem_map_init_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat_a);
static void dct_mem_clr_sync_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat);
static void sync_dcts_ready_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat_a);
static void startup_dct_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, u8 dct);
static void clear_dct_d(struct MCTStatStruc *p_mct_stat,
			struct DCTStatStruc *p_dct_stat, u8 dct);
static u8 auto_cyc_timing(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, u8 dct);
static void get_preset_max_f_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat);
static void spd_get_tcl_d(struct MCTStatStruc *p_mct_stat,
			struct DCTStatStruc *p_dct_stat, u8 dct);
static u8 auto_config_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, u8 dct);
static u8 platform_spec_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, u8 dct);
static u8 mct_platform_spec(struct MCTStatStruc *p_mct_stat,
					struct DCTStatStruc *p_dct_stat, u8 dct);
static void spd_set_banks_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, u8 dct);
static void stitch_memory_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, u8 dct);
static u8 get_def_trc_k_d(u8 k);
static u16 get_40tk_d(u8 k);
static u16 get_fk_d(u8 k);
static u8 dimm_supports_d(struct DCTStatStruc *p_dct_stat, u8 i, u8 j, u8 k);
static u8 sys_capability_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, int j, int k);
static u8 get_dimm_address_d(struct DCTStatStruc *p_dct_stat, u8 i);
static void mct_init_dct(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat);
static void mct_dram_init(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, u8 dct);
static void mct_sync_dcts_ready(struct DCTStatStruc *p_dct_stat);
static void get_trdrd(struct MCTStatStruc *p_mct_stat,
			struct DCTStatStruc *p_dct_stat, u8 dct);
static void mct_after_get_clt(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, u8 dct);
static u8 mct_spd_calc_width(struct MCTStatStruc *p_mct_stat,\
					struct DCTStatStruc *p_dct_stat, u8 dct);
static void mct_after_stitch_memory(struct MCTStatStruc *p_mct_stat,
					struct DCTStatStruc *p_dct_stat, u8 dct);
static u8 mct_dimm_presence(struct MCTStatStruc *p_mct_stat,
					struct DCTStatStruc *p_dct_stat, u8 dct);
static void set_other_timing(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, u8 dct);
static void get_twrwr(struct MCTStatStruc *p_mct_stat,
			struct DCTStatStruc *p_dct_stat, u8 dct);
static void get_twrrd(struct MCTStatStruc *p_mct_stat,
			struct DCTStatStruc *p_dct_stat, u8 dct);
static void get_trwt_to(struct MCTStatStruc *p_mct_stat,
			struct DCTStatStruc *p_dct_stat, u8 dct);
static void get_trwt_wb(struct MCTStatStruc *p_mct_stat,
			struct DCTStatStruc *p_dct_stat);
static u8 check_dqs_rcv_en_diff(struct DCTStatStruc *p_dct_stat, u8 dct,
				u32 dev, u32 index_reg, u32 index);
static u8 get_dqs_rcv_en_gross_diff(struct DCTStatStruc *p_dct_stat,
					u32 dev, u32 index_reg);
static u8 get_wr_dat_gross_diff(struct DCTStatStruc *p_dct_stat, u8 dct,
					u32 dev, u32 index_reg);
static u16 get_dqs_rcv_en_gross_max_min(struct DCTStatStruc *p_dct_stat,
				u32 dev, u32 index_reg, u32 index);
static void mct_final_mct_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat);
static u16 get_wr_dat_gross_max_min(struct DCTStatStruc *p_dct_stat, u8 dct,
				u32 dev, u32 index_reg, u32 index);
static void mct_initial_mct_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat);
static void mct_init(struct MCTStatStruc *p_mct_stat,
			struct DCTStatStruc *p_dct_stat);
static void clear_legacy_mode(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat);
static void mct_ht_mem_map_ext(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat_a);
static void set_cs_tristate(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, u8 dct);
static void set_odt_tristate(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, u8 dct);
static void init_phy_compensation(struct MCTStatStruc *p_mct_stat,
					struct DCTStatStruc *p_dct_stat, u8 dct);
static u32 mct_node_present_d(void);
static void wait_routine_d(u32 time);
static void mct_other_timing(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat_a);
static void mct_reset_data_struct_d(struct MCTStatStruc *p_mct_stat,
					struct DCTStatStruc *p_dct_stat_a);
static void mct_early_arb_en_d(struct MCTStatStruc *p_mct_stat,
					struct DCTStatStruc *p_dct_stat);
static void mct_before_dram_init__prod_d(struct MCTStatStruc *p_mct_stat,
					struct DCTStatStruc *p_dct_stat);
void mct_clr_cl_to_nb_d(struct MCTStatStruc *p_mct_stat,
			struct DCTStatStruc *p_dct_stat);
static u8 check_nb_cof_early_arb_en(struct MCTStatStruc *p_mct_stat,
					struct DCTStatStruc *p_dct_stat);
void mct_clr_wb_enh_wsb_dis_d(struct MCTStatStruc *p_mct_stat,
					struct DCTStatStruc *p_dct_stat);
static void mct_before_dqs_train_d(struct MCTStatStruc *p_mct_stat,
					struct DCTStatStruc *p_dct_stat_a);
static void after_dram_init_d(struct DCTStatStruc *p_dct_stat, u8 dct);
static void mct_reset_dll_d(struct MCTStatStruc *p_mct_stat,
					struct DCTStatStruc *p_dct_stat, u8 dct);
static u32 mct_dis_dll_shutdown_sr(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, u32 dram_config_lo, u8 dct);
static void mct_en_dll_shutdown_sr(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, u8 dct);

/*See mctAutoInitMCT header for index relationships to CL and T*/
static const u16 table_f_k[]	= {00,200,266,333,400,533 };
static const u8 table_t_k[]	= {0x00,0x50,0x3D,0x30,0x25, 0x18 };
static const u8 table_cl2_j[]	= {0x04,0x08,0x10,0x20,0x40, 0x80 };
static const u8 tab_def_trc_k[]	= {0x0,0x41,0x3C,0x3C,0x3A, 0x3A };
static const u16 tab_40t_k[]	= {00,200,150,120,100,75 };
static const u8 tab_bank_addr[]	= {0x0,0x08,0x09,0x10,0x0C,0x0D,0x11,0x0E,0x15,0x16,0x0F,0x17};
static const u8 tab_t_cl_j[]	= {0,2,3,4,5};
static const u8 tab_1k_tfaw_t_k[] = {00,8,10,13,14,20};
static const u8 tab_2k_tfaw_t_k[] = {00,10,14,17,18,24};
static const u8 tab_l1_clk_dis[]	= {8,8,6,4,2,0,8,8};
static const u8 tab_m2_clk_dis[]	= {2,0,8,8,2,0,2,0};
static const u8 tab_s1_clk_dis[]	= {8,0,8,8,8,0,8,0};
static const u8 table_comp_rise_slew_20x[] = {7, 3, 2, 2, 0xFF};
static const u8 table_comp_rise_slew_15x[] = {7, 7, 3, 2, 0xFF};
static const u8 table_comp_fall_slew_20x[] = {7, 5, 3, 2, 0xFF};
static const u8 table_comp_fall_slew_15x[] = {7, 7, 5, 3, 0xFF};

const u8 Table_DQSRcvEn_Offset[] = {0x00,0x01,0x10,0x11,0x2};

void mct_auto_init_mct_d(struct MCTStatStruc *p_mct_stat,
			struct DCTStatStruc *p_dct_stat_a)
{
	/*
	 * Memory may be mapped contiguously all the way up to 4GB (depending
	 * on setup options). It is the responsibility of PCI subsystem to
	 * create an uncacheable IO region below 4GB and to adjust TOP_MEM
	 * downward prior to any IO mapping or accesses. It is the same
	 * responsibility of the CPU sub-system prior to accessing LAPIC.
	 *
	 * Slot Number is an external convention, and is determined by OEM with
	 * accompanying silk screening. OEM may choose to use Slot number
	 * convention which is consistent with dimm number conventions.
	 * All AMD engineering
	 * platforms do.
	 *
	 * Run-Time Requirements:
	 * 1. Complete Hypertransport Bus Configuration
	 * 2. SMBus Controller Initialized
	 * 3. Checksummed or Valid NVRAM bits
	 * 4. MCG_CTL=-1, MC4_CTL_EN = 0 for all CPUs
	 * 5. MCi_STS from shutdown/warm reset recorded (if desired) prior to
	 *     entry
	 * 6. All var MTRRs reset to zero
	 * 7. State of NB_CFG.DisDatMsk set properly on all CPUs
	 * 8. All CPUs at 2GHz Speed (unless DQS training is not installed).
	 * 9. All cHT links at max Speed/Width (unless DQS training is not
	 *     installed).
	 *
	 *
	 * Global relationship between index values and item values:
	 * j CL(j)       k   F(k)
	 * --------------------------
	 * 0 2.0         -   -
	 * 1 3.0         1   200 MHz
	 * 2 4.0         2   266 MHz
	 * 3 5.0         3   333 MHz
	 * 4 6.0         4   400 MHz
	 * 5 7.0         5   533 MHz
	 */
	u8 node, nodes_wmem;
	u32 node_sys_base;

restartinit:
	mct_init_mem_gpios_a_d();		/* Set any required GPIOs*/
	nodes_wmem = 0;
	node_sys_base = 0;
	for (node = 0; node < MAX_NODES_SUPPORTED; node++) {
		struct DCTStatStruc *p_dct_stat;
		p_dct_stat = p_dct_stat_a + node;
		p_dct_stat->node_id = node;
		p_dct_stat->dev_host = PA_HOST(node);
		p_dct_stat->dev_map = PA_MAP(node);
		p_dct_stat->dev_dct = PA_DCT(node);
		p_dct_stat->dev_nbmisc = PA_NBMISC(node);
		p_dct_stat->node_sys_base = node_sys_base;

		if (mct_get_nv_bits(NV_PACK_TYPE) == PT_GR) {
			u32 dword;
			p_dct_stat->dual_node_package = 1;

			/* Get the internal node number */
			dword = get_nb32(p_dct_stat->dev_nbmisc, 0xe8);
			dword = (dword >> 30) & 0x3;
			p_dct_stat->internal_node_id = dword;
		} else {
			p_dct_stat->dual_node_package = 0;
		}

		print_tx("mct_auto_init_mct_d: mct_init node ", node);
		mct_init(p_mct_stat, p_dct_stat);
		mct_node_id_debug_port_d();
		p_dct_stat->node_present = node_present_d(node);
		if (p_dct_stat->node_present) {		/* See if node is there*/
			print_t("mct_auto_init_mct_d: clear_legacy_mode\n");
			clear_legacy_mode(p_mct_stat, p_dct_stat);
			p_dct_stat->logical_cpuid = get_logical_cpuid(node);

			print_t("mct_auto_init_mct_d: mct_initial_mct_d\n");
			mct_initial_mct_d(p_mct_stat, p_dct_stat);

			print_t("mct_auto_init_mct_d: mctSMBhub_Init\n");
			mctSMBhub_Init(node);		/* Switch SMBUS crossbar to proper node*/

			print_t("mct_auto_init_mct_d: mct_init_dct\n");
			mct_init_dct(p_mct_stat, p_dct_stat);
			if (p_dct_stat->err_code == SC_FATAL_ERR) {
				goto fatalexit;		/* any fatal errors?*/
			} else if (p_dct_stat->err_code < SC_STOP_ERR) {
				nodes_wmem++;
			}
		}	/* if node present */
		node_sys_base = p_dct_stat->node_sys_base;
		node_sys_base += (p_dct_stat->node_sys_limit + 2) & ~0x0F;
	}
	if (nodes_wmem == 0) {
		printk(BIOS_DEBUG, "No Nodes?!\n");
		goto fatalexit;
	}

	print_t("mct_auto_init_mct_d: sync_dcts_ready_d\n");
	sync_dcts_ready_d(p_mct_stat, p_dct_stat_a);	/* Make sure DCTs are ready for accesses.*/

	print_t("mct_auto_init_mct_d: ht_mem_map_init_d\n");
	ht_mem_map_init_d(p_mct_stat, p_dct_stat_a);	/* Map local memory into system address space.*/
	mct_hook_after_ht_map();

	print_t("mct_auto_init_mct_d: CPUMemTyping_D\n");
	CPUMemTyping_D(p_mct_stat, p_dct_stat_a);	/* Map dram into WB/UC CPU cacheability */
	mct_hook_after_cpu();			/* Setup external northbridge(s) */

	print_t("mct_auto_init_mct_d: rqs_timing_d\n");
	rqs_timing_d(p_mct_stat, p_dct_stat_a);	/* Get receiver Enable and DQS signal timing*/

	print_t("mct_auto_init_mct_d: UMAMemTyping_D\n");
	UMAMemTyping_D(p_mct_stat, p_dct_stat_a);	/* Fix up for UMA sizing */

	print_t("mct_auto_init_mct_d: :OtherTiming\n");
	mct_other_timing(p_mct_stat, p_dct_stat_a);

	if (reconfigure_dimm_spare_d(p_mct_stat, p_dct_stat_a)) { /* RESET# if 1st pass of dimm spare enabled*/
		goto restartinit;
	}

	InterleaveNodes_D(p_mct_stat, p_dct_stat_a);
	InterleaveChannels_D(p_mct_stat, p_dct_stat_a);

	print_t("mct_auto_init_mct_d: ECCInit_D\n");
	if (ECCInit_D(p_mct_stat, p_dct_stat_a)) {		/* Setup ECC control and ECC check-bits*/
		print_t("mct_auto_init_mct_d: mct_mem_clr_d\n");
		mct_mem_clr_d(p_mct_stat,p_dct_stat_a);
	}

	mct_final_mct_d(p_mct_stat, (p_dct_stat_a + 0));	// node 0
	print_tx("mct_auto_init_mct_d Done: Global status: ", p_mct_stat->g_status);
	return;

fatalexit:
	die("mct_d: fatalexit");
}


static u8 reconfigure_dimm_spare_d(struct MCTStatStruc *p_mct_stat,
					struct DCTStatStruc *p_dct_stat_a)
{
	u8 ret;

	if (mct_get_nv_bits(NV_CS_SPARE_CTL)) {
		if (MCT_DIMM_SPARE_NO_WARM) {
			/* Do no warm-reset dimm spare */
			if (p_mct_stat->g_status & (1 << GSB_EN_DIMM_SPARE_NW)) {
				load_dqs_sig_tmg_regs_d(p_mct_stat, p_dct_stat_a);
				ret = 0;
			} else {
				mct_reset_data_struct_d(p_mct_stat, p_dct_stat_a);
				p_mct_stat->g_status |= 1 << GSB_EN_DIMM_SPARE_NW;
				ret = 1;
			}
		} else {
			/* Do warm-reset dimm spare */
			if (mct_get_nv_bits(NV_DQS_TRAIN_CTL))
				mct_warm_reset_d();
			ret = 0;
		}


	} else {
		ret = 0;
	}

	return ret;
}


static void rqs_timing_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat_a)
{
	u8 nv_dqs_train_ctl;

	if (p_mct_stat->g_status & (1 << GSB_EN_DIMM_SPARE_NW)) {
		return;
	}
	nv_dqs_train_ctl = mct_get_nv_bits(NV_DQS_TRAIN_CTL);
	/* FIXME: BOZO- DQS training every time*/
	nv_dqs_train_ctl = 1;

	print_t("rqs_timing_d: mct_before_dqs_train_d:\n");
	mct_before_dqs_train_d(p_mct_stat, p_dct_stat_a);
	phyAssistedMemFnceTraining(p_mct_stat, p_dct_stat_a);

	if (nv_dqs_train_ctl) {
		mct_hook_before_any_training(p_mct_stat, p_dct_stat_a);

		print_t("rqs_timing_d: TrainReceiverEn_D FIRST_PASS:\n");
		TrainReceiverEn_D(p_mct_stat, p_dct_stat_a, FIRST_PASS);

		print_t("rqs_timing_d: mct_TrainDQSPos_D\n");
		mct_TrainDQSPos_D(p_mct_stat, p_dct_stat_a);

		// Second Pass never used for Barcelona!
		//print_t("rqs_timing_d: TrainReceiverEn_D SECOND_PASS:\n");
		//TrainReceiverEn_D(p_mct_stat, p_dct_stat_a, SECOND_PASS);

		print_t("rqs_timing_d: mctSetEccDQSRcvrEn_D\n");
		mctSetEccDQSRcvrEn_D(p_mct_stat, p_dct_stat_a);

		print_t("rqs_timing_d: TrainMaxReadLatency_D\n");
//FIXME - currently uses calculated value		TrainMaxReadLatency_D(p_mct_stat, p_dct_stat_a);
		mct_hook_after_any_training();
		mctSaveDQSSigTmg_D();

		print_t("rqs_timing_d: mct_EndDQSTraining_D\n");
		mct_EndDQSTraining_D(p_mct_stat, p_dct_stat_a);

		print_t("rqs_timing_d: mct_mem_clr_d\n");
		mct_mem_clr_d(p_mct_stat, p_dct_stat_a);
	} else {
		mctGetDQSSigTmg_D();	/* get values into data structure */
		load_dqs_sig_tmg_regs_d(p_mct_stat, p_dct_stat_a);	/* load values into registers.*/
		//mctDoWarmResetMemClr_D();
		mct_mem_clr_d(p_mct_stat, p_dct_stat_a);
	}
}


static void load_dqs_sig_tmg_regs_d(struct MCTStatStruc *p_mct_stat,
					struct DCTStatStruc *p_dct_stat_a)
{
	u8 node, receiver, channel, dir, dimm;
	u32 dev;
	u32 index_reg;
	u32 reg;
	u32 index;
	u32 val;


	for (node = 0; node < MAX_NODES_SUPPORTED; node++) {
		struct DCTStatStruc *p_dct_stat;
		p_dct_stat = p_dct_stat_a + node;

		if (p_dct_stat->dct_sys_limit) {
			dev = p_dct_stat->dev_dct;
			for (channel = 0;channel < 2; channel++) {
				/* there are four receiver pairs,
				   loosely associated with chipselects.*/
				index_reg = 0x98 + channel * 0x100;
				for (receiver = 0; receiver < 8; receiver += 2) {
					/* Set receiver Enable Values */
					mct_SetRcvrEnDly_D(p_dct_stat,
						0, /* RcvrEnDly */
						1, /* FinalValue, From stack */
						channel,
						receiver,
						dev, index_reg,
						(receiver >> 1) * 3 + 0x10, /* Addl_Index */
						2); /* Pass Second Pass ? */

				}
			}
			for (channel = 0; channel < 2; channel++) {
				SetEccDQSRcvrEn_D(p_dct_stat, channel);
			}

			for (channel = 0; channel < 2; channel++) {
				u8 *p;
				index_reg = 0x98 + channel * 0x100;

				/* NOTE:
				 * when 400, 533, 667, it will support dimm0/1/2/3,
				 * and set conf for dimm0, hw will copy to dimm1/2/3
				 * set for dimm1, hw will copy to dimm3
				 * Rev A/B only support DIMM0/1 when 800MHz and above
				 *   + 0x100 to next dimm
				 * Rev C support DIMM0/1/2/3 when 800MHz and above
				 *   + 0x100 to next dimm
				*/
				for (dimm = 0; dimm < 2; dimm++) {
					if (dimm == 0) {
						index = 0;	/* CHA Write Data Timing Low */
					} else {
						if (p_dct_stat->speed >= 4) {
							index = 0x100 * dimm;
						} else {
							break;
						}
					}
					for (dir = 0; dir < 2; dir++) {//RD/WR
						p = p_dct_stat->persistentData.ch_d_dir_b_dqs[channel][dimm][dir];
						val = stream_to_int(p); /* CHA Read Data Timing High */
						set_nb32_index_wait(dev, index_reg, index + 1, val);
						val = stream_to_int(p + 4); /* CHA Write Data Timing High */
						set_nb32_index_wait(dev, index_reg, index + 2, val);
						val = *(p + 8); /* CHA Write ECC Timing */
						set_nb32_index_wait(dev, index_reg, index + 3, val);
						index += 4;
					}
				}
			}

			for (channel = 0; channel < 2; channel++) {
				reg = 0x78 + channel * 0x100;
				val = get_nb32(dev, reg);
				val &= ~(0x3ff << 22);
				val |= ((u32) p_dct_stat->ch_max_rd_lat[channel] << 22);
				val &= ~(1 << DQS_RCV_EN_TRAIN);
				set_nb32(dev, reg, val);	/* program MaxRdLatency to correspond with current delay*/
			}
		}
	}
}

#ifdef UNUSED_CODE
static void ResetNBECCstat_D(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat_a);
static void ResetNBECCstat_D(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat_a)
{
	/* Clear MC4_STS for all Nodes in the system.  This is required in some
	 * circumstances to clear left over garbage from cold reset, shutdown,
	 * or normal ECC memory conditioning.
	 */

	//FIXME: this function depends on p_dct_stat Array (with node id) - Is this really a problem?

	u32 dev;
	u8 node;

	for (node = 0; node < MAX_NODES_SUPPORTED; node++) {
		struct DCTStatStruc *p_dct_stat;
		p_dct_stat = p_dct_stat_a + node;

		if (p_dct_stat->node_present) {
			dev = p_dct_stat->dev_nbmisc;
			/*MCA NB status Low (alias to MC4_STS[31:0] */
			set_nb32(dev, 0x48, 0);
			/* MCA NB status High (alias to MC4_STS[63:32] */
			set_nb32(dev, 0x4C, 0);
		}
	}
}
#endif

static void ht_mem_map_init_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat_a)
{
	u8 node;
	u32 next_base, bottom_io;
	u8 _mem_hole_remap, dram_hole_base;
	u32 hole_size, dram_sel_base_addr;

	u32 val;
	u32 base;
	u32 limit;
	u32 dev, devx;
	struct DCTStatStruc *p_dct_stat;

	_mem_hole_remap = mct_get_nv_bits(NV_MEM_HOLE);

	if (p_mct_stat->hole_base == 0) {
		dram_hole_base = mct_get_nv_bits(NV_BOTTOM_IO);
	} else {
		dram_hole_base = p_mct_stat->hole_base >> (24-8);
	}

	bottom_io = dram_hole_base << (24-8);

	next_base = 0;
	p_dct_stat = p_dct_stat_a + 0;
	dev = p_dct_stat->dev_map;


	for (node = 0; node < MAX_NODES_SUPPORTED; node++) {
		p_dct_stat = p_dct_stat_a + node;
		devx = p_dct_stat->dev_map;
		dram_sel_base_addr = 0;
		if (!p_dct_stat->ganged_mode) {
			dram_sel_base_addr = p_dct_stat->node_sys_limit - p_dct_stat->dct_sys_limit;
			/*In unganged mode, we must add DCT0 and DCT1 to dct_sys_limit */
			val = p_dct_stat->node_sys_limit;
			if ((val & 0xFF) == 0xFE) {
				dram_sel_base_addr++;
				val++;
			}
			p_dct_stat->dct_sys_limit = val;
		}

		base = p_dct_stat->dct_sys_base;
		limit = p_dct_stat->dct_sys_limit;
		if (limit > base) {
			base += next_base;
			limit += next_base;
			dram_sel_base_addr += next_base;
			printk(BIOS_DEBUG, " node: %02x  base: %02x  limit: %02x  bottom_io: %02x\n",
					node, base, limit, bottom_io);

			if (_mem_hole_remap) {
				if ((base < bottom_io) && (limit >= bottom_io)) {
					/* HW Dram Remap */
					p_dct_stat->status |= 1 << SB_HW_HOLE;
					p_mct_stat->g_status |= 1 << GSB_HW_HOLE;
					p_dct_stat->dct_sys_base = base;
					p_dct_stat->dct_sys_limit = limit;
					p_dct_stat->dct_hole_base = bottom_io;
					p_mct_stat->hole_base = bottom_io;
					hole_size = _4GB_RJ8 - bottom_io; /* hole_size[39:8] */
					if ((dram_sel_base_addr > 0) && (dram_sel_base_addr < bottom_io))
						base = dram_sel_base_addr;
					val = ((base + hole_size) >> (24-8)) & 0xFF;
					val <<= 8; /* shl 16, rol 24 */
					val |= dram_hole_base << 24;
					val |= 1 << DRAM_HOLE_VALID;
					set_nb32(devx, 0xF0, val); /* Dram Hole Address Reg */
					p_dct_stat->dct_sys_limit += hole_size;
					base = p_dct_stat->dct_sys_base;
					limit = p_dct_stat->dct_sys_limit;
				} else if (base == bottom_io) {
					/* SW node Hoist */
					p_mct_stat->g_status |= 1 << GSB_SP_INTLV_REMAP_HOLE;
					p_dct_stat->status |= 1 << SB_SW_NODE_HOLE;
					p_mct_stat->g_status |= 1 << GSB_SOFT_HOLE;
					p_mct_stat->hole_base = base;
					limit -= base;
					base = _4GB_RJ8;
					limit += base;
					p_dct_stat->dct_sys_base = base;
					p_dct_stat->dct_sys_limit = limit;
				} else {
					/* No Remapping. Normal Contiguous mapping */
					p_dct_stat->dct_sys_base = base;
					p_dct_stat->dct_sys_limit = limit;
				}
			} else {
				/*No Remapping. Normal Contiguous mapping*/
				p_dct_stat->dct_sys_base = base;
				p_dct_stat->dct_sys_limit = limit;
			}
			base |= 3;		/* set WE,RE fields*/
			p_mct_stat->sys_limit = limit;
		}
		set_nb32(dev, 0x40 + (node << 3), base); /* [node] + Dram Base 0 */

		/* if node limit > 1GB then set it to 1GB boundary for each node */
		if ((mctSetNodeBoundary_D()) && (limit > 0x00400000)) {
			limit++;
			limit &= 0xFFC00000;
			limit--;
		}
		val = limit & 0xFFFF0000;
		val |= node;
		set_nb32(dev, 0x44 + (node << 3), val);	/* set DstNode */

		limit = p_dct_stat->dct_sys_limit;
		if (limit) {
			next_base = (limit & 0xFFFF0000) + 0x10000;
			if ((mctSetNodeBoundary_D()) && (next_base > 0x00400000)) {
				next_base++;
				next_base &= 0xFFC00000;
				next_base--;
			}
		}
	}

	/* Copy dram map from node 0 to node 1-7 */
	for (node = 1; node < MAX_NODES_SUPPORTED; node++) {
		u32 reg;
		p_dct_stat = p_dct_stat_a + node;
		devx = p_dct_stat->dev_map;

		if (p_dct_stat->node_present) {
			printk(BIOS_DEBUG, " Copy dram map from node 0 to node %02x\n", node);
			reg = 0x40;		/*Dram Base 0*/
			do {
				val = get_nb32(dev, reg);
				set_nb32(devx, reg, val);
				reg += 4;
			} while (reg < 0x80);
		} else {
			break;			/* stop at first absent node */
		}
	}

	/*Copy dram map to F1x120/124*/
	mct_ht_mem_map_ext(p_mct_stat, p_dct_stat_a);
}


void mct_mem_clr_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat_a)
{

	/* Initiates a memory clear operation for all node. The mem clr
	 * is done in parallel. After the memclr is complete, all processors
	 * status are checked to ensure that memclr has completed.
	 */
	u8 node;
	struct DCTStatStruc *p_dct_stat;

	if (!mct_get_nv_bits(NV_DQS_TRAIN_CTL)) {
		// FIXME: callback to wrapper: mctDoWarmResetMemClr_D
	} else {	// NV_DQS_TRAIN_CTL == 1
		for (node = 0; node < MAX_NODES_SUPPORTED; node++) {
			p_dct_stat = p_dct_stat_a + node;

			if (p_dct_stat->node_present) {
				dct_mem_clr_init_d(p_mct_stat, p_dct_stat);
			}
		}
		for (node = 0; node < MAX_NODES_SUPPORTED; node++) {
			p_dct_stat = p_dct_stat_a + node;

			if (p_dct_stat->node_present) {
				dct_mem_clr_sync_d(p_mct_stat, p_dct_stat);
			}
		}
	}
}


void dct_mem_clr_init_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat)
{
	u32 val;
	u32 dev;
	u32 reg;

	/* Initiates a memory clear operation on one node */
	if (p_dct_stat->dct_sys_limit) {
		dev = p_dct_stat->dev_dct;
		reg = 0x110;

		do {
			val = get_nb32(dev, reg);
		} while (val & (1 << MEM_CLR_BUSY));

		val |= (1 << MEM_CLR_INIT);
		set_nb32(dev, reg, val);

	}
}


void mct_mem_clr_sync_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat_a)
{
	/* Ensures that memory clear has completed on all node.*/
	u8 node;
	struct DCTStatStruc *p_dct_stat;

	if (!mct_get_nv_bits(NV_DQS_TRAIN_CTL)) {
		// callback to wrapper: mctDoWarmResetMemClr_D
	} else {	// NV_DQS_TRAIN_CTL == 1
		for (node = 0; node < MAX_NODES_SUPPORTED; node++) {
			p_dct_stat = p_dct_stat_a + node;

			if (p_dct_stat->node_present) {
				dct_mem_clr_sync_d(p_mct_stat, p_dct_stat);
			}
		}
	}
}


static void dct_mem_clr_sync_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat)
{
	u32 val;
	u32 dev = p_dct_stat->dev_dct;
	u32 reg;

	/* Ensure that a memory clear operation has completed on one node */
	if (p_dct_stat->dct_sys_limit) {
		reg = 0x110;

		do {
			val = get_nb32(dev, reg);
		} while (val & (1 << MEM_CLR_BUSY));

		do {
			val = get_nb32(dev, reg);
		} while (!(val & (1 << DR_MEM_CLR_STATUS)));
	}

	/* Implement BKDG Rev 3.62 recommendations */
	val = 0x0FE40F80;
	if (!(get_logical_cpuid(0) & AMD_FAM10_LT_D) && mct_get_nv_bits(NV_Unganged))
		val |= (0x18 << 2);
	else
		val |= (0x10 << 2);
	val |= MCCH_FlushWrOnStpGnt;	// Set for S3
	set_nb32(dev, 0x11C, val);
}


u8 node_present_d(u8 node)
{
	/*
	 * Determine if a single Hammer node exists within the network.
	 */

	u32 dev;
	u32 val;
	u32 dword;
	u8 ret = 0;

	dev = PA_HOST(node);		/* Test device/vendor id at host bridge. */
	val = get_nb32(dev, 0);
	dword = mct_node_present_d();	/* FIXME: BOZO -11001022h rev for F */
	if (val == dword) {		/* AMD Hammer Family CPU HT Configuration */
		if (oemNodePresent_D(node, &ret))
			goto finish;
		/* node ID register */
		val = get_nb32(dev, 0x60);
		val &= 0x07;
		dword = node;
		if (val == dword)	/* current nodeID = requested nodeID ? */
			ret = 1;
finish:
		;
	}

	return ret;
}


static void dct_init_d(struct MCTStatStruc *p_mct_stat, struct DCTStatStruc *p_dct_stat, u8 dct)
{
	/*
	 * Initialize DRAM on single Athlon 64/Opteron node.
	 */

	u8 stop_dct_flag;
	u32 val;

	clear_dct_d(p_mct_stat, p_dct_stat, dct);
	stop_dct_flag = 1;		/*preload flag with 'disable' */
	if (mct_dimm_presence(p_mct_stat, p_dct_stat, dct) < SC_STOP_ERR) {
		print_t("\t\tDCTInit_D: mct_dimm_presence Done\n");
		if (mct_spd_calc_width(p_mct_stat, p_dct_stat, dct) < SC_STOP_ERR) {
			print_t("\t\tDCTInit_D: mct_spd_calc_width Done\n");
			if (auto_cyc_timing(p_mct_stat, p_dct_stat, dct) < SC_STOP_ERR) {
				print_t("\t\tDCTInit_D: auto_cyc_timing Done\n");
				if (auto_config_d(p_mct_stat, p_dct_stat, dct) < SC_STOP_ERR) {
					print_t("\t\tDCTInit_D: auto_config_d Done\n");
					if (platform_spec_d(p_mct_stat, p_dct_stat, dct) < SC_STOP_ERR) {
						print_t("\t\tDCTInit_D: platform_spec_d Done\n");
						stop_dct_flag = 0;
						if (!(p_mct_stat->g_status & (1 << GSB_EN_DIMM_SPARE_NW))) {
							print_t("\t\tDCTInit_D: startup_dct_d\n");
							startup_dct_d(p_mct_stat, p_dct_stat, dct);   /*yeaahhh! */
						}
					}
				}
			}
		}
	}
	if (stop_dct_flag) {
		u32 reg_off = dct * 0x100;
		val = 1 << DIS_DRAM_INTERFACE;
		set_nb32(p_dct_stat->dev_dct, reg_off + 0x94, val);
		/*To maximize power savings when DIS_DRAM_INTERFACE = 1b,
		  all of the MemClkDis bits should also be set.*/
		val = 0xFF000000;
		set_nb32(p_dct_stat->dev_dct, reg_off + 0x88, val);
	} else {
		mct_en_dll_shutdown_sr(p_mct_stat, p_dct_stat, dct);
	}
}


static void sync_dcts_ready_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat_a)
{
	/* Wait (and block further access to dram) for all DCTs to be ready,
	 * by polling all INIT_DRAM bits and waiting for possible memory clear
	 * operations to be complete. Read MEM_CLK_FREQ_VAL bit to see if
	 * the DIMMs are present in this node.
	 */

	u8 node;

	for (node = 0; node < MAX_NODES_SUPPORTED; node++) {
		struct DCTStatStruc *p_dct_stat;
		p_dct_stat = p_dct_stat_a + node;
		mct_sync_dcts_ready(p_dct_stat);
	}
}


static void startup_dct_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, u8 dct)
{
	/* Read MEM_CLK_FREQ_VAL bit to see if the DIMMs are present in this node.
	 * If the DIMMs are present then set the DRAM Enable bit for this node.
	 *
	 * Setting dram init starts up the DCT state machine, initializes the
	 * dram devices with MRS commands, and kicks off any
	 * HW memory clear process that the chip is capable of.	The sooner
	 * that dram init is set for all nodes, the faster the memory system
	 * initialization can complete.	Thus, the init loop is unrolled into
	 * two loops so as to start the processes for non BSP nodes sooner.
	 * This procedure will not wait for the process to finish.
	 * Synchronization is handled elsewhere.
	 */

	u32 val;
	u32 dev;
	u8 byte;
	u32 reg;
	u32 reg_off = dct * 0x100;

	dev = p_dct_stat->dev_dct;
	val = get_nb32(dev, 0x94 + reg_off);
	if (val & (1 << MEM_CLK_FREQ_VAL)) {
		print_t("\t\t\tStartupDCT_D: MEM_CLK_FREQ_VAL\n");
		byte = mct_get_nv_bits(NV_DQS_TRAIN_CTL);
		if (byte == 1) {
			/* Enable DQSRcvEn training mode */
			print_t("\t\t\tStartupDCT_D: DQS_RCV_EN_TRAIN set\n");
			reg = 0x78 + reg_off;
			val = get_nb32(dev, reg);
			/* Setting this bit forces a 1T window with hard left
			 * pass/fail edge and a probabilistic right pass/fail
			 * edge.  LEFT edge is referenced for final
			 * receiver enable position.*/
			val |= 1 << DQS_RCV_EN_TRAIN;
			set_nb32(dev, reg, val);
		}
		mct_hook_before_dram_init();	/* generalized Hook */
		print_t("\t\t\tStartupDCT_D: DramInit\n");
		mct_dram_init(p_mct_stat, p_dct_stat, dct);
		after_dram_init_d(p_dct_stat, dct);
		mct_hook_after_dram_init();		/* generalized Hook*/
	}
}


static void clear_dct_d(struct MCTStatStruc *p_mct_stat,
			struct DCTStatStruc *p_dct_stat, u8 dct)
{
	u32 reg_end;
	u32 dev = p_dct_stat->dev_dct;
	u32 reg = 0x40 + 0x100 * dct;
	u32 val = 0;

	if (p_mct_stat->g_status & (1 << GSB_EN_DIMM_SPARE_NW)) {
		reg_end = 0x78 + 0x100 * dct;
	} else {
		reg_end = 0xA4 + 0x100 * dct;
	}

	while (reg < reg_end) {
		set_nb32(dev, reg, val);
		reg += 4;
	}

	val = 0;
	dev = p_dct_stat->dev_map;
	reg = 0xF0;
	set_nb32(dev, reg, val);
}


static u8 auto_cyc_timing(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, u8 dct)
{
	/* Initialize  DCT Timing registers as per dimm SPD.
	 * For primary timing (T, CL) use best case T value.
	 * For secondary timing params., use most aggressive settings
	 * of slowest dimm.
	 *
	 * There are three components to determining "maximum frequency":
	 * SPD component, Bus load component, and "Preset" max frequency
	 * component.
	 *
	 * The SPD component is a function of the min cycle time specified
	 * by each dimm, and the interaction of cycle times from all DIMMs
	 * in conjunction with CAS latency. The SPD component only applies
	 * when user timing mode is 'Auto'.
	 *
	 * The Bus load component is a limiting factor determined by electrical
	 * characteristics on the bus as a result of varying number of device
	 * loads. The Bus load component is specific to each platform but may
	 * also be a function of other factors. The bus load component only
	 * applies when user timing mode is 'Auto'.
	 *
	 * The Preset component is subdivided into three items and is the
	 * minimum of the set: Silicon revision, user limit setting when user
	 * timing mode is 'Auto' and memclock mode is 'Limit', OEM build
	 * specification of the maximum frequency. The Preset component is only
	 * applies when user timing mode is 'Auto'.
	 */

	u8 i;
	u8 twr, trtp;
	u8 trp, trrd, trcd, tras, trc, trfc[4], rows;
	u32 dram_timing_lo, dram_timing_hi;
	u16 tk40;
	u8 twtr;
	u8 l_dimm;
	u8 ddr2_1066;
	u8 byte;
	u32 dword;
	u32 dev;
	u32 reg;
	u32 reg_off;
	u32 val;
	u16 smb_addr;

	/* Get primary timing (CAS Latency and Cycle Time) */
	if (p_dct_stat->speed == 0) {
		mct_get_max_load_freq(p_dct_stat);

		/* and Factor in presets (setup options, Si cap, etc.) */
		get_preset_max_f_d(p_mct_stat, p_dct_stat);

		/* Go get best T and CL as specified by dimm mfgs. and OEM */
		spd_get_tcl_d(p_mct_stat, p_dct_stat, dct);
		/* skip callback mctForce800to1067_D */
		p_dct_stat->speed = p_dct_stat->dimm_auto_speed;
		p_dct_stat->cas_latency = p_dct_stat->dimm_casl;

		/* if "manual" memclock mode */
		if (mct_get_nv_bits(NV_MCT_USR_TMG_MODE) == 2)
			p_dct_stat->speed = mct_get_nv_bits(NV_MEM_CK_VAL) + 1;

	}
	mct_after_get_clt(p_mct_stat, p_dct_stat, dct);

	/* Gather all dimm mini-max values for cycle timing data */
	rows = 0;
	trp = 0;
	trrd = 0;
	trcd = 0;
	trtp = 0;
	tras = 0;
	trc = 0;
	twr = 0;
	twtr = 0;
	for (i = 0; i < 4; i++)
		trfc[i] = 0;

	for (i = 0; i < MAX_DIMMS_SUPPORTED; i++) {
		l_dimm = i >> 1;
		if (p_dct_stat->dimm_valid & (1 << i)) {
			smb_addr = get_dimm_address_d(p_dct_stat, dct + i);
			byte = mctRead_SPD(smb_addr, SPD_ROWSZ);
			if (rows < byte)
				rows = byte;	/* keep track of largest row sz */

			byte = mctRead_SPD(smb_addr, SPD_TRP);
			if (trp < byte)
				trp = byte;

			byte = mctRead_SPD(smb_addr, SPD_TRRD);
			if (trrd < byte)
				trrd = byte;

			byte = mctRead_SPD(smb_addr, SPD_TRCD);
			if (trcd < byte)
				trcd = byte;

			byte = mctRead_SPD(smb_addr, SPD_TRTP);
			if (trtp < byte)
				trtp = byte;

			byte = mctRead_SPD(smb_addr, SPD_TWR);
			if (twr < byte)
				twr = byte;

			byte = mctRead_SPD(smb_addr, SPD_TWTR);
			if (twtr < byte)
				twtr = byte;

			val = mctRead_SPD(smb_addr, SPD_TRC);
			if ((val == 0) || (val == 0xFF)) {
				p_dct_stat->err_status |= 1 << SB_NO_TRC_TRFC;
				p_dct_stat->err_code = SC_VARIANCE_ERR;
				val = get_def_trc_k_d(p_dct_stat->speed);
			} else {
				byte = mctRead_SPD(smb_addr, SPD_TRCRFC);
				if (byte & 0xF0) {
					val++;	/* round up in case fractional extension is non-zero.*/
				}
			}
			if (trc < val)
				trc = val;

			/* dev density = rank size/#devs per rank */
			byte = mctRead_SPD(smb_addr, SPD_BANKSZ);

			val = ((byte >> 5) | (byte << 3)) & 0xFF;
			val <<= 2;

			byte = mctRead_SPD(smb_addr, SPD_DEVWIDTH) & 0xFE;     /* dev density = 2^(rows+columns+banks) */
			if (byte == 4) {
				val >>= 4;
			} else if (byte == 8) {
				val >>= 3;
			} else if (byte == 16) {
				val >>= 2;
			}

			byte = bsr(val);

			if (trfc[l_dimm] < byte)
				trfc[l_dimm] = byte;

			byte = mctRead_SPD(smb_addr, SPD_TRAS);
			if (tras < byte)
				tras = byte;
		}	/* Dimm Present */
	}

	/* Convert  DRAM CycleTiming values and store into DCT structure */
	ddr2_1066 = 0;
	byte = p_dct_stat->speed;
	if (byte == 5)
		ddr2_1066 = 1;
	tk40 = get_40tk_d(byte);

	/* Notes:
	 1. All secondary time values given in SPDs are in binary with units of ns.
	 2. Some time values are scaled by four, in order to have least count of 0.25 ns
	    (more accuracy).  JEDEC SPD spec. shows which ones are x1 and x4.
	 3. Internally to this SW, cycle time, Tk, is scaled by 10 to affect a
	    least count of 0.1 ns (more accuracy).
	 4. SPD values not scaled are multiplied by 10 and then divided by 10T to find
	    equivalent minimum number of bus clocks (a remainder causes round-up of clocks).
	 5. SPD values that are prescaled by 4 are multiplied by 10 and then divided by 40T to find
	    equivalent minimum number of bus clocks (a remainder causes round-up of clocks).*/

	/* tras */
	dword = tras * 40;
	p_dct_stat->dimm_tras = (u16)dword;
	val = dword / tk40;
	if (dword % tk40) {	/* round up number of busclocks */
		val++;
	}
	if (ddr2_1066) {
		if (val < MIN_TRAST_1066)
			val = MIN_TRAST_1066;
		else if (val > MAX_TRAST_1066)
			val = MAX_TRAST_1066;
	} else {
		if (val < MIN_TRAST)
			val = MIN_TRAST;
		else if (val > MAX_TRAST)
			val = MAX_TRAST;
	}
	p_dct_stat->tras = val;

	/* trp */
	dword = trp * 10;
	p_dct_stat->dimm_trp = dword;
	val = dword / tk40;
	if (dword % tk40) {	/* round up number of busclocks */
		val++;
	}
	if (ddr2_1066) {
		if (val < MIN_TRAST_1066)
			val = MIN_TRPT_1066;
		else if (val > MAX_TRPT_1066)
			val = MAX_TRPT_1066;
	} else {
		if (val < MIN_TRPT)
			val = MIN_TRPT;
		else if (val > MAX_TRPT)
			val = MAX_TRPT;
	}
	p_dct_stat->trp = val;

	/*trrd*/
	dword = trrd * 10;
	p_dct_stat->dimm_trrd = dword;
	val = dword / tk40;
	if (dword % tk40) {	/* round up number of busclocks */
		val++;
	}
	if (ddr2_1066) {
		if (val < MIN_TRRDT_1066)
			val = MIN_TRRDT_1066;
		else if (val > MAX_TRRDT_1066)
			val = MAX_TRRDT_1066;
	} else {
		if (val < MIN_TRRDT)
			val = MIN_TRRDT;
		else if (val > MAX_TRRDT)
			val = MAX_TRRDT;
	}
	p_dct_stat->trrd = val;

	/* trcd */
	dword = trcd * 10;
	p_dct_stat->dimm_trcd = dword;
	val = dword / tk40;
	if (dword % tk40) {	/* round up number of busclocks */
		val++;
	}
	if (ddr2_1066) {
		if (val < MIN_TRCDT_1066)
			val = MIN_TRCDT_1066;
		else if (val > MAX_TRCDT_1066)
			val = MAX_TRCDT_1066;
	} else {
		if (val < MIN_TRCDT)
			val = MIN_TRCDT;
		else if (val > MAX_TRCDT)
			val = MAX_TRCDT;
	}
	p_dct_stat->trcd = val;

	/* trc */
	dword = trc * 40;
	p_dct_stat->dimm_trc = dword;
	val = dword / tk40;
	if (dword % tk40) {	/* round up number of busclocks */
		val++;
	}
	if (ddr2_1066) {
		if (val < MIN_TRCT_1066)
			val = MIN_TRCT_1066;
		else if (val > MAX_TRCT_1066)
			val = MAX_TRCT_1066;
	} else {
		if (val < MIN_TRCT)
			val = MIN_TRCT;
		else if (val > MAX_TRCT)
			val = MAX_TRCT;
	}
	p_dct_stat->trc = val;

	/* trtp */
	dword = trtp * 10;
	p_dct_stat->dimm_trtp = dword;
	val = p_dct_stat->speed;
	if (val <= 2) {		/* 7.75ns / Speed in ns to get clock # */
		val = 2;	/* for DDR400/DDR533 */
	} else {		/* Note a speed of 3 will be a trtp of 3 */
		val = 3;	/* for DDR667/DDR800/DDR1066 */
	}
	p_dct_stat->trtp = val;

	/* twr */
	dword = twr * 10;
	p_dct_stat->dimm_twr = dword;
	val = dword / tk40;
	if (dword % tk40) {	/* round up number of busclocks */
		val++;
	}
	if (ddr2_1066) {
		if (val < MIN_TWRT_1066)
			val = MIN_TWRT_1066;
		else if (val > MAX_TWRT_1066)
			val = MAX_TWRT_1066;
	} else {
		if (val < MIN_TWRT)
			val = MIN_TWRT;
		else if (val > MAX_TWRT)
			val = MAX_TWRT;
	}
	p_dct_stat->twr = val;

	/* twtr */
	dword = twtr * 10;
	p_dct_stat->dimm_twtr = dword;
	val = dword / tk40;
	if (dword % tk40) {	/* round up number of busclocks */
		val++;
	}
	if (ddr2_1066) {
		if (val < MIN_TWRT_1066)
			val = MIN_TWTRT_1066;
		else if (val > MAX_TWTRT_1066)
			val = MAX_TWTRT_1066;
	} else {
		if (val < MIN_TWTRT)
			val = MIN_TWTRT;
		else if (val > MAX_TWTRT)
			val = MAX_TWTRT;
	}
	p_dct_stat->twtr = val;


	/* Trfc0-Trfc3 */
	for (i = 0; i < 4; i++)
		p_dct_stat->trfc[i] = trfc[i];

	mct_adjust_auto_cyc_tmg_d();

	/* Program DRAM Timing values */
	dram_timing_lo = 0;	/* Dram Timing Low init */
	val = p_dct_stat->cas_latency;
	val = tab_t_cl_j[val];
	dram_timing_lo |= val;

	val = p_dct_stat->trcd;
	if (ddr2_1066)
		val -= BIAS_TRCDT_1066;
	else
		val -= BIAS_TRCDT;

	dram_timing_lo |= val << 4;

	val = p_dct_stat->trp;
	if (ddr2_1066)
		val -= BIAS_TRPT_1066;
	else {
		val -= BIAS_TRPT;
		val <<= 1;
	}
	dram_timing_lo |= val << 7;

	val = p_dct_stat->trtp;
	val -= BIAS_TRTPT;
	dram_timing_lo |= val << 11;

	val = p_dct_stat->tras;
	if (ddr2_1066)
		val -= BIAS_TRAST_1066;
	else
		val -= BIAS_TRAST;
	dram_timing_lo |= val << 12;

	val = p_dct_stat->trc;
	val -= BIAS_TRCT;
	dram_timing_lo |= val << 16;

	if (!ddr2_1066) {
		val = p_dct_stat->twr;
		val -= BIAS_TWRT;
		dram_timing_lo |= val << 20;
	}

	val = p_dct_stat->trrd;
	if (ddr2_1066)
		val -= BIAS_TRRDT_1066;
	else
		val -= BIAS_TRRDT;
	dram_timing_lo |= val << 22;


	dram_timing_hi = 0;	/* Dram Timing Low init */
	val = p_dct_stat->twtr;
	if (ddr2_1066)
		val -= BIAS_TWTRT_1066;
	else
		val -= BIAS_TWTRT;
	dram_timing_hi |= val << 8;

	val = 2;
	dram_timing_hi |= val << 16;

	val = 0;
	for (i = 4; i > 0; i--) {
		val <<= 3;
		val |= trfc[i-1];
	}
	dram_timing_hi |= val << 20;


	dev = p_dct_stat->dev_dct;
	reg_off = 0x100 * dct;
	print_tx("AutoCycTiming: dram_timing_lo ", dram_timing_lo);
	print_tx("AutoCycTiming: dram_timing_hi ", dram_timing_hi);

	set_nb32(dev, 0x88 + reg_off, dram_timing_lo);	/*DCT Timing Low*/
	dram_timing_hi |=0x0000FC77;
	set_nb32(dev, 0x8c + reg_off, dram_timing_hi);	/*DCT Timing Hi*/

	if (ddr2_1066) {
		/* twr */
		dword = p_dct_stat->twr;
		dword -= BIAS_TWRT_1066;
		dword <<= 4;
		reg = 0x84 + reg_off;
		val = get_nb32(dev, reg);
		val &= 0x8F;
		val |= dword;
		set_nb32(dev, reg, val);
	}

	print_tx("AutoCycTiming: status ", p_dct_stat->status);
	print_tx("AutoCycTiming: err_status ", p_dct_stat->err_status);
	print_tx("AutoCycTiming: err_code ", p_dct_stat->err_code);
	print_t("AutoCycTiming: Done\n");

	mct_hook_after_auto_cyc_tmg();

	return p_dct_stat->err_code;
}


static void get_preset_max_f_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat)
{
	/* Get max frequency from OEM platform definition, from any user
	 * override (limiting) of max frequency, and from any Si Revision
	 * Specific information.  Return the least of these three in
	 * DCTStatStruc.preset_max_freq.
	 */

	u16 proposed_freq;
	u16 word;

	/* Get CPU Si Revision defined limit (NPT) */
	proposed_freq = 533;	 /* Rev F0 programmable max memclock is */

	/*Get User defined limit if  "limit" mode */
	if (mct_get_nv_bits(NV_MCT_USR_TMG_MODE) == 1) {
		word = get_fk_d(mct_get_nv_bits(NV_MEM_CK_VAL) + 1);
		if (word < proposed_freq)
			proposed_freq = word;

		/* Get Platform defined limit */
		word = mct_get_nv_bits(NV_MAX_MEMCLK);
		if (word < proposed_freq)
			proposed_freq = word;

		word = p_dct_stat->preset_max_freq;
		if (word > proposed_freq)
			word = proposed_freq;

		p_dct_stat->preset_max_freq = word;
	}
}



static void spd_get_tcl_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, u8 dct)
{
	/* Find the best T and CL primary timing parameter pair, per Mfg.,
	 * for the given set of DIMMs, and store into DCTStatStruc
	 * (.dimm_auto_speed and .dimm_casl). See "Global relationship between
	 *  index values and item values" for definition of CAS latency
	 *  index (j) and Frequency index (k).
	 */
	int i, j, k;
	u8 t1min, cl1min;

	/* i={0..7} (std. physical dimm number)
	 * j is an integer which enumerates increasing CAS latency.
	 * k is an integer which enumerates decreasing cycle time.
	 * CL no. {0,1,2} corresponds to CL X, CL X-.5, or CL X-1 (per individual dimm)
	 * Max timing values are per parameter, of all DIMMs, spec'd in ns like the SPD.
	 */

	cl1min = 0xFF;
	t1min = 0xFF;
	for (k = K_MAX; k >= K_MIN; k--) {
		for (j = J_MIN; j <= J_MAX; j++) {
			if (sys_capability_d(p_mct_stat, p_dct_stat, j, k)) {
				/* 1. check to see if DIMMi is populated.
				   2. check if DIMMi supports CLj and Tjk */
				for (i = 0; i < MAX_DIMMS_SUPPORTED; i++) {
					if (p_dct_stat->dimm_valid & (1 << i)) {
						if (dimm_supports_d(p_dct_stat, i, j, k))
							break;
					}
				}	/* while ++i */
				if (i == MAX_DIMMS_SUPPORTED) {
					t1min = k;
					cl1min = j;
					goto got_TCL;
				}
			}
		}	/* while ++j */
	}	/* while --k */

got_TCL:
	if (t1min != 0xFF) {
		p_dct_stat->dimm_casl = cl1min;	/*mfg. optimized */
		p_dct_stat->dimm_auto_speed = t1min;
		print_tx("spd_get_tcl_d: dimm_casl ", p_dct_stat->dimm_casl);
		print_tx("spd_get_tcl_d: dimm_auto_speed ", p_dct_stat->dimm_auto_speed);

	} else {
		p_dct_stat->dimm_casl = CL_DEF;	/* failsafe values (running in min. mode) */
		p_dct_stat->dimm_auto_speed = T_DEF;
		p_dct_stat->err_status |= 1 << SB_DIMM_MISMATCH_T;
		p_dct_stat->err_status |= 1 << SB_MINIMUM_MODE;
		p_dct_stat->err_code = SC_VARIANCE_ERR;
	}
	print_tx("spd_get_tcl_d: status ", p_dct_stat->status);
	print_tx("spd_get_tcl_d: err_status ", p_dct_stat->err_status);
	print_tx("spd_get_tcl_d: err_code ", p_dct_stat->err_code);
	print_t("spd_get_tcl_d: Done\n");
}


static u8 platform_spec_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, u8 dct)
{
	u32 dev;
	u32 reg;
	u32 val;

	mctGet_PS_Cfg_D(p_mct_stat, p_dct_stat, dct);

	if (p_dct_stat->ganged_mode) {
		mctGet_PS_Cfg_D(p_mct_stat, p_dct_stat, 1);
	}

	if (p_dct_stat->_2t_mode == 2) {
		dev = p_dct_stat->dev_dct;
		reg = 0x94 + 0x100 * dct; /* Dram Configuration Hi */
		val = get_nb32(dev, reg);
		val |= 1 << 20;		       /* 2T CMD mode */
		set_nb32(dev, reg, val);
	}

	mct_platform_spec(p_mct_stat, p_dct_stat, dct);
	init_phy_compensation(p_mct_stat, p_dct_stat, dct);
	mct_hook_after_ps_cfg();
	return p_dct_stat->err_code;
}


static u8 auto_config_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, u8 dct)
{
	u32 dram_control, dram_timing_lo, status;
	u32 dram_config_lo, dram_config_hi, dram_config_misc, dram_config_misc2;
	u32 val;
	u32 reg_off;
	u32 dev;
	u16 word;
	u32 dword;
	u8 byte;

	print_tx("auto_config_d: DCT: ", dct);

	dram_config_lo = 0;
	dram_config_hi = 0;
	dram_config_misc = 0;
	dram_config_misc2 = 0;

	/* set bank addressing and Masks, plus CS pops */
	spd_set_banks_d(p_mct_stat, p_dct_stat, dct);
	if (p_dct_stat->err_code == SC_STOP_ERR)
		goto AutoConfig_exit;

	/* map chip-selects into local address space */
	stitch_memory_d(p_mct_stat, p_dct_stat, dct);
	InterleaveBanks_D(p_mct_stat, p_dct_stat, dct);

	/* temp image of status (for convenience). RO usage! */
	status = p_dct_stat->status;

	dev = p_dct_stat->dev_dct;
	reg_off = 0x100 * dct;


	/* Build Dram Control Register Value */
	dram_config_misc2 = get_nb32 (dev, 0xA8 + reg_off);	/* Dram Control*/
	dram_control = get_nb32 (dev, 0x78 + reg_off);		/* Dram Control*/

	if (mct_get_nv_bits(NV_CLK_HZ_ALT_VID_C3))
		dram_control |= 1 << 16;

	// FIXME: Add support(skip) for Ax and Cx versions
	dram_control |= 5;	/* RdPtrInit */


	/* Build Dram Config Lo Register Value */
	dram_config_lo |= 1 << 4;					/* 75 Ohms ODT */
	if (mct_get_nv_bits(NV_MAX_DIMMS) == 8) {
		if (p_dct_stat->speed == 3) {
			if (p_dct_stat->ma_dimms[dct] == 4)
				dram_config_lo |= 1 << 5;		/* 50 Ohms ODT */
		} else if (p_dct_stat->speed == 4) {
			if (p_dct_stat->ma_dimms[dct] != 1)
				dram_config_lo |= 1 << 5;		/* 50 Ohms ODT */
		}
	} else {
		// FIXME: Skip for Ax versions
		if (p_dct_stat->ma_dimms[dct] == 4) {
			if (p_dct_stat->dimm_qr_present != 0) {
				if ((p_dct_stat->speed == 3) || (p_dct_stat->speed == 4)) {
					dram_config_lo |= 1 << 5;	/* 50 Ohms ODT */
				}
			} else if (p_dct_stat->ma_dimms[dct] == 4) {
				if (p_dct_stat->speed == 4) {
					if (p_dct_stat->dimm_qr_present != 0) {
						dram_config_lo |= 1 << 5;	/* 50 Ohms ODT */
					}
				}
			}
		} else if (p_dct_stat->ma_dimms[dct] == 2) {
			dram_config_lo |= 1 << 5;		/* 50 Ohms ODT */
		}

	}

	// FIXME: Skip for Ax versions
	/* callback not required - if (!mctParityControl_D()) */
	if (status & (1 << SB_PAR_DIMMS)) {
		dram_config_lo |= 1 << PAR_EN;
		dram_config_misc2 |= 1 << ACTIVE_CMD_AT_RST;
	} else {
		dram_config_lo  &= ~(1 << PAR_EN);
		dram_config_misc2 &= ~(1 << ACTIVE_CMD_AT_RST);
	}

	if (mct_get_nv_bits(NV_BURST_LEN_32)) {
		if (!p_dct_stat->ganged_mode)
			dram_config_lo |= 1 << BURST_LENGTH_32;
	}

	if (status & (1 << SB_128_BIT_MODE))
		dram_config_lo |= 1 << WIDTH_128;	/* 128-bit mode (normal) */

	word = dct;
	dword = X4_DIMM;
	while (word < 8) {
		if (p_dct_stat->dimm_x4_present & (1 << word))
			dram_config_lo |= 1 << dword;	/* X4_DIMM[3:0] */
		word++;
		word++;
		dword++;
	}

	if (!(status & (1 << SB_REGISTERED)))
		dram_config_lo |= 1 << UN_BUFF_DIMM;	/* Unbuffered DIMMs */

	if (mct_get_nv_bits(NV_ECC_CAP))
		if (status & (1 << SB_ECC_DIMMS))
			if (mct_get_nv_bits(NV_ECC))
				dram_config_lo |= 1 << DIMM_EC_EN;

	dram_config_lo = mct_dis_dll_shutdown_sr(p_mct_stat, p_dct_stat, dram_config_lo, dct);

	/* Build Dram Config Hi Register Value */
	dword = p_dct_stat->speed;
	dram_config_hi |= dword - 1;	/* get MemClk encoding */
	dram_config_hi |= 1 << MEM_CLK_FREQ_VAL;

	if (status & (1 << SB_REGISTERED))
		if ((p_dct_stat->dimm_x4_present != 0) && (p_dct_stat->dimm_x8_present != 0))
			/* set only if x8 Registered DIMMs in System*/
			dram_config_hi |= 1 << RDQS_EN;

	if (mct_get_nv_bits(NV_CKE_PDEN)) {
		dram_config_hi |= 1 << 15;		/* POWER_DOWN_EN */
		if (mct_get_nv_bits(NV_CKE_CTL))
			/*Chip Select control of CKE*/
			dram_config_hi |= 1 << 16;
	}

	/* Control Bank Swizzle */
	if (0) /* call back not needed mctBankSwizzleControl_D()) */
		dram_config_hi &= ~(1 << BANK_SWIZZLE_MODE);
	else
		dram_config_hi |= 1 << BANK_SWIZZLE_MODE; /* recommended setting (default) */

	/* Check for Quadrank dimm presence */
	if (p_dct_stat->dimm_qr_present != 0) {
		byte = mct_get_nv_bits(NV_4_RANK_TYPE);
		if (byte == 2)
			dram_config_hi |= 1 << 17;	/* S4 (4-Rank SO-DIMMs) */
		else if (byte == 1)
			dram_config_hi |= 1 << 18;	/* R4 (4-Rank Registered DIMMs) */
	}

	if (0) /* call back not needed mctOverrideDcqBypMax_D) */
		val = mct_get_nv_bits(NV_BYP_MAX);
	else
		val = 0x0f; // recommended setting (default)
	dram_config_hi |= val << 24;

	val = p_dct_stat->dimm_2k_page;
	if (p_dct_stat->ganged_mode != 0) {
		if (dct != 0) {
			val &= 0x55;
		} else {
			val &= 0xAA;
		}
	}
	if (val)
		val = tab_2k_tfaw_t_k[p_dct_stat->speed];
	else
		val = tab_1k_tfaw_t_k[p_dct_stat->speed];

	if (p_dct_stat->speed == 5)
		val >>= 1;

	val -= Bias_TFAW_T;
	val <<= 28;
	dram_config_hi |= val;	/* Tfaw for 1K or 2K paged drams */

	// FIXME: Skip for Ax versions
	dram_config_hi |= 1 << DCQ_ARB_BYPASS_EN;


	/* Build MemClkDis Value from Dram Timing Lo and
	   Dram Config Misc Registers
	 1. We will assume that MemClkDis field has been preset prior to this
	    point.
	 2. We will only set MemClkDis bits if a dimm is NOT present AND if:
	    NV_ALL_MEM_CLKS <>0 AND SB_DIAG_CLKS == 0 */


	/* Dram Timing Low (owns Clock Enable bits) */
	dram_timing_lo = get_nb32(dev, 0x88 + reg_off);
	if (mct_get_nv_bits(NV_ALL_MEM_CLKS) == 0) {
		/* Special Jedec SPD diagnostic bit - "enable all clocks" */
		if (!(p_dct_stat->status & (1 << SB_DIAG_CLKS))) {
			const u8 *p;
			byte = mct_get_nv_bits(NV_PACK_TYPE);
			if (byte == PT_L1)
				p = tab_l1_clk_dis;
			else if (byte == PT_M2)
				p = tab_m2_clk_dis;
			else
				p = tab_s1_clk_dis;

			dword = 0;
			while (dword < MAX_DIMMS_SUPPORTED) {
				val = p[dword];
				print_tx("dram_timing_lo: val=", val);
				if (!(p_dct_stat->dimm_valid & (1 << val)))
					/*disable memclk*/
					dram_timing_lo |= 1 << (dword + 24);
				dword++;
			}
		}
	}

	print_tx("auto_config_d: dram_control: ", dram_control);
	print_tx("auto_config_d: dram_timing_lo: ", dram_timing_lo);
	print_tx("auto_config_d: dram_config_misc: ", dram_config_misc);
	print_tx("auto_config_d: dram_config_misc2: ", dram_config_misc2);
	print_tx("auto_config_d: dram_config_lo: ", dram_config_lo);
	print_tx("auto_config_d: dram_config_hi: ", dram_config_hi);

	/* Write Values to the registers */
	set_nb32(dev, 0x78 + reg_off, dram_control);
	set_nb32(dev, 0x88 + reg_off, dram_timing_lo);
	set_nb32(dev, 0xA0 + reg_off, dram_config_misc);
	set_nb32(dev, 0xA8 + reg_off, dram_config_misc2);
	set_nb32(dev, 0x90 + reg_off, dram_config_lo);
	mct_set_dram_config_hi_d(p_dct_stat, dct, dram_config_hi);
	mct_ForceAutoPrecharge_D(p_dct_stat, dct);
	mct_early_arb_en_d(p_mct_stat, p_dct_stat);
	mct_hook_after_auto_cfg();

	print_tx("AutoConfig: status ", p_dct_stat->status);
	print_tx("AutoConfig: err_status ", p_dct_stat->err_status);
	print_tx("AutoConfig: err_code ", p_dct_stat->err_code);
	print_t("AutoConfig: Done\n");
AutoConfig_exit:
	return p_dct_stat->err_code;
}


static void spd_set_banks_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, u8 dct)
{
	/* Set bank addressing, program Mask values and build a chip-select
	 * population map. This routine programs PCI 0:24N:2x80 config register
	 * and PCI 0:24N:2x60,64,68,6C config registers (CS Mask 0-3).
	 */

	u8 chip_sel, rows, cols, ranks, banks;
	u32 bank_addr_reg, cs_mask;

	u32 val;
	u32 reg;
	u32 dev;
	u32 reg_off;
	u8 byte;
	u16 word;
	u32 dword;
	u16 smb_addr;

	dev = p_dct_stat->dev_dct;
	reg_off = 0x100 * dct;

	bank_addr_reg = 0;
	for (chip_sel = 0; chip_sel < MAX_CS_SUPPORTED; chip_sel+=2) {
		byte = chip_sel;
		if ((p_dct_stat->status & (1 << SB_64_MUXED_MODE)) && chip_sel >=4)
			byte -= 3;

		if (p_dct_stat->dimm_valid & (1 << byte)) {
			smb_addr = get_dimm_address_d(p_dct_stat, (chip_sel + dct));

			byte = mctRead_SPD(smb_addr, SPD_ROWSZ);
			rows = byte & 0x1f;

			byte = mctRead_SPD(smb_addr, SPD_COLSZ);
			cols = byte & 0x1f;

			banks = mctRead_SPD(smb_addr, SPD_LBANKS);

			byte = mctRead_SPD(smb_addr, SPD_DEVWIDTH);

			byte = mctRead_SPD(smb_addr, SPD_DMBANKS);
			ranks = (byte & 7) + 1;

			/* Configure Bank encoding
			 * Use a 6-bit key into a lookup table.
			 * Key (index) = CCCBRR, where CCC is the number of
			 * Columns minus 9,RR is the number of rows minus 13,
			 * and B is the number of banks minus 2.
			 * See "6-bit Bank Addressing Table" at the end of
			 * this file.*/
			byte = cols - 9;	/* 9 cols is smallest dev size */
			byte <<= 3;		/* make room for row and bank bits*/
			if (banks == 8)
				byte |= 4;

			/* 13 rows is smallest dev size */
			byte |= rows - 13;	/* CCCBRR internal encode */

			for (dword = 0; dword < 12; dword++) {
				if (byte == tab_bank_addr[dword])
					break;
			}

			if (dword < 12) {

				/* bit no. of CS field in address mapping reg.*/
				dword <<= (chip_sel << 1);
				bank_addr_reg |= dword;

				/* Mask value=(2pow(rows+cols+banks+3)-1)>>8,
				   or 2pow(rows+cols+banks-5)-1*/
				cs_mask = 0;

				byte = rows + cols;		/* cl = rows+cols*/
				if (banks == 8)
					byte -= 2;		/* 3 banks - 5 */
				else
					byte -= 3;		/* 2 banks - 5 */
								/* mask size (64-bit rank only) */

				if (p_dct_stat->status & (1 << SB_128_BIT_MODE))
					byte++;		/* double mask size if in 128-bit mode*/

				cs_mask |= 1 << byte;
				cs_mask--;

				/*set ChipSelect population indicator even bits*/
				p_dct_stat->cs_present |= (1 << chip_sel);
				if (ranks >= 2)
					/*set ChipSelect population indicator odd bits*/
					p_dct_stat->cs_present |= 1 << (chip_sel + 1);

				reg = 0x60+(chip_sel << 1) + reg_off;	/*Dram CS Mask Register */
				val = cs_mask;
				val &= 0x1FF83FE0;	/* Mask out reserved bits.*/
				set_nb32(dev, reg, val);
			}
		} else {
			if (p_dct_stat->dimm_spd_checksum_err & (1 << chip_sel))
				p_dct_stat->cs_test_fail |= (1 << chip_sel);
		}	/* if dimm_valid*/
	}	/* while chip_sel*/

	set_cs_tristate(p_mct_stat, p_dct_stat, dct);
	/* set_cke_tristate */
	set_odt_tristate(p_mct_stat, p_dct_stat, dct);

	if (p_dct_stat->status & (1 << SB_128_BIT_MODE)) {
		set_cs_tristate(p_mct_stat, p_dct_stat, 1); /* force dct1) */
		set_odt_tristate(p_mct_stat, p_dct_stat, 1); /* force dct1) */
	}
	word = p_dct_stat->cs_present;
	mct_get_cs_exclude_map();		/* mask out specified chip-selects */
	word ^= p_dct_stat->cs_present;
	p_dct_stat->cs_test_fail |= word;	/* enable ODT to disabled DIMMs */
	if (!p_dct_stat->cs_present)
		p_dct_stat->err_code = SC_STOP_ERR;

	reg = 0x80 + reg_off;		/* Bank Addressing Register */
	set_nb32(dev, reg, bank_addr_reg);

	print_tx("SPDSetBanks: status ", p_dct_stat->status);
	print_tx("SPDSetBanks: err_status ", p_dct_stat->err_status);
	print_tx("SPDSetBanks: err_code ", p_dct_stat->err_code);
	print_t("SPDSetBanks: Done\n");
}


static void spd_calc_width_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat)
{
	/* Per SPDs, check the symmetry of dimm pairs (dimm on channel A
	 *  matching with dimm on channel B), the overall dimm population,
	 * and determine the width mode: 64-bit, 64-bit muxed, 128-bit.
	 */

	u8 i;
	u8 smb_addr, smbaddr1;
	u8 byte, byte1;

	/* Check Symmetry of channel A and channel B DIMMs
	  (must be matched for 128-bit mode).*/
	for (i = 0; i < MAX_DIMMS_SUPPORTED; i += 2) {
		if ((p_dct_stat->dimm_valid & (1 << i)) && (p_dct_stat->dimm_valid & (1 << (i + 1)))) {
			smb_addr = get_dimm_address_d(p_dct_stat, i);
			smbaddr1 = get_dimm_address_d(p_dct_stat, i + 1);

			byte = mctRead_SPD(smb_addr, SPD_ROWSZ) & 0x1f;
			byte1 = mctRead_SPD(smbaddr1, SPD_ROWSZ) & 0x1f;
			if (byte != byte1) {
				p_dct_stat->err_status |= (1 << SB_DIMM_MISMATCH_O);
				break;
			}

			byte =	 mctRead_SPD(smb_addr, SPD_COLSZ) & 0x1f;
			byte1 =	 mctRead_SPD(smbaddr1, SPD_COLSZ) & 0x1f;
			if (byte != byte1) {
				p_dct_stat->err_status |= (1 << SB_DIMM_MISMATCH_O);
				break;
			}

			byte = mctRead_SPD(smb_addr, SPD_BANKSZ);
			byte1 = mctRead_SPD(smbaddr1, SPD_BANKSZ);
			if (byte != byte1) {
				p_dct_stat->err_status |= (1 << SB_DIMM_MISMATCH_O);
				break;
			}

			byte = mctRead_SPD(smb_addr, SPD_DEVWIDTH) & 0x7f;
			byte1 = mctRead_SPD(smbaddr1, SPD_DEVWIDTH) & 0x7f;
			if (byte != byte1) {
				p_dct_stat->err_status |= (1 << SB_DIMM_MISMATCH_O);
				break;
			}

			byte = mctRead_SPD(smb_addr, SPD_DMBANKS) & 7;	 /* #ranks-1 */
			byte1 = mctRead_SPD(smbaddr1, SPD_DMBANKS) & 7;	  /* #ranks-1 */
			if (byte != byte1) {
				p_dct_stat->err_status |= (1 << SB_DIMM_MISMATCH_O);
				break;
			}

		}
	}

}


static void stitch_memory_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, u8 dct)
{
	/* Requires that Mask values for each bank be programmed first and that
	 * the chip-select population indicator is correctly set.
	 */

	u8 b = 0;
	u32 nxtcs_base, curcs_base;
	u8 p, q;
	u32 biggest_bank;
	u8 _d_spare_en;

	u16 word;
	u32 dev;
	u32 reg;
	u32 reg_off;
	u32 val;


	dev = p_dct_stat->dev_dct;
	reg_off = 0x100 * dct;

	_d_spare_en = 0;

	/* CS Sparing 1 = enabled, 0 = disabled */
	if (mct_get_nv_bits(NV_CS_SPARE_CTL) & 1) {
		if (MCT_DIMM_SPARE_NO_WARM) {
			/* Do no warm-reset dimm spare */
			if (p_mct_stat->g_status & 1 << GSB_EN_DIMM_SPARE_NW) {
				word = p_dct_stat->cs_present;
				val = bsf(word);
				word &= ~(1 << val);
				if (word)
					/* Make sure at least two chip-selects are available */
					_d_spare_en = 1;
				else
					p_dct_stat->err_status |= 1 << SB_SPARE_DIS;
			}
		} else {
			if (!mct_get_nv_bits(NV_DQS_TRAIN_CTL)) { /*DQS Training 1 = enabled, 0 = disabled */
				word = p_dct_stat->cs_present;
				val = bsf(word);
				word &= ~(1 << val);
				if (word)
					/* Make sure at least two chip-selects are available */
					_d_spare_en = 1;
				else
					p_dct_stat->err_status |= 1 << SB_SPARE_DIS;
			}
		}
	}

	nxtcs_base = 0;		/* Next available cs base ADDR[39:8] */
	for (p = 0; p < MAX_DIMMS_SUPPORTED; p++) {
		biggest_bank = 0;
		for (q = 0; q < MAX_CS_SUPPORTED; q++) { /* from DIMMS to CS */
			if (p_dct_stat->cs_present & (1 << q)) {  /* bank present? */
				reg  = 0x40 + (q << 2) + reg_off;  /* Base[q] reg.*/
				val = get_nb32(dev, reg);
				if (!(val & 3)) {	/* (CS_ENABLE|SPARE == 1)bank is enabled already? */
					reg = 0x60 + ((q << 1) & 0xc) + reg_off; /*Mask[q] reg.*/
					val = get_nb32(dev, reg);
					val >>= 19;
					val++;
					val <<= 19;
					if (val > biggest_bank) {
						/*Bingo! possibly Map this chip-select next! */
						biggest_bank = val;
						b = q;
					}
				}
			}	/*if bank present */
		}	/* while q */
		if (biggest_bank !=0) {
			curcs_base = nxtcs_base;		/* curcs_base = nxtcs_base*/
			/* DRAM CS Base b Address Register offset */
			reg = 0x40 + (b << 2) + reg_off;
			if (_d_spare_en) {
				biggest_bank = 0;
				val = 1 << SPARE;	/* spare Enable*/
			} else {
				val = curcs_base;
				val |= 1 << CS_ENABLE;	/* Bank Enable */
			}
			set_nb32(dev, reg, val);
			if (_d_spare_en)
				_d_spare_en = 0;
			else
				/* let nxtcs_base+=Size[b] */
				nxtcs_base += biggest_bank;
		}

		/* bank present but disabled?*/
		if (p_dct_stat->cs_test_fail & (1 << p)) {
			/* DRAM CS Base b Address Register offset */
			reg = (p << 2) + 0x40 + reg_off;
			val = 1 << TEST_FAIL;
			set_nb32(dev, reg, val);
		}
	}

	if (nxtcs_base) {
		p_dct_stat->dct_sys_limit = nxtcs_base - 1;
		mct_after_stitch_memory(p_mct_stat, p_dct_stat, dct);
	}

	print_tx("StitchMemory: status ", p_dct_stat->status);
	print_tx("StitchMemory: err_status ", p_dct_stat->err_status);
	print_tx("StitchMemory: err_code ", p_dct_stat->err_code);
	print_t("StitchMemory: Done\n");
}


static u8 get_tk_d(u8 k)
{
	return table_t_k[k];
}


static u8 get_clj_d(u8 j)
{
	return table_cl2_j[j];
}

static u8 get_def_trc_k_d(u8 k)
{
	return tab_def_trc_k[k];
}


static u16 get_40tk_d(u8 k)
{
	return tab_40t_k[k]; /* FIXME: k or k<<1 ?*/
}


static u16 get_fk_d(u8 k)
{
	return table_f_k[k]; /* FIXME: k or k<<1 ? */
}


static u8 dimm_supports_d(struct DCTStatStruc *p_dct_stat,
				u8 i, u8 j, u8 k)
{
	u8 Tk, CLj, CL_i;
	u8 ret = 0;

	u32 DIMMi;
	u8 byte;
	u16 word, wordx;

	DIMMi = get_dimm_address_d(p_dct_stat, i);

	CLj = get_clj_d(j);

	/* check if DIMMi supports CLj */
	CL_i = mctRead_SPD(DIMMi, SPD_CASLAT);
	byte = CL_i & CLj;
	if (byte) {
		/*find out if its CL X, CLX-1, or CLX-2 */
		word = bsr(byte);	/* bit position of CLj */
		wordx = bsr(CL_i);	/* bit position of CLX of CLi */
		wordx -= word;		/* CL number (CL no. = 0,1, 2, or 3) */
		wordx <<= 3;		/* 8 bits per SPD byte index */
		/*get T from SPD byte 9, 23, 25*/
		word = (ENCODED_T_SPD >> wordx) & 0xFF;
		Tk = get_tk_d(k);
		byte = mctRead_SPD(DIMMi, word);	/* DIMMi speed */
		if (Tk < byte) {
			ret = 1;
		} else if (byte == 0) {
			p_dct_stat->err_status |= 1 << SB_NO_CYC_TIME;
			ret = 1;
		} else {
			ret = 0;	/* dimm is capable! */
		}
	} else {
		ret = 1;
	}
	return ret;
}


static u8 dimm_presence_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat)
{
	/* Check DIMMs present, verify checksum, flag SDRAM type,
	 * build population indicator bitmaps, and preload bus loading
	 * of DIMMs into DCTStatStruc.
	 * MAAload = number of devices on the "A" bus.
	 * MABload = number of devices on the "B" bus.
	 * maa_dimms = number of DIMMs on the "A" bus slots.
	 * MABdimms = number of DIMMs on the "B" bus slots.
	 * DATAAload = number of ranks on the "A" bus slots.
	 * DATABload = number of ranks on the "B" bus slots.
	 */

	u16 i, j, k;
	u8 smb_addr, index;
	u16 checksum;
	u8 spd_ctrl;
	u16 reg_dimm_present, max_dimms;
	u8 devwidth;
	u16 dimm_slots;
	u8 byte = 0, bytex;
	u16 word;

	/* preload data structure with addrs */
	mct_get_dimm_addr(p_dct_stat, p_dct_stat->node_id);

	dimm_slots = max_dimms = mct_get_nv_bits(NV_MAX_DIMMS);

	spd_ctrl = mct_get_nv_bits(NV_SPDCHK_RESTRT);

	reg_dimm_present = 0;
	p_dct_stat->dimm_qr_present = 0;

	for (i = 0;  i < MAX_DIMMS_SUPPORTED; i++) {
		if (i >= max_dimms)
			break;

		if ((p_dct_stat->dimm_qr_present & (1 << i)) || (i < dimm_slots)) {
			print_tx("\t DIMMPresence: i=", i);
			smb_addr = get_dimm_address_d(p_dct_stat, i);
			print_tx("\t DIMMPresence: smb_addr=", smb_addr);
			if (smb_addr) {
				checksum = 0;
				for (index = 0; index < 64; index++) {
					int status;
					status = mctRead_SPD(smb_addr, index);
					if (status < 0)
						break;
					byte = status & 0xFF;
					if (index < 63)
						checksum += byte;
				}

				if (index == 64) {
					p_dct_stat->dimm_present |= 1 << i;
					if ((checksum & 0xFF) == byte) {
						byte = mctRead_SPD(smb_addr, SPD_TYPE);
						if (byte == JED_DDR2SDRAM) {
							/*Dimm is 'Present'*/
							p_dct_stat->dimm_valid |= 1 << i;
						}
					} else {
						p_dct_stat->dimm_spd_checksum_err = 1 << i;
						if (spd_ctrl == 0) {
							p_dct_stat->err_status |= 1 << SB_DIMM_CHKSUM;
							p_dct_stat->err_code = SC_STOP_ERR;
						} else {
							/*if NV_SPDCHK_RESTRT is set to 1, ignore faulty SPD checksum*/
							p_dct_stat->err_status |= 1 << SB_DIMM_CHKSUM;
							byte = mctRead_SPD(smb_addr, SPD_TYPE);
							if (byte == JED_DDR2SDRAM)
								p_dct_stat->dimm_valid |= 1 << i;
						}
					}
					/* Get module information for SMBIOS */
					if (p_dct_stat->dimm_valid & (1 << i)) {
						p_dct_stat->dimm_manufacturer_id[i] = 0;
						for (k = 0; k < 8; k++)
							p_dct_stat->dimm_manufacturer_id[i] |= ((u64)mctRead_SPD(smb_addr, SPD_MANID_START + k)) << (k * 8);
						for (k = 0; k < SPD_PARTN_LENGTH; k++)
							p_dct_stat->dimm_part_number[i][k] = mctRead_SPD(smb_addr, SPD_PARTN_START + k);
						p_dct_stat->dimm_part_number[i][SPD_PARTN_LENGTH] = 0;
						p_dct_stat->dimm_revision_number[i] = 0;
						for (k = 0; k < 2; k++)
							p_dct_stat->dimm_revision_number[i] |= ((u16)mctRead_SPD(smb_addr, SPD_REVNO_START + k)) << (k * 8);
						p_dct_stat->dimm_serial_number[i] = 0;
						for (k = 0; k < 4; k++)
							p_dct_stat->dimm_serial_number[i] |= ((u32)mctRead_SPD(smb_addr, SPD_SERIAL_START + k)) << (k * 8);
						p_dct_stat->dimm_rows[i] = mctRead_SPD(smb_addr, SPD_ROWSZ) & 0xf;
						p_dct_stat->dimm_cols[i] = mctRead_SPD(smb_addr, SPD_COLSZ) & 0xf;
						p_dct_stat->dimm_ranks[i] = (mctRead_SPD(smb_addr, SPD_DMBANKS) & 0x7) + 1;
						p_dct_stat->dimm_banks[i] = mctRead_SPD(smb_addr, SPD_LBANKS);
						p_dct_stat->dimm_width[i] = mctRead_SPD(smb_addr, SPD_DEVWIDTH);
					}
					/* Check module type */
					byte = mctRead_SPD(smb_addr, SPD_DIMMTYPE);
					if (byte & JED_REGADCMSK) {
						reg_dimm_present |= 1 << i;
						p_dct_stat->dimm_registered[i] = 1;
					} else {
						p_dct_stat->dimm_registered[i] = 0;
					}
					/* Check ECC capable */
					byte = mctRead_SPD(smb_addr, SPD_EDCTYPE);
					if (byte & JED_ECC) {
						/* dimm is ECC capable */
						p_dct_stat->dimm_ecc_present |= 1 << i;
					}
					if (byte & JED_ADRCPAR) {
						/* dimm is ECC capable */
						p_dct_stat->dimm_parity_present |= 1 << i;
					}
					/* Check if x4 device */
					devwidth = mctRead_SPD(smb_addr, SPD_DEVWIDTH) & 0xFE;
					if (devwidth == 4) {
						/* dimm is made with x4 or x16 drams */
						p_dct_stat->dimm_x4_present |= 1 << i;
					} else if (devwidth == 8) {
						p_dct_stat->dimm_x8_present |= 1 << i;
					} else if (devwidth == 16) {
						p_dct_stat->dimm_x16_present |= 1 << i;
					}
					/* check page size */
					byte = mctRead_SPD(smb_addr, SPD_COLSZ);
					byte &= 0x0F;
					word = 1 << byte;
					word >>= 3;
					word *= devwidth;	/* (((2^COLBITS) / 8) * ORG) / 2048 */
					word >>= 11;
					if (word)
						p_dct_stat->dimm_2k_page |= 1 << i;

					/*Check if SPD diag bit 'analysis probe installed' is set */
					byte = mctRead_SPD(smb_addr, SPD_ATTRIB);
					if (byte & JED_PROBEMSK)
						p_dct_stat->status |= 1 << SB_DIAG_CLKS;

					byte = mctRead_SPD(smb_addr, SPD_DMBANKS);
					if (!(byte & (1 << SPD_PLBIT)))
						p_dct_stat->dimm_pl_present |= 1 << i;
					byte &= 7;
					byte++;		 /* ranks */
					if (byte > 2) {
						/* if any DIMMs are QR, we have to make two passes through DIMMs*/
						if (p_dct_stat->dimm_qr_present == 0) {
							max_dimms <<= 1;
						}
						if (i < dimm_slots) {
							p_dct_stat->dimm_qr_present |= (1 << i) | (1 << (i + 4));
						}
						byte = 2;	/* upper two ranks of QR dimm will be counted on another dimm number iteration*/
					} else if (byte == 2) {
						p_dct_stat->dimm_dr_present |= 1 << i;
					}
					bytex = devwidth;
					if (devwidth == 16)
						bytex = 4;
					else if (devwidth == 4)
						bytex = 16;

					if (byte == 2)
						bytex <<= 1;	/*double Addr bus load value for dual rank DIMMs*/

					j = i & (1 << 0);
					p_dct_stat->data_load[j] += byte;	/*number of ranks on DATA bus*/
					p_dct_stat->ma_load[j] += bytex;	/*number of devices on CMD/ADDR bus*/
					p_dct_stat->ma_dimms[j]++;		/*number of DIMMs on A bus */
					/*check for DRAM package Year <= 06*/
					byte = mctRead_SPD(smb_addr, SPD_MANDATEYR);
					if (byte < MYEAR06) {
						/*Year < 06 and hence Week < 24 of 06 */
						p_dct_stat->dimm_yr_06 |= 1 << i;
						p_dct_stat->dimm_wk_2406 |= 1 << i;
					} else if (byte == MYEAR06) {
						/*Year = 06, check if Week <= 24 */
						p_dct_stat->dimm_yr_06 |= 1 << i;
						byte = mctRead_SPD(smb_addr, SPD_MANDATEWK);
						if (byte <= MWEEK24)
							p_dct_stat->dimm_wk_2406 |= 1 << i;
					}
				}
			}
		}
	}
	print_tx("\t DIMMPresence: dimm_valid=", p_dct_stat->dimm_valid);
	print_tx("\t DIMMPresence: dimm_present=", p_dct_stat->dimm_present);
	print_tx("\t DIMMPresence: reg_dimm_present=", reg_dimm_present);
	print_tx("\t DIMMPresence: dimm_ecc_present=", p_dct_stat->dimm_ecc_present);
	print_tx("\t DIMMPresence: dimm_parity_present=", p_dct_stat->dimm_parity_present);
	print_tx("\t DIMMPresence: dimm_x4_present=", p_dct_stat->dimm_x4_present);
	print_tx("\t DIMMPresence: dimm_x8_present=", p_dct_stat->dimm_x8_present);
	print_tx("\t DIMMPresence: dimm_x16_present=", p_dct_stat->dimm_x16_present);
	print_tx("\t DIMMPresence: DimmPlPresent=", p_dct_stat->dimm_pl_present);
	print_tx("\t DIMMPresence: DimmDRPresent=", p_dct_stat->dimm_dr_present);
	print_tx("\t DIMMPresence: DimmQRPresent=", p_dct_stat->dimm_qr_present);
	print_tx("\t DIMMPresence: data_load[0]=", p_dct_stat->data_load[0]);
	print_tx("\t DIMMPresence: ma_load[0]=", p_dct_stat->ma_load[0]);
	print_tx("\t DIMMPresence: ma_dimms[0]=", p_dct_stat->ma_dimms[0]);
	print_tx("\t DIMMPresence: data_load[1]=", p_dct_stat->data_load[1]);
	print_tx("\t DIMMPresence: ma_load[1]=", p_dct_stat->ma_load[1]);
	print_tx("\t DIMMPresence: ma_dimms[1]=", p_dct_stat->ma_dimms[1]);

	if (p_dct_stat->dimm_valid != 0) {	/* If any DIMMs are present...*/
		if (reg_dimm_present != 0) {
			if ((reg_dimm_present ^ p_dct_stat->dimm_valid) !=0) {
				/* module type dimm mismatch (reg'ed, unbuffered) */
				p_dct_stat->err_status |= 1 << SB_DIMM_MISMATCH_M;
				p_dct_stat->err_code = SC_STOP_ERR;
			} else{
				/* all DIMMs are registered */
				p_dct_stat->status |= 1 << SB_REGISTERED;
			}
		}
		if (p_dct_stat->dimm_ecc_present != 0) {
			if ((p_dct_stat->dimm_ecc_present ^ p_dct_stat->dimm_valid) == 0) {
				/* all DIMMs are ECC capable */
				p_dct_stat->status |= 1 << SB_ECC_DIMMS;
			}
		}
		if (p_dct_stat->dimm_parity_present != 0) {
			if ((p_dct_stat->dimm_parity_present ^ p_dct_stat->dimm_valid) == 0) {
				/*all DIMMs are Parity capable */
				p_dct_stat->status |= 1 << SB_PAR_DIMMS;
			}
		}
	} else {
		/* no DIMMs present or no DIMMs that qualified. */
		p_dct_stat->err_status |= 1 << SB_NO_DIMMS;
		p_dct_stat->err_code = SC_STOP_ERR;
	}

	print_tx("\t DIMMPresence: status ", p_dct_stat->status);
	print_tx("\t DIMMPresence: err_status ", p_dct_stat->err_status);
	print_tx("\t DIMMPresence: err_code ", p_dct_stat->err_code);
	print_t("\t DIMMPresence: Done\n");

	mct_hook_after_dimm_pre();

	return p_dct_stat->err_code;
}


static u8 sys_capability_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, int j, int k)
{
	/* Determine if system is capable of operating at given input
	 * parameters for CL, and T.  There are three components to
	 * determining "maximum frequency" in AUTO mode: SPD component,
	 * Bus load component, and "Preset" max frequency component.
	 * This procedure is used to help find the SPD component and relies
	 * on pre-determination of the bus load component and the Preset
	 * components.  The generalized algorithm for finding maximum
	 * frequency is structured this way so as to optimize for CAS
	 * latency (which might get better as a result of reduced frequency).
	 * See "Global relationship between index values and item values"
	 * for definition of CAS latency index (j) and Frequency index (k).
	 */
	u8 freq_ok, cl_ok;
	u8 ret = 0;

	if (get_fk_d(k) > p_dct_stat->preset_max_freq)
		freq_ok = 0;
	else
		freq_ok = 1;

	/* compare proposed CAS latency with AMD Si capabilities */
	if ((j < J_MIN) || (j > J_MAX))
		cl_ok = 0;
	else
		cl_ok = 1;

	if (freq_ok && cl_ok)
		ret = 1;

	return ret;
}


static u8 get_dimm_address_d(struct DCTStatStruc *p_dct_stat, u8 i)
{
	u8 *p;

	p = p_dct_stat->dimm_addr;
	return p[i];
}


static void mct_init_dct(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat)
{
	u32 val;
	u8 err_code;

	/* Config. DCT0 for Ganged or unganged mode */
	print_t("\tmct_initDCT: dct_init_d 0\n");
	dct_init_d(p_mct_stat, p_dct_stat, 0);
	if (p_dct_stat->err_code == SC_FATAL_ERR) {
		// Do nothing goto exitDCTInit;	/* any fatal errors? */
	} else {
		/* Configure DCT1 if unganged and enabled*/
		if (!p_dct_stat->ganged_mode) {
			if (p_dct_stat->dimm_valid_dct[1] > 0) {
				print_t("\tmct_initDCT: dct_init_d 1\n");
				err_code = p_dct_stat->err_code;		/* save DCT0 errors */
				p_dct_stat->err_code = 0;
				dct_init_d(p_mct_stat, p_dct_stat, 1);
				if (p_dct_stat->err_code == 2)		/* DCT1 is not Running */
					p_dct_stat->err_code = err_code;	/* Using DCT0 Error code to update p_dct_stat.err_code */
			} else {
				val = 1 << DIS_DRAM_INTERFACE;
				set_nb32(p_dct_stat->dev_dct, 0x100 + 0x94, val);
			}
		}
	}
// exitDCTInit:
}


static void mct_dram_init(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, u8 dct)
{
	u32 val;

	mct_before_dram_init__prod_d(p_mct_stat, p_dct_stat);
	// FIXME: for rev A: mct_BeforeDramInit_D(p_dct_stat, dct);

	/* Disable auto refresh before Dram init when in ganged mode (Erratum 278) */
	if (p_dct_stat->logical_cpuid & (AMD_DR_B0 | AMD_DR_B1 | AMD_DR_BA)) {
		if (p_dct_stat->ganged_mode) {
			val = get_nb32(p_dct_stat->dev_dct, 0x8C + (0x100 * dct));
			val |= 1 << DIS_AUTO_REFRESH;
			set_nb32(p_dct_stat->dev_dct, 0x8C + (0x100 * dct), val);
		}
	}

	mct_DramInit_Hw_D(p_mct_stat, p_dct_stat, dct);

	/* Re-enable auto refresh after Dram init when in ganged mode
	 * to ensure both DCTs are in sync (Erratum 278)
	 */

	if (p_dct_stat->logical_cpuid & (AMD_DR_B0 | AMD_DR_B1 | AMD_DR_BA)) {
		if (p_dct_stat->ganged_mode) {
			do {
				val = get_nb32(p_dct_stat->dev_dct, 0x90 + (0x100 * dct));
			} while (!(val & (1 << INIT_DRAM)));

			wait_routine_d(50);

			val = get_nb32(p_dct_stat->dev_dct, 0x8C + (0x100 * dct));
			val &= ~(1 << DIS_AUTO_REFRESH);
			set_nb32(p_dct_stat->dev_dct, 0x8C + (0x100 * dct), val);
			val |= 1 << DIS_AUTO_REFRESH;
			set_nb32(p_dct_stat->dev_dct, 0x8C + (0x100 * dct), val);
			val &= ~(1 << DIS_AUTO_REFRESH);
			set_nb32(p_dct_stat->dev_dct, 0x8C + (0x100 * dct), val);
		}
	}
}


static u8 mct_set_mode(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat)
{
	u8 byte;
	u8 bytex;
	u32 val;
	u32 reg;

	byte = bytex = p_dct_stat->dimm_valid;
	bytex &= 0x55;		/* CHA dimm pop */
	p_dct_stat->dimm_valid_dct[0] = bytex;

	byte &= 0xAA;		/* CHB dimm popa */
	byte >>= 1;
	p_dct_stat->dimm_valid_dct[1] = byte;

	if (byte != bytex) {
		p_dct_stat->err_status &= ~(1 << SB_DIMM_MISMATCH_O);
	} else {
		if (mct_get_nv_bits(NV_Unganged))
			p_dct_stat->err_status |= (1 << SB_DIMM_MISMATCH_O);

		if (!(p_dct_stat->err_status & (1 << SB_DIMM_MISMATCH_O))) {
			p_dct_stat->ganged_mode = 1;
			/* valid 128-bit mode population. */
			p_dct_stat->status |= 1 << SB_128_BIT_MODE;
			reg = 0x110;
			val = get_nb32(p_dct_stat->dev_dct, reg);
			val |= 1 << DCT_GANG_EN;
			set_nb32(p_dct_stat->dev_dct, reg, val);
			print_tx("setMode: DRAM Controller Select Low Register = ", val);
		}
	}
	return p_dct_stat->err_code;
}


u32 get_nb32(u32 dev, u32 reg)
{
	return pci_read_config32(dev, reg);
}


void set_nb32(u32 dev, u32 reg, u32 val)
{
	pci_write_config32(dev, reg, val);
}


u32 get_nb32_index(u32 dev, u32 index_reg, u32 index)
{
	u32 dword;

	set_nb32(dev, index_reg, index);
	dword = get_nb32(dev, index_reg + 0x4);

	return dword;
}

void set_nb32_index(u32 dev, u32 index_reg, u32 index, u32 data)
{
	set_nb32(dev, index_reg, index);
	set_nb32(dev, index_reg + 0x4, data);
}


u32 get_nb32_index_wait(u32 dev, u32 index_reg, u32 index)
{
	u32 dword;

	index &= ~(1 << DCT_ACCESS_WRITE);
	set_nb32(dev, index_reg, index);
	do {
		dword = get_nb32(dev, index_reg);
	} while (!(dword & (1 << DCT_ACCESS_DONE)));
	dword = get_nb32(dev, index_reg + 0x4);

	return dword;
}


void set_nb32_index_wait(u32 dev, u32 index_reg, u32 index, u32 data)
{
	u32 dword;

	set_nb32(dev, index_reg + 0x4, data);
	index |= (1 << DCT_ACCESS_WRITE);
	set_nb32(dev, index_reg, index);
	do {
		dword = get_nb32(dev, index_reg);
	} while (!(dword & (1 << DCT_ACCESS_DONE)));

}


static u8 mct_platform_spec(struct MCTStatStruc *p_mct_stat,
					struct DCTStatStruc *p_dct_stat, u8 dct)
{
	/* Get platform specific config/timing values from the interface layer
	 * and program them into DCT.
	 */

	u32 dev = p_dct_stat->dev_dct;
	u32 index_reg;
	u8 i, i_start, i_end;

	if (p_dct_stat->ganged_mode) {
		SyncSetting(p_dct_stat);
		i_start = 0;
		i_end = 2;
	} else {
		i_start = dct;
		i_end = dct + 1;
	}
	for (i = i_start; i < i_end; i++) {
		index_reg = 0x98 + (i * 0x100);
		set_nb32_index_wait(dev, index_reg, 0x00, p_dct_stat->ch_odc_ctl[i]); /* channel A Output Driver Compensation Control */
		set_nb32_index_wait(dev, index_reg, 0x04, p_dct_stat->ch_addr_tmg[i]); /* channel A Output Driver Compensation Control */
	}

	return p_dct_stat->err_code;

}


static void mct_sync_dcts_ready(struct DCTStatStruc *p_dct_stat)
{
	u32 dev;
	u32 val;

	if (p_dct_stat->node_present) {
		print_tx("mct_sync_dcts_ready: node ", p_dct_stat->node_id);
		dev = p_dct_stat->dev_dct;

		if ((p_dct_stat->dimm_valid_dct[0]) || (p_dct_stat->dimm_valid_dct[1])) {		/* This node has dram */
			do {
				val = get_nb32(dev, 0x110);
			} while (!(val & (1 << DRAM_ENABLED)));
			print_t("mct_sync_dcts_ready: DRAM_ENABLED\n");
		}
	}	/* node is present */
}


static void mct_after_get_clt(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, u8 dct)
{
	if (!p_dct_stat->ganged_mode) {
		if (dct == 0) {
			p_dct_stat->dimm_valid = p_dct_stat->dimm_valid_dct[dct];
			if (p_dct_stat->dimm_valid_dct[dct] == 0)
				p_dct_stat->err_code = SC_STOP_ERR;
		} else {
			p_dct_stat->cs_present = 0;
			p_dct_stat->cs_test_fail = 0;
			p_dct_stat->dimm_valid = p_dct_stat->dimm_valid_dct[dct];
			if (p_dct_stat->dimm_valid_dct[dct] == 0)
				p_dct_stat->err_code = SC_STOP_ERR;
		}
	}
}

static u8 mct_spd_calc_width(struct MCTStatStruc *p_mct_stat,
					struct DCTStatStruc *p_dct_stat, u8 dct)
{
	u8 ret;

	if (dct == 0) {
		spd_calc_width_d(p_mct_stat, p_dct_stat);
		ret = mct_set_mode(p_mct_stat, p_dct_stat);
	} else {
		ret = p_dct_stat->err_code;
	}

	print_tx("SPDCalcWidth: status ", p_dct_stat->status);
	print_tx("SPDCalcWidth: err_status ", p_dct_stat->err_status);
	print_tx("SPDCalcWidth: err_code ", p_dct_stat->err_code);
	print_t("SPDCalcWidth: Done\n");

	return ret;
}


static void mct_after_stitch_memory(struct MCTStatStruc *p_mct_stat,
					struct DCTStatStruc *p_dct_stat, u8 dct)
{
	u32 val;
	u32 dword;
	u32 dev;
	u32 reg;
	u8 _mem_hole_remap;
	u32 dram_hole_base;

	_mem_hole_remap = mct_get_nv_bits(NV_MEM_HOLE);
	dram_hole_base = mct_get_nv_bits(NV_BOTTOM_IO);
	dram_hole_base <<= 8;
	/* Increase hole size so;[31:24]to[31:16]
	 * it has granularity of 128MB shl eax,8
	 * Set 'effective' bottom IOmov dram_hole_base,eax
	 */
	p_mct_stat->hole_base = (dram_hole_base & 0xFFFFF800) << 8;

	/* In unganged mode, we must add DCT0 and DCT1 to dct_sys_limit */
	if (!p_dct_stat->ganged_mode) {
		dev = p_dct_stat->dev_dct;
		p_dct_stat->node_sys_limit += p_dct_stat->dct_sys_limit;
		/* if DCT0 and DCT1 both exist, set DctSelBaseAddr[47:27] to the top of DCT0 */
		if (dct == 0) {
			if (p_dct_stat->dimm_valid_dct[1] > 0) {
				dword = p_dct_stat->dct_sys_limit + 1;
				dword += p_dct_stat->node_sys_base;
				dword >>= 8; /* scale [39:8] to [47:27],and to F2x110[31:11] */
				if ((dword >= dram_hole_base) && _mem_hole_remap) {
					p_mct_stat->hole_base = (dram_hole_base & 0xFFFFF800) << 8;
					val = p_mct_stat->hole_base;
					val >>= 16;
					val = (((~val) & 0xFF) + 1);
					val <<= 8;
					dword += val;
				}
				reg = 0x110;
				val = get_nb32(dev, reg);
				val &= 0x7F;
				val |= dword;
				val |= 3;  /* Set F2x110[DctSelHiRngEn], F2x110[DctSelHi] */
				set_nb32(dev, reg, val);
				print_tx("AfterStitch DCT0 and DCT1: DRAM Controller Select Low Register = ", val);
				print_tx("AfterStitch DCT0 and DCT1: DRAM Controller Select High Register = ", dword);

				reg = 0x114;
				val = dword;
				set_nb32(dev, reg, val);
			}
		} else {
			/* Program the DctSelBaseAddr value to 0
			   if DCT 0 is disabled */
			if (p_dct_stat->dimm_valid_dct[0] == 0) {
				dword = p_dct_stat->node_sys_base;
				dword >>= 8;
				if ((dword >= dram_hole_base) && _mem_hole_remap) {
					p_mct_stat->hole_base = (dram_hole_base & 0xFFFFF800) << 8;
					val = p_mct_stat->hole_base;
					val >>= 8;
					val &= ~(0xFFFF);
					val |= (((~val) & 0xFFFF) + 1);
					dword += val;
				}
				reg = 0x114;
				val = dword;
				set_nb32(dev, reg, val);

				reg = 0x110;
				val |= 3;	/* Set F2x110[DctSelHiRngEn], F2x110[DctSelHi] */
				set_nb32(dev, reg, val);
				print_tx("AfterStitch DCT1 only: DRAM Controller Select Low Register = ", val);
				print_tx("AfterStitch DCT1 only: DRAM Controller Select High Register = ", dword);
			}
		}
	} else {
		p_dct_stat->node_sys_limit += p_dct_stat->dct_sys_limit;
	}
	print_tx("AfterStitch p_dct_stat->node_sys_base = ", p_dct_stat->node_sys_base);
	print_tx("mct_after_stitch_memory: p_dct_stat->node_sys_limit ", p_dct_stat->node_sys_limit);
}


static u8 mct_dimm_presence(struct MCTStatStruc *p_mct_stat,
					struct DCTStatStruc *p_dct_stat, u8 dct)
{
	u8 ret;

	if (dct == 0)
		ret = dimm_presence_d(p_mct_stat, p_dct_stat);
	else
		ret = p_dct_stat->err_code;

	return ret;
}


/* mct_BeforeGetDIMMAddress inline in C */


static void mct_other_timing(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat_a)
{
	u8 node;

	for (node = 0; node < MAX_NODES_SUPPORTED; node++) {
		struct DCTStatStruc *p_dct_stat;
		p_dct_stat = p_dct_stat_a + node;
		if (p_dct_stat->node_present) {
			if (p_dct_stat->dimm_valid_dct[0]) {
				p_dct_stat->dimm_valid = p_dct_stat->dimm_valid_dct[0];
				set_other_timing(p_mct_stat, p_dct_stat, 0);
			}
			if (p_dct_stat->dimm_valid_dct[1] && !p_dct_stat->ganged_mode) {
				p_dct_stat->dimm_valid = p_dct_stat->dimm_valid_dct[1];
				set_other_timing(p_mct_stat, p_dct_stat, 1);
			}
		}	/* node is present*/
	}	/* while node */
}


static void set_other_timing(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, u8 dct)
{
	u32 reg;
	u32 reg_off = 0x100 * dct;
	u32 val;
	u32 dword;
	u32 dev = p_dct_stat->dev_dct;

	get_trdrd(p_mct_stat, p_dct_stat, dct);
	get_twrwr(p_mct_stat, p_dct_stat, dct);
	get_twrrd(p_mct_stat, p_dct_stat, dct);
	get_trwt_to(p_mct_stat, p_dct_stat, dct);
	get_trwt_wb(p_mct_stat, p_dct_stat);

	reg = 0x8C + reg_off;		/* Dram Timing Hi */
	val = get_nb32(dev, reg);
	val &= 0xffff0300;
	dword = p_dct_stat->trwt_to; //0x07
	val |= dword << 4;
	dword = p_dct_stat->twrrd; //0x03
	val |= dword << 10;
	dword = p_dct_stat->twrwr; //0x03
	val |= dword << 12;
	dword = p_dct_stat->trdrd; //0x03
	val |= dword << 14;
	dword = p_dct_stat->trwt_wb; //0x07
	val |= dword;
	val = OtherTiming_A_D(p_dct_stat, val);
	set_nb32(dev, reg, val);

}


static void get_trdrd(struct MCTStatStruc *p_mct_stat,
			struct DCTStatStruc *p_dct_stat, u8 dct)
{
	u8 trdrd;
	u8 byte;
	u32 dword;
	u32 val;
	u32 index_reg = 0x98 + 0x100 * dct;
	u32 dev = p_dct_stat->dev_dct;

	if ((p_dct_stat->dimm_x4_present != 0) && (p_dct_stat->dimm_x8_present != 0)) {
		/* mixed (x4 or x8) dimm types
		  the largest DqsRcvEnGrossDelay of any dimm minus the DqsRcvEnGrossDelay
		  of any other dimm is equal to the Critical Gross Delay Difference (CGDD) for trdrd.*/
		byte = get_dqs_rcv_en_gross_diff(p_dct_stat, dev, index_reg);
		if (byte == 0)
			trdrd = 1;
		else
			trdrd = 2;

	} else {
		/*
		 trdrd with non-mixed dimm types
		 RdDqsTime are the same for all DIMMs and DqsRcvEn difference between
		 any two DIMMs is less than half of a MEMCLK, BIOS should program trdrd to 0000b,
		 else BIOS should program trdrd to 0001b.

		 RdDqsTime are the same for all DIMMs
		 DDR400~DDR667 only use one set register
		 DDR800 have two set register for DIMM0 and DIMM1 */
		trdrd = 1;
		if (p_dct_stat->speed > 3) {
			/* DIMM0+DIMM1 exist */ //NOTE it should be 5
			val = bsf(p_dct_stat->dimm_valid);
			dword = bsr(p_dct_stat->dimm_valid);
			if (dword != val && dword != 0)  {
				/* DCT Read DQS Timing Control - DIMM0 - Low */
				dword = get_nb32_index_wait(dev, index_reg, 0x05);
				/* DCT Read DQS Timing Control - DIMM1 - Low */
				val = get_nb32_index_wait(dev, index_reg, 0x105);
				if (val != dword)
					goto Trdrd_1;

				/* DCT Read DQS Timing Control - DIMM0 - High */
				dword = get_nb32_index_wait(dev, index_reg, 0x06);
				/* DCT Read DQS Timing Control - DIMM1 - High */
				val = get_nb32_index_wait(dev, index_reg, 0x106);
				if (val != dword)
					goto Trdrd_1;
			}
		}

		/* DqsRcvEn difference between any two DIMMs is
		   less than half of a MEMCLK */
		/* DqsRcvEn byte 1,0*/
		if (check_dqs_rcv_en_diff(p_dct_stat, dct, dev, index_reg, 0x10))
			goto Trdrd_1;
		/* DqsRcvEn byte 3,2*/
		if (check_dqs_rcv_en_diff(p_dct_stat, dct, dev, index_reg, 0x11))
			goto Trdrd_1;
		/* DqsRcvEn byte 5,4*/
		if (check_dqs_rcv_en_diff(p_dct_stat, dct, dev, index_reg, 0x20))
			goto Trdrd_1;
		/* DqsRcvEn byte 7,6*/
		if (check_dqs_rcv_en_diff(p_dct_stat, dct, dev, index_reg, 0x21))
			goto Trdrd_1;
		/* DqsRcvEn ECC*/
		if (check_dqs_rcv_en_diff(p_dct_stat, dct, dev, index_reg, 0x12))
			goto Trdrd_1;
		trdrd = 0;
	Trdrd_1:
		;
	}
	p_dct_stat->trdrd = trdrd;

}


static void get_twrwr(struct MCTStatStruc *p_mct_stat,
			struct DCTStatStruc *p_dct_stat, u8 dct)
{
	u8 twrwr = 0;
	u32 index_reg = 0x98 + 0x100 * dct;
	u32 dev = p_dct_stat->dev_dct;
	u32 val;
	u32 dword;

	/* WrDatGrossDlyByte only use one set register when DDR400~DDR667
	   DDR800 have two set register for DIMM0 and DIMM1 */
	if (p_dct_stat->speed > 3) {
		val = bsf(p_dct_stat->dimm_valid);
		dword = bsr(p_dct_stat->dimm_valid);
		if (dword != val && dword != 0)  {
			/*the largest WrDatGrossDlyByte of any dimm minus the
			  WrDatGrossDlyByte of any other dimm is equal to CGDD */
			val = get_wr_dat_gross_diff(p_dct_stat, dct, dev, index_reg);
		}
		if (val == 0)
			twrwr = 2;
		else
			twrwr = 3;
	}
	p_dct_stat->twrwr = twrwr;
}


static void get_twrrd(struct MCTStatStruc *p_mct_stat,
			struct DCTStatStruc *p_dct_stat, u8 dct)
{
	u8 byte, bytex, val;
	u32 index_reg = 0x98 + 0x100 * dct;
	u32 dev = p_dct_stat->dev_dct;

	/* On any given byte lane, the largest WrDatGrossDlyByte delay of
	   any dimm minus the DqsRcvEnGrossDelay delay of any other dimm is
	   equal to the Critical Gross Delay Difference (CGDD) for Twrrd.*/

	/* WrDatGrossDlyByte only use one set register when DDR400~DDR667
	   DDR800 have two set register for DIMM0 and DIMM1 */
	if (p_dct_stat->speed > 3) {
		val = get_wr_dat_gross_diff(p_dct_stat, dct, dev, index_reg);
	} else {
		val = get_wr_dat_gross_max_min(p_dct_stat, dct, dev, index_reg, 1);	/* WrDatGrossDlyByte byte 0,1,2,3 for DIMM0 */
		p_dct_stat->wr_dat_gross_h = (u8) val; /* low byte = max value */
	}

	get_dqs_rcv_en_gross_diff(p_dct_stat, dev, index_reg);

	bytex = p_dct_stat->dqs_rcv_en_gross_l;
	byte = p_dct_stat->wr_dat_gross_h;
	if (byte > bytex) {
		byte -= bytex;
		if (byte == 1)
			bytex = 1;
		else
			bytex = 2;
	} else {
		bytex = 0;
	}
	p_dct_stat->twrrd = bytex;
}


static void get_trwt_to(struct MCTStatStruc *p_mct_stat,
			struct DCTStatStruc *p_dct_stat, u8 dct)
{
	u8 byte, bytex;
	u32 index_reg = 0x98 + 0x100 * dct;
	u32 dev = p_dct_stat->dev_dct;

	/* On any given byte lane, the largest WrDatGrossDlyByte delay of
	   any dimm minus the DqsRcvEnGrossDelay delay of any other dimm is
	   equal to the Critical Gross Delay Difference (CGDD) for TrwtTO. */
	get_dqs_rcv_en_gross_diff(p_dct_stat, dev, index_reg);
	get_wr_dat_gross_diff(p_dct_stat, dct, dev, index_reg);
	bytex = p_dct_stat->dqs_rcv_en_gross_l;
	byte = p_dct_stat->wr_dat_gross_h;
	if (bytex > byte) {
		bytex -= byte;
		if ((bytex == 1) || (bytex == 2))
			bytex = 3;
		else
			bytex = 4;
	} else {
		byte -= bytex;
		if ((byte == 0) || (byte == 1))
			bytex = 2;
		else
			bytex = 1;
	}

	p_dct_stat->trwt_to = bytex;
}


static void get_trwt_wb(struct MCTStatStruc *p_mct_stat,
			struct DCTStatStruc *p_dct_stat)
{
	/* trwt_wb ensures read-to-write data-bus turnaround.
	   This value should be one more than the programmed TrwtTO.*/
	p_dct_stat->trwt_wb = p_dct_stat->trwt_to + 1;
}


static u8 check_dqs_rcv_en_diff(struct DCTStatStruc *p_dct_stat,
					u8 dct, u32 dev, u32 index_reg,
					u32 index)
{
	u8 smallest_0, largest_0, smallest_1, largest_1;
	u8 i;
	u32 val;
	u8 byte;
	u8 ecc_reg = 0;

	smallest_0 = 0xFF;
	smallest_1 = 0xFF;
	largest_0 = 0;
	largest_1 = 0;

	if (index == 0x12)
		ecc_reg = 1;

	for (i = 0; i < 8; i+=2) {
		if (p_dct_stat->dimm_valid & (1 << i)) {
			val = get_nb32_index_wait(dev, index_reg, index);
			byte = val & 0xFF;
			if (byte < smallest_0)
				smallest_0 = byte;
			if (byte > largest_0)
				largest_0 = byte;
			if (!(ecc_reg)) {
				byte = (val >> 16) & 0xFF;
				if (byte < smallest_1)
					smallest_1 = byte;
				if (byte > largest_1)
					largest_1 = byte;
			}
		}
		index += 3;
	}	/* while ++i */

	/* check if total DqsRcvEn delay difference between any
	   two DIMMs is less than half of a MEMCLK */
	if ((largest_0 - smallest_0) > 31)
		return 1;
	if (!(ecc_reg))
		if ((largest_1 - smallest_1) > 31)
			return 1;
	return 0;
}


static u8 get_dqs_rcv_en_gross_diff(struct DCTStatStruc *p_dct_stat,
					u32 dev, u32 index_reg)
{
	u8 smallest, largest;
	u32 val;
	u8 byte, bytex;

	/* The largest DqsRcvEnGrossDelay of any dimm minus the
	   DqsRcvEnGrossDelay of any other dimm is equal to the Critical
	   Gross Delay Difference (CGDD) */
	/* DqsRcvEn byte 1,0 */
	val = get_dqs_rcv_en_gross_max_min(p_dct_stat, dev, index_reg, 0x10);
	largest = val & 0xFF;
	smallest = (val >> 8) & 0xFF;

	/* DqsRcvEn byte 3,2 */
	val = get_dqs_rcv_en_gross_max_min(p_dct_stat, dev, index_reg, 0x11);
	byte = val & 0xFF;
	bytex = (val >> 8) & 0xFF;
	if (bytex < smallest)
		smallest = bytex;
	if (byte > largest)
		largest = byte;

	/* DqsRcvEn byte 5,4 */
	val = get_dqs_rcv_en_gross_max_min(p_dct_stat, dev, index_reg, 0x20);
	byte = val & 0xFF;
	bytex = (val >> 8) & 0xFF;
	if (bytex < smallest)
		smallest = bytex;
	if (byte > largest)
		largest = byte;

	/* DqsRcvEn byte 7,6 */
	val = get_dqs_rcv_en_gross_max_min(p_dct_stat, dev, index_reg, 0x21);
	byte = val & 0xFF;
	bytex = (val >> 8) & 0xFF;
	if (bytex < smallest)
		smallest = bytex;
	if (byte > largest)
		largest = byte;

	if (p_dct_stat->dimm_ecc_present > 0) {
		/*DqsRcvEn Ecc */
		val = get_dqs_rcv_en_gross_max_min(p_dct_stat, dev, index_reg, 0x12);
		byte = val & 0xFF;
		bytex = (val >> 8) & 0xFF;
		if (bytex < smallest)
			smallest = bytex;
		if (byte > largest)
			largest = byte;
	}

	p_dct_stat->dqs_rcv_en_gross_l = largest;
	return largest - smallest;
}


static u8 get_wr_dat_gross_diff(struct DCTStatStruc *p_dct_stat,
					u8 dct, u32 dev, u32 index_reg)
{
	u8 smallest, largest;
	u32 val;
	u8 byte, bytex;

	/* The largest WrDatGrossDlyByte of any dimm minus the
	  WrDatGrossDlyByte of any other dimm is equal to CGDD */
	val = get_wr_dat_gross_max_min(p_dct_stat, dct, dev, index_reg, 0x01);	/* WrDatGrossDlyByte byte 0,1,2,3 for DIMM0 */
	largest = val & 0xFF;
	smallest = (val >> 8) & 0xFF;
	val = get_wr_dat_gross_max_min(p_dct_stat, dct, dev, index_reg, 0x101);	/* WrDatGrossDlyByte byte 0,1,2,3 for DIMM1 */
	byte = val & 0xFF;
	bytex = (val >> 8) & 0xFF;
	if (bytex < smallest)
		smallest = bytex;
	if (byte > largest)
		largest = byte;

	// FIXME: Add Cx support.

	p_dct_stat->wr_dat_gross_h = largest;
	return largest - smallest;
}

static u16 get_dqs_rcv_en_gross_max_min(struct DCTStatStruc *p_dct_stat,
					u32 dev, u32 index_reg,
					u32 index)
{
	u8 smallest, largest;
	u8 i;
	u8 byte;
	u32 val;
	u16 word;
	u8 ecc_reg = 0;

	smallest = 7;
	largest = 0;

	if (index == 0x12)
		ecc_reg = 1;

	for (i = 0; i < 8; i+=2) {
		if (p_dct_stat->dimm_valid & (1 << i)) {
			val = get_nb32_index_wait(dev, index_reg, index);
			val &= 0x00E000E0;
			byte = (val >> 5) & 0xFF;
			if (byte < smallest)
				smallest = byte;
			if (byte > largest)
				largest = byte;
			if (!(ecc_reg)) {
				byte = (val >> (16 + 5)) & 0xFF;
				if (byte < smallest)
					smallest = byte;
				if (byte > largest)
					largest = byte;
			}
		}
		index += 3;
	}	/* while ++i */

	word = smallest;
	word <<= 8;
	word |= largest;

	return word;
}

static u16 get_wr_dat_gross_max_min(struct DCTStatStruc *p_dct_stat,
					u8 dct, u32 dev, u32 index_reg,
					u32 index)
{
	u8 smallest, largest;
	u8 i, j;
	u32 val;
	u8 byte;
	u16 word;

	smallest = 3;
	largest = 0;
	for (i = 0; i < 2; i++) {
		val = get_nb32_index_wait(dev, index_reg, index);
		val &= 0x60606060;
		val >>= 5;
		for (j = 0; j < 4; j++) {
			byte = val & 0xFF;
			if (byte < smallest)
				smallest = byte;
			if (byte > largest)
				largest = byte;
			val >>= 8;
		}	/* while ++j */
		index++;
	}	/*while ++i*/

	if (p_dct_stat->dimm_ecc_present > 0) {
		index++;
		val = get_nb32_index_wait(dev, index_reg, index);
		val &= 0x00000060;
		val >>= 5;
		byte = val & 0xFF;
		if (byte < smallest)
			smallest = byte;
		if (byte > largest)
			largest = byte;
	}

	word = smallest;
	word <<= 8;
	word |= largest;

	return word;
}



static void mct_final_mct_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat)
{
	print_t("\tmct_FinalMCT_D: Clr Cl, Wb\n");

	/* ClrClToNB_D postponed until we're done executing from ROM */
	mct_clr_wb_enh_wsb_dis_d(p_mct_stat, p_dct_stat);
}


static void mct_initial_mct_d(struct MCTStatStruc *p_mct_stat, struct DCTStatStruc *p_dct_stat)
{
	print_t("\tmct_InitialMCT_D: Set Cl, Wb\n");
	mct_set_cl_to_nb_d(p_mct_stat, p_dct_stat);
	mct_set_wb_enh_wsb_dis_d(p_mct_stat, p_dct_stat);
}


static u32 mct_node_present_d(void)
{
	u32 val;
	val = 0x12001022;
	return val;
}


static void mct_init(struct MCTStatStruc *p_mct_stat,
			struct DCTStatStruc *p_dct_stat)
{
	u32 lo, hi;
	u32 addr;

	p_dct_stat->ganged_mode = 0;
	p_dct_stat->dr_present = 1;

	/* enable extend PCI configuration access */
	addr = NB_CFG_MSR;
	_RDMSR(addr, &lo, &hi);
	if (hi & (1 << (46-32))) {
		p_dct_stat->status |= 1 << SB_EXT_CONFIG;
	} else {
		hi |= 1 << (46-32);
		_WRMSR(addr, lo, hi);
	}
}


static void clear_legacy_mode(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat)
{
	u32 reg;
	u32 val;
	u32 dev = p_dct_stat->dev_dct;

	/* Clear Legacy BIOS Mode bit */
	reg = 0x94;
	val = get_nb32(dev, reg);
	val &= ~(1 << LEGACY_BIOS_MODE);
	set_nb32(dev, reg, val);
}


static void mct_ht_mem_map_ext(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat_a)
{
	u8 node;
	u32 dram_base, dram_limit;
	u32 val;
	u32 reg;
	u32 dev;
	u32 devx;
	u32 dword;
	struct DCTStatStruc *p_dct_stat;

	p_dct_stat = p_dct_stat_a + 0;
	dev = p_dct_stat->dev_map;

	/* Copy dram map from F1x40/44,F1x48/4c,
	   to F1x120/124(Node0),F1x120/124(Node1),...*/
	for (node = 0; node < MAX_NODES_SUPPORTED; node++) {
		p_dct_stat = p_dct_stat_a + node;
		devx = p_dct_stat->dev_map;

		/* get base/limit from Node0 */
		reg = 0x40 + (node << 3);		/* Node0/Dram Base 0 */
		val = get_nb32(dev, reg);
		dram_base = val >> (16 + 3);

		reg = 0x44 + (node << 3);		/* Node0/Dram Base 0 */
		val = get_nb32(dev, reg);
		dram_limit = val >> (16 + 3);

		/* set base/limit to F1x120/124 per node */
		if (p_dct_stat->node_present) {
			reg = 0x120;		/* F1x120,DramBase[47:27] */
			val = get_nb32(devx, reg);
			val &= 0xFFE00000;
			val |= dram_base;
			set_nb32(devx, reg, val);

			reg = 0x124;
			val = get_nb32(devx, reg);
			val &= 0xFFE00000;
			val |= dram_limit;
			set_nb32(devx, reg, val);

			if (p_mct_stat->g_status & (1 << GSB_HW_HOLE)) {
				reg = 0xF0;
				val = get_nb32(devx, reg);
				val |= (1 << DRAM_MEM_HOIST_VALID);
				val &= ~(0xFF << 24);
				dword = (p_mct_stat->hole_base >> (24 - 8)) & 0xFF;
				dword <<= 24;
				val |= dword;
				set_nb32(devx, reg, val);
			}

		}
	}
}

static void set_cs_tristate(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, u8 dct)
{
	u32 val;
	u32 dev = p_dct_stat->dev_dct;
	u32 index_reg = 0x98 + 0x100 * dct;
	u8 cs;
	u32 index;
	u16 word;

	/* Tri-state unused chipselects when motherboard
	   termination is available */

	// FIXME: skip for Ax

	word = p_dct_stat->cs_present;
	if (p_dct_stat->status & (1 << SB_REGISTERED)) {
		for (cs = 0; cs < 8; cs++) {
			if (word & (1 << cs)) {
				if (!(cs & 1))
					word |= 1 << (cs + 1);
			}
		}
	}
	word = (~word) & 0xFF;
	index  = 0x0c;
	val = get_nb32_index_wait(dev, index_reg, index);
	val |= word;
	set_nb32_index_wait(dev, index_reg, index, val);
}


#ifdef UNUSED_CODE
static void set_cke_tristate(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, u8 dct)
{
	u32 val;
	u32 dev;
	u32 index_reg = 0x98 + 0x100 * dct;
	u8 cs;
	u32 index;
	u16 word;

	/* Tri-state unused CKEs when motherboard termination is available */

	// FIXME: skip for Ax

	dev = p_dct_stat->dev_dct;
	word = 0x101;
	for (cs = 0; cs < 8; cs++) {
		if (p_dct_stat->cs_present & (1 << cs)) {
			if (!(cs & 1))
				word &= 0xFF00;
			else
				word &= 0x00FF;
		}
	}

	index  = 0x0c;
	val = get_nb32_index_wait(dev, index_reg, index);
	if ((word & 0x00FF) == 1)
		val |= 1 << 12;
	else
		val &= ~(1 << 12);

	if ((word >> 8) == 1)
		val |= 1 << 13;
	else
		val &= ~(1 << 13);

	set_nb32_index_wait(dev, index_reg, index, val);
}
#endif

static void set_odt_tristate(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, u8 dct)
{
	u32 val;
	u32 dev;
	u32 index_reg = 0x98 + 0x100 * dct;
	u8 cs;
	u32 index;
	u8 odt;
	u8 max_dimms;

	// FIXME: skip for Ax

	dev = p_dct_stat->dev_dct;

	/* Tri-state unused ODTs when motherboard termination is available */
	max_dimms = (u8) mct_get_nv_bits(NV_MAX_DIMMS);
	odt = 0x0F;	/* tristate all the pins then clear the used ones. */

	for (cs = 0; cs < 8; cs += 2) {
		if (p_dct_stat->cs_present & (1 << cs)) {
			odt &= ~(1 << (cs / 2));

			/* if quad-rank capable platform clear additional pins */
			if (max_dimms != MAX_CS_SUPPORTED) {
				if (p_dct_stat->cs_present & (1 << (cs + 1)))
					odt &= ~(4 << (cs / 2));
			}
		}
	}

	index  = 0x0C;
	val = get_nb32_index_wait(dev, index_reg, index);
	val |= (odt << 8);
	set_nb32_index_wait(dev, index_reg, index, val);

}


static void init_phy_compensation(struct MCTStatStruc *p_mct_stat,
					struct DCTStatStruc *p_dct_stat, u8 dct)
{
	u8 i;
	u32 index_reg = 0x98 + 0x100 * dct;
	u32 dev = p_dct_stat->dev_dct;
	u32 val;
	u32 valx = 0;
	u32 dword;
	const u8 *p;

	val = get_nb32_index_wait(dev, index_reg, 0x00);
	dword = 0;
	for (i = 0; i < 6; i++) {
		switch (i) {
			case 0:
			case 4:
				p = table_comp_rise_slew_15x;
				valx = p[(val >> 16) & 3];
				break;
			case 1:
			case 5:
				p = table_comp_fall_slew_15x;
				valx = p[(val >> 16) & 3];
				break;
			case 2:
				p = table_comp_rise_slew_20x;
				valx = p[(val >> 8) & 3];
				break;
			case 3:
				p = table_comp_fall_slew_20x;
				valx = p[(val >> 8) & 3];
				break;

		}
		dword |= valx << (5 * i);
	}

	/* Override/Exception */
	if (!p_dct_stat->ganged_mode) {
		i = 0; /* use i for the dct setting required */
		if (p_dct_stat->ma_dimms[0] < 4)
			i = 1;
		if (((p_dct_stat->speed == 2) || (p_dct_stat->speed == 3)) && (p_dct_stat->ma_dimms[i] == 4)) {
			dword &= 0xF18FFF18;
			index_reg = 0x98;	/* force dct = 0 */
		}
	}

	set_nb32_index_wait(dev, index_reg, 0x0a, dword);
}


static void wait_routine_d(u32 time)
{
	while (time) {
		_EXECFENCE;
		time--;
	}
}


static void mct_early_arb_en_d(struct MCTStatStruc *p_mct_stat,
					struct DCTStatStruc *p_dct_stat)
{
	u32 reg;
	u32 val;
	u32 dev = p_dct_stat->dev_dct;

	/* GhEnhancement #18429 modified by askar: For low NB CLK :
	 * Memclk ratio, the DCT may need to arbitrate early to avoid
	 * unnecessary bubbles.
	 * bit 19 of F2x[1,0]78 Dram  Control Register, set this bit only when
	 * NB CLK : Memclk ratio is between 3:1 (inclusive) to 4:5 (inclusive)
	 */

	reg = 0x78;
	val = get_nb32(dev, reg);

	//FIXME: check for Cx
	if (check_nb_cof_early_arb_en(p_mct_stat, p_dct_stat))
		val |= (1 << EARLY_ARB_EN);

	set_nb32(dev, reg, val);

}


static u8 check_nb_cof_early_arb_en(struct MCTStatStruc *p_mct_stat,
					struct DCTStatStruc *p_dct_stat)
{
	u32 reg;
	u32 val;
	u32 tmp;
	u32 rem;
	u32 dev = p_dct_stat->dev_dct;
	u32 hi, lo;
	u8 nb_did = 0;

	/* Check if NB COF >= 4*Memclk, if it is not, return a fatal error
	 */

	/* 3*(Fn2xD4[NBFid]+4)/(2^nb_did)/(3+Fn2x94[MemClkFreq]) */
	_RDMSR(MSR_COFVID_STS, &lo, &hi);
	if (lo & (1 << 22))
		nb_did |= 1;


	reg = 0x94;
	val = get_nb32(dev, reg);
	if (!(val & (1 << MEM_CLK_FREQ_VAL)))
		val = get_nb32(dev, reg * 0x100);	/* get the DCT1 value */

	val &= 0x07;
	val += 3;
	if (nb_did)
		val <<= 1;
	tmp = val;

	dev = p_dct_stat->dev_nbmisc;
	reg = 0xD4;
	val = get_nb32(dev, reg);
	val &= 0x1F;
	val += 3;
	val *= 3;
	val = val / tmp;
	rem = val % tmp;
	tmp >>= 1;

	// Yes this could be nicer but this was how the asm was....
	if (val < 3) {				/* NClk:MemClk < 3:1 */
		return 0;
	} else if (val > 4) {			/* NClk:MemClk >= 5:1 */
		return 0;
	} else if ((val == 4) && (rem > tmp)) { /* NClk:MemClk > 4.5:1 */
		return 0;
	} else {
		return 1;			/* 3:1 <= NClk:MemClk <= 4.5:1*/
	}
}


static void mct_reset_data_struct_d(struct MCTStatStruc *p_mct_stat,
					struct DCTStatStruc *p_dct_stat_a)
{
	u8 node;
	struct DCTStatStruc *p_dct_stat;

	/* Initialize Data structures by clearing all entries to 0 */
	memset(p_mct_stat, 0x00, sizeof(*p_mct_stat));

	for (node = 0; node < 8; node++) {
		p_dct_stat = p_dct_stat_a + node;

		/* Clear all entries except persistentData */
		memset(p_dct_stat, 0x00, sizeof(*p_dct_stat) - sizeof(p_dct_stat->persistentData));
	}
}


static void mct_before_dram_init__prod_d(struct MCTStatStruc *p_mct_stat,
					struct DCTStatStruc *p_dct_stat)
{
	u8 i;
	u32 reg_off;
	u32 dev = p_dct_stat->dev_dct;

	// FIXME: skip for Ax
	if ((p_dct_stat->speed == 3) || (p_dct_stat->speed == 2)) { // MemClkFreq = 667MHz or 533MHz
		for (i = 0; i < 2; i++) {
			reg_off = 0x100 * i;
			set_nb32(dev,  0x98 + reg_off, 0x0D000030);
			set_nb32(dev,  0x9C + reg_off, 0x00000806);
			set_nb32(dev,  0x98 + reg_off, 0x4D040F30);
		}
	}
}


void mct_adjust_delay_range_d(struct MCTStatStruc *p_mct_stat,
			struct DCTStatStruc *p_dct_stat, u8 *dqs_pos)
{
	// FIXME: Skip for Ax
	if ((p_dct_stat->speed == 3) || (p_dct_stat->speed == 2)) { // MemClkFreq = 667MHz or 533MHz
		*dqs_pos = 32;
	}
}

static u32 mct_dis_dll_shutdown_sr(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, u32 dram_config_lo, u8 dct)
{
	u32 reg_off = 0x100 * dct;
	u32 dev = p_dct_stat->dev_dct;

	/* Write 0000_07D0h to register F2x[1, 0]98_x4D0FE006 */
	if (p_dct_stat->logical_cpuid & (AMD_DA_C2 | AMD_RB_C3)) {
		set_nb32(dev,  0x9C + reg_off, 0x7D0);
		set_nb32(dev,  0x98 + reg_off, 0x4D0FE006);
		set_nb32(dev,  0x9C + reg_off, 0x190);
		set_nb32(dev,  0x98 + reg_off, 0x4D0FE007);
	}

	return dram_config_lo | /* DisDllShutdownSR */ 1 << 27;
}

static void mct_en_dll_shutdown_sr(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, u8 dct)
{
	u32 reg_off = 0x100 * dct;
	u32 dev = p_dct_stat->dev_dct, val;

	/* Write 0000_07D0h to register F2x[1, 0]98_x4D0FE006 */
	if (p_dct_stat->logical_cpuid & (AMD_DA_C2 | AMD_RB_C3)) {
		set_nb32(dev,  0x9C + reg_off, 0x1C);
		set_nb32(dev,  0x98 + reg_off, 0x4D0FE006);
		set_nb32(dev,  0x9C + reg_off, 0x13D);
		set_nb32(dev,  0x98 + reg_off, 0x4D0FE007);

		val = get_nb32(dev, 0x90 + reg_off);
		val &= ~(1 << 27/* DisDllShutdownSR */);
		set_nb32(dev, 0x90 + reg_off, val);
	}
}

void mct_set_cl_to_nb_d(struct MCTStatStruc *p_mct_stat,
			struct DCTStatStruc *p_dct_stat)
{
	u32 lo, hi;
	u32 msr;

	// FIXME: Maybe check the CPUID? - not for now.
	// p_dct_stat->logical_cpuid;

	msr = BU_CFG2_MSR;
	_RDMSR(msr, &lo, &hi);
	lo |= 1 << CL_LINES_TO_NB_DIS;
	_WRMSR(msr, lo, hi);
}


void mct_clr_cl_to_nb_d(struct MCTStatStruc *p_mct_stat,
			struct DCTStatStruc *p_dct_stat)
{

	u32 lo, hi;
	u32 msr;

	// FIXME: Maybe check the CPUID? - not for now.
	// p_dct_stat->logical_cpuid;

	msr = BU_CFG2_MSR;
	_RDMSR(msr, &lo, &hi);
	if (!p_dct_stat->cl_to_nb_tag)
		lo &= ~(1 << CL_LINES_TO_NB_DIS);
	_WRMSR(msr, lo, hi);

}


void mct_set_wb_enh_wsb_dis_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat)
{
	u32 lo, hi;
	u32 msr;

	// FIXME: Maybe check the CPUID? - not for now.
	// p_dct_stat->logical_cpuid;

	msr = BU_CFG_MSR;
	_RDMSR(msr, &lo, &hi);
	hi |= (1 << WB_ENH_WSB_DIS_D);
	_WRMSR(msr, lo, hi);
}


void mct_clr_wb_enh_wsb_dis_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat)
{
	u32 lo, hi;
	u32 msr;

	// FIXME: Maybe check the CPUID? - not for now.
	// p_dct_stat->logical_cpuid;

	msr = BU_CFG_MSR;
	_RDMSR(msr, &lo, &hi);
	hi &= ~(1 << WB_ENH_WSB_DIS_D);
	_WRMSR(msr, lo, hi);
}


void mct_set_dram_config_hi_d(struct DCTStatStruc *p_dct_stat, u32 dct,
				u32 dram_config_hi)
{
	/* Bug#15114: Comp. update interrupted by Freq. change can cause
	 * subsequent update to be invalid during any MemClk frequency change:
	 * Solution: From the bug report:
	 *  1. A software-initiated frequency change should be wrapped into the
	 *     following sequence :
	 *	a) Disable Compensation (F2[1, 0]9C_x08[30])
	 *	b) Reset the Begin Compensation bit (D3CMP->COMP_CONFIG[0]) in
	 *	   all the compensation engines
	 *	c) Do frequency change
	 *	d) Enable Compensation (F2[1, 0]9C_x08[30])
	 *  2. A software-initiated Disable Compensation should always be
	 *     followed by step b) of the above steps.
	 * Silicon status: Fixed In Rev B0
	 *
	 * Errata#177: DRAM Phy Automatic Compensation Updates May Be Invalid
	 * Solution: BIOS should disable the phy automatic compensation prior
	 * to initiating a memory clock frequency change as follows:
	 *  1. Disable PhyAutoComp by writing 1'b1 to F2x[1, 0]9C_x08[30]
	 *  2. Reset the Begin Compensation bits by writing 32'h0 to
	 *     F2x[1, 0]9C_x4D004F00
	 *  3. Perform frequency change
	 *  4. Enable PhyAutoComp by writing 1'b0 to F2x[1, 0]9C_08[30]
	 *  In addition, any time software disables the automatic phy
	 *   compensation it should reset the begin compensation bit per step 2.
	 *   Silicon status: Fixed in DR-B0
	 */

	u32 dev = p_dct_stat->dev_dct;
	u32 index_reg = 0x98 + 0x100 * dct;
	u32 index;

	u32 val;

	index = 0x08;
	val = get_nb32_index_wait(dev, index_reg, index);
	set_nb32_index_wait(dev, index_reg, index, val | (1 << DIS_AUTO_COMP));

	//FIXME: check for Bx Cx CPU
	// if Ax mct_SetDramConfigHi_Samp_D

	/* errata#177 */
	index = 0x4D014F00;	/* F2x[1, 0]9C_x[D0FFFFF:D000000] DRAM Phy Debug Registers */
	index |= 1 << DCT_ACCESS_WRITE;
	val = 0;
	set_nb32_index_wait(dev, index_reg, index, val);

	set_nb32(dev, 0x94 + 0x100 * dct, dram_config_hi);

	index = 0x08;
	val = get_nb32_index_wait(dev, index_reg, index);
	set_nb32_index_wait(dev, index_reg, index, val & (~(1 << DIS_AUTO_COMP)));
}

static void mct_before_dqs_train_d(struct MCTStatStruc *p_mct_stat,
					struct DCTStatStruc *p_dct_stat_a)
{
	u8 node;
	struct DCTStatStruc *p_dct_stat;

	/* Errata 178
	 *
	 * Bug#15115: Uncertainty In The Sync Chain Leads To Setup Violations
	 *            In TX FIFO
	 * Solution: BIOS should program DRAM Control Register[RdPtrInit] =
	 *            5h, (F2x[1, 0]78[3:0] = 5h).
	 * Silicon status: Fixed In Rev B0
	 *
	 * Bug#15880: Determine validity of reset settings for DDR PHY timing.
	 * Solution: At least, set WrDqs fine delay to be 0 for DDR2 training.
	 */

	for (node = 0; node < 8; node++) {
		p_dct_stat = p_dct_stat_a + node;

		if (p_dct_stat->node_present) {
			mct_BeforeDQSTrain_Samp_D(p_mct_stat, p_dct_stat);
			mct_reset_dll_d(p_mct_stat, p_dct_stat, 0);
			mct_reset_dll_d(p_mct_stat, p_dct_stat, 1);
		}
	}
}

static void mct_reset_dll_d(struct MCTStatStruc *p_mct_stat,
					struct DCTStatStruc *p_dct_stat, u8 dct)
{
	u8 receiver;
	u32 dev = p_dct_stat->dev_dct;
	u32 reg_off = 0x100 * dct;
	u32 addr;
	u32 lo, hi;
	u8 wrap32dis = 0;
	u8 valid = 0;

	/* Skip reset DLL for B3 */
	if (p_dct_stat->logical_cpuid & AMD_DR_B3) {
		return;
	}

	addr = HWCR_MSR;
	_RDMSR(addr, &lo, &hi);
	if (lo & (1 << 17)) {		/* save the old value */
		wrap32dis = 1;
	}
	lo |= (1 << 17);			/* HWCR.wrap32dis */
	lo &= ~(1 << 15);			/* SSEDIS */
	/* Setting wrap32dis allows 64-bit memory references in 32bit mode */
	_WRMSR(addr, lo, hi);


	p_dct_stat->channel = dct;
	receiver = mct_InitReceiver_D(p_dct_stat, dct);
	/* there are four receiver pairs, loosely associated with chipselects.*/
	for (; receiver < 8; receiver += 2) {
		if (mct_RcvrRankEnabled_D(p_mct_stat, p_dct_stat, dct, receiver)) {
			addr = mct_GetRcvrSysAddr_D(p_mct_stat, p_dct_stat, dct, receiver, &valid);
			if (valid) {
				mct_Read1LTestPattern_D(p_mct_stat, p_dct_stat, addr);	/* cache fills */

				/* Write 0000_8000h to register F2x[1,0]9C_xD080F0C */
				set_nb32_index_wait(dev, 0x98 + reg_off, 0x4D080F0C, 0x00008000);
				mct_Wait(80); /* wait >= 300ns */

				/* Write 0000_0000h to register F2x[1,0]9C_xD080F0C */
				set_nb32_index_wait(dev, 0x98 + reg_off, 0x4D080F0C, 0x00000000);
				mct_Wait(800); /* wait >= 2us */
				break;
			}
		}
	}
	if (!wrap32dis) {
		addr = HWCR_MSR;
		_RDMSR(addr, &lo, &hi);
		lo &= ~(1 << 17);		/* restore HWCR.wrap32dis */
		_WRMSR(addr, lo, hi);
	}
}


void mct_enable_dat_intlv_d(struct MCTStatStruc *p_mct_stat,
					struct DCTStatStruc *p_dct_stat)
{
	u32 dev = p_dct_stat->dev_dct;
	u32 val;

	/*  Enable F2x110[DctDatIntlv] */
	// Call back not required mctHookBeforeDatIntlv_D()
	// FIXME Skip for Ax
	if (!p_dct_stat->ganged_mode) {
		val = get_nb32(dev, 0x110);
		val |= 1 << 5;			// DctDatIntlv
		set_nb32(dev, 0x110, val);

		// FIXME Skip for Cx
		dev = p_dct_stat->dev_nbmisc;
		val = get_nb32(dev, 0x8C);	// NB Configuration Hi
		val |= 1 << (36-32);		// DisDatMask
		set_nb32(dev, 0x8C, val);
	}
}

#ifdef UNUSED_CODE
static void mct_setup_sync_d(struct MCTStatStruc *p_mct_stat,
					struct DCTStatStruc *p_dct_stat)
{
	/* set F2x78[CH_SETUP_SYNC] when F2x[1, 0]9C_x04[AddrCmdSetup, CsOdtSetup,
	 * CkeSetup] setups for one DCT are all 0s and at least one of the setups,
	 * F2x[1, 0]9C_x04[AddrCmdSetup, CsOdtSetup, CkeSetup], of the other
	 * controller is 1
	 */
	u32 cha, chb;
	u32 dev = p_dct_stat->dev_dct;
	u32 val;

	cha = p_dct_stat->ch_addr_tmg[0] & 0x0202020;
	chb = p_dct_stat->ch_addr_tmg[1] & 0x0202020;

	if ((cha != chb) && ((cha == 0) || (chb == 0))) {
		val = get_nb32(dev, 0x78);
		val |= CH_SETUP_SYNC;
		set_nb32(dev, 0x78, val);
	}
}
#endif

static void after_dram_init_d(struct DCTStatStruc *p_dct_stat, u8 dct) {

	u32 val;
	u32 reg_off = 0x100 * dct;
	u32 dev = p_dct_stat->dev_dct;

	if (p_dct_stat->logical_cpuid & (AMD_DR_B2 | AMD_DR_B3)) {
		mct_Wait(10000);	/* Wait 50 us*/
		val = get_nb32(dev, 0x110);
		if (val & (1 << DRAM_ENABLED)) {
			/* If 50 us expires while DramEnable =0 then do the following */
			val = get_nb32(dev, 0x90 + reg_off);
			val &= ~(1 << WIDTH_128);		/* Program WIDTH_128 = 0 */
			set_nb32(dev, 0x90 + reg_off, val);

			val = get_nb32_index_wait(dev, 0x98 + reg_off, 0x05);	/* Perform dummy CSR read to F2x09C_x05 */

			if (p_dct_stat->ganged_mode) {
				val = get_nb32(dev, 0x90 + reg_off);
				val |= 1 << WIDTH_128;		/* Program WIDTH_128 = 0 */
				set_nb32(dev, 0x90 + reg_off, val);
			}
		}
	}
}


/* ==========================================================
 *  6-bit Bank Addressing Table
 *  RR = rows-13 binary
 *  B = banks-2 binary
 *  CCC = Columns-9 binary
 * ==========================================================
 *  DCT	CCCBRR	rows	banks	Columns	64-bit CS Size
 *  Encoding
 *  0000	000000	13	2	9	128MB
 *  0001	001000	13	2	10	256MB
 *  0010	001001	14	2	10	512MB
 *  0011	010000	13	2	11	512MB
 *  0100	001100	13	3	10	512MB
 *  0101	001101	14	3	10	1GB
 *  0110	010001	14	2	11	1GB
 *  0111	001110	15	3	10	2GB
 *  1000	010101	14	3	11	2GB
 *  1001	010110	15	3	11	4GB
 *  1010	001111	16	3	10	4GB
 *  1011	010111	16	3	11	8GB
 */
