/* SPDX-License-Identifier: GPL-2.0-only */

#include <cpu/power/istep_13.h>
#include <cpu/power/vpd_data.h>
#include <console/console.h>
#include <timer.h>
#include <device/dram/rcd.h>

#include "istep_13_scom.h"

#define SPD_I2C_BUS		3

static void draminit_cke_helper(chiplet_id_t id, int mca_i)
{
	/*
	 * Hostboot stops CCS before sending new programs. I'm not sure it is wise
	 * to do, unless there are infinite loops. Don't do and see what happens.
	MC01.MCBIST.MBA_SCOMFIR.CCS_CNTLQ          // 0x070123A5
	      [all] 0
	      [1]   CCS_CNTLQ_CCS_STOP = 1
	timeout(50*10ns):
	    if MC01.MCBIST.MBA_SCOMFIR.CCS_STATQ[0] (CCS_STATQ_CCS_IP) != 1: break    // 0x070123A6
	    delay(10ns)
	*/

	ccs_add_instruction(id, 0, 0xF, 0xF, 400);
	ccs_execute(id, mca_i);
}

static void rcd_load(mca_data_t *mca, int d)
{
	uint8_t val;
	rdimm_data_t *dimm = &mca->dimm[d];
	uint8_t *spd = dimm->spd;

	/* Raw card specifications are JEDEC documents MODULE4.20.28.x, where x is A-E */

	/*
	F0RC00  = 0x0   // Depends on reference raw card used, sometimes 0x2 (ref. A, B, C and custom?)
	                // Seems that 'custom' is used for > C, which means 0x2 is always set.
	F0RC01  = 0x0   // Depends on reference raw card used, sometimes 0xC (ref. C?).
	                // JESD82-31: "The system must read the module SPD to determine
	                // which clock outputs are used by the module". R/C C and D use
	                // only Y0-Y1, other R/C use all 4 signals.
	*/
	/*
	 * F0RC01 is effectively based on dimm->mranks, but maybe future reference R/C
	 * will use different clocks than Y0-Y1, which technically is possible...
	 *
	 * (spd[131] & 0x1F) is 0x02 for C and 0x03 for D, this line tests for both
	 */
	val = ((spd[131] & 0x1E) == 0x02) ? 0xC2 : 0x02;
	rcd_write_reg(SPD_I2C_BUS, mca->dimm[d].rcd_i2c_addr, F0RC00_01, val);

	/*
	F0RC02  =
	  [0] = 1 if(!(16Gb density && x4 width))     // disable A17?     // Why not use SPD[5]?
	                // Hostboot waits for tSTAB, however it is not necessary as long as bit 3 is not changed.
	F0RC03  =
	  [0-1] SPD[137][4-5]   // Address/Command drive strength
	  [2-3] SPD[137][6-7]   // CS drive strength
	                // There is also a workaround for NVDIMM hybrids, not needed for plain RDIMM
	*/
	val = spd[137] & 0xF0;	// F0RC03
	if (dimm->density != DENSITY_16Gb || dimm->width != WIDTH_x4)
		val |= 1;
	rcd_write_reg(SPD_I2C_BUS, mca->dimm[d].rcd_i2c_addr, F0RC02_03, val);

	/*
	F0RC04  =
	  // BUG? Hostboot reverses bitfields order for RC04, 05
	  [0-1] SPD[137][2-3]   // ODT drive strength
	  [2-3] SPD[137][0-1]   // CKE drive strength
	                // There is also a workaround for NVDIMM hybrids, not needed for plain RDIMM
	F0RC05  =
	  [0-1] SPD[138][2-3]   // Clocks drive strength, A side (1,3)
	  [2-3] SPD[138][0-1]   // Clocks drive strength, B side (0,2)
	                // There is also a workaround for NVDIMM hybrids, not needed for plain RDIMM
	*/
	/* First read both nibbles as they are in SPD, then swap pairs of bit fields */
	val = (spd[137] & 0x0F) | ((spd[138] & 0x0F) << 4);
	val = ((val & 0x33) << 2) | ((val & 0xCC) >> 2);
	rcd_write_reg(SPD_I2C_BUS, mca->dimm[d].rcd_i2c_addr, F0RC04_05, val);

	/*
	F0RC06  = 0xf   // This is a command register, either don't touch it or use NOP (F)
	F0RC07  = 0x0   // This is a command register, either don't touch it or use NOP (0)
	*/

	/*
	F0RC08  =
	  [0-1] =
	      1 if master ranks == 4 (SPD[12])        // C0 and C1 enabled
	      3 if not 3DS (check SPD[6] and SPD[10]) // all disabled
	      2 if slave ranks <= 2                   // C0 enabled
	      1 if slave ranks <= 4                   // C0 and C1 enabled
	      0 otherwise (3DS with 5-8 slave ranks)  // C0, C1 and C2 enabled
	  [3] = 1 if(!(16Gb density && x4 width))     // disable A17?     // Why not use SPD[5]?
	F0RC09  =
	  [2] =
	      // TODO: add test for it, write 1 for now
	      0 if this DIMM's ODTs are used for writes or reads that target the other DIMM on the same port
	      1 otherwise
	  [3] = 1     // Register CKE Power Down. CKE must be high at the moment of writing to this register and stay high.
	              // TODO: For how long? Indefinitely, tMRD, tInDIS, tFixedOutput or anything else?
	*/
	/* Assume no 4R */
	val = (dimm->mranks == dimm->log_ranks) ? 3 :
	      (2 - (dimm->log_ranks / dimm->mranks) / 4);
	if (dimm->density != DENSITY_16Gb || dimm->width != WIDTH_x4)
		val |= 8;
	val |= 0xC0;
	rcd_write_reg(SPD_I2C_BUS, mca->dimm[d].rcd_i2c_addr, F0RC08_09, val);

	/*
	F0RC0A  =
	  [0-2] =     // There are other valid values not used by Hostboot
	      1 if 1866 MT/s
	      2 if 2133 MT/s
	      3 if 2400 MT/s
	      4 if 2666 MT/s
	F0RC0B  = 0xe   // External VrefCA connected to QVrefCA and BVrefCA
	*/
	val = mem_data.speed == 1866 ? 1 :
	      mem_data.speed == 2133 ? 2 :
	      mem_data.speed == 2400 ? 3 : 4;
	val |= 0xE0;
	rcd_write_reg(SPD_I2C_BUS, mca->dimm[d].rcd_i2c_addr, F0RC0A_0B, val);

	/*
	F0RC0C  = 0     // Normal operating mode
	F0RC0D  =
	  [0-1] =         // CS mode
	      3 if master ranks == 4 (SPD[12])    // encoded QuadCS
	      0 otherwise                         // direct DualCS
	  [2] = 1         // RDIMM
	  [3] = SPD[136]  // Address mirroring for MRS commands
	*/
	/* Assume RDIMM and that there are no 4R configurations, add when needed */
	val = 0x40;
	if (spd[136])
		val |= 0x80;
	rcd_write_reg(SPD_I2C_BUS, mca->dimm[d].rcd_i2c_addr, F0RC0C_0D, val);

	/*
	F0RC0E  = 0xd     // Parity enable, ALERT_n assertion and re-enable
	F0RC0F  = 0       // Normal mode
	*/
	val = 0x0D;
	rcd_write_reg(SPD_I2C_BUS, mca->dimm[d].rcd_i2c_addr, F0RC0E_0F, val);

	/*
	F0RC1x  = 0       // Normal mode, VDD/2
	F0RC2x  = 0       // Normal mode, all I2C accesses enabled
	*/

	/*
	F0RC3x  =
	      0x1f if 1866 MT/s
	      0x2c if 2133 MT/s
	      0x39 if 2400 MT/s
	      0x47 if 2666 MT/s
	*/
	val = mem_data.speed == 1866 ? 0x1F :
	      mem_data.speed == 2133 ? 0x2C :
	      mem_data.speed == 2400 ? 0x39 : 0x47;
	rcd_write_reg(SPD_I2C_BUS, mca->dimm[d].rcd_i2c_addr, F0RC3x, val);

	/*
	F0RC4x  = 0       // Should not be touched at all, it is used to access different function spaces
	F0RC5x  = 0       // Should not be touched at all, it is used to access different function spaces
	F0RC6x  = 0       // Should not be touched at all, it is used to access different function spaces
	F0RC7x  = 0       // Value comes from VPD, 0 is default, it doesn't seem to be changed anywhere in the code...
	F0RC8x  = 0       // Default QxODT timing for reads and for writes
	F0RC9x  = 0       // QxODT not asserted during writes, all ranks
	F0RCAx  = 0       // QxODT not asserted during reads, all ranks
	*/

	/*
	F0RCBx  =
	  [0-2] =       // Note that only the first line is different than F0RC08 (C0 vs. C0 & C1)
	      6 if master ranks == 4 (SPD[12])        // C0 enabled
	      7 if not 3DS (check SPD[6] and SPD[10]) // all disabled
	      6 if slave ranks <= 2                   // C0 enabled
	      4 if slave ranks <= 4                   // C0 and C1 enabled
	      0 otherwise (3DS with 5-8 slave ranks)  // C0, C1 and C2 enabled
	*/
	/* Assume no 4R */
	val = (dimm->mranks == dimm->log_ranks) ? 7 :
	      (dimm->log_ranks / dimm->mranks) == 2 ? 6 :
	      (dimm->log_ranks / dimm->mranks) == 4 ? 4 : 0;
	rcd_write_reg(SPD_I2C_BUS, mca->dimm[d].rcd_i2c_addr, F0RCBx, val);

	/*
	 * After all RCWs are set, DRAM gets reset "to ensure it is reset properly".
	 * Comment: "Note: the minimum for a FORC06 soft reset is 32 cycles, but we
	 * empirically tested it at 8k cycles". Shouldn't we rather wait (again!)
	 * for periods defined in JESD79-4C (200us low and 500us high)?
	 *
	 * Do we even need it in the first place?
	 */
	/*
	F0RC06 = 0x2      // Set QRST_n to active (low)
	delay(8000 memclocks)
	F0RC06 = 0x3      // Set QRST_n to inactive (high)
	delay(8000 memclocks)
	*/
	val = 0x2;
	rcd_write_reg(SPD_I2C_BUS, mca->dimm[d].rcd_i2c_addr, F0RC06_07, val);
	delay_nck(8000);
	val = 0x3;
	rcd_write_reg(SPD_I2C_BUS, mca->dimm[d].rcd_i2c_addr, F0RC06_07, val);
	delay_nck(8000);

	/*
	 * Dumped values from currently installed DIMM, from Petitboot:
	 * 0xc7 0x18 0x42 0x00 0x00 0x00 0x00 0x00    VID[2], DID[2], RID[1], 3x reserved
	 * 0x02 0x01 0x00 0x03 0xcb 0xe4 0x40 0x0d    F0RC00-0F (4b each)
	 * 0x00 0x00 0x47 0x00 0x00 0x00 0x00 0x00    F0RC1x-8x (8b each)
	 * 0x00 0x00 0x07                             F0RC9x-Bx (8b each), then all zeroes (Error Log Registers)
	 *
	 * Below is a copy of above values, this also tests RCD/I2C API. Command
	 * register is changed to NOP (was "Clear DRAM Reset" in dump).
	 */
	/*
	rcd_write_32b(SPD_I2C_BUS, mca->dimm[d].rcd_i2c_addr, F0RC00_01, 0x0201000f);
	rcd_write_32b(SPD_I2C_BUS, mca->dimm[d].rcd_i2c_addr, F0RC08_09, 0xcbe4400d);
	rcd_write_reg(SPD_I2C_BUS, mca->dimm[d].rcd_i2c_addr, F0RC3x, 0x47);
	rcd_write_reg(SPD_I2C_BUS, mca->dimm[d].rcd_i2c_addr, F0RCBx, 0x07);
	*/
}

/*
 * Programming the Mode Registers consists of entering special mode using MRS
 * (Mode Register Set) command and sending MR# values, one at a time, in a
 * specific order (3,6,5,4,2,1,0). Those values are sent using address lines,
 * including bank and bank group lines, which select which MR to write to.
 * One of the implications is that these values cannot be read back. PHY
 * controller holds the mirrors of last written values in its registers, but the
 * mapping of bits is not clear. This mirror is RW, so there is a possibility
 * that the values are not the same as the real ones (but this would be a bad
 * idea as these bits are used by a controller). It gets further complicated
 * when PDA mode was used at any point, as there is just one mirror register per
 * rank pair.
 *
 * We have to write a whole register even when changing just one bit, this means
 * that we have to remember what was written, or be able to (re)generate valid
 * data. For this platform we have CCS which can be programmed to push all MRs
 * in one sequence of instructions, including all required timeouts. There are
 * two main timeout parameters: tMRD (minimal amount of time between two MRS
 * commands) and tMOD (time between MRS and non-MRS and non-DES command). For
 * all Speed Bins tMRD = 8nCK, tMOD = max(24nCK, 15ns) = 24nCK. Exceptions to
 * those are:
 * - gear down mode
 * - PDA mode
 * - settings to command & address lines: C/A parity latency, CS to C/A latency
 *   (only tMRC doesn't apply)
 * - VrefDQ training
 * - DLL Enable, DLL Reset (only tMOD doesn't apply)
 * - maximum power saving mode (only tMOD doesn't apply)
 *
 * MRS are written per rank usually, although most of them must have the same
 * values across the DIMM or even port. There are some settings that apply to
 * individual DRAMs instead of whole rank (e.g. Vref in MR6). Normally settings
 * written to MR# are passed to each DRAM, if individual DRAM has to have its
 * settings changed independently of others we must use Per DRAM Addressability
 * (PDA) mode. PDA is possible only after write leveling was performed.
 *
 * CCS is per MCBIST, so we need at most 4 (ports) * 2 (DIMMs per port) *
 * 2 (master ranks per DIMM) * 2 (A- and B-side) *
 * (7 (# of MRS) + 1 (final DES)) = 256 instructions. CCS holds space for 32
 * instructions, so we have to divide it and send a set of instructions per DIMM
 * or even smaller chunks.
 *
 * TODO: is 4 ranks on RDIMM possible/used? PHY supports two ranks per DIMM (see
 * 2.1 in any of the volumes of the registers specification), but Hostboot has
 * configurations even for RDIMMs with 4 master ranks (see xlate_map vector in
 * src/import/chips/p9/procedures/hwp/memory/lib/mc/xlate.C). Maybe those are
 * counted in different places, i.e. before and after RCD, and thanks to Encoded
 * QuadCS 4R DIMMs are visible to the PHY as 2R devices?
 */
static void mrs_load(int mcs_i, int mca_i, int d)
{
	chiplet_id_t id = mcs_ids[mcs_i];
	mca_data_t *mca = &mem_data.mcs[mcs_i].mca[mca_i];
	int mirrored = mca->dimm[d].spd[136] & 1;
	mrs_cmd_t mrs;
	int vpd_idx = (mca->dimm[d].mranks - 1) * 2 + (!!mca->dimm[d ^ 1].present);
	enum rank_selection ranks;

	if (d == 0) {
		if (mca->dimm[d].mranks == 2)
			ranks = DIMM0_ALL_RANKS;
		else
			ranks = DIMM0_RANK0;
	}
	else {
		if (mca->dimm[d].mranks == 2)
			ranks = DIMM1_ALL_RANKS;
		else
			ranks = DIMM1_RANK0;
	}

	/*
	 * If any of the following are changed, make sure to change istep 13.11 too,
	 * some of the pre-/post-workarounds are also writing to these registers.
	 */

	mrs = ddr4_get_mr3(DDR4_MR3_MPR_SERIAL,
                       DDR4_MR3_CRC_DM_5,
                       DDR4_MR3_FINE_GRAN_REF_NORMAL,
                       DDR4_MR3_TEMP_SENSOR_DISABLE,
                       DDR4_MR3_PDA_DISABLE,
                       DDR4_MR3_GEARDOWN_1_2_RATE,
                       DDR4_MR3_MPR_NORMAL,
                       0);
	ccs_add_mrs(id, mrs, ranks, mirrored, tMRD);

	mrs = ddr4_get_mr6(mca->nccd_l,
                       DDR4_MR6_VREFDQ_TRAINING_DISABLE,
                       DDR4_MR6_VREFDQ_TRAINING_RANGE_1, /* Don't care when disabled */
                       0);
	ccs_add_mrs(id, mrs, ranks, mirrored, tMRD);

	mrs = ddr4_get_mr5(DDR4_MR5_RD_DBI_DISABLE,
                       DDR4_MR5_WR_DBI_DISABLE,
                       DDR4_MR5_DATA_MASK_DISABLE,
                       vpd_to_rtt_park(ATTR_MSS_VPD_MT_DRAM_RTT_PARK[vpd_idx]),
                       DDR4_MR5_ODT_PD_ACTIVADED,
                       DDR4_MR5_CA_PARITY_LAT_DISABLE);
	ccs_add_mrs(id, mrs, ranks, mirrored, tMRD);

	mrs = ddr4_get_mr4(DDR4_MR4_HPPR_DISABLE,
                       DDR4_MR4_WR_PREAMBLE_1, /* ATTR_MSS_VPD_MT_PREAMBLE - always 0 */
                       DDR4_MR4_RD_PREAMBLE_1, /* ATTR_MSS_VPD_MT_PREAMBLE - always 0 */
                       DDR4_MR4_RD_PREAMBLE_TRAINING_DISABLE,
                       DDR4_MR4_SELF_REFRESH_ABORT_DISABLE,
                       DDR4_MR4_CS_TO_CMD_LAT_DISABLE,
                       DDR4_MR4_SPPR_DISABLE,
                       DDR4_MR4_INTERNAL_VREF_MON_DISABLE,
                       DDR4_MR4_TEMP_CONTROLLED_REFR_DISABLE,
                       DDR4_MR4_MAX_PD_MODE_DISABLE);
	ccs_add_mrs(id, mrs, ranks, mirrored, tMRD);

	/*
	 * Regarding RTT_WR: OFF seems to be the safest option, but it is not always
	 * the case in VPD.
	 * See "Write leveling - pre-workaround" (and post-workaround) in 13.11,
	 * maybe write 0 here and don't do pre-?
	 */
	mrs = ddr4_get_mr2(DDR4_MR2_WR_CRC_DISABLE, /* ATTR_MSS_MRW_DRAM_WRITE_CRC, default 0 */
                       vpd_to_rtt_wr(ATTR_MSS_VPD_MT_DRAM_RTT_WR[vpd_idx]),
                       /* ATTR_MSS_MRW_REFRESH_RATE_REQUEST, default DOUBLE.
                        * Do we need to half tREFI as well? */
                       DDR4_MR2_ASR_MANUAL_EXTENDED_RANGE,
                       mem_data.cwl);
	ccs_add_mrs(id, mrs, ranks, mirrored, tMRD);

	mrs = ddr4_get_mr1(DDR4_MR1_QOFF_ENABLE,
                       mca->dimm[d].width == WIDTH_x8 ? DDR4_MR1_TQDS_ENABLE : DDR4_MR1_TQDS_DISABLE,
                       vpd_to_rtt_nom(ATTR_MSS_VPD_MT_DRAM_RTT_NOM[vpd_idx]),
                       DDR4_MR1_WRLVL_DISABLE,
                       DDR4_MR1_ODIMP_RZQ_7, /* ATTR_MSS_VPD_MT_DRAM_DRV_IMP_DQ_DQS, always 34 Ohms */
                       DDR4_MR1_AL_DISABLE,
                       DDR4_MR1_DLL_ENABLE);
	ccs_add_mrs(id, mrs, ranks, mirrored, tMRD);

	mrs = ddr4_get_mr0(mca->nwr,
                       DDR4_MR0_DLL_RESET_YES,
                       DDR4_MR0_MODE_NORMAL,
                       mca->cl,
                       DDR4_MR0_BURST_TYPE_SEQUENTIAL,
                       DDR4_MR0_BURST_LENGTH_FIXED_8);
	ccs_add_mrs(id, mrs, ranks, mirrored, tMOD);

	ccs_execute(id, mca_i);
}

/*
 * 13.10 mss_draminit: Dram initialize
 *
 * a) p9_mss_draminit.C (mcbist) -- Nimbus
 * b) p9c_mss_draminit.C (mba) -- Cumulus
 *    - RCD parity errors are checked before logging other errors - HWP will
 *      exit with RC
 *    - De-assert dram reset
 *    - De-assert bit (Scom) that forces mem clock low - dram clocks start
 *    - Raise CKE
 *    - Load RCD Control Words
 *    - Load MRS - for each dimm pair/ports/rank
 *      - ODT Values
 *      - MR0-MR6
 * c) Check for attentions (even if HWP has error)
 *    - FW
 *      - Call PRD
 *        - If finds and error, commit HWP RC as informational
 *        - Else commit HWP RC as normal
 *      - Trigger reconfig loop is anything was deconfigured
 */
void istep_13_10(void)
{
	printk(BIOS_EMERG, "starting istep 13.10\n");
	int mcs_i, mca_i, dimm;

	report_istep(13,10);

	for (mcs_i = 0; mcs_i < MCS_PER_PROC; mcs_i++) {
		if (!mem_data.mcs[mcs_i].functional)
			continue;

		/* MC01.MCBIST.MBA_SCOMFIR.CCS_MODEQ
			// "It's unclear if we want to run with this true or false. Right now (10/15) this
			// has to be false. Shelton was unclear if this should be on or off in general BRS"
			[0]   CCS_MODEQ_CCS_STOP_ON_ERR =           0
			[1]   CCS_MODEQ_CCS_UE_DISABLE =            0
			[24]  CCS_MODEQ_CFG_CCS_PARITY_AFTER_CMD =  1
			[26]  CCS_MODEQ_COPY_CKE_TO_SPARE_CKE =     1   // Docs: "Does not apply for POWER9. No spare chips to copy to."
			// The following are set in 13.11, but we can do it here, one less RMW
			// "Hm. Centaur sets this up for the longest duration possible. Can we do better?"
			// This is timeout so we should only hit it in the case of error. What is the unit of this field? Memclocks?
			[8-23]  CCS_MODEQ_DDR_CAL_TIMEOUT_CNT =       0xffff
			[30-31] CCS_MODEQ_DDR_CAL_TIMEOUT_CNT_MULT =  3
		*/
		scom_and_or_for_chiplet(mcs_ids[mcs_i], CCS_MODEQ,
		                        ~(PPC_BIT(CCS_MODEQ_CCS_STOP_ON_ERR) |
		                          PPC_BIT(CCS_MODEQ_CCS_UE_DISABLE)),
		                        PPC_BIT(CCS_MODEQ_CFG_CCS_PARITY_AFTER_CMD) |
		                        PPC_BIT(CCS_MODEQ_COPY_CKE_TO_SPARE_CKE) |
		                        PPC_SHIFT(0xFFFF, CCS_MODEQ_DDR_CAL_TIMEOUT_CNT) |
		                        PPC_SHIFT(3, CCS_MODEQ_DDR_CAL_TIMEOUT_CNT_MULT));

		for (mca_i = 0; mca_i < MCA_PER_MCS; mca_i++) {
			mca_data_t *mca = &mem_data.mcs[mcs_i].mca[mca_i];

			if (!mca->functional)
				continue;

			/* MC01.PORT0.SRQ.MBA_FARB5Q                     // 0x07010918
				  // RESET_N should stay low for at least 200us (JEDEC fig 7) for cold boot. Who and when sets it low?
				  // "Up, down P down, up N. Somewhat magic numbers - came from Centaur and proven to be the
				  // same on Nimbus. Why these are what they are might be lost to time ..."
				  [0-1] MBA_FARB5Q_CFG_DDR_DPHY_NCLK =          0x1     // 0b01     // 2nd RMW
				  [2-3] MBA_FARB5Q_CFG_DDR_DPHY_PCLK =          0x2     // 0b10     // 2nd RMW
				  [4]   MBA_FARB5Q_CFG_DDR_RESETN =             1                   // 3rd RMW (optional (?), only if changes)
				  [5]   MBA_FARB5Q_CFG_CCS_ADDR_MUX_SEL =       1                   // 1st RMW (optional, only if changes)
				  [6]   MBA_FARB5Q_CFG_CCS_INST_RESET_ENABLE =  0                   // 1st RMW (optional, only if changes)
			*/
			mca_and_or(mcs_ids[mcs_i], mca_i, MBA_FARB5Q,
			           ~PPC_BIT(MBA_FARB5Q_CFG_CCS_INST_RESET_ENABLE),
			           PPC_BIT(MBA_FARB5Q_CFG_CCS_ADDR_MUX_SEL));
			mca_and_or(mcs_ids[mcs_i], mca_i, MBA_FARB5Q, ~PPC_BITMASK(0, 3),
			           PPC_SHIFT(0x6, MBA_FARB5Q_CFG_DDR_DPHY_PCLK));
			mca_and_or(mcs_ids[mcs_i], mca_i, MBA_FARB5Q, ~0,
			           PPC_BIT(MBA_FARB5Q_CFG_DDR_RESETN));

			udelay(500);  /* part of 3rd RMW, but delay is unconditional */
		}

		/*
		 * JEDEC, fig 7,8: delays above and below end at the same point, they
		 * are not consecutive. RDIMM spec says that clocks must be stable for
		 * 16nCK before RESET_n = 1. This is not explicitly ensured.
		 *
		 * Below seems unnecessary, we are starting clocks at the same time as
		 * deasserting reset (are we?)
		 */
		/*
		 * max(10ns, 5tCK), but for all DDR4 Speed Bins 10ns is bigger.
		 * coreboot API doesn't have enough precision anyway.
		 */
		udelay(1);

		/*
		 * draminit_cke_helper() is called only for the first functional MCA
		 * because CCS_ADDR_MUX_SEL is set.
		 */
		for (mca_i = 0; mca_i < MCA_PER_MCS; mca_i++) {
			if (mem_data.mcs[mcs_i].mca[mca_i].functional)
				break;
		}
		draminit_cke_helper(mcs_ids[mcs_i], mca_i);

		for (mca_i = 0; mca_i < MCA_PER_MCS; mca_i++) {
			mca_data_t *mca = &mem_data.mcs[mcs_i].mca[mca_i];

			if (!mca->functional)
				continue;

			/*
			 * "Per conversation with Shelton and Steve, turn off addr_mux_sel
			 * after the CKE CCS but before the RCD/MRS CCSs"
			 *
			 * Needs to be disabled for all MCAs before next instructions, hence
			 * separate loop.
			MC01.PORT0.SRQ.MBA_FARB5Q
			      [5]   MBA_FARB5Q_CFG_CCS_ADDR_MUX_SEL = 0
			 */
			mca_and_or(mcs_ids[mcs_i], mca_i, MBA_FARB5Q,
			           ~PPC_BIT(MBA_FARB5Q_CFG_CCS_ADDR_MUX_SEL), 0);
		}

		for (mca_i = 0; mca_i < MCA_PER_MCS; mca_i++) {
			mca_data_t *mca = &mem_data.mcs[mcs_i].mca[mca_i];

			if (!mca->functional)
				continue;

			for (dimm = 0; dimm < DIMMS_PER_MCA; dimm++) {
				if (!mca->dimm[dimm].present)
					continue;

				rcd_load(mca, dimm);
				// bcw_load();		/* LRDIMM only */
				mrs_load(mcs_i, mca_i, dimm);
				dump_rcd(SPD_I2C_BUS, mca->dimm[dimm].rcd_i2c_addr);
			}
		}
	}

	printk(BIOS_EMERG, "ending istep 13.10\n");
}
