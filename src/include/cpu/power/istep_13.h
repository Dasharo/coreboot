/* SPDX-License-Identifier: GPL-2.0-only */

#include <arch/io.h>
#include <device/dram/ddr4.h>
#include <spd_bin.h>
#include <delay.h>
#include <cpu/power/scom.h>

#define MCS_PER_PROC		2
#define MCA_PER_MCS		2
#define MCA_PER_PROC		(MCA_PER_MCS * MCS_PER_PROC)
#define DIMMS_PER_MCA		2
#define DIMMS_PER_MCS		(DIMMS_PER_MCA * MCA_PER_MCS)

/* These should be in one of the SPD headers. */
/*
 * Note: code in 13.3 depends on width/density having values as encoded in SPD
 * and below. Please do not change them.
 */
#define WIDTH_x4		0
#define WIDTH_x8		1

#define DENSITY_256Mb		0
#define DENSITY_512Mb		1
#define DENSITY_1Gb		2
#define DENSITY_2Gb		3
#define DENSITY_4Gb		4
#define DENSITY_8Gb		5
#define DENSITY_16Gb		6
#define DENSITY_32Gb		7

#define PSEC_PER_NSEC		1000
#define PSEC_PER_USEC		1000000

/* Values are the same across all supported speed bins */
static const int tMRD = 8;
static const int tMOD = 24;
static const int tZQinit = 1024;

typedef struct {
	bool present;
	uint8_t mranks;
	uint8_t log_ranks;	// In total, not per mrank
	uint8_t width;
	uint8_t density;
	uint8_t *spd;
	uint8_t rcd_i2c_addr;
	uint16_t size_gb;	// 2S8Rx4 8Gb DIMMs are 256GB
} rdimm_data_t;

typedef struct {
	bool functional;
	rdimm_data_t dimm[DIMMS_PER_MCA];

	/*
	 * The following fields are read and/or calculated from SPD obtained
	 * from DIMMs, but they are here because we can only set them per
	 * MCA/port/channel and not per DIMM. All units are clock cycles,
	 * absolute time values are rarely used.
	 */
	uint16_t nfaw;
	uint16_t nras;
	uint16_t nrfc;
	uint16_t nrfc_dlr;	// nRFC for Different Logical Rank (3DS only)
	uint8_t cl;
	uint8_t nccd_l;
	uint8_t nwtr_s;
	uint8_t nwtr_l;
	uint8_t nrcd;
	uint8_t nrp;
	uint8_t nwr;
	uint8_t nrrd_s;
	uint8_t nrrd_l;
} mca_data_t;

typedef struct {
	bool functional;
	mca_data_t mca[MCA_PER_MCS];
} mcs_data_t;

typedef struct {
	/* Do we need 'bool functional' here as well? */
	mcs_data_t mcs[MCS_PER_PROC];

	/*
	 * Unclear whether we can have different speeds between MCSs.
	 * Documentation says we can, but ring ID in 13.3 is sent per MCBIST.
	 * ATTR_MSS_FREQ is defined for SYSTEM target type, implying only one
	 * speed for whole platform.
	 *
	 * FIXME: maybe these should be in mcs_data_t and 13.3 should send
	 * a second Ring ID for the second MCS. How to test it?
	 */
	uint16_t speed;	// MT/s
	/*
	 * These depend just on memory frequency (and specification), and even
	 * though they describe DRAM/DIMM/MCA settings, there is no need to have
	 * multiple copies of identical data.
	 */
	uint16_t nrefi;	// 7.8 us in normal temperature range (0-85 deg Celsius)
	uint8_t cwl;
	uint8_t nrtp;	// max(4 nCK, 7.5 ns) = 7.5 ns for every supported speed
} mcbist_data_t;

extern mcbist_data_t mem_data;
static const chiplet_id_t mcs_ids[MCS_PER_PROC] = {MC01_CHIPLET_ID, MC23_CHIPLET_ID};

/*
 * All time conversion functions assume that both MCSs have the same frequency.
 * Change it if proven otherwise by adding a second argument - memory speed or
 * MCS index.
 *
 * These functions should not be used before setting mem_data.speed to a valid
 * non-0 value.
 */
static inline uint64_t tck_in_ps(void)
{
	/*
	 * Speed is in MT/s, we need to divide it by 2 to get MHz.
	 * tCK(avg) should be rounded down to the next valid speed bin, which
	 * corresponds to value obtained by using standardized MT/s values.
	 */
	return 1000000 / (mem_data.speed / 2);
}

static inline uint64_t ps_to_nck(uint64_t ps)
{
	/* Algorithm taken from JEDEC Standard No. 21-C */
	return ((ps * 1000 / tck_in_ps()) + 974) / 1000;
}

static inline uint64_t mtb_ftb_to_nck(uint64_t mtb, int8_t ftb)
{
	/* ftb is signed (always byte?) */
	return ps_to_nck(mtb * 125 + ftb);
}

static inline uint64_t ns_to_nck(uint64_t ns)
{
	return ps_to_nck(ns * PSEC_PER_NSEC);
}

static inline uint64_t nck_to_ps(uint64_t nck)
{
	return nck * tck_in_ps();
}

/*
 * To be used in delays, so always round up.
 *
 * Microsecond is the best precision exposed by coreboot API. tCK is somewhere
 * around 1 ns, so most smaller delays will be rounded up to 1 us. For better
 * resolution we would have to read TBR (Time Base Register) directly.
 */
static inline uint64_t nck_to_us(uint64_t nck)
{
	return (nck_to_ps(nck) + PSEC_PER_USEC - 1) / PSEC_PER_USEC;
}

static inline void delay_nck(uint64_t nck)
{
	udelay(nck_to_us(nck));
}

/* TODO: discover which MCAs are used on second MCS (0,1,6,7? 0,1,4,5?) */
/* TODO: consider non-RMW variants */
static inline void mca_and_or(chiplet_id_t mcs, int mca, uint64_t scom,
                              uint64_t and, uint64_t or)
{
	/*
	 * Indirect registers have different stride than the direct ones in
	 * general, except for (only?) direct PHY registers.
	 */
	unsigned mul = (scom & PPC_BIT(0) ||
	                (scom & 0xFFFFF000) == 0x07011000) ? 0x400 : 0x40;
	scom_and_or_for_chiplet(mcs, scom + mca * mul, and, or);
}

static inline void dp_mca_and_or(chiplet_id_t mcs, int dp, int mca,
                                 uint64_t scom, uint64_t and, uint64_t or)
{
	mca_and_or(mcs, mca, scom + dp * 0x40000000000, and, or);
}

static inline uint64_t mca_read(chiplet_id_t mcs, int mca, uint64_t scom)
{
	/* Indirect registers have different stride than the direct ones in
	 * general, except for (only?) direct PHY registers. */
	unsigned mul = (scom & PPC_BIT(0) ||
	                (scom & 0xFFFFF000) == 0x07011000) ? 0x400 : 0x40;
	return read_scom_for_chiplet(mcs, scom + mca * mul);
}

static inline void mca_write(chiplet_id_t mcs, int mca, uint64_t scom, uint64_t val)
{
	/* Indirect registers have different stride than the direct ones in
	 * general, except for (only?) direct PHY registers. */
	unsigned mul = (scom & PPC_BIT(0) ||
	                (scom & 0xFFFFF000) == 0x07011000) ? 0x400 : 0x40;
	write_scom_for_chiplet(mcs, scom + mca * mul, val);
}
static inline uint64_t dp_mca_read(chiplet_id_t mcs, int dp, int mca, uint64_t scom)
{
	return mca_read(mcs, mca, scom + dp * 0x40000000000);
}

enum rank_selection {
	NO_RANKS =		0,
	DIMM0_RANK0 =		1 << 0,
	DIMM0_RANK1 =		1 << 1,
	DIMM0_ALL_RANKS = 	DIMM0_RANK0 | DIMM0_RANK1,
	DIMM1_RANK0 =		1 << 2,
	DIMM1_RANK1 =		1 << 3,
	DIMM1_ALL_RANKS = 	DIMM1_RANK0 | DIMM1_RANK1,
	BOTH_DIMMS_1R =	DIMM0_RANK0 | DIMM1_RANK0,
	BOTH_DIMMS_2R =	DIMM0_ALL_RANKS | DIMM1_ALL_RANKS
};

enum cal_config {
	CAL_WR_LEVEL =			PPC_BIT(48),
	CAL_INITIAL_PAT_WR =		PPC_BIT(49),
	CAL_DQS_ALIGN =		PPC_BIT(50),
	CAL_RDCLK_ALIGN =		PPC_BIT(51),
	CAL_READ_CTR =			PPC_BIT(52),
	CAL_WRITE_CTR =		PPC_BIT(53),
	CAL_INITIAL_COARSE_WR =	PPC_BIT(54),
	CAL_COARSE_RD =		PPC_BIT(55),
	CAL_CUSTOM_RD =		PPC_BIT(56),
	CAL_CUSTOM_WR =		PPC_BIT(57)
};

void ccs_add_instruction(chiplet_id_t id, mrs_cmd_t mrs, uint8_t csn,
                         uint8_t cke, uint16_t idles);
void ccs_add_mrs(chiplet_id_t id, mrs_cmd_t mrs, enum rank_selection ranks,
                 int mirror, uint16_t idles);
void ccs_phy_hw_step(chiplet_id_t id, int mca_i, int rp, enum cal_config conf,
                     uint64_t step_cycles);
void ccs_execute(chiplet_id_t id, int mca_i);

static inline enum ddr4_mr5_rtt_park vpd_to_rtt_park(uint8_t vpd)
{
	/* Fun fact: this is 240/vpd with bit order reversed */
	switch (vpd) {
		case 34:
			return DDR4_MR5_RTT_PARK_RZQ_7;
		case 40:
			return DDR4_MR5_RTT_PARK_RZQ_6;
		case 48:
			return DDR4_MR5_RTT_PARK_RZQ_5;
		case 60:
			return DDR4_MR5_RTT_PARK_RZQ_4;
		case 80:
			return DDR4_MR5_RTT_PARK_RZQ_3;
		case 120:
			return DDR4_MR5_RTT_PARK_RZQ_2;
		case 240:
			return DDR4_MR5_RTT_PARK_RZQ_1;
		default:
			return DDR4_MR5_RTT_PARK_OFF;
	}
}

static inline enum ddr4_mr2_rtt_wr vpd_to_rtt_wr(uint8_t vpd)
{
	switch (vpd) {
		case 0:
			return DDR4_MR2_RTT_WR_OFF;
		case 80:
			return DDR4_MR2_RTT_WR_RZQ_3;
		case 120:
			return DDR4_MR2_RTT_WR_RZQ_2;
		case 240:
			return DDR4_MR2_RTT_WR_RZQ_1;
		default:
			/* High-Z is 1 in VPD */
			return DDR4_MR2_RTT_WR_HI_Z;
	}
}

static inline enum ddr4_mr1_rtt_nom vpd_to_rtt_nom(uint8_t vpd)
{
	/* Fun fact: this is 240/vpd with bit order reversed */
	switch (vpd) {
		case 34:
			return DDR4_MR1_RTT_NOM_RZQ_7;
		case 40:
			return DDR4_MR1_RTT_NOM_RZQ_6;
		case 48:
			return DDR4_MR1_RTT_NOM_RZQ_5;
		case 60:
			return DDR4_MR1_RTT_NOM_RZQ_4;
		case 80:
			return DDR4_MR1_RTT_NOM_RZQ_3;
		case 120:
			return DDR4_MR1_RTT_NOM_RZQ_2;
		case 240:
			return DDR4_MR1_RTT_NOM_RZQ_1;
		default:
			return DDR4_MR1_RTT_NOM_OFF;
	}
}

void istep_13_2(void);
void istep_13_3(void);
void istep_13_4(void);
void istep_13_6(void);
void istep_13_8(void);	// TODO: takes epsilon values from 8.6 and MSS data from 7.4
void istep_13_9(void);
void istep_13_10(void);
void istep_13_11(void);
void istep_13_13(void);
void istep_14_1(void);
void istep_14_5(void);
