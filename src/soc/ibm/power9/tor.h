/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __SOC_IBM_POWER9_TOR_H
#define __SOC_IBM_POWER9_TOR_H

#include <stdbool.h>
#include <stdint.h>

#define UNDEFINED_RING_ID      (uint16_t)0xffff
#define UNDEFINED_RING_VARIANT (uint8_t)0xff
#define UNDEFINED_INSTANCE_ID  (uint8_t)0xff

#define MAX_RING_BUF_SIZE   (uint32_t)60000
#define MAX_TOR_RING_OFFSET (uint16_t)0xffff

/* List of all Ring IDs as they appear in data */
enum ring_id {
	/* Perv Chiplet Rings */
	PERV_FURE = 0,
	PERV_GPTR = 1,
	PERV_TIME = 2,
	OCC_FURE = 3,
	OCC_GPTR = 4,
	OCC_TIME = 5,
	PERV_ANA_FUNC = 6,
	PERV_ANA_GPTR = 7,
	PERV_PLL_GPTR = 8,
	PERV_PLL_BNDY = 9,
	PERV_PLL_BNDY_BUCKET_1 = 10,
	PERV_PLL_BNDY_BUCKET_2 = 11,
	PERV_PLL_BNDY_BUCKET_3 = 12,
	PERV_PLL_BNDY_BUCKET_4 = 13,
	PERV_PLL_BNDY_BUCKET_5 = 14,
	PERV_PLL_FUNC = 15,
	PERV_REPR = 16,
	OCC_REPR = 17,
	SBE_FURE = 18,
	SBE_GPTR = 19,
	SBE_REPR = 20,

	/* Nest Chiplet Rings - N0 */
	N0_FURE = 21,
	N0_GPTR = 22,
	N0_TIME = 23,
	N0_NX_FURE = 24,
	N0_NX_GPTR = 25,
	N0_NX_TIME = 26,
	N0_CXA0_FURE = 27,
	N0_CXA0_GPTR = 28,
	N0_CXA0_TIME = 29,
	N0_REPR = 30,
	N0_NX_REPR = 31,
	N0_CXA0_REPR = 32,

	/* Nest Chiplet Rings - N1 */
	N1_FURE = 33,
	N1_GPTR = 34,
	N1_TIME = 35,
	N1_IOO0_FURE = 36,
	N1_IOO0_GPTR = 37,
	N1_IOO0_TIME = 38,
	N1_IOO1_FURE = 39,
	N1_IOO1_GPTR = 40,
	N1_IOO1_TIME = 41,
	N1_MCS23_FURE = 42,
	N1_MCS23_GPTR = 43,
	N1_MCS23_TIME = 44,
	N1_REPR = 45,
	N1_IOO0_REPR = 46,
	N1_IOO1_REPR = 47,
	N1_MCS23_REPR = 48,

	/* Nest Chiplet Rings - N2 */
	N2_FURE = 49,
	N2_GPTR = 50,
	N2_TIME = 51,
	N2_CXA1_FURE = 52,
	N2_CXA1_GPTR = 53,
	N2_CXA1_TIME = 54,
	N2_PSI_FURE = 55,
	N2_PSI_GPTR = 56,
	N2_PSI_TIME = 57,
	N2_REPR = 58,
	N2_CXA1_REPR = 59,
	/* Values 60-61 unused */

	/* Nest Chiplet Rings - N3 */
	N3_FURE = 62,
	N3_GPTR = 63,
	N3_TIME = 64,
	N3_MCS01_FURE = 65,
	N3_MCS01_GPTR = 66,
	N3_MCS01_TIME = 67,
	N3_NP_FURE = 68,
	N3_NP_GPTR = 69,
	N3_NP_TIME = 70,
	N3_REPR = 71,
	N3_MCS01_REPR = 72,
	N3_NP_REPR = 73,
	N3_BR_FURE = 74,

	/* X-Bus Chiplet Rings */
	/* Common - apply to all instances of X-Bus */
	XB_FURE = 75,
	XB_GPTR = 76,
	XB_TIME = 77,
	XB_IO0_FURE = 78,
	XB_IO0_GPTR = 79,
	XB_IO0_TIME = 80,
	XB_IO1_FURE = 81,
	XB_IO1_GPTR = 82,
	XB_IO1_TIME = 83,
	XB_IO2_FURE = 84,
	XB_IO2_GPTR = 85,
	XB_IO2_TIME = 86,
	XB_PLL_GPTR = 87,
	XB_PLL_BNDY  = 88,
	XB_PLL_FUNC = 89,

	/* X-Bus Chiplet Rings */
	/* X0, X1 and X2 instance specific Rings */
	XB_REPR = 90,
	XB_IO0_REPR = 91,
	XB_IO1_REPR = 92,
	XB_IO2_REPR = 93,
	/* Values 94-95 unused */

	/* MC Chiplet Rings */
	/* Common - apply to all instances of MC */
	MC_FURE = 96,
	MC_GPTR = 97,
	MC_TIME = 98,
	MC_IOM01_FURE = 99,
	MC_IOM01_GPTR = 100,
	MC_IOM01_TIME = 101,
	MC_IOM23_FURE = 102,
	MC_IOM23_GPTR = 103,
	MC_IOM23_TIME = 104,
	MC_PLL_GPTR = 105,
	MC_PLL_BNDY  = 106,
	MC_PLL_BNDY_BUCKET_1 = 107,
	MC_PLL_BNDY_BUCKET_2 = 108,
	MC_PLL_BNDY_BUCKET_3 = 109,
	MC_PLL_BNDY_BUCKET_4 = 110,
	MC_PLL_BNDY_BUCKET_5 = 111,
	MC_PLL_FUNC = 112,

	/* MC Chiplet Rings */
	/* MC01 and MC23 instance specific Rings */
	MC_REPR = 113,
	/* Value 114 unused */
	MC_IOM23_REPR = 115,

	/* OB0 Chiplet Rings */
	OB0_PLL_BNDY = 116,
	OB0_PLL_BNDY_BUCKET_1 = 117,
	OB0_PLL_BNDY_BUCKET_2 = 118,
	OB0_GPTR = 119,
	OB0_TIME = 120,
	OB0_PLL_GPTR = 121,
	OB0_FURE = 122,
	OB0_PLL_BNDY_BUCKET_3 = 123,

	/* OB0 Chiplet instance specific Ring */
	OB0_REPR = 124,

	/* OB1 Chiplet Rings */
	OB1_PLL_BNDY = 125,
	OB1_PLL_BNDY_BUCKET_1 = 126,
	OB1_PLL_BNDY_BUCKET_2 = 127,
	OB1_GPTR = 128,
	OB1_TIME = 129,
	OB1_PLL_GPTR = 130,
	OB1_FURE = 131,
	OB1_PLL_BNDY_BUCKET_3 = 132,

	/* OB1 Chiplet instance specific Ring */
	OB1_REPR = 133,

	/* OB2 Chiplet Rings */
	OB2_PLL_BNDY = 134,
	OB2_PLL_BNDY_BUCKET_1 = 135,
	OB2_PLL_BNDY_BUCKET_2 = 136,
	OB2_GPTR = 137,
	OB2_TIME = 138,
	OB2_PLL_GPTR = 139,
	OB2_FURE = 140,
	OB2_PLL_BNDY_BUCKET_3 = 141,

	/* OB2 Chiplet instance specific Ring */
	OB2_REPR = 142,

	/* OB3 Chiplet Rings */
	OB3_PLL_BNDY = 143,
	OB3_PLL_BNDY_BUCKET_1 = 144,
	OB3_PLL_BNDY_BUCKET_2 = 145,
	OB3_GPTR = 146,
	OB3_TIME = 147,
	OB3_PLL_GPTR = 148,
	OB3_FURE = 149,
	OB3_PLL_BNDY_BUCKET_3 = 150,

	/* OB3 Chiplet instance specific Rings */
	OB3_REPR = 151,

	/* Values 152-153 unused */

	/* PCI Chiplet Rings */
	/* PCI0 Common Rings */
	PCI0_FURE = 154,
	PCI0_GPTR = 155,
	PCI0_TIME = 156,
	PCI0_PLL_BNDY = 157,
	PCI0_PLL_GPTR = 158,
	/* Instance specific Rings */
	PCI0_REPR = 159,

	/* PCI1 Common Rings */
	PCI1_FURE = 160,
	PCI1_GPTR = 161,
	PCI1_TIME = 162,
	PCI1_PLL_BNDY = 163,
	PCI1_PLL_GPTR = 164,
	/* Instance specific Rings */
	PCI1_REPR = 165,

	/* PCI2 Common Rings */
	PCI2_FURE = 166,
	PCI2_GPTR = 167,
	PCI2_TIME = 168,
	PCI2_PLL_BNDY = 169,
	PCI2_PLL_GPTR = 170,
	/* Instance specific Rings */
	PCI2_REPR = 171,

	/* Quad Chiplet Rings */
	/* Common - apply to all Quad instances */
	EQ_FURE = 172,
	EQ_GPTR = 173,
	EQ_TIME = 174,
	EQ_INEX = 175,
	EX_L3_FURE = 176,
	EX_L3_GPTR = 177,
	EX_L3_TIME = 178,
	EX_L2_MODE = 179,
	EX_L2_FURE = 180,
	EX_L2_GPTR = 181,
	EX_L2_TIME = 182,
	EX_L3_REFR_FURE = 183,
	EX_L3_REFR_GPTR = 184,
	EX_L3_REFR_TIME = 185,
	EQ_ANA_FUNC = 186,
	EQ_ANA_GPTR = 187,
	EQ_DPLL_FUNC = 188,
	EQ_DPLL_GPTR = 189,
	EQ_DPLL_MODE = 190,
	EQ_ANA_BNDY = 191,
	EQ_ANA_BNDY_BUCKET_0 = 192,
	EQ_ANA_BNDY_BUCKET_1 = 193,
	EQ_ANA_BNDY_BUCKET_2 = 194,
	EQ_ANA_BNDY_BUCKET_3 = 195,
	EQ_ANA_BNDY_BUCKET_4 = 196,
	EQ_ANA_BNDY_BUCKET_5 = 197,
	EQ_ANA_BNDY_BUCKET_6 = 198,
	EQ_ANA_BNDY_BUCKET_7 = 199,
	EQ_ANA_BNDY_BUCKET_8 = 200,
	EQ_ANA_BNDY_BUCKET_9 = 201,
	EQ_ANA_BNDY_BUCKET_10 = 202,
	EQ_ANA_BNDY_BUCKET_11 = 203,
	EQ_ANA_BNDY_BUCKET_12 = 204,
	EQ_ANA_BNDY_BUCKET_13 = 205,
	EQ_ANA_BNDY_BUCKET_14 = 206,
	EQ_ANA_BNDY_BUCKET_15 = 207,
	EQ_ANA_BNDY_BUCKET_16 = 208,
	EQ_ANA_BNDY_BUCKET_17 = 209,
	EQ_ANA_BNDY_BUCKET_18 = 210,
	EQ_ANA_BNDY_BUCKET_19 = 211,
	EQ_ANA_BNDY_BUCKET_20 = 212,
	EQ_ANA_BNDY_BUCKET_21 = 213,
	EQ_ANA_BNDY_BUCKET_22 = 214,
	EQ_ANA_BNDY_BUCKET_23 = 215,
	EQ_ANA_BNDY_BUCKET_24 = 216,
	EQ_ANA_BNDY_BUCKET_25 = 217,
	EQ_ANA_BNDY_BUCKET_L3DCC = 218,
	EQ_ANA_MODE = 219,

	/* Quad Chiplet Rings */
	/* EQ0 - EQ5 instance specific Rings */
	EQ_REPR = 220,
	EX_L3_REPR = 221,
	EX_L2_REPR = 222,
	EX_L3_REFR_REPR = 223,

	/* Core Chiplet Rings */
	/* Common - apply to all Core instances */
	EC_FUNC = 224,
	EC_GPTR = 225,
	EC_TIME = 226,
	EC_MODE = 227,

	/* Core Chiplet Rings */
	/* EC0 - EC23 instance specific Ring */
	EC_REPR = 228,

	/* Values 229-230 unused */

	/* Core Chiplet Rings */
	/* ABIST engine mode */
	EC_ABST = 231,

	/* Additional rings for Nimbus DD2 */
	EQ_ANA_BNDY_BUCKET_26 = 232,
	EQ_ANA_BNDY_BUCKET_27 = 233,
	EQ_ANA_BNDY_BUCKET_28 = 234,
	EQ_ANA_BNDY_BUCKET_29 = 235,
	EQ_ANA_BNDY_BUCKET_30 = 236,
	EQ_ANA_BNDY_BUCKET_31 = 237,
	EQ_ANA_BNDY_BUCKET_32 = 238,
	EQ_ANA_BNDY_BUCKET_33 = 239,
	EQ_ANA_BNDY_BUCKET_34 = 240,
	EQ_ANA_BNDY_BUCKET_35 = 241,
	EQ_ANA_BNDY_BUCKET_36 = 242,
	EQ_ANA_BNDY_BUCKET_37 = 243,
	EQ_ANA_BNDY_BUCKET_38 = 244,
	EQ_ANA_BNDY_BUCKET_39 = 245,
	EQ_ANA_BNDY_BUCKET_40 = 246,
	EQ_ANA_BNDY_BUCKET_41 = 247,

	/* EQ Inex ring bucket */
	EQ_INEX_BUCKET_1      = 248,
	EQ_INEX_BUCKET_2      = 249,
	EQ_INEX_BUCKET_3      = 250,
	EQ_INEX_BUCKET_4      = 251,

	/* CMSK ring */
	EC_CMSK = 252,

	/* Perv PLL filter override rings */
	PERV_PLL_BNDY_FLT_1   = 253,
	PERV_PLL_BNDY_FLT_2   = 254,
	PERV_PLL_BNDY_FLT_3   = 255,
	PERV_PLL_BNDY_FLT_4   = 256,

	/* MC OMI rings */
	MC_OMI0_FURE          = 257,
	MC_OMI0_GPTR          = 258,
	MC_OMI1_FURE          = 259,
	MC_OMI1_GPTR          = 260,
	MC_OMI2_FURE          = 261,
	MC_OMI2_GPTR          = 262,
	MC_OMIPPE_FURE        = 263,
	MC_OMIPPE_GPTR        = 264,
	MC_OMIPPE_TIME        = 265,
	/* Instance rings */
	MC_OMIPPE_REPR        = 266,

	NUM_RING_IDS = 267
};

/* Supported ring variants. Values match order in ring sections. */
enum ring_variant {
	RV_BASE,
	RV_CC,
	RV_RL,		// Kernel and user protection
	RV_RL2,		// Kernel only protection
	RV_RL3,		// Rugby v4
	RV_RL4,		// Java performance
	RV_RL5,		// Spare
	NUM_RING_VARIANTS
};

/* List of groups of rings */
enum ring_class {
	RING_CLASS_NEST,       	// Common NEST rings except GPTR #G rings
	RING_CLASS_GPTR_NEST,	// Common GPTR #G rings-NEST
	RING_CLASS_GPTR_EQ,    	// Common GPTR #G rings-EQ
	RING_CLASS_GPTR_EX,    	// Common GPTR #G rings-EX
	RING_CLASS_GPTR_EC,    	// Common GPTR #G rings-EC
	RING_CLASS_EQ,		// Common EQ rings
	RING_CLASS_EX,		// Common EX rings
	RING_CLASS_EC,		// Common EC rings
	RING_CLASS_EQ_INS,     	// Instance EQ rings
	RING_CLASS_EX_INS,     	// Instance EX rings
	RING_CLASS_EC_INS,     	// Instance EC rings
};

/* PPE types, enum values match indices inside rings section */
enum ppe_type {
	PT_SBE,
	PT_CME,
	PT_SGPE,
};

/* Available ring access operations */
enum ring_operation {
	GET_RING_DATA,
	GET_RING_PUT_INFO,
	GET_PPE_LEVEL_RINGS,
};

/* Result of calling tor_fetch_and_insert_vpd_rings() */
enum ring_status {
	RING_NOT_FOUND,
	RING_FOUND,
	RING_REDUNDANT,
};

/* Information necessary to put a ring into a ring section */
struct ring_put_info {
	uint32_t chiplet_offset;	// Relative to ring section
	uint32_t ring_slot_offset;	// Relative to ring section
};

/* Describes ring search characteristics for tor_fetch_and_insert_vpd_rings() */
struct ring_query {
	enum ring_id ring_id;
	enum ring_class ring_class;
	char kwd_name[3];		// Keyword name
	uint8_t min_instance_id;
	uint8_t max_instance_id;
};

/* Header of a ring section */
struct tor_hdr {
	uint32_t magic;		// One of TOR_MAGIC_*
	uint8_t  version;
	uint8_t  chip_type;
	uint8_t  dd_level;
	uint8_t  undefined;
	uint32_t size;
	uint8_t	 data[];
} __attribute__((packed));

/* Either reads ring into the buffer (on GET_RING_DATA) or treats it as an
 * instance of ring_put_info (on GET_RING_PUT_INFO) */
bool tor_access_ring(struct tor_hdr *ring_section, uint16_t ring_id,
		     enum ppe_type ppe_type, uint8_t ring_variant,
		     uint8_t instance_id, void *data_buf,
		     uint32_t *data_buf_size, enum ring_operation operation);

/*
 * Extracts rings from CP00 record of MVPD and appends them to the ring section
 * applying overlay if necessary.  All buffers must be be at least
 * MAX_RING_BUF_SIZE bytes in length.
 */
void tor_fetch_and_insert_vpd_rings(uint8_t chip,
				    struct tor_hdr *ring_section,
				    uint32_t *ring_section_size,
				    uint32_t max_ring_section_size,
				    struct tor_hdr *overlays_section,
				    enum ppe_type ppe_type,
				    uint8_t *buf1,
				    uint8_t *buf2,
				    uint8_t *buf3);

#endif // __SOC_IBM_POWER9_TOR_H
