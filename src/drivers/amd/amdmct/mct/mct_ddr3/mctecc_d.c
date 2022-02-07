/* SPDX-License-Identifier: GPL-2.0-only */

#include <arch/cpu.h>
#include <stdint.h>
#include <console/console.h>
#include <device/pci_ops.h>
#include <drivers/amd/amdmct/wrappers/mcti.h>
#include "mct_d_gcc.h"

static void set_sync_on_un_ecc_en_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat_a);
static u8 is_dram_ecc_en_d(struct DCTStatStruc *p_dct_stat);

/* Initialize ECC modes of Integrated Dram+Memory Controllers of a network of
 * Hammer processors.  Use Dram background scrubber to fast initialize ECC bits
 * of all dram.
 *
 * Notes:
 *
 * Order that items are set:
 *  1. eccen bit in NB
 *  2. Scrub Base
 *  3. Temp node Base
 *  4. Temp node Limit
 *  5. Redir bit in NB
 *  6. Scrub CTL
 *
 * Conditions for setting background scrubber.
 *  1. node is present
 *  2. node has dram functioning (WE = RE = 1)
 *  3. all eccdimms (or bit 17 of offset 90,fn 2)
 *  4. no chip-select gap exists
 *
 * The dram background scrubber is used under very controlled circumstances to
 * initialize all the ECC bits on the DIMMs of the entire dram address map
 * (including hidden or lost dram and dram above 4GB). We will turn the scrub
 * rate up to maximum, which should clear 4GB of dram in about 2.7 seconds.
 * We will activate the scrubbers of all nodes with ecc dram and let them run in
 * parallel, thereby reducing even further the time required to condition dram.
 * Finally, we will go through each node and either disable background scrubber,
 *  or set the scrub rate to the user setup specified rate.
 *
 * To allow the NB to scrub, we need to wait a time period long enough
 * to guarantee that the NB scrubs the entire dram on its node. Do do this,
 * we simply sample the scrub ADDR once, for an initial value, then we sample and poll
 * until the polled value of scrub ADDR has wrapped around at least once:
 * Scrub ADDRi+1 < Scrub ADDRi. Since we let all Nodes run in parallel,
 * we need to guarantee that all nodes have wrapped. To do this efficiently,
 * we need only to sample one of the nodes, the node with the largest ammount
 * of dram populated is the one which will take the longest amount of time
 * (the scrub rate is set to max, the same rate, on all nodes). So, during
 * setup of scrub Base, we determine how much memory and which node has the
 * largest memory installed.
 *
 * Scrubbing should not ordinarily be enabled on a node with a chip-select gap
 * (aka SW memhole, cs hoisting, etc..).To init ECC memory on this node, the
 * scrubber is used in two steps.  First, the Dram Limit for the node is adjusted
 * down to the bottom of the gap, and that ECC dram is initialized.  Second, the
 * original Limit is restored, the Scrub base is set to 4GB, and scrubber is
 * allowed to run until the Scrub Addr wraps around to zero.
 */
u8 ecc_init_d(struct MCTStatStruc *p_mct_stat, struct DCTStatStruc *p_dct_stat_a)
{
	u8 node;
	u8 all_ecc;
	u16 ob_nb_ecc;
	u32 cur_base;
	u16 ob_ecc_redir;
	u32 l_dram_ecc;
	u32 of_scrub_ctl;
	u16 ob_chip_kill;
	u8 mem_clr_ecc;

	u32 dev;
	u32 reg;
	u32 val;
	u16 nvbits;

	u32 dword;
	u8 sync_flood_on_dram_err[MAX_NODES_SUPPORTED];
	u8 sync_flood_on_any_uc_err[MAX_NODES_SUPPORTED];

	mct_hook_before_ecc();

	/* Construct these booleans, based on setup options, for easy handling
	later in this procedure */
	ob_nb_ecc = mct_get_nv_bits(NV_NBECC);		/* MCA ECC (MCE) enable bit */

	ob_ecc_redir =  mct_get_nv_bits(NV_ECC_REDIR);	/* ECC Redirection */

	ob_chip_kill = mct_get_nv_bits(NV_CHIP_KILL);	/* ECC Chip-kill mode */
	of_scrub_ctl = 0;				/* Scrub CTL for Dcache, L2, and dram */

	if (!is_fam15h()) {
		nvbits = mct_get_nv_bits(NV_DC_BK_SCRUB);
		/* mct_adjust_scrub_d(p_dct_stat_a, &nvbits); */	/* Need not adjust */
		of_scrub_ctl |= (u32) nvbits << 16;

		nvbits = mct_get_nv_bits(NV_L2_BK_SCRUB);
		of_scrub_ctl |= (u32) nvbits << 8;
	}

	nvbits = mct_get_nv_bits(NV_L3_BK_SCRUB);
	of_scrub_ctl |= (nvbits & 0x1f) << 24;		/* L3Scrub = NV_L3_BK_SCRUB */

	nvbits = mct_get_nv_bits(NV_DRAM_BK_SCRUB);
	of_scrub_ctl |= nvbits;				/* DramScrub = NV_DRAM_BK_SCRUB */

	/* Prevent lockups on DRAM errors during ECC init */
	for (node = 0; node < MAX_NODES_SUPPORTED; node++) {
		struct DCTStatStruc *p_dct_stat;
		p_dct_stat = p_dct_stat_a + node;

		if (node_present_d(node)) {
			dword = get_nb32(p_dct_stat->dev_nbmisc, 0x44);
			sync_flood_on_dram_err[node] = (dword >> 30) & 0x1;
			sync_flood_on_any_uc_err[node] = (dword >> 21) & 0x1;
			dword &= ~(0x1 << 30);
			dword &= ~(0x1 << 21);
			set_nb32(p_dct_stat->dev_nbmisc, 0x44, dword);

			u32 mc4_status_high = pci_read_config32(p_dct_stat->dev_nbmisc, 0x4c);
			u32 mc4_status_low = pci_read_config32(p_dct_stat->dev_nbmisc, 0x48);
			if ((mc4_status_high & (0x1 << 31))
			&& (mc4_status_high != 0xffffffff)) {
				printk(BIOS_WARNING,
					"WARNING: MC4 Machine Check Exception detected!\n"
					"Signature: %08x%08x\n",
					mc4_status_high, mc4_status_low);
			}

			/* Clear MC4 error status */
			pci_write_config32(p_dct_stat->dev_nbmisc, 0x48, 0x0);
			pci_write_config32(p_dct_stat->dev_nbmisc, 0x4c, 0x0);
		}
	}

	all_ecc = 1;
	mem_clr_ecc = 0;
	for (node = 0; node < MAX_NODES_SUPPORTED; node++) {
		struct DCTStatStruc *p_dct_stat;
		p_dct_stat = p_dct_stat_a + node;
		l_dram_ecc = 0;
		if (node_present_d(node)) {	/*If node is present */
			dev = p_dct_stat->dev_map;
			reg = 0x40+(node << 3);	/* Dram Base node 0 + index */
			val = get_nb32(dev, reg);

			/* WE/RE is checked */
			if ((val & 3) == 3) {	/* node has dram populated */
				/* Negate 'all nodes/dimms ECC' flag if non ecc
				   memory populated */
				if (p_dct_stat->status & (1 << SB_ECC_DIMMS)) {
					l_dram_ecc = is_dram_ecc_en_d(p_dct_stat);
					if (p_dct_stat->err_code != SC_RUNNING_OK) {
						p_dct_stat->status &=  ~(1 << SB_ECC_DIMMS);
						if (!ob_nb_ecc) {
							p_dct_stat->err_status
								|= (1 << SB_DRAM_ECC_DIS);
						}
						all_ecc = 0;
						l_dram_ecc = 0;
					}
				} else {
					all_ecc = 0;
				}
				if (l_dram_ecc) {	/* if ECC is enabled on this dram */
					if (ob_nb_ecc) {
						mct_enable_dat_intlv_d(p_mct_stat, p_dct_stat);
						val = get_nb32(p_dct_stat->dev_dct, 0x110);
						val |= 1 << 5;	/* DctDatIntLv = 1 */
						set_nb32(p_dct_stat->dev_dct, 0x110, val);
						dev = p_dct_stat->dev_nbmisc;
						reg = 0x44;	/* MCA NB Configuration */
						val = get_nb32(dev, reg);
						val |= 1 << 22;	/* EccEn */
						set_nb32(dev, reg, val);
						dct_mem_clr_init_d(p_mct_stat, p_dct_stat);
						mem_clr_ecc = 1;
						printk(BIOS_DEBUG,
							"  ECC enabled on node: %02x\n", node);
					}
				}	/* this node has ECC enabled dram */

				if (mem_clr_ecc) {
					dct_mem_clr_sync_d(p_mct_stat, p_dct_stat);
				}
			} else {
				l_dram_ecc = 0;
			}	/* node has Dram */
		}	/* if node present */
	}

	if (all_ecc)
		p_mct_stat->g_status |= 1 << GSB_ECCDIMMS;
	else
		p_mct_stat->g_status &= ~(1 << GSB_ECCDIMMS);

	/* Program the Dram BKScrub CTL to the proper (user selected) value.*/
	/* Reset MC4_STS. */
	for (node = 0; node < MAX_NODES_SUPPORTED; node++) {
		struct DCTStatStruc *p_dct_stat;
		p_dct_stat = p_dct_stat_a + node;
		l_dram_ecc = 0;
		if (node_present_d(node)) {	/* If node is present */
			reg = 0x40+(node << 3);	/* Dram Base node 0 + index */
			val = get_nb32(p_dct_stat->dev_map, reg);
			cur_base = val & 0xffff0000;
			/*WE/RE is checked because memory config may have been */
			if ((val & 3) == 3) {	/* node has dram populated */
				/* if ECC is enabled on this dram */
				if (is_dram_ecc_en_d(p_dct_stat)) {
					dev = p_dct_stat->dev_nbmisc;
					val = cur_base << 8;
					if (ob_ecc_redir) {
						/* Enable redirection */
						val |= (1 << 0);
					}
					/* Dram Scrub Addr Low */
					set_nb32(dev, 0x5c, val);
					val = cur_base >> 24;
					/* Dram Scrub Addr High */
					set_nb32(dev, 0x60, val);

					/* Set scrub rate controls */
					if (is_fam15h()) {
						/* Erratum 505 */
						fam15h_switch_dct(p_dct_stat->dev_map, 0);
					}
					set_nb32(dev, 0x58, of_scrub_ctl);/* Scrub Control */
					if (is_fam15h()) {
						/* Erratum 505 */
						fam15h_switch_dct(p_dct_stat->dev_map, 1);
						/* Scrub Control */
						set_nb32(dev, 0x58, of_scrub_ctl);
						/* Erratum 505 */
						fam15h_switch_dct(p_dct_stat->dev_map, 0);
					}

					if (!is_fam15h()) {
						/* Divisor should not be set deeper than
						 * divide by 16 when Dcache scrubber or
						 * L2 scrubber is enabled.
						 */
						if ((of_scrub_ctl & (0x1F << 16)) || (of_scrub_ctl & (0x1F << 8))) {
							val = get_nb32(dev, 0x84);
							/* Get F3x84h[31:29]ClkDivisor for C1 */
							if ((val & 0xE0000000) > 0x80000000) {
								/* If ClkDivisor is deeper than
								 * divide-by-16
								 */
								val &= 0x1FFFFFFF;
								/* set it to divide-by-16 */
								val |= 0x80000000;
								set_nb32(dev, 0x84, val);
							}
						}
					}

					if (p_dct_stat->logical_cpuid & (AMD_DR_GT_D0 | AMD_FAM15_ALL)) {
						/* Set up message triggered C1E */
						val = pci_read_config32(
							p_dct_stat->dev_nbmisc, 0xd4);
						/* StutterScrubEn = DRAM scrub enabled */
						val &= ~(0x1 << 15);
						val |= (mct_get_nv_bits(NV_DRAM_BK_SCRUB)
								? 1 : 0) << 15;
						pci_write_config32(
							p_dct_stat->dev_nbmisc, 0xd4, val);
					}
				}	/* this node has ECC enabled dram */
			}	/*node has Dram */
		}	/*if node present */
	}

	/* Restore previous MCA error handling settings */
	for (node = 0; node < MAX_NODES_SUPPORTED; node++) {
		struct DCTStatStruc *p_dct_stat;
		p_dct_stat = p_dct_stat_a + node;

		if (node_present_d(node)) {
			dev = p_dct_stat->dev_map;
			reg = 0x40 + (node << 3);	/* Dram Base node 0 + index */
			val = get_nb32(dev, reg);

			/* WE/RE is checked */
			if ((val & 0x3) == 0x3) {	/* node has dram populated */
				u32 mc4_status_high = pci_read_config32(
								p_dct_stat->dev_nbmisc, 0x4c);
				u32 mc4_status_low = pci_read_config32(
								p_dct_stat->dev_nbmisc, 0x48);
				if ((mc4_status_high & (0x1 << 31))
				&& (mc4_status_high != 0xffffffff)) {
					printk(BIOS_WARNING,
						"WARNING: MC4 Machine Check Exception detected!\n"
						"Signature: %08x%08x\n",
						mc4_status_high, mc4_status_low);
				}

				/* Clear MC4 error status */
				pci_write_config32(p_dct_stat->dev_nbmisc, 0x48, 0x0);
				pci_write_config32(p_dct_stat->dev_nbmisc, 0x4c, 0x0);

				/* Restore previous MCA error handling settings */
				dword = get_nb32(p_dct_stat->dev_nbmisc, 0x44);
				dword |= (sync_flood_on_dram_err[node] & 0x1) << 30;
				dword |= (sync_flood_on_any_uc_err[node] & 0x1) << 21;
				set_nb32(p_dct_stat->dev_nbmisc, 0x44, dword);
			}
		}
	}

	if (mct_get_nv_bits(NV_SYNC_ON_UN_ECC_EN))
		set_sync_on_un_ecc_en_d(p_mct_stat, p_dct_stat_a);

	mct_hook_after_ecc();
	for (node = 0; node < MAX_NODES_SUPPORTED; node++) {
		struct DCTStatStruc *p_dct_stat;
		p_dct_stat = p_dct_stat_a + node;
		if (node_present_d(node)) {
			printk(BIOS_DEBUG, "ECCInit: node %02x\n", node);
			printk(BIOS_DEBUG, "ECCInit: status %x\n", p_dct_stat->status);
			printk(BIOS_DEBUG, "ECCInit: err_status %x\n", p_dct_stat->err_status);
			printk(BIOS_DEBUG, "ECCInit: err_code %x\n", p_dct_stat->err_code);
			printk(BIOS_DEBUG, "ECCInit: Done\n");
		}
	}
	return mem_clr_ecc;
}

static void set_sync_on_un_ecc_en_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat_a)
{
	u32 node;
	u32 reg;
	u32 dev;
	u32 val;

	for (node = 0; node < MAX_NODES_SUPPORTED; node++) {
		struct DCTStatStruc *p_dct_stat;
		p_dct_stat = p_dct_stat_a + node;
		if (node_present_d(node)) {	/* If node is present*/
			reg = 0x40+(node << 3);	/* Dram Base node 0 + index*/
			val = get_nb32(p_dct_stat->dev_map, reg);
			/*WE/RE is checked because memory config may have been*/
			if ((val & 3) == 3) {	/* node has dram populated*/
				if (is_dram_ecc_en_d(p_dct_stat)) {
					/*if ECC is enabled on this dram*/
					dev = p_dct_stat->dev_nbmisc;
					reg = 0x44;	/* MCA NB Configuration*/
					val = get_nb32(dev, reg);
					val |= (1 << SYNC_ON_UC_ECC_EN);
					set_nb32(dev, reg, val);
				}
			}	/* node has Dram*/
		}	/* if node present*/
	}
}

static u8 is_dram_ecc_en_d(struct DCTStatStruc *p_dct_stat)
{
	u32 reg;
	u32 val;
	u8 i;
	u32 dev = p_dct_stat->dev_dct;
	u8 ch_end;
	u8 is_dimm_ecc_en = 0;

	if (p_dct_stat->ganged_mode) {
		ch_end = 1;
	} else {
		ch_end = 2;
	}
	for (i = 0; i < ch_end; i++) {
		if (p_dct_stat->dimm_valid_dct[i] > 0) {
			reg = 0x90;		/* Dram Config Low */
			val = get_nb32_dct(dev, i, reg);
			if (val & (1 << DIMM_EC_EN)) {
				/* set local flag 'dram ecc capable' */
				is_dimm_ecc_en = 1;
				break;
			}
		}
	}
	return is_dimm_ecc_en;
}
