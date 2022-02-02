/* SPDX-License-Identifier: GPL-2.0-only */

/* Call-backs */

#include <arch/cpu.h>
#include <cpu/amd/msr.h>
#include <console/console.h>
#include <option.h>
#include <types.h>

#include "mcti.h"

#define NVRAM_DDR2_800 0
#define NVRAM_DDR2_667 1
#define NVRAM_DDR2_533 2
#define NVRAM_DDR2_400 3

#define NVRAM_DDR3_1600 0
#define NVRAM_DDR3_1333 1
#define NVRAM_DDR3_1066 2
#define NVRAM_DDR3_800  3

static inline u8 is_fam_15h(void)
{
	u8 fam_15h = 0;
	u32 family;

	family = cpuid_eax(0x80000001);
	family = ((family & 0xf00000) >> 16) | ((family & 0xf00) >> 8);

	if (family >= 0x6f)
		/* Family 15h or later */
		fam_15h = 1;

	return fam_15h;
}

/* The recommended maximum GFX Upper Memory Area
 * size is 256M, however, to be on the safe side
 * move TOM down by 512M.
 */
#define MAXIMUM_GFXUMA_SIZE 0x20000000

/* Do not allow less than 16M of DRAM in 32-bit space.
 * This number is not hardware constrained and can be
 * changed as needed.
 */
#define MINIMUM_DRAM_BELOW_4G 0x1000000

u16 mct_get_nv_bits(u8 index)
{
	u16 val = 0;

	switch (index) {
	case NV_PACK_TYPE:
#if CONFIG_CPU_SOCKET_TYPE == 0x10	/* Socket F */
		val = 0;
#elif CONFIG_CPU_SOCKET_TYPE == 0x11	/* AM3 */
		val = 1;
#elif CONFIG_CPU_SOCKET_TYPE == 0x13	/* ASB2 */
		val = 4;
#elif CONFIG_CPU_SOCKET_TYPE == 0x14	/* C32 */
		val = 5;
#elif CONFIG_CPU_SOCKET_TYPE == 0x15	/* G34 */
		val = 3;
#elif CONFIG_CPU_SOCKET_TYPE == 0x16	/* FM2 */
		val = 6;
//#elif SYSTEM_TYPE == MOBILE
//		val = 2;
#endif
		break;
	case NV_MAX_NODES:
		val = MAX_NODES_SUPPORTED;
		break;
	case NV_MAX_DIMMS:
		val = MAX_DIMMS_SUPPORTED;
		//val = 8;
		break;
	case NV_MAX_DIMMS_PER_CH:
		/* FIXME
		 * Mainboards need to be able to specify the maximum number of DIMMs installable per channel
		 * For now assume a maximum of 2 DIMMs per channel can be installed
		 */
		val = 2;
		break;
	case NV_MAX_MEMCLK:
		/* Maximum platform supported memclk */
		val =  MEM_MAX_LOAD_FREQ;
		u8 mem_clk = get_uint_option("max_mem_clock", 0);
		int limit = val;

		if (CONFIG(DIMM_DDR3))
			limit = ddr3_limits[mem_clk & 0xf];
		else if (CONFIG(DIMM_DDR2))
			limit = ddr2_limits[mem_clk & 0x3];
		val = MIN(limit, val);

		break;
	case NV_MIN_MEMCLK:
		/* Minimum platform supported memclk */
		if (is_fam15h())
			val =  MEM_MIN_PLATFORM_FREQ_FAM15;
		else
			val =  MEM_MIN_PLATFORM_FREQ_FAM10;
		break;
	case NV_ECC_CAP:
#if SYSTEM_TYPE == SERVER
		val = 1;	/* memory bus ECC capable */
#else
		val = 0;	/* memory bus ECC not capable */
#endif
		break;
	case NV_4_RANK_TYPE:
		/* Quad Rank DIMM slot type */
		val = 0;	/* normal */
		//val = 1;	/* R4 (registered DIMMs in AMD server configuration) */
		//val = 2;	/* S4 (Unbuffered SO-DIMMS) */
		break;
	case NV_BYP_MAX:
#if !CONFIG(GFXUMA)
		val = 4;
#elif CONFIG(GFXUMA)
		val = 7;
#endif
		break;
	case NV_RD_WR_QBYP:
#if !CONFIG(GFXUMA)
		val = 2;
#elif CONFIG(GFXUMA)
		val = 3;
#endif
		break;
	case NV_MCT_USR_TMG_MODE:
		val = 0;	/* Automatic (recommended) */
		//val = 1;	/* Limited */
		//val = 2;	/* Manual */
		break;
	case NV_MEM_CK_VAL:
		//val = 0;	/* 200MHz */
		//val = 1;	/* 266MHz */
		val = 2;	/* 333MHz */
		break;
	case NV_BANK_INTLV:
		/* Bank (chip select) interleaving */
		val =  get_uint_option("interleave_chip_selects", 1);
		break;
	case NV_MEM_HOLE:
		//val = 0;	/* Disabled */
		val = 1;	/* Enabled (recommended) */
		break;
	case NV_ALL_MEM_CLKS:
		val = 0;	/* Normal (only to slots that have enabled DIMMs) */
		//val = 1;	/* Enable all memclocks */
		break;
	case NV_SPDCHK_RESTRT:
		//val = 0;	/* Exit current node initialization if any DIMM has SPD checksum error */
		//val = 1;	/* Ignore faulty SPD checksum (DIMM will still be disabled), continue current node initialization */
		//val = 2;	/* Override faulty SPD checksum (DIMM will be enabled), continue current node initialization */

		val = get_uint_option("dimm_spd_checksum", 0) & 0x3;

		if (val > 2)
			val = 2;

		break;
	case NV_DQS_TRAIN_CTL:
		//val = 0;	/*Skip dqs training */
		val = 1;	/* Perform dqs training */
		break;
	case NV_NODE_INTLV:
		val = get_uint_option("interleave_nodes", 0);
		break;
	case NV_BURST_LEN_32:
#if !CONFIG(GFXUMA)
		val = 0;	/* 64 byte mode */
#elif CONFIG(GFXUMA)
		val = 1;	/* 32 byte mode */
#endif
		break;
	case NV_CKE_PDEN:
		//val = 0;	/* Disable */
		val = 1;	/* Enable */
		break;
	case NV_CKE_CTL:
		val = 0;	/* per channel control */
		//val = 1;	/* per chip select control */
		break;
	case NV_CLK_HZ_ALT_VID_C3:
		val = 0;	/* disable */
		//val = 1;	/* enable */
		break;
	case NV_BOTTOM_IO:
	case NV_BOTTOM_UMA:
		/* address bits [31:24] */
#if !CONFIG(GFXUMA)
		val = (CONFIG_MMCONF_BASE_ADDRESS >> 24);
#elif CONFIG(GFXUMA)
	#if (CONFIG_MMCONF_BASE_ADDRESS < (MAXIMUM_GFXUMA_SIZE + MINIMUM_DRAM_BELOW_4G))
	#error "MMCONF_BASE_ADDRESS is too small"
	#endif
		val = ((CONFIG_MMCONF_BASE_ADDRESS - MAXIMUM_GFXUMA_SIZE) >> 24);
#endif
		break;
	case NV_ECC:
		if (CONFIG(SYSTEM_TYPE_SERVER))
			val = 1;	/* Enable */
		else
			val = 0;	/* Disable */

		val = get_uint_option("ECC_memory", val);
		break;
	case NV_NBECC:
		if (CONFIG(SYSTEM_TYPE_SERVER))
			val = 1;	/* Enable */
		else
			val = 0;	/* Disable */

		break;
	case NV_CHIP_KILL:
		if (CONFIG(SYSTEM_TYPE_SERVER))
			val = 1;	/* Enable */
		else
			val = 0;	/* Disable */

		break;
	case NV_ECC_REDIR:
		/*
		 * 0: Disable
		 * 1: Enable
		 */
		val = get_uint_option("ECC_redirection", 0);
		break;
	case NV_DRAM_BK_SCRUB:
		/*
		 * 0x00: Disabled
		 * 0x01: 40ns
		 * 0x02: 80ns
		 * 0x03: 160ns
		 * 0x04: 320ns
		 * 0x05: 640ns
		 * 0x06: 1.28us
		 * 0x07: 2.56us
		 * 0x08: 5.12us
		 * 0x09: 10.2us
		 * 0x0a: 20.5us
		 * 0x0b: 41us
		 * 0x0c: 81.9us
		 * 0x0d: 163.8us
		 * 0x0e: 327.7us
		 * 0x0f: 655.4us
		 * 0x10: 1.31ms
		 * 0x11: 2.62ms
		 * 0x12: 5.24ms
		 * 0x13: 10.49ms
		 * 0x14: 20.97sms
		 * 0x15: 42ms
		 * 0x16: 84ms
		 */
		val = get_uint_option("ecc_scrub_rate", 6);
		break;
	case NV_L2_BK_SCRUB:
		val = 0;	/* Disabled - See L2Scrub in BKDG */
		break;
	case NV_L3_BK_SCRUB:
		val = 0;	/* Disabled - See L3Scrub in BKDG */
		break;
	case NV_DC_BK_SCRUB:
		val = 0;	/* Disabled - See DcacheScrub in BKDG */
		break;
	case NV_CS_SPARE_CTL:
		val = 0;	/* Disabled */
		//val = 1;	/* Enabled */
		break;
	case NV_SYNC_ON_UN_ECC_EN:
		val = 0;	/* Disabled */
		//val = 1;	/* Enabled */
		break;
	case NV_UNGANGED:
		/* channel interleave is better performance than ganged mode at this time */
		val = get_uint_option("interleave_memory_channels", 1);
		break;
	case NV_CHANNEL_INTLV:
		val = 5;	/* Not currently checked in mctchi_d.c */
	/* Bit 0 =     0 - Disable
	 *             1 - Enable
	 * Bits[2:1] = 00b - Address bits 6
	 *             01b - Address bits 1
	 *             10b - Hash*, XOR of address bits [20:16, 6]
	 *             11b - Hash*, XOR of address bits [20:16, 9]
	 */
		break;
	}

	return val;
}


void mct_hook_after_dimm_pre(void)
{
}


void mct_get_max_load_freq(struct DCTStatStruc *p_dct_stat)
{
	p_dct_stat->preset_max_freq = mct_get_nv_bits(NV_MAX_MEMCLK);

	/* Determine the number of installed DIMMs */
	int ch1_count = 0;
	int ch2_count = 0;
	u8 ch1_registered = 0;
	u8 ch2_registered = 0;
	u8 ch1_voltage = 0;
	u8 ch2_voltage = 0;
	u8 highest_rank_count[2];
	u8 dimm;
	int i;
	for (i = 0; i < 15; i = i + 2) {
		if (p_dct_stat->dimm_valid & (1 << i))
			ch1_count++;
		if (p_dct_stat->dimm_valid & (1 << (i + 1)))
			ch2_count++;
	}
	for (i = 0; i < MAX_DIMMS_SUPPORTED; i = i + 2) {
		if (p_dct_stat->dimm_registered[i])
			ch1_registered = 1;
		if (p_dct_stat->dimm_registered[i + 1])
			ch2_registered = 1;
	}
	if (CONFIG(DEBUG_RAM_SETUP)) {
		printk(BIOS_DEBUG, "mct_get_max_load_freq: Channel 1: %d DIMM(s) detected\n", ch1_count);
		printk(BIOS_DEBUG, "mct_get_max_load_freq: Channel 2: %d DIMM(s) detected\n", ch2_count);
	}

#if CONFIG(DIMM_DDR3)
	for (i = 0; i < MAX_DIMMS_SUPPORTED; i = i + 2) {
		if (p_dct_stat->dimm_valid & (1 << i))
			ch1_voltage |= p_dct_stat->dimm_configured_voltage[i];
		if (p_dct_stat->dimm_valid & (1 << (i + 1)))
			ch2_voltage |= p_dct_stat->dimm_configured_voltage[i + 1];
	}
#endif

	for (i = 0; i < 2; i++) {
		highest_rank_count[i] = 0x0;
		for (dimm = 0; dimm < MAX_DIMMS_SUPPORTED; dimm++) {
			if (p_dct_stat->dimm_ranks[dimm] > highest_rank_count[i])
				highest_rank_count[i] = p_dct_stat->dimm_ranks[dimm];
		}
	}

	/* Set limits if needed */
	p_dct_stat->preset_max_freq = mct_MaxLoadFreq(MAX(ch1_count, ch2_count),
					MAX(highest_rank_count[0], highest_rank_count[1]),
					(ch1_registered || ch2_registered),
					(ch1_voltage | ch2_voltage), p_dct_stat->preset_max_freq);
}

void mct_get_dimm_addr(struct DCTStatStruc *p_dct_stat, u32 node)
{
	int j;
	struct sys_info *sysinfo = get_sysinfo();
	struct mem_controller *ctrl = &(sysinfo->ctrl[node]);

	for (j = 0; j < DIMM_SOCKETS; j++) {
		p_dct_stat->dimm_addr[j * 2] = ctrl->spd_addr[j] & 0xff;
		p_dct_stat->dimm_addr[j * 2 + 1] = ctrl->spd_addr[DIMM_SOCKETS + j] & 0xff;
	}

}

void mct_adjust_auto_cyc_tmg_d(void)
{
}


void mct_hook_after_auto_cyc_tmg(void)
{
}


void mct_get_cs_exclude_map(void)
{
}


void mct_hook_after_auto_cfg(void)
{
}


void mct_hook_after_ps_cfg(void)
{
}


void mct_hook_after_ht_map(void)
{
}


void mct_hook_after_cpu(void)
{
}


#if CONFIG(DIMM_DDR2)
void mct_save_dqs_sig_tmg_d(void)
{
}

void mct_get_dqs_sig_tmg_d(void)
{
}
#endif

void mct_hook_before_ecc(void)
{
}

void mct_hook_after_ecc(void)
{
}

#ifdef UNUSED_CODE
void mct_init_mem_gpios_a(void)
{
}
#endif


void mct_init_mem_gpios_a_d(void)
{
}


void mct_node_id_debug_port_d(void)
{
}


void mct_warm_reset_d(void)
{
}


void mct_hook_before_dram_init(void)
{
}


void mct_hook_after_dram_init(void)
{
}

#if CONFIG(DIMM_DDR3)
void v_erratum_372(struct DCTStatStruc *p_dct_stat)
{
	msr_t msr = rdmsr(NB_CFG_MSR);

	int nbPstate1supported = !(msr.hi & (1 << (NB_GFX_NB_PSTATE_DIS -32)));

	// is this the right way to check for NB pstate 1 or DDR3-1333 ?
	if (((p_dct_stat->preset_max_freq == 1333) || (nbPstate1supported))
	    && (!p_dct_stat->ganged_mode)) {
		/* DisableCf8ExtCfg */
		msr.hi &= ~(3 << (51 - 32));
		wrmsr(NB_CFG_MSR, msr);
	}
}

void v_erratum_414(struct DCTStatStruc *p_dct_stat)
{
	int dct = 0;
	for (; dct < 2 ; dct++) {
		int dRAMConfigHi = get_nb32(p_dct_stat->dev_dct,0x94 + (0x100 * dct));
		int powerDown =  dRAMConfigHi & (1 << POWER_DOWN_EN);
		int ddr3 = dRAMConfigHi & (1 << DDR3_MODE);
		int dRAMMRS = get_nb32(p_dct_stat->dev_dct,0x84 + (0x100 * dct));
		int pchgPDModeSel = dRAMMRS & (1 << PCHG_PD_MODE_SEL);
		if (powerDown && ddr3 && pchgPDModeSel)
			set_nb32(p_dct_stat->dev_dct,0x84 + (0x100 * dct), dRAMMRS & ~(1 << PCHG_PD_MODE_SEL));
	}
}
#endif


void mct_hook_before_any_training(struct MCTStatStruc *p_mct_stat, struct DCTStatStruc *p_dct_stat_a)
{
#if CONFIG(DIMM_DDR3)
	/* FIXME :  as of 25.6.2010 errata 350 and 372 should apply to  ((RB|BL|DA)-C[23])|(HY-D[01])|(PH-E0) but I don't find constants for all of them */
	if (p_dct_stat_a->logical_cpuid & (AMD_DRBH_Cx | AMD_DR_Dx)) {
		v_erratum_372(p_dct_stat_a);
		v_erratum_414(p_dct_stat_a);
	}
#endif
}

#if CONFIG(DIMM_DDR3)
u32 mct_adjust_spd_timings(struct MCTStatStruc *p_mct_stat, struct DCTStatStruc *p_dct_stat_a, u32 val)
{
	if (p_dct_stat_a->logical_cpuid & AMD_DR_Bx) {
		if (p_dct_stat_a->status & (1 << SB_REGISTERED)) {
			val ++;
		}
	}
	return val;
}
#endif

void mct_hook_after_any_training(void)
{
}

#if CONFIG(DIMM_DDR2)
u8 mct_set_node_boundary_d(void)
{
	return 0;
}
#endif
