#ifndef CPU_PPC64_VPD_DATA_H
#define CPU_PPC64_VPD_DATA_H

/* Memory rotator data */

/* FIXME: these can be updated by MVPD in istep 7.5. Values below (from MEMD)
 * are different than in documentation. */
extern uint8_t ATTR_MSS_VPD_MR_TSYS_ADR[4];
extern uint8_t ATTR_MSS_VPD_MR_TSYS_DATA[4];

/* This data is the same for all configurations */
extern uint8_t ATTR_MSS_VPD_MR_DPHY_GPO;
extern uint8_t ATTR_MSS_VPD_MR_DPHY_RLO;
extern uint8_t ATTR_MSS_VPD_MR_DPHY_WLO;
extern uint8_t ATTR_MSS_VPD_MR_MC_2N_MODE_AUTOSET;

/*
 * 43 tables for 43 signals. These probably are platform specific so in the
 * final version we should read this from VPD partition. Hardcoding it will make
 * one less possible fault point.
 *
 * Also, VPD layout may change. Right npw Talos uses first version of layout,
 * but there is a newer version with one additional field __in the middle__ of
 * the structure.
 *
 * Order:
 *  - J0 - PROC 0 MCS 0, 1 DIMM, 1866 MT/s
 *  - J1 - PROC 0 MCS 1, 1 DIMM, 1866 MT/s
 *  - J2 - PROC 1 MCS 0, 1 DIMM, 1866 MT/s
 *  - J3 - PROC 1 MCS 1, 1 DIMM, 1866 MT/s
 *  - J4 - PROC 0 MCS 0, 2 DIMMs, 1866 MT/s
 *  - J5
 *  - J6
 *  - J7 - PROC 1 MCS 1, 2 DIMMs, 1866 MT/s
 *  - J8 - PROC 0 MCS 0, 1 DIMM, 2133 MT/s
 *  - J9
 *  - JA
 *  - JB - PROC 1 MCS 1, 1 DIMM, 2133 MT/s
 *  - JC - PROC 0 MCS 0, 2 DIMMs, 2133 MT/s
 *  - JD
 *  - JE
 *  - JF - PROC 1 MCS 1, 2 DIMMs, 2133 MT/s
 *  - JG - PROC 0 MCS 0, 1 DIMM, 2400 MT/s
 *  - JH
 *  - JI
 *  - JJ - PROC 1 MCS 1, 1 DIMM, 2400 MT/s
 *  - JK - PROC 0 MCS 0, 2 DIMMs, 2400 MT/s
 *  - JL
 *  - JM
 *  - JN - PROC 1 MCS 1, 2 DIMMs, 2400 MT/s
 *  - JO - PROC 0 MCS 0, 1 DIMM, 2666 MT/s
 *  - JP
 *  - JQ
 *  - JR - PROC 1 MCS 1, 1 DIMM, 2666 MT/s
 *
 * 2 DIMMs, 2666 MT/s is not supported (this is ensured by prepare_dimm_data()).
 */
extern uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D0_CSN0[28][MCA_PER_MCS];
extern uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_CMD_ADDR_WEN_A14[28][MCA_PER_MCS];
extern uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D1_ODT1[28][MCA_PER_MCS];
extern uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_C0[28][MCA_PER_MCS];
extern uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_BA1[28][MCA_PER_MCS];
extern uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A10[28][MCA_PER_MCS];
extern uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D0_ODT1[28][MCA_PER_MCS];
extern uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_BA0[28][MCA_PER_MCS];
extern uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A00[28][MCA_PER_MCS];
extern uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D1_ODT0[28][MCA_PER_MCS];
extern uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D0_ODT0[28][MCA_PER_MCS];
extern uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_CMD_ADDR_CASN_A15[28][MCA_PER_MCS];
extern uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A13[28][MCA_PER_MCS];
extern uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D0_CSN1[28][MCA_PER_MCS];
extern uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_D0_CLKN[28][MCA_PER_MCS];
extern uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_D0_CLKP[28][MCA_PER_MCS];
extern uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A17[28][MCA_PER_MCS];
extern uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_C1[28][MCA_PER_MCS];
extern uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_D1_CLKN[28][MCA_PER_MCS];
extern uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_D1_CLKP[28][MCA_PER_MCS];
extern uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_C2[28][MCA_PER_MCS];
extern uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D1_CSN1[28][MCA_PER_MCS];
extern uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A02[28][MCA_PER_MCS];
extern uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_CMD_PAR[28][MCA_PER_MCS];
extern uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D1_CSN0[28][MCA_PER_MCS];
extern uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_CMD_ADDR_RASN_A16[28][MCA_PER_MCS];
extern uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A08[28][MCA_PER_MCS];
extern uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A05[28][MCA_PER_MCS];
extern uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A03[28][MCA_PER_MCS];
extern uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A01[28][MCA_PER_MCS];
extern uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A04[28][MCA_PER_MCS];
extern uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A07[28][MCA_PER_MCS];
extern uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A09[28][MCA_PER_MCS];
extern uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A06[28][MCA_PER_MCS];
extern uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D0_CKE1[28][MCA_PER_MCS];
extern uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A12[28][MCA_PER_MCS];
extern uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_CMD_ACTN[28][MCA_PER_MCS];
extern uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A11[28][MCA_PER_MCS];
extern uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_BG0[28][MCA_PER_MCS];
extern uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D0_CKE0[28][MCA_PER_MCS];
extern uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D1_CKE1[28][MCA_PER_MCS];
extern uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_BG1[28][MCA_PER_MCS];
extern uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D1_CKE0[28][MCA_PER_MCS];

/* End of rotator data */

/* Memory terminator data */

/*
 * VPD has per rank settings, but both ranks (if present) are the same.  Order:
 *  - 1R in DIMM0 and no DIMM1
 *  - 1R in both DIMMs
 *  - 2R in DIMM0 and no DIMM1
 *  - 2R in both DIMMs
 */
extern uint32_t ATTR_MSS_VPD_MT_VREF_MC_RD[4];
extern uint8_t ATTR_MSS_VPD_MT_VREF_DRAM_WR[4];
extern uint8_t ATTR_MSS_VPD_MT_DRAM_RTT_PARK[4];
extern uint8_t ATTR_MSS_VPD_MT_DRAM_RTT_WR[4];
extern uint8_t ATTR_MSS_VPD_MT_DRAM_RTT_NOM[4];

/*
 * Warning: this is not a 1:1 copy from VPD.
 *
 * VPD uses uint8_t [2][2][4] table, indexed as [MCA][DIMM][RANK]. It tries to
 * be generic, but for RDIMMs only 2 ranks are supported. This format also
 * allows for different settings across MCAs, but in Talos they are identical.
 *
 * Tables below are uint8_t [4][2][2], indexed as [rank config.][DIMM][RANK].
 *
 * There are 4 rank configurations, see comments in ATTR_MSS_VPD_MT_VREF_MC_RD.
 */
extern uint8_t ATTR_MSS_VPD_MT_ODT_RD[4][2][2];
extern uint8_t ATTR_MSS_VPD_MT_ODT_WR[4][2][2];

/* This data is the same for all configurations */
extern uint8_t ATTR_MSS_VPD_MT_DRAM_DRV_IMP_DQ_DQS;
extern uint32_t ATTR_MSS_VPD_MT_MC_DQ_ACBOOST_RD_UP;
extern uint32_t ATTR_MSS_VPD_MT_MC_DQ_ACBOOST_WR_DOWN;
extern uint32_t ATTR_MSS_VPD_MT_MC_DQ_ACBOOST_WR_UP;
extern uint64_t ATTR_MSS_VPD_MT_MC_DQ_CTLE_CAP;
extern uint64_t ATTR_MSS_VPD_MT_MC_DQ_CTLE_RES;
extern uint8_t ATTR_MSS_VPD_MT_MC_DRV_IMP_CLK;
extern uint8_t ATTR_MSS_VPD_MT_MC_DRV_IMP_CMD_ADDR;
extern uint8_t ATTR_MSS_VPD_MT_MC_DRV_IMP_CNTL;
extern uint8_t ATTR_MSS_VPD_MT_MC_DRV_IMP_CSCID;
extern uint8_t ATTR_MSS_VPD_MT_MC_DRV_IMP_DQ_DQS;
extern uint8_t ATTR_MSS_VPD_MT_MC_RCV_IMP_DQ_DQS;
extern uint8_t ATTR_MSS_VPD_MT_PREAMBLE;
extern uint16_t ATTR_MSS_VPD_MT_WINDAGE_RD_CTR;

#endif /* CPU_PPC64_VPD_DATA_H */
