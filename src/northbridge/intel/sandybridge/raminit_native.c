/* SPDX-License-Identifier: GPL-2.0-only */

#include <commonlib/clamp.h>
#include <console/console.h>
#include <console/usb.h>
#include <delay.h>
#include <device/device.h>
#include <device/pci_def.h>
#include <device/pci_ops.h>
#include <northbridge/intel/sandybridge/chip.h>
#include <stdbool.h>
#include <stdint.h>

#include "raminit_native.h"
#include "raminit_common.h"
#include "raminit_tables.h"

#define SNB_MIN_DCLK_133_MULT	3
#define SNB_MAX_DCLK_133_MULT	8
#define IVB_MIN_DCLK_133_MULT	3
#define IVB_MAX_DCLK_133_MULT	10
#define IVB_MIN_DCLK_100_MULT	7
#define IVB_MAX_DCLK_100_MULT	12

/* Frequency multiplier */
static u32 get_FRQ(const ramctr_timing *ctrl)
{
	const u32 FRQ = 256000 / (ctrl->tCK * ctrl->base_freq);

	if (IS_IVY_CPU(ctrl->cpu)) {
		if (ctrl->base_freq == 100)
			return clamp_u32(IVB_MIN_DCLK_100_MULT, FRQ, IVB_MAX_DCLK_100_MULT);

		if (ctrl->base_freq == 133)
			return clamp_u32(IVB_MIN_DCLK_133_MULT, FRQ, IVB_MAX_DCLK_133_MULT);

	} else if (IS_SANDY_CPU(ctrl->cpu)) {
		if (ctrl->base_freq == 133)
			return clamp_u32(SNB_MIN_DCLK_133_MULT, FRQ, SNB_MAX_DCLK_133_MULT);
	}

	die("Unsupported CPU or base frequency.");
}

/* Get REFI based on frequency index, tREFI = 7.8usec */
static u32 get_REFI(u32 FRQ, u8 base_freq)
{
	if (base_freq == 100)
		return frq_refi_map[1][FRQ - 7];

	else
		return frq_refi_map[0][FRQ - 3];
}

/* Get XSOffset based on frequency index, tXS-Offset: tXS = tRFC + 10ns */
static u8 get_XSOffset(u32 FRQ, u8 base_freq)
{
	if (base_freq == 100)
		return frq_xs_map[1][FRQ - 7];

	else
		return frq_xs_map[0][FRQ - 3];
}

/* Get MOD based on frequency index */
static u8 get_MOD(u32 FRQ, u8 base_freq)
{
	if (base_freq == 100)
		return frq_mod_map[1][FRQ - 7];

	else
		return frq_mod_map[0][FRQ - 3];
}

/* Get Write Leveling Output delay based on frequency index */
static u8 get_WLO(u32 FRQ, u8 base_freq)
{
	if (base_freq == 100)
		return frq_wlo_map[1][FRQ - 7];

	else
		return frq_wlo_map[0][FRQ - 3];
}

/* Get CKE based on frequency index */
static u8 get_CKE(u32 FRQ, u8 base_freq)
{
	if (base_freq == 100)
		return frq_cke_map[1][FRQ - 7];

	else
		return frq_cke_map[0][FRQ - 3];
}

/* Get XPDLL based on frequency index */
static u8 get_XPDLL(u32 FRQ, u8 base_freq)
{
	if (base_freq == 100)
		return frq_xpdll_map[1][FRQ - 7];

	else
		return frq_xpdll_map[0][FRQ - 3];
}

/* Get XP based on frequency index */
static u8 get_XP(u32 FRQ, u8 base_freq)
{
	if (base_freq == 100)
		return frq_xp_map[1][FRQ - 7];

	else
		return frq_xp_map[0][FRQ - 3];
}

/* Get AONPD based on frequency index */
static u8 get_AONPD(u32 FRQ, u8 base_freq)
{
	if (base_freq == 100)
		return frq_aonpd_map[1][FRQ - 7];

	else
		return frq_aonpd_map[0][FRQ - 3];
}

/* Get COMP2 based on frequency index */
static u32 get_COMP2(u32 FRQ, u8 base_freq)
{
	if (base_freq == 100)
		return frq_comp2_map[1][FRQ - 7];

	else
		return frq_comp2_map[0][FRQ - 3];
}

static void normalize_tclk(ramctr_timing *ctrl, bool ref_100mhz_support)
{
	if (ctrl->tCK <= TCK_1200MHZ) {
		ctrl->tCK = TCK_1200MHZ;
		ctrl->base_freq = 100;
	} else if (ctrl->tCK <= TCK_1100MHZ) {
		ctrl->tCK = TCK_1100MHZ;
		ctrl->base_freq = 100;
	} else if (ctrl->tCK <= TCK_1066MHZ) {
		ctrl->tCK = TCK_1066MHZ;
		ctrl->base_freq = 133;
	} else if (ctrl->tCK <= TCK_1000MHZ) {
		ctrl->tCK = TCK_1000MHZ;
		ctrl->base_freq = 100;
	} else if (ctrl->tCK <= TCK_933MHZ) {
		ctrl->tCK = TCK_933MHZ;
		ctrl->base_freq = 133;
	} else if (ctrl->tCK <= TCK_900MHZ) {
		ctrl->tCK = TCK_900MHZ;
		ctrl->base_freq = 100;
	} else if (ctrl->tCK <= TCK_800MHZ) {
		ctrl->tCK = TCK_800MHZ;
		ctrl->base_freq = 133;
	} else if (ctrl->tCK <= TCK_700MHZ) {
		ctrl->tCK = TCK_700MHZ;
		ctrl->base_freq = 100;
	} else if (ctrl->tCK <= TCK_666MHZ) {
		ctrl->tCK = TCK_666MHZ;
		ctrl->base_freq = 133;
	} else if (ctrl->tCK <= TCK_533MHZ) {
		ctrl->tCK = TCK_533MHZ;
		ctrl->base_freq = 133;
	} else if (ctrl->tCK <= TCK_400MHZ) {
		ctrl->tCK = TCK_400MHZ;
		ctrl->base_freq = 133;
	} else {
		ctrl->tCK = 0;
		return;
	}

	if (!ref_100mhz_support && ctrl->base_freq == 100) {
		/* Skip unsupported frequency */
		ctrl->tCK++;
		normalize_tclk(ctrl, ref_100mhz_support);
	}
}

#define DEFAULT_TCK	TCK_800MHZ

static unsigned int get_mem_min_tck(void)
{
	u32 reg32;
	u8 rev;
	const struct northbridge_intel_sandybridge_config *cfg = NULL;

	/* Actually, config of MCH or Host Bridge */
	cfg = config_of_soc();

	/* If non-zero, it was set in the devicetree */
	if (cfg->max_mem_clock_mhz) {

		if (cfg->max_mem_clock_mhz >= 1066)
			return TCK_1066MHZ;

		else if (cfg->max_mem_clock_mhz >= 933)
			return TCK_933MHZ;

		else if (cfg->max_mem_clock_mhz >= 800)
			return TCK_800MHZ;

		else if (cfg->max_mem_clock_mhz >= 666)
			return TCK_666MHZ;

		else if (cfg->max_mem_clock_mhz >= 533)
			return TCK_533MHZ;

		else
			return TCK_400MHZ;
	}

	if (CONFIG(NATIVE_RAMINIT_IGNORE_MAX_MEM_FUSES))
		return TCK_1333MHZ;

	rev = pci_read_config8(HOST_BRIDGE, PCI_DEVICE_ID);

	if ((rev & BASE_REV_MASK) == BASE_REV_SNB) {
		/* Read Capabilities A Register DMFC bits */
		reg32 = pci_read_config32(HOST_BRIDGE, CAPID0_A);
		reg32 &= 0x7;

		switch (reg32) {
		case 7: return TCK_533MHZ;
		case 6: return TCK_666MHZ;
		case 5: return TCK_800MHZ;
		/* Reserved */
		default:
			break;
		}
	} else {
		/* Read Capabilities B Register DMFC bits */
		reg32 = pci_read_config32(HOST_BRIDGE, CAPID0_B);
		reg32 = (reg32 >> 4) & 0x7;

		switch (reg32) {
		case 7: return TCK_533MHZ;
		case 6: return TCK_666MHZ;
		case 5: return TCK_800MHZ;
		case 4: return TCK_933MHZ;
		case 3: return TCK_1066MHZ;
		case 2: return TCK_1200MHZ;
		case 1: return TCK_1333MHZ;
		/* Reserved */
		default:
			break;
		}
	}
	return DEFAULT_TCK;
}

static void find_cas_tck(ramctr_timing *ctrl)
{
	u8 val;
	u32 reg32;
	u8 ref_100mhz_support;

	/* 100 MHz reference clock supported */
	reg32 = pci_read_config32(HOST_BRIDGE, CAPID0_B);
	ref_100mhz_support = (reg32 >> 21) & 0x7;
	printk(BIOS_DEBUG, "100MHz reference clock support: %s\n", ref_100mhz_support ? "yes"
										      : "no");

	printk(BIOS_DEBUG, "PLL_REF100_CFG value: 0x%x\n", ref_100mhz_support);

	ctrl->tCK = get_mem_min_tck();

	/* Find CAS latency */
	while (1) {
		/*
		 * Normalising tCK before computing clock could potentially
		 * result in a lower selected CAS, which is desired.
		 */
		normalize_tclk(ctrl, ref_100mhz_support);
		if (!(ctrl->tCK))
			die("Couldn't find compatible clock / CAS settings\n");

		val = DIV_ROUND_UP(ctrl->tAA, ctrl->tCK);
		printk(BIOS_DEBUG, "Trying CAS %u, tCK %u.\n", val, ctrl->tCK);
		for (; val <= MAX_CAS; val++)
			if ((ctrl->cas_supported >> (val - MIN_CAS)) & 1)
				break;

		if (val == (MAX_CAS + 1)) {
			ctrl->tCK++;
			continue;
		} else {
			printk(BIOS_DEBUG, "Found compatible clock, CAS pair.\n");
			break;
		}
	}

	/* Frequency multiplier */
	ctrl->FRQ = get_FRQ(ctrl);

	printk(BIOS_DEBUG, "Selected DRAM frequency: %u MHz\n", NS2MHZ_DIV256 / ctrl->tCK);
	printk(BIOS_DEBUG, "Selected CAS latency   : %uT\n", val);
	ctrl->CAS = val;
}

static void dram_timing(ramctr_timing *ctrl)
{
	/*
	 * On Sandy Bridge, the maximum supported DDR3 frequency is 1066MHz (DDR3 2133).
	 * Cap it for faster DIMMs, and align it to the closest JEDEC standard frequency.
	 */
	/*
	 * On Ivy Bridge, the maximum supported DDR3 frequency is 1400MHz (DDR3 2800).
	 * Cap it at 1200MHz (DDR3 2400), and align it to the closest JEDEC standard frequency.
	 */
	if (ctrl->tCK == TCK_1200MHZ) {
		ctrl->edge_offset[0] = 18; //XXX: guessed
		ctrl->edge_offset[1] = 8;
		ctrl->edge_offset[2] = 8;
		ctrl->timC_offset[0] = 20; //XXX: guessed
		ctrl->timC_offset[1] = 8;
		ctrl->timC_offset[2] = 8;
		ctrl->pi_coding_threshold = 10;

	} else if (ctrl->tCK == TCK_1100MHZ) {
		ctrl->edge_offset[0] = 17; //XXX: guessed
		ctrl->edge_offset[1] = 7;
		ctrl->edge_offset[2] = 7;
		ctrl->timC_offset[0] = 19; //XXX: guessed
		ctrl->timC_offset[1] = 7;
		ctrl->timC_offset[2] = 7;
		ctrl->pi_coding_threshold = 13;

	} else if (ctrl->tCK == TCK_1066MHZ) {
		ctrl->edge_offset[0] = 16;
		ctrl->edge_offset[1] = 7;
		ctrl->edge_offset[2] = 7;
		ctrl->timC_offset[0] = 18;
		ctrl->timC_offset[1] = 7;
		ctrl->timC_offset[2] = 7;
		ctrl->pi_coding_threshold = 13;

	} else if (ctrl->tCK == TCK_1000MHZ) {
		ctrl->edge_offset[0] = 15; //XXX: guessed
		ctrl->edge_offset[1] = 6;
		ctrl->edge_offset[2] = 6;
		ctrl->timC_offset[0] = 17; //XXX: guessed
		ctrl->timC_offset[1] = 6;
		ctrl->timC_offset[2] = 6;
		ctrl->pi_coding_threshold = 13;

	} else if (ctrl->tCK == TCK_933MHZ) {
		ctrl->edge_offset[0] = 14;
		ctrl->edge_offset[1] = 6;
		ctrl->edge_offset[2] = 6;
		ctrl->timC_offset[0] = 15;
		ctrl->timC_offset[1] = 6;
		ctrl->timC_offset[2] = 6;
		ctrl->pi_coding_threshold = 15;

	} else if (ctrl->tCK == TCK_900MHZ) {
		ctrl->edge_offset[0] = 14; //XXX: guessed
		ctrl->edge_offset[1] = 6;
		ctrl->edge_offset[2] = 6;
		ctrl->timC_offset[0] = 15; //XXX: guessed
		ctrl->timC_offset[1] = 6;
		ctrl->timC_offset[2] = 6;
		ctrl->pi_coding_threshold = 12;

	} else if (ctrl->tCK == TCK_800MHZ) {
		ctrl->edge_offset[0] = 13;
		ctrl->edge_offset[1] = 5;
		ctrl->edge_offset[2] = 5;
		ctrl->timC_offset[0] = 14;
		ctrl->timC_offset[1] = 5;
		ctrl->timC_offset[2] = 5;
		ctrl->pi_coding_threshold = 15;

	} else if (ctrl->tCK == TCK_700MHZ) {
		ctrl->edge_offset[0] = 13; //XXX: guessed
		ctrl->edge_offset[1] = 5;
		ctrl->edge_offset[2] = 5;
		ctrl->timC_offset[0] = 14; //XXX: guessed
		ctrl->timC_offset[1] = 5;
		ctrl->timC_offset[2] = 5;
		ctrl->pi_coding_threshold = 16;

	} else if (ctrl->tCK == TCK_666MHZ) {
		ctrl->edge_offset[0] = 10;
		ctrl->edge_offset[1] = 4;
		ctrl->edge_offset[2] = 4;
		ctrl->timC_offset[0] = 11;
		ctrl->timC_offset[1] = 4;
		ctrl->timC_offset[2] = 4;
		ctrl->pi_coding_threshold = 16;

	} else if (ctrl->tCK == TCK_533MHZ) {
		ctrl->edge_offset[0] = 8;
		ctrl->edge_offset[1] = 3;
		ctrl->edge_offset[2] = 3;
		ctrl->timC_offset[0] = 9;
		ctrl->timC_offset[1] = 3;
		ctrl->timC_offset[2] = 3;
		ctrl->pi_coding_threshold = 17;

	} else  { /* TCK_400MHZ */
		ctrl->edge_offset[0] = 6;
		ctrl->edge_offset[1] = 2;
		ctrl->edge_offset[2] = 2;
		ctrl->timC_offset[0] = 6;
		ctrl->timC_offset[1] = 2;
		ctrl->timC_offset[2] = 2;
		ctrl->pi_coding_threshold = 17;
	}

	/* Initial phase between CLK/CMD pins */
	ctrl->pi_code_offset = (256000 / ctrl->tCK) / 66;

	/* DLL_CONFIG_MDLL_W_TIMER */
	ctrl->mdll_wake_delay = (128000 / ctrl->tCK) + 3;

	if (ctrl->tCWL)
		ctrl->CWL = DIV_ROUND_UP(ctrl->tCWL, ctrl->tCK);
	else
		ctrl->CWL = get_CWL(ctrl->tCK);

	printk(BIOS_DEBUG, "Selected CWL latency   : %uT\n", ctrl->CWL);

	/* Find tRCD */
	ctrl->tRCD = DIV_ROUND_UP(ctrl->tRCD, ctrl->tCK);
	printk(BIOS_DEBUG, "Selected tRCD          : %uT\n", ctrl->tRCD);

	ctrl->tRP  = DIV_ROUND_UP(ctrl->tRP,  ctrl->tCK);
	printk(BIOS_DEBUG, "Selected tRP           : %uT\n", ctrl->tRP);

	/* Find tRAS */
	ctrl->tRAS = DIV_ROUND_UP(ctrl->tRAS, ctrl->tCK);
	printk(BIOS_DEBUG, "Selected tRAS          : %uT\n", ctrl->tRAS);

	/* Find tWR */
	ctrl->tWR  = DIV_ROUND_UP(ctrl->tWR,  ctrl->tCK);
	printk(BIOS_DEBUG, "Selected tWR           : %uT\n", ctrl->tWR);

	/* Find tFAW */
	ctrl->tFAW = DIV_ROUND_UP(ctrl->tFAW, ctrl->tCK);
	printk(BIOS_DEBUG, "Selected tFAW          : %uT\n", ctrl->tFAW);

	/* Find tRRD */
	ctrl->tRRD = DIV_ROUND_UP(ctrl->tRRD, ctrl->tCK);
	printk(BIOS_DEBUG, "Selected tRRD          : %uT\n", ctrl->tRRD);

	/* Find tRTP */
	ctrl->tRTP = DIV_ROUND_UP(ctrl->tRTP, ctrl->tCK);
	printk(BIOS_DEBUG, "Selected tRTP          : %uT\n", ctrl->tRTP);

	/* Find tWTR */
	ctrl->tWTR = DIV_ROUND_UP(ctrl->tWTR, ctrl->tCK);
	printk(BIOS_DEBUG, "Selected tWTR          : %uT\n", ctrl->tWTR);

	/* Refresh-to-Active or Refresh-to-Refresh (tRFC) */
	ctrl->tRFC = DIV_ROUND_UP(ctrl->tRFC, ctrl->tCK);
	printk(BIOS_DEBUG, "Selected tRFC          : %uT\n", ctrl->tRFC);

	ctrl->tREFI     =     get_REFI(ctrl->FRQ, ctrl->base_freq);
	ctrl->tMOD      =      get_MOD(ctrl->FRQ, ctrl->base_freq);
	ctrl->tXSOffset = get_XSOffset(ctrl->FRQ, ctrl->base_freq);
	ctrl->tWLO      =      get_WLO(ctrl->FRQ, ctrl->base_freq);
	ctrl->tCKE      =      get_CKE(ctrl->FRQ, ctrl->base_freq);
	ctrl->tXPDLL    =    get_XPDLL(ctrl->FRQ, ctrl->base_freq);
	ctrl->tXP       =       get_XP(ctrl->FRQ, ctrl->base_freq);
	ctrl->tAONPD    =    get_AONPD(ctrl->FRQ, ctrl->base_freq);
}

static void dram_freq(ramctr_timing *ctrl)
{
	if (ctrl->tCK > TCK_400MHZ) {
		printk(BIOS_ERR,
			"DRAM frequency is under lowest supported frequency (400 MHz). "
			"Increasing to 400 MHz as last resort");
		ctrl->tCK = TCK_400MHZ;
	}

	while (1) {
		u8 val2;
		u32 reg1 = 0;

		/* Step 1 - Set target PCU frequency */
		find_cas_tck(ctrl);

		/*
		 * The PLL will never lock if the required frequency is already set.
		 * Exit early to prevent a system hang.
		 */
		reg1 = MCHBAR32(MC_BIOS_DATA);
		val2 = (u8) reg1;
		if (val2)
			return;

		/* Step 2 - Select frequency in the MCU */
		reg1 = ctrl->FRQ;
		if (ctrl->base_freq == 100)
			reg1 |= 0x100;	/* Enable 100Mhz REF clock */

		reg1 |= 0x80000000;	/* set running bit */
		MCHBAR32(MC_BIOS_REQ) = reg1;
		int i = 0;
		printk(BIOS_DEBUG, "PLL busy... ");
		while (reg1 & 0x80000000) {
			udelay(10);
			i++;
			reg1 = MCHBAR32(MC_BIOS_REQ);
		}
		printk(BIOS_DEBUG, "done in %d us\n", i * 10);

		/* Step 3 - Verify lock frequency */
		reg1 = MCHBAR32(MC_BIOS_DATA);
		val2 = (u8) reg1;
		if (val2 >= ctrl->FRQ) {
			printk(BIOS_DEBUG, "MCU frequency is set at : %d MHz\n",
			       (1000 << 8) / ctrl->tCK);
			return;
		}
		printk(BIOS_DEBUG, "PLL didn't lock. Retrying at lower frequency\n");
		ctrl->tCK++;
	}
}

static void dram_ioregs(ramctr_timing *ctrl)
{
	u32 reg;

	int channel;

	/* IO clock */
	FOR_ALL_CHANNELS {
		MCHBAR32(GDCRCLKRANKSUSED_ch(channel)) = ctrl->rankmap[channel];
	}

	/* IO command */
	FOR_ALL_CHANNELS {
		MCHBAR32(GDCRCTLRANKSUSED_ch(channel)) = ctrl->rankmap[channel];
	}

	/* IO control */
	FOR_ALL_POPULATED_CHANNELS {
		program_timings(ctrl, channel);
	}

	/* Perform RCOMP */
	printram("RCOMP...");
	while (!(MCHBAR32(RCOMP_TIMER) & (1 << 16)))
		;

	printram("done\n");

	/* Set COMP2 */
	MCHBAR32(CRCOMPOFST2) = get_COMP2(ctrl->FRQ, ctrl->base_freq);
	printram("COMP2 done\n");

	/* Set COMP1 */
	FOR_ALL_POPULATED_CHANNELS {
		reg = MCHBAR32(CRCOMPOFST1_ch(channel));
		reg = (reg & ~0x00000e00) | (1 <<  9);	/* ODT */
		reg = (reg & ~0x00e00000) | (1 << 21);	/* clk drive up */
		reg = (reg & ~0x38000000) | (1 << 27);	/* ctl drive up */
		MCHBAR32(CRCOMPOFST1_ch(channel)) = reg;
	}
	printram("COMP1 done\n");

	printram("FORCE RCOMP and wait 20us...");
	MCHBAR32(M_COMP) |= (1 << 8);
	udelay(20);
	printram("done\n");
}

int try_init_dram_ddr3(ramctr_timing *ctrl, int fast_boot, int s3resume, int me_uma_size)
{
	int err;

	printk(BIOS_DEBUG, "Starting %s Bridge RAM training (%s).\n",
			IS_SANDY_CPU(ctrl->cpu) ? "Sandy" : "Ivy",
			fast_boot ? "fast boot" : "full initialization");

	if (!fast_boot) {
		/* Find fastest common supported parameters */
		dram_find_common_params(ctrl);

		dram_dimm_mapping(ctrl);
	}

	/* Set MC frequency */
	dram_freq(ctrl);

	if (!fast_boot) {
		/* Calculate timings */
		dram_timing(ctrl);
	}

	/* Set version register */
	MCHBAR32(MRC_REVISION) = 0xc04eb002;

	/* Enable crossover */
	dram_xover(ctrl);

	/* Set timing and refresh registers */
	dram_timing_regs(ctrl);

	/* Power mode preset */
	MCHBAR32(PM_THML_STAT) = 0x5500;

	/* Set scheduler chicken bits */
	MCHBAR32(SCHED_CBIT) = 0x10100005;

	/* Set up watermarks and starvation counter */
	set_wmm_behavior(ctrl->cpu);

	/* Clear IO reset bit */
	MCHBAR32(MC_INIT_STATE_G) &= ~(1 << 5);

	/* Set MAD-DIMM registers */
	dram_dimm_set_mapping(ctrl, 1);
	printk(BIOS_DEBUG, "Done dimm mapping\n");

	/* Zone config */
	dram_zones(ctrl, 1);

	/* Set memory map */
	dram_memorymap(ctrl, me_uma_size);
	printk(BIOS_DEBUG, "Done memory map\n");

	/* Set IO registers */
	dram_ioregs(ctrl);
	printk(BIOS_DEBUG, "Done io registers\n");

	udelay(1);

	if (fast_boot) {
		restore_timings(ctrl);
	} else {
		/* Do JEDEC DDR3 reset sequence */
		dram_jedecreset(ctrl);
		printk(BIOS_DEBUG, "Done jedec reset\n");

		/* MRS commands */
		dram_mrscommands(ctrl);
		printk(BIOS_DEBUG, "Done MRS commands\n");

		/* Prepare for memory training */
		prepare_training(ctrl);

		err = read_training(ctrl);
		if (err)
			return err;

		err = write_training(ctrl);
		if (err)
			return err;

		printram("CP5a\n");

		err = discover_edges(ctrl);
		if (err)
			return err;

		printram("CP5b\n");

		err = command_training(ctrl);
		if (err)
			return err;

		printram("CP5c\n");

		err = discover_edges_write(ctrl);
		if (err)
			return err;

		err = discover_timC_write(ctrl);
		if (err)
			return err;

		normalize_training(ctrl);
	}

	set_read_write_timings(ctrl);

	write_controller_mr(ctrl);

	if (!s3resume) {
		err = channel_test(ctrl);
		if (err)
			return err;
	}

	/* Set MAD-DIMM registers */
	dram_dimm_set_mapping(ctrl, 0);

	return 0;
}
