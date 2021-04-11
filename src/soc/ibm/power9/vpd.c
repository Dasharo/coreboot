/* SPDX-License-Identifier: GPL-2.0-only */

#include <cpu/power/vpd.h>

#include <assert.h>
#include <arch/io.h>
#include <commonlib/region.h>
#include <console/console.h>
#include <cpu/power/memd.h>
#include <endian.h>
#include <string.h>

#include "../../../../3rdparty/ffs/ffs/ffs.h"

/* Properly rounded size of MEMD header */
#define MEMD_HDR_SIZE ALIGN(sizeof(struct memd_hdr), 16)
/* Divisor used for section size in MEMD header */
#define MEMD_SECTION_ROUNDING_DIVISOR 1000

#define VPD_RECORD_NAME_LEN  4
#define VPD_RECORD_SIZE_LEN  2
#define VPD_KWD_NAME_LEN 2

/* Supported mapping layout version */
#define VPD_MAPPING_VERSION    1
/* Size of entries in MR and MT mappings */
#define VPD_MAPPING_ENTRY_SIZE 6

/*
 * Structure of nesting:
 *  - MEMD
 *    - VPD blob
 *      - VPD keyword (VPD name, keyword mapping, attributes, something else)
 *
 * Either part of VDP record or of MEMD header (depending on the source):
 *  - 11 bytes  -- ECC (unimplemented, should be ignored)
 *  - 0x84 byte -- resource type (this byte is missing from PNOR image) (opt)
 *
 * VPD Record (this is a part of binary VPD which is stored in .rvpd-files):
 *  - 2 bytes    -- size of the record's data (>= 40)
 *  - RT keyword -- always the first record with 4 bytes of data
 *  - other keywords (as many as data size allows)
 *  - PF keyword -- padding, always present
 *  - 0x78 byte  -- closing resource type
 *
 * Keyword:
 *  - 2 bytes      -- keyword name
 *  - 1 or 2 bytes -- keyword's data size (little endian)
 *                    (2 bytes if first char of keyword name is #)
 *  - N bytes      -- N == data size
 *
 * Minimal record size is 40 bytes.  If it exceeds that, padding aligned on word
 * boundary (where a word is 4 bytes long).
 *
 * Format of MR, MT, Q0 and CK keywords that provide mapping:
 *  - Header:
 *    - 1 byte -- version
 *    - 1 byte -- entry count
 *    - 1 byte -- entry size in bytes (only for Q0 and CK keywords)
 *    - 1 byte -- reserved
 *  - Entry (0xff value in first 5 fields means "matches everything"):
 *    - 1 byte -- mcs mask (high byte)
 *    - 1 byte -- mcs mask (low byte)
 *    - 1 byte -- rank mask (high byte)
 *    - 1 byte -- rank mask (low byte)
 *    - 1 byte -- frequency mask
 *                0x80 - 1866
 *                0x40 - 2133
 *                0x20 - 2400
 *                0x10 - 2666
 *    - 1 byte -- kw ([0-9A-Z])
 *                0x00 for row after last
 *                0xff for unsupported configuration
 *
 * Glossary:
 *  - MR keyword       -- the mapping
 *  - MR configuration -- one of J#, X#, etc.
 *
 * See the following sources in talos-hostboot:
 *  - src/import/chips/p9/procedures/hwp/accessors/p9_get_mem_vpd_keyword.C
 *  - src/import/chips/p9/procedures/hwp/accessors/p9_get_mem_vpd_keyword.H
 *  - src/import/chips/p9/procedures/hwp/memory/lib/dimm/eff_dimm.C
 *  - src/import/chips/p9/procedures/hwp/memory/lib/mss_vpd_decoder.H
 *  - src/usr/fapi2/test/getVpdTest.C
 */

/* Size of this structure should be rounded to 16 bytes */
struct memd_hdr {
	char eyecatch[4];	// Magic number to determine validity "OKOK"
	char header_version[4]; // Version of this header
	char memd_version[4];	// Version of the MEMD payload
	uint32_t section_size;	// <max MEMD instance size in bytes>/1000 + 1
	uint16_t section_count; // Number of MEMD instances
	char reserved[8];	// Reserved bytes
} __attribute__((packed));

/* Combines pointer to VPD area with configuration information */
struct vpd_info {
	const uint8_t *data; // VPD area pointer
	int mcs_i;           // MCS position (spans CPUs)
	int freq;            // Frequency in MHz
	int dimm0_rank;
	int dimm1_rank;
};

/* Memory terminator data */

uint8_t ATTR_MSS_VPD_MT_DRAM_RTT_NOM[4];
uint8_t ATTR_MSS_VPD_MT_DRAM_RTT_PARK[4];
uint8_t ATTR_MSS_VPD_MT_DRAM_RTT_WR[4];

uint8_t ATTR_MSS_VPD_MT_ODT_RD[4][2][2];
uint8_t ATTR_MSS_VPD_MT_ODT_WR[4][2][2];

uint8_t ATTR_MSS_VPD_MT_VREF_DRAM_WR[4];
uint32_t ATTR_MSS_VPD_MT_VREF_MC_RD[4];

uint8_t ATTR_MSS_VPD_MT_DRAM_DRV_IMP_DQ_DQS;
uint32_t ATTR_MSS_VPD_MT_MC_DQ_ACBOOST_RD_UP;
uint32_t ATTR_MSS_VPD_MT_MC_DQ_ACBOOST_WR_DOWN;
uint32_t ATTR_MSS_VPD_MT_MC_DQ_ACBOOST_WR_UP;
uint64_t ATTR_MSS_VPD_MT_MC_DQ_CTLE_CAP;
uint64_t ATTR_MSS_VPD_MT_MC_DQ_CTLE_RES;
uint8_t ATTR_MSS_VPD_MT_MC_DRV_IMP_CLK;
uint8_t ATTR_MSS_VPD_MT_MC_DRV_IMP_CMD_ADDR;
uint8_t ATTR_MSS_VPD_MT_MC_DRV_IMP_CNTL;
uint8_t ATTR_MSS_VPD_MT_MC_DRV_IMP_CSCID;
uint8_t ATTR_MSS_VPD_MT_MC_DRV_IMP_DQ_DQS;
uint8_t ATTR_MSS_VPD_MT_MC_RCV_IMP_DQ_DQS;
uint8_t ATTR_MSS_VPD_MT_PREAMBLE;
uint16_t ATTR_MSS_VPD_MT_WINDAGE_RD_CTR;

/* End of terminator data */

/* Memory rotator data */

uint8_t ATTR_MSS_VPD_MR_DPHY_GPO;
uint8_t ATTR_MSS_VPD_MR_DPHY_RLO;
uint8_t ATTR_MSS_VPD_MR_DPHY_WLO;
uint8_t ATTR_MSS_VPD_MR_MC_2N_MODE_AUTOSET;

uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A00[28][2];
uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A01[28][2];
uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A02[28][2];
uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A03[28][2];
uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A04[28][2];
uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A05[28][2];
uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A06[28][2];
uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A07[28][2];
uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A08[28][2];
uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A09[28][2];
uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A10[28][2];
uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A11[28][2];
uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A12[28][2];
uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A13[28][2];
uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A17[28][2];
uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_BA0[28][2];
uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_BA1[28][2];
uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_BG0[28][2];
uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_BG1[28][2];
uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_C0[28][2];
uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_C1[28][2];
uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_C2[28][2];

uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_CMD_ACTN[28][2];
uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_CMD_ADDR_CASN_A15[28][2];
uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_CMD_ADDR_RASN_A16[28][2];
uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_CMD_ADDR_WEN_A14[28][2];
uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_CMD_PAR[28][2];

uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D0_CKE0[28][2];
uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D0_CKE1[28][2];
uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D0_CSN0[28][2];
uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D0_CSN1[28][2];
uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D0_ODT0[28][2];
uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D0_ODT1[28][2];
uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D1_CKE0[28][2];
uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D1_CKE1[28][2];
uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D1_CSN0[28][2];
uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D1_CSN1[28][2];
uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D1_ODT0[28][2];
uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D1_ODT1[28][2];

uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_D0_CLKN[28][2];
uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_D0_CLKP[28][2];
uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_D1_CLKN[28][2];
uint8_t ATTR_MSS_VPD_MR_MC_PHASE_ROT_D1_CLKP[28][2];

uint8_t ATTR_MSS_VPD_MR_TSYS_ADR[4];
uint8_t ATTR_MSS_VPD_MR_TSYS_DATA[4];

/* End of rotator data */

/* Looks up an entry matching specified configuration in an MT or MR mapping.
 * Returns a character or '\0' on lookup failure. */
static char mapping_lookup(const struct vpd_info *vpd,
			   const uint8_t *mapping,
			   size_t size)
{
	int i = 0;
	int entry_count = 0;
	int offset = 0;
	uint16_t mcs_mask = 0;
	uint16_t freq_mask = 0;
	uint16_t rank_mask = 0;

	/* Mapping header size */
	if (size < 3)
		die("Mapping is too small!\n");
	offset = 3;

	if (mapping[0] != VPD_MAPPING_VERSION)
		die("Unsupported mapping version!\n");

	/* 0x8000 is a mask for MCS #0 */
	assert(vpd->mcs_i >= 0 && vpd->mcs_i <= 15);
	mcs_mask = 0x8000 >> vpd->mcs_i;

	/* (0, 0) -> 0x8000; (0, 1) -> 0x4000; ...; (1, 0) -> 0x0800; ... */
	assert(vpd->dimm0_rank >= 0 && vpd->dimm0_rank <= 3);
	assert(vpd->dimm1_rank >= 0 && vpd->dimm1_rank <= 3);
	rank_mask = 0x8000 >> (vpd->dimm0_rank*4 + vpd->dimm1_rank);

	switch (vpd->freq) {
	case 1866:
		freq_mask = 0x80;
		break;
	case 2133:
		freq_mask = 0x40;
		break;
	case 2400:
		freq_mask = 0x20;
		break;
	case 2666:
		freq_mask = 0x10;
		break;
	default:
		die("Unhandled frequency value: %d\n", vpd->freq);
		break;
	}

	entry_count = mapping[1];
	for (i = 0; i < entry_count; ++i, offset += VPD_MAPPING_ENTRY_SIZE) {
		const uint16_t mcs_mask_value =
			(mapping[offset + 0] << 8) | mapping[offset + 1];
		const uint16_t rank_mask_value =
			(mapping[offset + 2] << 8) | mapping[offset + 3];

		/* Data ended sooner than expected */
		if (mapping[offset + 5] == 0x00)
			continue;

		if ((mcs_mask_value & mcs_mask) != mcs_mask)
			continue;
		if ((rank_mask_value & rank_mask) != rank_mask)
			continue;
		if ((mapping[offset + 4] & freq_mask) != freq_mask)
			continue;

		return mapping[offset + 5];
	}

	return '\0';
}

/* Finds a keyword by its name. Retrieves its size too. Returns NULL on
 * failure. */
static const uint8_t *find_vpd_kwd(const struct vpd_info *vpd, const char *name,
				   size_t *size)
{
	const uint8_t *data = vpd->data;

	size_t offset = 0;
	uint16_t record_size = 0;

	if (strlen(name) != VPD_KWD_NAME_LEN)
		die("Keyword name has wrong length!\n");

	memcpy(&record_size, &data[offset], sizeof(record_size));
	offset += VPD_RECORD_SIZE_LEN;
	record_size = le16toh(record_size);

	/* Skip mandatory "RT" and one byte of data size (always 4) */
	offset += VPD_KWD_NAME_LEN + 1;

	if (memcmp(&data[offset], "MEMD", VPD_RECORD_NAME_LEN))
		die("Failed to find MEMD record!\n");
	offset += VPD_RECORD_NAME_LEN;

	while (offset < record_size) {
		uint16_t kwd_size = 0;
		bool match = false;
		const int two_byte_size = (data[offset] == '#');

		/* This is always the last keyword */
		if (!memcmp(&data[offset], "PF", VPD_KWD_NAME_LEN))
			break;

		match = (!memcmp(&data[offset], name, VPD_KWD_NAME_LEN));

		offset += VPD_KWD_NAME_LEN;

		if (two_byte_size) {
			memcpy(&kwd_size, &data[offset], sizeof(kwd_size));
			kwd_size = le16toh(kwd_size);
			offset += 2;
		} else {
			kwd_size = data[offset];
			offset += 1;
		}

		if (match) {
			*size = kwd_size;
			return &data[offset];
		}

		offset += kwd_size;
	}

	return NULL;
}

/* Looks up configuration in specified mapping and loads it or dies */
static const uint8_t *find_vpd_conf(const struct vpd_info *vpd,
				    const char *mapping_name,
				    size_t *size)
{
	const uint8_t *mapping = NULL;
	const uint8_t *conf = NULL;
	size_t kwd_size = 0;

	char conf_name[3] = {};

	if (!strcmp(mapping_name, "MR")) {
		conf_name[0] = 'J';
	} else if (!strcmp(mapping_name, "MT")) {
		conf_name[0] = 'X';
	} else {
		die("Unsupported %s mapping type\n", mapping_name);
	}

	mapping = find_vpd_kwd(vpd, mapping_name, &kwd_size);
	if (!mapping)
		die("VPD is missing %s keyword!\n", mapping_name);

	conf_name[1] = mapping_lookup(vpd, mapping, kwd_size);
	if (!conf_name[1])
		die("Failed to find matching %s configuration!\n",
		    mapping_name);

	conf = find_vpd_kwd(vpd, conf_name, &kwd_size);
	if (!conf)
		die("Failed to read %s configuration!\n", mapping_name);

	*size = kwd_size;
	return conf;
}

static void load_mt_attrs(const uint8_t *mt_conf, size_t size, int vpd_idx)
{
	uint8_t version_layout;
	uint8_t version_data;

	if (size < 2)
		die("MT configuration is way too small!\n");

	version_layout = mt_conf[0];
	version_data = mt_conf[1];

	if (version_layout > 1)
		die("Unsupported layout of MT configuration!\n");

	if (size < 218)
		die("MT configuration is smaller than expected!\n");

	ATTR_MSS_VPD_MT_DRAM_RTT_NOM[vpd_idx] = mt_conf[38];
	ATTR_MSS_VPD_MT_DRAM_RTT_PARK[vpd_idx] = mt_conf[54];
	ATTR_MSS_VPD_MT_DRAM_RTT_WR[vpd_idx] = mt_conf[70];

	switch (version_layout) {
	case 0:
		memcpy(&ATTR_MSS_VPD_MT_ODT_RD[vpd_idx][0][0], &mt_conf[170],
		       2);
		memcpy(&ATTR_MSS_VPD_MT_ODT_RD[vpd_idx][1][0], &mt_conf[174],
		       2);
		memcpy(&ATTR_MSS_VPD_MT_ODT_WR[vpd_idx][0][0], &mt_conf[186],
		       2);
		memcpy(&ATTR_MSS_VPD_MT_ODT_WR[vpd_idx][1][0], &mt_conf[190],
		       2);
		ATTR_MSS_VPD_MT_VREF_DRAM_WR[vpd_idx] = mt_conf[204];
		memcpy(&ATTR_MSS_VPD_MT_VREF_MC_RD[vpd_idx], &mt_conf[206], 4);

		/* The following data is the same for all configurations */
		if (vpd_idx == 0) {
			memcpy(&ATTR_MSS_VPD_MT_MC_DQ_ACBOOST_RD_UP,
			       &mt_conf[86], 4);
			memcpy(&ATTR_MSS_VPD_MT_MC_DQ_ACBOOST_WR_DOWN,
			       &mt_conf[94], 4);
			memcpy(&ATTR_MSS_VPD_MT_MC_DQ_ACBOOST_WR_UP,
			       &mt_conf[102], 4);

			memcpy(&ATTR_MSS_VPD_MT_MC_DQ_CTLE_CAP, &mt_conf[110],
			       8);
			memcpy(&ATTR_MSS_VPD_MT_MC_DQ_CTLE_RES, &mt_conf[126],
			       8);

			ATTR_MSS_VPD_MT_MC_DRV_IMP_CLK = mt_conf[142];
			ATTR_MSS_VPD_MT_MC_DRV_IMP_CMD_ADDR = mt_conf[144];
			ATTR_MSS_VPD_MT_MC_DRV_IMP_CNTL = mt_conf[146];
			ATTR_MSS_VPD_MT_MC_DRV_IMP_CSCID = mt_conf[148];

			ATTR_MSS_VPD_MT_MC_DRV_IMP_DQ_DQS = mt_conf[150];
			ATTR_MSS_VPD_MT_MC_RCV_IMP_DQ_DQS = mt_conf[160];

			memcpy(&ATTR_MSS_VPD_MT_PREAMBLE, &mt_conf[202], 2);

			memcpy(&ATTR_MSS_VPD_MT_WINDAGE_RD_CTR, &mt_conf[214],
			       2);
		}
		break;
	case 1:
		memcpy(&ATTR_MSS_VPD_MT_ODT_RD[vpd_idx][0][0], &mt_conf[172],
		       2);
		memcpy(&ATTR_MSS_VPD_MT_ODT_RD[vpd_idx][1][0], &mt_conf[176],
		       2);
		memcpy(&ATTR_MSS_VPD_MT_ODT_WR[vpd_idx][0][0], &mt_conf[188],
		       2);
		memcpy(&ATTR_MSS_VPD_MT_ODT_WR[vpd_idx][1][0], &mt_conf[192],
		       2);
		ATTR_MSS_VPD_MT_VREF_DRAM_WR[vpd_idx] = mt_conf[206];
		memcpy(&ATTR_MSS_VPD_MT_VREF_MC_RD[vpd_idx], &mt_conf[208], 4);

		/* The following data is the same for all configurations */
		if (vpd_idx == 0) {
			memcpy(&ATTR_MSS_VPD_MT_MC_DQ_ACBOOST_RD_UP,
			       &mt_conf[88], 4);
			memcpy(&ATTR_MSS_VPD_MT_MC_DQ_ACBOOST_WR_DOWN,
			       &mt_conf[96], 4);
			memcpy(&ATTR_MSS_VPD_MT_MC_DQ_ACBOOST_WR_UP,
			       &mt_conf[104], 4);

			memcpy(&ATTR_MSS_VPD_MT_MC_DQ_CTLE_CAP, &mt_conf[112],
			       8);
			memcpy(&ATTR_MSS_VPD_MT_MC_DQ_CTLE_RES, &mt_conf[128],
			       8);

			ATTR_MSS_VPD_MT_MC_DRV_IMP_CLK = mt_conf[144];
			ATTR_MSS_VPD_MT_MC_DRV_IMP_CMD_ADDR = mt_conf[146];
			ATTR_MSS_VPD_MT_MC_DRV_IMP_CNTL = mt_conf[148];
			ATTR_MSS_VPD_MT_MC_DRV_IMP_CSCID = mt_conf[150];

			ATTR_MSS_VPD_MT_MC_DRV_IMP_DQ_DQS = mt_conf[152];
			ATTR_MSS_VPD_MT_MC_RCV_IMP_DQ_DQS = mt_conf[162];

			ATTR_MSS_VPD_MT_PREAMBLE = mt_conf[204];

			memcpy(&ATTR_MSS_VPD_MT_WINDAGE_RD_CTR, &mt_conf[216],
			       2);
		}
		break;
	}

	ATTR_MSS_VPD_MT_VREF_MC_RD[vpd_idx] =
		be32toh(ATTR_MSS_VPD_MT_VREF_MC_RD[vpd_idx]);

	/* The following data is the same for all configurations */
	if (vpd_idx == 0) {
		ATTR_MSS_VPD_MT_DRAM_DRV_IMP_DQ_DQS = mt_conf[22];

		ATTR_MSS_VPD_MT_MC_DQ_ACBOOST_RD_UP =
			be32toh(ATTR_MSS_VPD_MT_MC_DQ_ACBOOST_RD_UP);
		ATTR_MSS_VPD_MT_MC_DQ_ACBOOST_WR_DOWN =
			be32toh(ATTR_MSS_VPD_MT_MC_DQ_ACBOOST_WR_DOWN);
		ATTR_MSS_VPD_MT_MC_DQ_ACBOOST_WR_UP =
			be32toh(ATTR_MSS_VPD_MT_MC_DQ_ACBOOST_WR_UP);

		ATTR_MSS_VPD_MT_MC_DQ_CTLE_CAP =
			be64toh(ATTR_MSS_VPD_MT_MC_DQ_CTLE_CAP);
		ATTR_MSS_VPD_MT_MC_DQ_CTLE_RES =
			be64toh(ATTR_MSS_VPD_MT_MC_DQ_CTLE_RES);

		ATTR_MSS_VPD_MT_WINDAGE_RD_CTR =
			be16toh(ATTR_MSS_VPD_MT_WINDAGE_RD_CTR);
	}
}

static void load_mt(const uint8_t *vpd_data)
{
	int vpd_idx = 0;

	/* Assuming that data differs only per DIMM pairs */
	for (vpd_idx = 0; vpd_idx < 4; ++vpd_idx) {
		const int dimm0_rank = 1 + vpd_idx/2;
		const int dimm1_rank = (vpd_idx%2 ? dimm0_rank : 0);

		struct vpd_info vpd = {
			.data = vpd_data,
			.mcs_i = 0,
			.freq = 1866,
			.dimm0_rank = dimm0_rank,
			.dimm1_rank = dimm1_rank,
		};

		const uint8_t *mt_conf = NULL;
		size_t size = 0;

		mt_conf = find_vpd_conf(&vpd, "MT", &size);
		if (!mt_conf)
			die("Failed to read MT configuration!\n");

		load_mt_attrs(mt_conf, size, vpd_idx);
	}
}

static void load_mr_attrs(const uint8_t *mr_conf, size_t size, int vpd_idx)
{
	uint8_t version_layout;
	uint8_t version_data;

	if (size < 2)
		die("MR configuration is way too small!\n");

	version_layout = mr_conf[0];
	version_data = mr_conf[1];

	if (version_layout != 0)
		die("Unsupported layout of MR configuration!\n");

	if (size < 101)
		die("MR configuration is smaller than expected!\n");

	/* The following data is the same for all configurations */
	if (vpd_idx == 0) {
		ATTR_MSS_VPD_MR_DPHY_GPO = mr_conf[6];
		ATTR_MSS_VPD_MR_DPHY_RLO = mr_conf[8];
		ATTR_MSS_VPD_MR_DPHY_WLO = mr_conf[10];

		memcpy(ATTR_MSS_VPD_MR_MC_PHASE_ROT_D0_CLKN, &mr_conf[58], 2);
		memcpy(ATTR_MSS_VPD_MR_MC_PHASE_ROT_D0_CLKP, &mr_conf[56], 2);
		memcpy(ATTR_MSS_VPD_MR_MC_PHASE_ROT_D1_CLKN, &mr_conf[62], 2);
		memcpy(ATTR_MSS_VPD_MR_MC_PHASE_ROT_D1_CLKP, &mr_conf[60], 2);

		ATTR_MSS_VPD_MR_MC_2N_MODE_AUTOSET = mr_conf[98];
	}

	/* The following data changes per frequency */
	if (vpd_idx%8 == 0) {
		const int freq_i = vpd_idx/8;
		ATTR_MSS_VPD_MR_TSYS_ADR[freq_i] = mr_conf[99];
		ATTR_MSS_VPD_MR_TSYS_DATA[freq_i] = mr_conf[100];
	}

	memcpy(ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A00[vpd_idx], &mr_conf[12], 2);
	memcpy(ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A01[vpd_idx], &mr_conf[14], 2);
	memcpy(ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A02[vpd_idx], &mr_conf[16], 2);
	memcpy(ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A03[vpd_idx], &mr_conf[18], 2);
	memcpy(ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A04[vpd_idx], &mr_conf[20], 2);
	memcpy(ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A05[vpd_idx], &mr_conf[22], 2);
	memcpy(ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A06[vpd_idx], &mr_conf[24], 2);
	memcpy(ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A07[vpd_idx], &mr_conf[26], 2);
	memcpy(ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A08[vpd_idx], &mr_conf[28], 2);
	memcpy(ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A09[vpd_idx], &mr_conf[30], 2);
	memcpy(ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A10[vpd_idx], &mr_conf[32], 2);
	memcpy(ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A11[vpd_idx], &mr_conf[34], 2);
	memcpy(ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A12[vpd_idx], &mr_conf[36], 2);
	memcpy(ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A13[vpd_idx], &mr_conf[38], 2);
	memcpy(ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_A17[vpd_idx], &mr_conf[40], 2);
	memcpy(ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_BA0[vpd_idx], &mr_conf[42], 2);
	memcpy(ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_BA1[vpd_idx], &mr_conf[44], 2);
	memcpy(ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_BG0[vpd_idx], &mr_conf[46], 2);
	memcpy(ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_BG1[vpd_idx], &mr_conf[48], 2);
	memcpy(ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_C0[vpd_idx], &mr_conf[50], 2);
	memcpy(ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_C1[vpd_idx], &mr_conf[52], 2);
	memcpy(ATTR_MSS_VPD_MR_MC_PHASE_ROT_ADDR_C2[vpd_idx], &mr_conf[54], 2);

	memcpy(ATTR_MSS_VPD_MR_MC_PHASE_ROT_CMD_ACTN[vpd_idx], &mr_conf[64], 2);
	memcpy(ATTR_MSS_VPD_MR_MC_PHASE_ROT_CMD_ADDR_CASN_A15[vpd_idx],
	       &mr_conf[66], 2);
	memcpy(ATTR_MSS_VPD_MR_MC_PHASE_ROT_CMD_ADDR_RASN_A16[vpd_idx],
	       &mr_conf[68], 2);
	memcpy(ATTR_MSS_VPD_MR_MC_PHASE_ROT_CMD_ADDR_WEN_A14[vpd_idx],
	       &mr_conf[70], 2);
	memcpy(ATTR_MSS_VPD_MR_MC_PHASE_ROT_CMD_PAR[vpd_idx], &mr_conf[72], 2);

	memcpy(ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D0_CKE0[vpd_idx], &mr_conf[74],
	       2);
	memcpy(ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D0_CKE1[vpd_idx], &mr_conf[76],
	       2);
	memcpy(ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D0_CSN0[vpd_idx], &mr_conf[82],
	       2);
	memcpy(ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D0_CSN1[vpd_idx], &mr_conf[84],
	       2);
	memcpy(ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D0_ODT0[vpd_idx], &mr_conf[90],
	       2);
	memcpy(ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D0_ODT1[vpd_idx], &mr_conf[92],
	       2);
	memcpy(ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D1_CKE0[vpd_idx], &mr_conf[78],
	       2);
	memcpy(ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D1_CKE1[vpd_idx], &mr_conf[80],
	       2);
	memcpy(ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D1_CSN0[vpd_idx], &mr_conf[86],
	       2);
	memcpy(ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D1_CSN1[vpd_idx], &mr_conf[88],
	       2);
	memcpy(ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D1_ODT0[vpd_idx], &mr_conf[94],
	       2);
	memcpy(ATTR_MSS_VPD_MR_MC_PHASE_ROT_CNTL_D1_ODT1[vpd_idx], &mr_conf[96],
	       2);
}

static void load_mr(const uint8_t *vpd_data)
{
	const int freqs[] = { 1866, 2133, 2400, 2666 };
	int vpd_idx = 0;

	/* Index matches indexing of ATTR_MSS_VPD_MR_MC_PHASE_ROT_* data */
	for (vpd_idx = 0; vpd_idx < 28; ++vpd_idx) {
		const int freq = freqs[vpd_idx/8];
		const int mcs_i = vpd_idx%4;
		const int dimm1_rank = (vpd_idx%8 >= 4 ? 1 : 0);

		struct vpd_info v = {
			.data = vpd_data,
			.mcs_i = mcs_i,
			.freq = freq,
			/* Configurations differ only per DIMM presence */
			.dimm0_rank = 1,
			.dimm1_rank = dimm1_rank,
		};

		const uint8_t *mr_conf = NULL;
		size_t size = 0;

		mr_conf = find_vpd_conf(&v, "MR", &size);
		if (!mr_conf)
			die("Failed to read MT configuration!\n");

		load_mr_attrs(mr_conf, size, vpd_idx);
	}
}

static void load_vpd_attrs(const uint8_t *vpd_data)
{
	load_mr(vpd_data);
	load_mt(vpd_data);
}

void vpd_pnor_main(void)
{
	const struct region_device *memd_device = NULL;

	uint8_t buf[MEMD_HDR_SIZE];
	struct memd_hdr *hdr_memd = (struct memd_hdr *)buf;

	const uint8_t *vpd_data = NULL;
	size_t vpd_size = 0;

	memd_device_init();
	memd_device = memd_device_ro();

	/* Copy all header at once */
	if (rdev_readat(memd_device, buf, 0, sizeof(buf)) != sizeof(buf))
		die("Failed to read MEMD header!\n");

	if (memcmp(hdr_memd->eyecatch, "OKOK", 4))
		die("Invalid MEMD header!\n");
	if (memcmp(hdr_memd->header_version, "01.0", 4))
		die("Unsupported MEMD header version!\n");
	if (memcmp(hdr_memd->memd_version, "01.0", 4))
		die("Unsupported MEMD version!\n");

	/* We don't loop over sections */
	if (hdr_memd->section_count != 1)
		die("Failed to map VPD data!\n");

	vpd_size = hdr_memd->section_size*MEMD_SECTION_ROUNDING_DIVISOR;
	vpd_data = rdev_mmap(memd_device, MEMD_HDR_SIZE, vpd_size);
	if (!vpd_data)
		die("Failed to map VPD data!\n");

	load_vpd_attrs(vpd_data);

	if (rdev_munmap(memd_device, (void *)vpd_data))
		die("Failed to unmap VPD data!\n");

	memd_device_unmount();
}
