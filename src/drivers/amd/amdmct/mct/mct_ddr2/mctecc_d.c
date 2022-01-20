/* SPDX-License-Identifier: GPL-2.0-only */

#include <drivers/amd/amdmct/wrappers/mcti.h>

static void setSyncOnUnEccEn_D(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat_a);
#ifdef UNUSED_CODE
static u32 GetScrubAddr_D(u32 Node);
#endif
static u8 isDramECCEn_D(struct DCTStatStruc *p_dct_stat);


/* Initialize ECC modes of Integrated Dram+Memory Controllers of a network of
 * Hammer processors.  Use Dram background scrubber to fast initialize ECC bits
 * of all dram.
 *
 * Notes:
 *
 * Order that items are set:
 *  1. eccen bit in NB
 *  2. Scrub Base
 *  3. Temp Node Base
 *  4. Temp Node Limit
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
 * To allow the NB to scrub, we need to wait a time period long enough to
 * guarantee that the NB scrubs the entire dram on its node. Do do this, we
 * simply sample the scrub ADDR once, for an initial value, then we sample and poll until the polled value of scrub ADDR
 * has wrapped around at least once: Scrub ADDRi+1 < Scrub ADDRi. Since we let all
 * Nodes run in parallel, we need to guarantee that all nodes have wrapped. To do
 * this efficiently, we need only to sample one of the nodes, the node with the
 * largest ammount of dram populated is the one which will take the longest amount
 * of time (the scrub rate is set to max, the same rate, on all nodes).  So,
 * during setup of scrub Base, we determine how much memory and which node has
 * the largest memory installed.
 *
 * Scrubbing should not ordinarily be enabled on a Node with a chip-select gap
 * (aka SW memhole, cs hoisting, etc..).To init ECC memory on this node, the
 * scrubber is used in two steps.  First, the Dram Limit for the node is adjusted
 * down to the bottom of the gap, and that ECC dram is initialized.  Second, the
 * original Limit is restored, the Scrub base is set to 4GB, and scrubber is
 * allowed to run until the Scrub Addr wraps around to zero.
 */
u8 ECCInit_D(struct MCTStatStruc *p_mct_stat, struct DCTStatStruc *p_dct_stat_a)
{
	u8 Node;
	u8 AllECC;
	u16 OB_NBECC;
	u32 curBase;
	u16 OB_ECCRedir;
	u32 LDramECC;
	u32 OF_ScrubCTL;
	u8 MemClrECC;

	u32 dev;
	u32 reg;
	u32 val;
	u16 nvbits;

	mct_hook_before_ecc();

	/* Construct these booleans, based on setup options, for easy handling
	later in this procedure */
	OB_NBECC = mct_get_nv_bits(NV_NBECC);	/* MCA ECC (MCE) enable bit */

	OB_ECCRedir =  mct_get_nv_bits(NV_ECC_REDIR);	/* ECC Redirection */

	mct_get_nv_bits(NV_CHIP_KILL);	/* ECC Chip-kill mode */

	OF_ScrubCTL = 0;		/* Scrub CTL for Dcache, L2, and dram */
	nvbits = mct_get_nv_bits(NV_DC_BK_SCRUB);
	mct_AdjustScrub_D(p_dct_stat_a, &nvbits);
	OF_ScrubCTL |= (u32) nvbits << 16;

	nvbits = mct_get_nv_bits(NV_L2_BK_SCRUB);
	OF_ScrubCTL |= (u32) nvbits << 8;

	nvbits = mct_get_nv_bits(NV_DRAM_BK_SCRUB);
	OF_ScrubCTL |= nvbits;

	AllECC = 1;
	MemClrECC = 0;
	for (Node = 0; Node < MAX_NODES_SUPPORTED; Node++) {
		struct DCTStatStruc *p_dct_stat;
		p_dct_stat = p_dct_stat_a + Node;
		LDramECC = 0;
		if (node_present_d(Node)) {	/*If Node is present */
			dev = p_dct_stat->dev_map;
			reg = 0x40+(Node << 3);	/* Dram Base Node 0 + index */
			val = get_nb32(dev, reg);

			/* WE/RE is checked */
			if ((val & 3) == 3) {	/* Node has dram populated */
				/* Negate 'all nodes/dimms ECC' flag if non ecc
				   memory populated */
				if (p_dct_stat->status & (1 << SB_ECC_DIMMS)) {
					LDramECC = isDramECCEn_D(p_dct_stat);
					if (p_dct_stat->err_code != SC_RUNNING_OK) {
						p_dct_stat->status &=  ~(1 << SB_ECC_DIMMS);
						if (!OB_NBECC) {
							p_dct_stat->err_status |= (1 << SB_DRAM_ECC_DIS);
						}
						AllECC = 0;
						LDramECC =0;
					}
				} else {
					AllECC = 0;
				}
				if (LDramECC) {	/* if ECC is enabled on this dram */
					if (OB_NBECC) {
						mct_enable_dat_intlv_d(p_mct_stat, p_dct_stat);
						dev = p_dct_stat->dev_nbmisc;
						reg =0x44;	/* MCA NB Configuration */
						val = get_nb32(dev, reg);
						val |= 1 << 22;	/* EccEn */
						set_nb32(dev, reg, val);
						dct_mem_clr_init_d(p_mct_stat, p_dct_stat);
						MemClrECC = 1;
						print_tx("  ECC enabled on node: ", Node);
					}
				}	/* this node has ECC enabled dram */
			} else {
				LDramECC = 0;
			}	/* Node has Dram */

			if (MemClrECC) {
				mct_mem_clr_sync_d(p_mct_stat, p_dct_stat_a);
			}
		}	/* if Node present */
	}

	if (AllECC)
		p_mct_stat->g_status |= 1 << GSB_ECCDIMMS;
	else
		p_mct_stat->g_status &= ~(1 << GSB_ECCDIMMS);

	/* Program the Dram BKScrub CTL to the proper (user selected) value.*/
	/* Reset MC4_STS. */
	for (Node = 0; Node < MAX_NODES_SUPPORTED; Node++) {
		struct DCTStatStruc *p_dct_stat;
		p_dct_stat = p_dct_stat_a + Node;
		LDramECC = 0;
		if (node_present_d(Node)) {	/* If Node is present */
			reg = 0x40+(Node << 3);	/* Dram Base Node 0 + index */
			val = get_nb32(p_dct_stat->dev_map, reg);
			curBase = val & 0xffff0000;
			/*WE/RE is checked because memory config may have been */
			if ((val & 3) == 3) {	/* Node has dram populated */
				if (isDramECCEn_D(p_dct_stat)) {	/* if ECC is enabled on this dram */
					dev = p_dct_stat->dev_nbmisc;
					val = curBase << 8;
					if (OB_ECCRedir) {
						val |= (1 << 0); /* enable redirection */
					}
					set_nb32(dev, 0x5C, val); /* Dram Scrub Addr Low */
					val = curBase >> 24;
					set_nb32(dev, 0x60, val); /* Dram Scrub Addr High */
					set_nb32(dev, 0x58, OF_ScrubCTL);	/*Scrub Control */

					/* Divisor should not be set deeper than
					 * divide by 16 when Dcache scrubber or
					 * L2 scrubber is enabled.
					 */
					if ((OF_ScrubCTL & (0x1F << 16)) || (OF_ScrubCTL & (0x1F << 8))) {
						val = get_nb32(dev, 0x84);
						if ((val & 0xE0000000) > 0x80000000) {	/* Get F3x84h[31:29]ClkDivisor for C1 */
							val &= 0x1FFFFFFF;	/* If ClkDivisor is deeper than divide-by-16 */
							val |= 0x80000000;	/* set it to divide-by-16 */
							set_nb32(dev, 0x84, val);
						}
					}
				}	/* this node has ECC enabled dram */
			}	/*Node has Dram */
		}	/*if Node present */
	}

	if (mct_get_nv_bits(NV_SYNC_ON_UN_ECC_EN))
		setSyncOnUnEccEn_D(p_mct_stat, p_dct_stat_a);

	mct_hook_after_ecc();
	for (Node = 0; Node < MAX_NODES_SUPPORTED; Node++) {
		struct DCTStatStruc *p_dct_stat;
		p_dct_stat = p_dct_stat_a + Node;
		if (node_present_d(Node)) {
			print_tx("ECCInit: Node ", Node);
			print_tx("ECCInit: status ", p_dct_stat->status);
			print_tx("ECCInit: err_status ", p_dct_stat->err_status);
			print_tx("ECCInit: err_code ", p_dct_stat->err_code);
			print_t("ECCInit: Done\n");
		}
	}
	return MemClrECC;
}


static void setSyncOnUnEccEn_D(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat_a)
{
	u32 Node;
	u32 reg;
	u32 dev;
	u32 val;

	for (Node = 0; Node < MAX_NODES_SUPPORTED; Node++) {
		struct DCTStatStruc *p_dct_stat;
		p_dct_stat = p_dct_stat_a + Node;
		if (node_present_d(Node)) {	/* If Node is present*/
			reg = 0x40+(Node << 3);	/* Dram Base Node 0 + index*/
			val = get_nb32(p_dct_stat->dev_map, reg);
			/*WE/RE is checked because memory config may have been*/
			if ((val & 3) == 3) {	/* Node has dram populated*/
				if (isDramECCEn_D(p_dct_stat)) {
					/*if ECC is enabled on this dram*/
					dev = p_dct_stat->dev_nbmisc;
					reg = 0x44;	/* MCA NB Configuration*/
					val = get_nb32(dev, reg);
					val |= (1 << SYNC_ON_UC_ECC_EN);
					set_nb32(dev, reg, val);
				}
			}	/* Node has Dram*/
		}	/* if Node present*/
	}
}

#ifdef UNUSED_CODE
static u32 GetScrubAddr_D(u32 Node)
{
	/* Get the current 40-bit Scrub ADDR address, scaled to 32-bits,
	 * of the specified Node.
	 */

	u32 reg;
	u32 regx;
	u32 lo, hi;
	u32 val;
	u32 dev = PA_NBMISC(Node);


	reg = 0x60;		/* Scrub Addr High */
	hi = get_nb32(dev, reg);

	regx = 0x5C;		/* Scrub Addr Low */
	lo = get_nb32(dev, regx);
				/* Scrub Addr High again, detect 32-bit wrap */
	val = get_nb32(dev, reg);
	if (val != hi) {
		hi = val;	/* Scrub Addr Low again, if wrap occurred */
		lo = get_nb32(dev, regx);
	}

	val = hi << 24;
	val |= lo >> 8;

	return val;		/* ScrubAddr[39:8] */
}
#endif

static u8 isDramECCEn_D(struct DCTStatStruc *p_dct_stat)
{
	u32 reg;
	u32 val;
	u8 i;
	u32 dev = p_dct_stat->dev_dct;
	u8 ch_end;
	u8 isDimmECCEn = 0;

	if (p_dct_stat->ganged_mode) {
		ch_end = 1;
	} else {
		ch_end = 2;
	}
	for (i = 0; i < ch_end; i++) {
		if (p_dct_stat->dimm_valid_dct[i] > 0) {
			reg = 0x90 + i * 0x100;		/* Dram Config Low */
			val = get_nb32(dev, reg);
			if (val & (1 << DIMM_EC_EN)) {
				/* set local flag 'dram ecc capable' */
				isDimmECCEn = 1;
				break;
			}
		}
	}
	return isDimmECCEn;
}
