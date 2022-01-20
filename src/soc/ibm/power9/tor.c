/* SPDX-License-Identifier: GPL-2.0-only */

#include "tor.h"

#include <cpu/power/mvpd.h>
#include <lib.h>

#include <assert.h>
#include <endian.h>
#include <stdbool.h>
#include <string.h>

#include "rs4.h"

#define UNDEFINED_PPE_TYPE (uint8_t)0xff

#define NUM_OF_CORES   (uint8_t)24
#define NUM_OF_QUADS   (uint8_t)6
#define CORES_PER_QUAD (NUM_OF_CORES/NUM_OF_QUADS)

#define TOR_VERSION 7

#define TOR_MAGIC      (uint32_t)0x544F52   // "TOR"
#define TOR_MAGIC_HW   (uint32_t)0x544F5248 // "TORH"
#define TOR_MAGIC_SGPE (uint32_t)0x544F5247 // "TORG"
#define TOR_MAGIC_CME  (uint32_t)0x544F524D // "TORM"
#define TOR_MAGIC_OVLY (uint32_t)0x544F524C // "TORL"

#define NUM_CHIP_TYPES 4

/*
 * Structure of a TOR section:
 *  - Header (tor_hdr)
 *  - Payload
 *    - either array of PPE blocks that point to ring sections (HW TOR)
 *    - or ring section
 *
 * PPE block:
 *  - uint32_t -- offset (relative to payload) in BE to a ring section
 *  - uint32_t -- size in BE
 *
 * Ring section:
 *  - Array of chiplet blocks (we assume size of one for non-overlay rings)
 *    - Chiplet block
 *      - Array of TOR slots (value of 0 means "no such ring")
 *      - Array of rings pointed to by TOR slots
 *
 * Chiplet block:
 *  - uint32_t -- offset (relative to payload) to slots for common rings in BE
 *  - uint32_t -- offset (relative to payload) to slots for instance rings in BE
 *
 * TOR slot (`(max_instance_id - min_instance_id + 1)*ring_count` of them):
 *  - uint16_t -- offset (relative to chiplet block) to a ring in BE
 */

enum chiplet_type {
	PERV_TYPE,
	N0_TYPE,
	N1_TYPE,
	N2_TYPE,
	N3_TYPE,
	XB_TYPE,
	MC_TYPE,
	OB0_TYPE,
	OB1_TYPE,
	OB2_TYPE,
	OB3_TYPE,
	PCI0_TYPE,
	PCI1_TYPE,
	PCI2_TYPE,
	EQ_TYPE,
	EC_TYPE,
	SBE_NOOF_CHIPLETS
};

/* Description of a PPE block */
struct tor_ppe_block {
	uint32_t offset;
	uint32_t size;
} __attribute__((packed));

/* Offsets to different kinds of rings within a section */
struct tor_chiplet_block {
	uint32_t common_offset;
	uint32_t instance_offset;
} __attribute__((packed));

/* Static information about a ring to be searched for by the name */
struct ring_info {
	enum ring_id ring_id;
	uint8_t min_instance_id;	// Lower bound of instance id range
	uint8_t max_instance_id;	// Upper bound of instance id range
};

/* Static information about a chiplet */
struct chiplet_info {
	uint8_t common_rings_count;     // [0..common_rings_count)
	uint8_t instance_rings_count;	// [0..instance_rings_count)
};

const struct ring_info EQ_COMMON_RING_INFO[] = {
	{EQ_FURE                    , 0x10, 0x10},
	{EQ_GPTR                    , 0x10, 0x10},
	{EQ_TIME                    , 0x10, 0x10},
	{EQ_INEX                    , 0x10, 0x10},
	{EX_L3_FURE                 , 0x10, 0x10},
	{EX_L3_GPTR                 , 0x10, 0x10},
	{EX_L3_TIME                 , 0x10, 0x10},
	{EX_L2_MODE                 , 0x10, 0x10},
	{EX_L2_FURE                 , 0x10, 0x10},
	{EX_L2_GPTR                 , 0x10, 0x10},
	{EX_L2_TIME                 , 0x10, 0x10},
	{EX_L3_REFR_FURE            , 0x10, 0x10},
	{EX_L3_REFR_GPTR            , 0x10, 0x10},
	{EQ_ANA_FUNC                , 0x10, 0x10},
	{EQ_ANA_GPTR                , 0x10, 0x10},
	{EQ_DPLL_FUNC               , 0x10, 0x10},
	{EQ_DPLL_GPTR               , 0x10, 0x10},
	{EQ_DPLL_MODE               , 0x10, 0x10},
	{EQ_ANA_BNDY_BUCKET_0       , 0x10, 0x10},
	{EQ_ANA_BNDY_BUCKET_1       , 0x10, 0x10},
	{EQ_ANA_BNDY_BUCKET_2       , 0x10, 0x10},
	{EQ_ANA_BNDY_BUCKET_3       , 0x10, 0x10},
	{EQ_ANA_BNDY_BUCKET_4       , 0x10, 0x10},
	{EQ_ANA_BNDY_BUCKET_5       , 0x10, 0x10},
	{EQ_ANA_BNDY_BUCKET_6       , 0x10, 0x10},
	{EQ_ANA_BNDY_BUCKET_7       , 0x10, 0x10},
	{EQ_ANA_BNDY_BUCKET_8       , 0x10, 0x10},
	{EQ_ANA_BNDY_BUCKET_9       , 0x10, 0x10},
	{EQ_ANA_BNDY_BUCKET_10      , 0x10, 0x10},
	{EQ_ANA_BNDY_BUCKET_11      , 0x10, 0x10},
	{EQ_ANA_BNDY_BUCKET_12      , 0x10, 0x10},
	{EQ_ANA_BNDY_BUCKET_13      , 0x10, 0x10},
	{EQ_ANA_BNDY_BUCKET_14      , 0x10, 0x10},
	{EQ_ANA_BNDY_BUCKET_15      , 0x10, 0x10},
	{EQ_ANA_BNDY_BUCKET_16      , 0x10, 0x10},
	{EQ_ANA_BNDY_BUCKET_17      , 0x10, 0x10},
	{EQ_ANA_BNDY_BUCKET_18      , 0x10, 0x10},
	{EQ_ANA_BNDY_BUCKET_19      , 0x10, 0x10},
	{EQ_ANA_BNDY_BUCKET_20      , 0x10, 0x10},
	{EQ_ANA_BNDY_BUCKET_21      , 0x10, 0x10},
	{EQ_ANA_BNDY_BUCKET_22      , 0x10, 0x10},
	{EQ_ANA_BNDY_BUCKET_23      , 0x10, 0x10},
	{EQ_ANA_BNDY_BUCKET_24      , 0x10, 0x10},
	{EQ_ANA_BNDY_BUCKET_25      , 0x10, 0x10},
	{EQ_ANA_BNDY_BUCKET_L3DCC   , 0x10, 0x10},
	{EQ_ANA_MODE                , 0x10, 0x10},
	{EQ_ANA_BNDY_BUCKET_26      , 0x10, 0x10},
	{EQ_ANA_BNDY_BUCKET_27      , 0x10, 0x10},
	{EQ_ANA_BNDY_BUCKET_28      , 0x10, 0x10},
	{EQ_ANA_BNDY_BUCKET_29      , 0x10, 0x10},
	{EQ_ANA_BNDY_BUCKET_30      , 0x10, 0x10},
	{EQ_ANA_BNDY_BUCKET_31      , 0x10, 0x10},
	{EQ_ANA_BNDY_BUCKET_32      , 0x10, 0x10},
	{EQ_ANA_BNDY_BUCKET_33      , 0x10, 0x10},
	{EQ_ANA_BNDY_BUCKET_34      , 0x10, 0x10},
	{EQ_ANA_BNDY_BUCKET_35      , 0x10, 0x10},
	{EQ_ANA_BNDY_BUCKET_36      , 0x10, 0x10},
	{EQ_ANA_BNDY_BUCKET_37      , 0x10, 0x10},
	{EQ_ANA_BNDY_BUCKET_38      , 0x10, 0x10},
	{EQ_ANA_BNDY_BUCKET_39      , 0x10, 0x10},
	{EQ_ANA_BNDY_BUCKET_40      , 0x10, 0x10},
	{EQ_ANA_BNDY_BUCKET_41      , 0x10, 0x10},
	{EQ_INEX_BUCKET_1           , 0x10, 0x10},
	{EQ_INEX_BUCKET_2           , 0x10, 0x10},
	{EQ_INEX_BUCKET_3           , 0x10, 0x10},
	{EQ_INEX_BUCKET_4           , 0x10, 0x10},
};

const struct ring_info EQ_INSTANCE_RING_INFO[] = {
	{EQ_REPR                    , 0x10, 0x1b},
	{EX_L3_REPR                 , 0x10, 0x1b},
	{EX_L2_REPR                 , 0x10, 0x1b},
	{EX_L3_REFR_REPR            , 0x10, 0x1b},
	{EX_L3_REFR_TIME            , 0x10, 0x1b},
};

static const struct chiplet_info EQ_CHIPLET_INFO = {
	66, // 66 common rings for Quad chiplet.
	5,  // 5 instance specific rings for each EQ chiplet
};

const struct ring_info EC_COMMON_RING_INFO[] = {
	{EC_FUNC            , 0x20, 0x20},
	{EC_GPTR            , 0x20, 0x20},
	{EC_TIME            , 0x20, 0x20},
	{EC_MODE            , 0x20, 0x20},
	{EC_ABST            , 0x20, 0x20},
	{EC_CMSK            , 0xFF, 0xFF},
};

const struct ring_info EC_INSTANCE_RING_INFO[] = {
	{EC_REPR            , 0x20, 0x37},
};

static const struct chiplet_info EC_CHIPLET_INFO = {
	6,  // 6 common rings for Core chiplet
	1,  // 1 instance specific ring for each Core chiplet
};

static const struct ring_query RING_QUERIES_PDG[] = {
    /* ring_id          ring_class          kwd_name  instance_id  */
    /*                                                min   max    */
    { PERV_GPTR       , RING_CLASS_GPTR_NEST , "#G" , 0x01 , 0x01 },
    { PERV_TIME       , RING_CLASS_NEST      , "#G" , 0x01 , 0x01 },
    { OCC_GPTR        , RING_CLASS_GPTR_NEST , "#G" , 0x01 , 0x01 },
    { OCC_TIME        , RING_CLASS_NEST      , "#G" , 0x01 , 0x01 },
    { SBE_GPTR        , RING_CLASS_GPTR_NEST , "#G" , 0x01 , 0x01 },
    { PERV_ANA_GPTR   , RING_CLASS_GPTR_NEST , "#G" , 0x01 , 0x01 },
    { PERV_PLL_GPTR   , RING_CLASS_GPTR_NEST , "#G" , 0x01 , 0x01 },
    { N0_GPTR         , RING_CLASS_GPTR_NEST , "#G" , 0x02 , 0x02 },
    { N0_TIME         , RING_CLASS_NEST      , "#G" , 0x02 , 0x02 },
    { N0_NX_GPTR      , RING_CLASS_GPTR_NEST , "#G" , 0x02 , 0x02 },
    { N0_NX_TIME      , RING_CLASS_NEST      , "#G" , 0x02 , 0x02 },
    { N0_CXA0_GPTR    , RING_CLASS_GPTR_NEST , "#G" , 0x02 , 0x02 },
    { N0_CXA0_TIME    , RING_CLASS_NEST      , "#G" , 0x02 , 0x02 },
    { N1_GPTR         , RING_CLASS_GPTR_NEST , "#G" , 0x03 , 0x03 },
    { N1_TIME         , RING_CLASS_NEST      , "#G" , 0x03 , 0x03 },
    { N1_IOO0_GPTR    , RING_CLASS_GPTR_NEST , "#G" , 0x03 , 0x03 },
    { N1_IOO0_TIME    , RING_CLASS_NEST      , "#G" , 0x03 , 0x03 },
    { N1_IOO1_GPTR    , RING_CLASS_GPTR_NEST , "#G" , 0x03 , 0x03 },
    { N1_IOO1_TIME    , RING_CLASS_NEST      , "#G" , 0x03 , 0x03 },
    { N1_MCS23_GPTR   , RING_CLASS_GPTR_NEST , "#G" , 0x03 , 0x03 },
    { N1_MCS23_TIME   , RING_CLASS_NEST      , "#G" , 0x03 , 0x03 },
    { N2_GPTR         , RING_CLASS_GPTR_NEST , "#G" , 0x04 , 0x04 },
    { N2_TIME         , RING_CLASS_NEST      , "#G" , 0x04 , 0x04 },
    { N2_CXA1_GPTR    , RING_CLASS_GPTR_NEST , "#G" , 0x04 , 0x04 },
    { N2_CXA1_TIME    , RING_CLASS_NEST      , "#G" , 0x04 , 0x04 },
    { N2_PSI_GPTR     , RING_CLASS_GPTR_NEST , "#G" , 0x04 , 0x04 },
    { N3_GPTR         , RING_CLASS_GPTR_NEST , "#G" , 0x05 , 0x05 },
    { N3_TIME         , RING_CLASS_NEST      , "#G" , 0x05 , 0x05 },
    { N3_MCS01_GPTR   , RING_CLASS_GPTR_NEST , "#G" , 0x05 , 0x05 },
    { N3_MCS01_TIME   , RING_CLASS_NEST      , "#G" , 0x05 , 0x05 },
    { N3_NP_GPTR      , RING_CLASS_GPTR_NEST , "#G" , 0x05 , 0x05 },
    { N3_NP_TIME      , RING_CLASS_NEST      , "#G" , 0x05 , 0x05 },
    { XB_GPTR         , RING_CLASS_GPTR_NEST , "#G" , 0x06 , 0x06 },
    { XB_TIME         , RING_CLASS_NEST      , "#G" , 0x06 , 0x06 },
    { XB_IO0_GPTR     , RING_CLASS_GPTR_NEST , "#G" , 0x06 , 0x06 },
    { XB_IO0_TIME     , RING_CLASS_NEST      , "#G" , 0x06 , 0x06 },
    { XB_IO1_GPTR     , RING_CLASS_GPTR_NEST , "#G" , 0x06 , 0x06 },
    { XB_IO1_TIME     , RING_CLASS_NEST      , "#G" , 0x06 , 0x06 },
    { XB_IO2_GPTR     , RING_CLASS_GPTR_NEST , "#G" , 0x06 , 0x06 },
    { XB_IO2_TIME     , RING_CLASS_NEST      , "#G" , 0x06 , 0x06 },
    { XB_PLL_GPTR     , RING_CLASS_GPTR_NEST , "#G" , 0x06 , 0x06 },
    { MC_GPTR         , RING_CLASS_GPTR_NEST , "#G" , 0x07 , 0xFF },
    { MC_TIME         , RING_CLASS_NEST      , "#G" , 0x07 , 0xFF },
    { MC_IOM01_GPTR   , RING_CLASS_GPTR_NEST , "#G" , 0x07 , 0xFF },
    { MC_IOM23_GPTR   , RING_CLASS_GPTR_NEST , "#G" , 0x07 , 0xFF },
    { MC_PLL_GPTR     , RING_CLASS_GPTR_NEST , "#G" , 0x07 , 0xFF },
    { MC_OMI0_GPTR    , RING_CLASS_GPTR_NEST , "#G" , 0x07 , 0xFF },
    { MC_OMI1_GPTR    , RING_CLASS_GPTR_NEST , "#G" , 0x07 , 0xFF },
    { MC_OMI2_GPTR    , RING_CLASS_GPTR_NEST , "#G" , 0x07 , 0xFF },
    { MC_OMIPPE_GPTR  , RING_CLASS_GPTR_NEST , "#G" , 0x07 , 0xFF },
    { MC_OMIPPE_TIME  , RING_CLASS_NEST      , "#G" , 0x07 , 0xFF },
    { OB0_GPTR        , RING_CLASS_GPTR_NEST , "#G" , 0x09 , 0x09 },
    { OB0_TIME        , RING_CLASS_NEST      , "#G" , 0x09 , 0x09 },
    { OB0_PLL_GPTR    , RING_CLASS_GPTR_NEST , "#G" , 0x09 , 0x09 },
    { OB1_GPTR        , RING_CLASS_GPTR_NEST , "#G" , 0x0A , 0x0A },
    { OB1_TIME        , RING_CLASS_NEST      , "#G" , 0x0A , 0x0A },
    { OB1_PLL_GPTR    , RING_CLASS_GPTR_NEST , "#G" , 0x0A , 0x0A },
    { OB2_GPTR        , RING_CLASS_GPTR_NEST , "#G" , 0x0B , 0x0B },
    { OB2_TIME        , RING_CLASS_NEST      , "#G" , 0x0B , 0x0B },
    { OB2_PLL_GPTR    , RING_CLASS_GPTR_NEST , "#G" , 0x0B , 0x0B },
    { OB3_GPTR        , RING_CLASS_GPTR_NEST , "#G" , 0x0C , 0x0C },
    { OB3_TIME        , RING_CLASS_NEST      , "#G" , 0x0C , 0x0C },
    { OB3_PLL_GPTR    , RING_CLASS_GPTR_NEST , "#G" , 0x0C , 0x0C },
    { PCI0_GPTR       , RING_CLASS_GPTR_NEST , "#G" , 0x0D , 0x0D },
    { PCI0_TIME       , RING_CLASS_NEST      , "#G" , 0x0D , 0x0D },
    { PCI0_PLL_GPTR   , RING_CLASS_GPTR_NEST , "#G" , 0x0D , 0x0D },
    { PCI1_GPTR       , RING_CLASS_GPTR_NEST , "#G" , 0x0E , 0x0E },
    { PCI1_TIME       , RING_CLASS_NEST      , "#G" , 0x0E , 0x0E },
    { PCI1_PLL_GPTR   , RING_CLASS_GPTR_NEST , "#G" , 0x0E , 0x0E },
    { PCI2_GPTR       , RING_CLASS_GPTR_NEST , "#G" , 0x0F , 0x0F },
    { PCI2_TIME       , RING_CLASS_NEST      , "#G" , 0x0F , 0x0F },
    { PCI2_PLL_GPTR   , RING_CLASS_GPTR_NEST , "#G" , 0x0F , 0x0F },
    { EQ_GPTR         , RING_CLASS_GPTR_EQ   , "#G" , 0x10 , 0xFF },
    { EQ_TIME         , RING_CLASS_EQ        , "#G" , 0x10 , 0xFF },
    { EX_L3_GPTR      , RING_CLASS_GPTR_EX   , "#G" , 0x10 , 0xFF },
    { EX_L3_TIME      , RING_CLASS_EX        , "#G" , 0x10 , 0xFF },
    { EX_L2_GPTR      , RING_CLASS_GPTR_EX   , "#G" , 0x10 , 0xFF },
    { EX_L2_TIME      , RING_CLASS_EX        , "#G" , 0x10 , 0xFF },
    { EX_L3_REFR_GPTR , RING_CLASS_GPTR_EX   , "#G" , 0x10 , 0xFF },
    { EQ_ANA_GPTR     , RING_CLASS_GPTR_EQ   , "#G" , 0x10 , 0xFF },
    { EQ_DPLL_GPTR    , RING_CLASS_GPTR_EQ   , "#G" , 0x10 , 0xFF },
    { EC_GPTR         , RING_CLASS_GPTR_EC   , "#G" , 0x20 , 0xFF },
    { EC_TIME         , RING_CLASS_EC        , "#G" , 0x20 , 0xFF },
};

static const struct ring_query RING_QUERIES_PDR[] = {
    /* ring_id          ring_class          kwd_name  instance_id  */
    /*                                                min   max    */
    { PERV_REPR       , RING_CLASS_NEST      , "#R" , 0x01 , 0x01 },
    { OCC_REPR        , RING_CLASS_NEST      , "#R" , 0x01 , 0x01 },
    { SBE_REPR        , RING_CLASS_NEST      , "#R" , 0x01 , 0x01 },
    { N0_REPR         , RING_CLASS_NEST      , "#R" , 0x02 , 0x02 },
    { N0_NX_REPR      , RING_CLASS_NEST      , "#R" , 0x02 , 0x02 },
    { N0_CXA0_REPR    , RING_CLASS_NEST      , "#R" , 0x02 , 0x02 },
    { N1_REPR         , RING_CLASS_NEST      , "#R" , 0x03 , 0x03 },
    { N1_IOO0_REPR    , RING_CLASS_NEST      , "#R" , 0x03 , 0x03 },
    { N1_IOO1_REPR    , RING_CLASS_NEST      , "#R" , 0x03 , 0x03 },
    { N1_MCS23_REPR   , RING_CLASS_NEST      , "#R" , 0x03 , 0x03 },
    { N2_REPR         , RING_CLASS_NEST      , "#R" , 0x04 , 0x04 },
    { N2_CXA1_REPR    , RING_CLASS_NEST      , "#R" , 0x04 , 0x04 },
    { N3_REPR         , RING_CLASS_NEST      , "#R" , 0x05 , 0x05 },
    { N3_MCS01_REPR   , RING_CLASS_NEST      , "#R" , 0x05 , 0x05 },
    { N3_NP_REPR      , RING_CLASS_NEST      , "#R" , 0x05 , 0x05 },
    { XB_REPR         , RING_CLASS_NEST      , "#R" , 0x06 , 0x06 },
    { XB_IO0_REPR     , RING_CLASS_NEST      , "#R" , 0x06 , 0x06 },
    { XB_IO1_REPR     , RING_CLASS_NEST      , "#R" , 0x06 , 0x06 },
    { XB_IO2_REPR     , RING_CLASS_NEST      , "#R" , 0x06 , 0x06 },
    { MC_REPR         , RING_CLASS_NEST      , "#R" , 0x07 , 0x08 },
    { MC_IOM23_REPR   , RING_CLASS_NEST      , "#R" , 0x07 , 0x08 },
    { MC_OMIPPE_REPR  , RING_CLASS_NEST      , "#R" , 0x07 , 0x08 },
    { OB0_REPR        , RING_CLASS_NEST      , "#R" , 0x09 , 0x09 },
    { OB1_REPR        , RING_CLASS_NEST      , "#R" , 0x0A , 0x0A },
    { OB2_REPR        , RING_CLASS_NEST      , "#R" , 0x0B , 0x0B },
    { OB3_REPR        , RING_CLASS_NEST      , "#R" , 0x0C , 0x0C },
    { PCI0_REPR       , RING_CLASS_NEST      , "#R" , 0x0D , 0x0D },
    { PCI1_REPR       , RING_CLASS_NEST      , "#R" , 0x0E , 0x0E },
    { PCI2_REPR       , RING_CLASS_NEST      , "#R" , 0x0F , 0x0F },
    { EQ_REPR         , RING_CLASS_EQ_INS    , "#R" , 0x10 , 0x15 },
    { EX_L3_REFR_TIME , RING_CLASS_EX_INS    , "#R" , 0x10 , 0x15 },
    { EX_L3_REPR      , RING_CLASS_EX_INS    , "#R" , 0x10 , 0x15 },
    { EX_L2_REPR      , RING_CLASS_EX_INS    , "#R" , 0x10 , 0x15 },
    { EX_L3_REFR_REPR , RING_CLASS_EX_INS    , "#R" , 0x10 , 0x15 },
    { EC_REPR         , RING_CLASS_EC_INS    , "#R" , 0x20 , 0x37 },
};

/* Retrieves properties for specified kind of TOR */
static void get_section_properties(uint32_t tor_magic,
				   uint8_t chiplet_type,
				   const struct chiplet_info **chiplet_info,
				   const struct ring_info **common_ring_info,
				   const struct ring_info **instance_ring_info)
{
	if (tor_magic == TOR_MAGIC_CME) {
		chiplet_type = EC_TYPE;
	} else if (tor_magic == TOR_MAGIC_SGPE) {
		chiplet_type = EQ_TYPE;
	} else if (tor_magic != TOR_MAGIC_OVLY) {
		die("Unexpected TOR type\n");
	}

	switch (chiplet_type) {
		case EC_TYPE:
			*chiplet_info       = &EC_CHIPLET_INFO;
			*common_ring_info   = EC_COMMON_RING_INFO;
			*instance_ring_info = EC_INSTANCE_RING_INFO;
			break;
		case EQ_TYPE:
			*chiplet_info       = &EQ_CHIPLET_INFO;
			*common_ring_info   = EQ_COMMON_RING_INFO;
			*instance_ring_info = EQ_INSTANCE_RING_INFO;
			break;
		default:
			*chiplet_info       = NULL;
			*common_ring_info   = NULL;
			*instance_ring_info = NULL;
			break;
	};
}

/* Either reads ring into the buffer (on GET_RING_DATA) or treats it as an
 * instance of ring_put_info (on GET_RING_PUT_INFO) */
static bool ring_access(struct tor_hdr *ring_section, uint16_t ring_id,
			uint8_t ring_variant, uint8_t instance_id,
			void *data_buf, uint32_t *data_buf_size,
			enum ring_operation operation)
{
	const bool overlay = (be32toh(ring_section->magic) == TOR_MAGIC_OVLY);
	uint8_t i = 0;
	uint8_t chiplet_count = (overlay ? SBE_NOOF_CHIPLETS : 1);
	uint8_t max_variants = (overlay ? 1 : NUM_RING_VARIANTS);

	assert(ring_section->version == TOR_VERSION);

	for (i = 0; i < chiplet_count*2; ++i) {
		const uint8_t chiplet_idx = i/2;
		const bool instance_rings = (i%2 == 1);

		uint32_t tor_slot_idx = 0;
		uint8_t instance = 0;

		const struct ring_info *ring_info;
		uint8_t ring_count;
		struct tor_chiplet_block *blocks;
		uint32_t chiplet_offset;
		uint8_t variant_count;

		const struct chiplet_info *chiplet_info;
		const struct ring_info *common_ring_info;
		const struct ring_info *instance_ring_info;

		get_section_properties(be32toh(ring_section->magic),
				       chiplet_idx, &chiplet_info,
				       &common_ring_info, &instance_ring_info);
		if (chiplet_info == NULL)
			continue;

		ring_info = instance_rings ? instance_ring_info : common_ring_info;

		ring_count = instance_rings
			   ? chiplet_info->instance_rings_count
			   : chiplet_info->common_rings_count;
		blocks = (void *)ring_section->data;
		chiplet_offset = instance_rings
			       ? be32toh(blocks[chiplet_idx].instance_offset)
			       : be32toh(blocks[chiplet_idx].common_offset);
		/* Instance rings have only BASE variant and both EC and EQ have
		 * all of them and their order matches enumeration values */
		variant_count = (instance_rings ? 1 : max_variants);

		for (instance = ring_info->min_instance_id;
		     instance <= ring_info->max_instance_id;
		     ++instance) {
			uint8_t ring_idx;
			for (ring_idx = 0; ring_idx < ring_count; ++ring_idx) {
				if (ring_info[ring_idx].ring_id != ring_id ||
				    (instance_rings && instance != instance_id)) {
					/* Jump over all variants of the ring */
					tor_slot_idx += variant_count;
					continue;
				}

				if (variant_count > 1)
					/* Skip to the slot with the variant */
					tor_slot_idx += ring_variant;

				uint16_t *tor_slots = (void *)&ring_section->data[chiplet_offset];
				if (operation == GET_RING_DATA) {
					uint16_t slot_value = be16toh(tor_slots[tor_slot_idx]);
					uint32_t ring_slot_offset = chiplet_offset + slot_value;
					struct ring_hdr *ring = (void *)&ring_section->data[ring_slot_offset];
					uint32_t ring_size = be16toh(ring->size);

					if (slot_value == 0)
						/* Didn't find the ring */
						return false;

					if (ring->magic != htobe16(RS4_MAGIC))
						die("Got junk instead of a ring!");

					if (*data_buf_size != 0 && *data_buf_size >= ring_size)
						memcpy(data_buf, ring, ring_size);
					*data_buf_size = ring_size;
				} else if (operation == GET_RING_PUT_INFO) {
					struct ring_put_info *put_info = data_buf;

					if (tor_slots[tor_slot_idx] != 0)
						die("Slot isn't empty!");

					if (*data_buf_size != sizeof(struct ring_put_info))
						die("Invalid parameters for GET_RING_PUT_INFO!");

					put_info->chiplet_offset = sizeof(*ring_section)
								 + chiplet_offset;
					put_info->ring_slot_offset = (uint8_t*)&tor_slots[tor_slot_idx]
								   - (uint8_t*)ring_section;
				}
				return true;
			}
		}
	}

	return false;
}

/* A wrapper around ring_access() that does safety checks and tor traversal if
 * necessary*/
bool tor_access_ring(struct tor_hdr *ring_section, uint16_t ring_id,
		     enum ppe_type ppe_type, uint8_t ring_variant,
		     uint8_t instance_id, void *data_buf,
		     uint32_t *data_buf_size, enum ring_operation operation)
{
	if (be32toh(ring_section->magic) >> 8 != TOR_MAGIC ||
	    ring_section->version == 0 ||
	    ring_section->version > TOR_VERSION ||
	    ring_section->chip_type >= NUM_CHIP_TYPES)
		die("Invalid call to tor_access_ring()!");

	if (operation == GET_RING_DATA || operation == GET_RING_PUT_INFO) {
		struct tor_hdr *section = ring_section;
		if (be32toh(ring_section->magic) == TOR_MAGIC_HW) {
			struct tor_ppe_block *tor_ppe_block = (void *)ring_section->data;
			const uint32_t section_offset = be32toh(tor_ppe_block[ppe_type].offset);
			section = (void *)&ring_section->data[section_offset];
		}

		return ring_access(section, ring_id, ring_variant, instance_id,
				   data_buf, data_buf_size, operation);
	}

	if (operation == GET_PPE_LEVEL_RINGS) {
		uint32_t section_size = 0;
		uint32_t section_offset = 0;
		struct tor_ppe_block *tor_ppe_block = (void *)ring_section->data;

		assert(ring_id == UNDEFINED_RING_ID);
		assert(ring_variant == UNDEFINED_RING_VARIANT);
		assert(instance_id == UNDEFINED_INSTANCE_ID);
		assert(be32toh(ring_section->magic) == TOR_MAGIC_HW);

		section_size = be32toh(tor_ppe_block[ppe_type].size);
		section_offset = be32toh(tor_ppe_block[ppe_type].offset);

		if (*data_buf_size != 0 && *data_buf_size >= section_size)
			memcpy(data_buf, &ring_section->data[section_offset],
			       section_size);

		*data_buf_size = section_size;
		return true;
	}

	die("Unhandled TOR ring access operation!");
}

/* Retrieves an overlay ring in both compressed and uncompressed forms */
static bool get_overlays_ring(struct tor_hdr *overlays_section,
			      uint16_t ring_id, void *rs4_buf, void *raw_buf)
{
	uint32_t uncompressed_bit_size = 0;
	uint32_t rs4_buf_size = 0xFFFFFFFF;

	if (!tor_access_ring(overlays_section, ring_id, UNDEFINED_PPE_TYPE,
			     UNDEFINED_RING_VARIANT, UNDEFINED_INSTANCE_ID,
			     rs4_buf, &rs4_buf_size, GET_RING_DATA))
		return false;

	rs4_decompress(raw_buf, raw_buf + MAX_RING_BUF_SIZE/2,
		       MAX_RING_BUF_SIZE/2, &uncompressed_bit_size,
		       (struct ring_hdr *)rs4_buf);
	return true;
}

/* Decompress ring, modify it to leave only data allowed by overlay mask and
 * compress back */
static void apply_overlays_ring(struct ring_hdr *ring, uint8_t *rs4_buf,
				const uint8_t *raw_buf)
{
	uint8_t *data = rs4_buf;
	uint8_t *care = rs4_buf + MAX_RING_BUF_SIZE/2;
	const uint8_t *overlay = raw_buf + MAX_RING_BUF_SIZE/2;
	uint32_t uncompressed_bit_size;

	rs4_decompress(data, care, MAX_RING_BUF_SIZE/2, &uncompressed_bit_size,
		       ring);

	/*
	 * Copies bits from raw_buf into both data and care only if bit at the
	 * same index in overlay is set.
	 */
	for (uint32_t bit = 0; bit < uncompressed_bit_size; ++bit) {
		const int byte_idx = bit/8;
		const uint8_t bit_mask = (0x80 >> bit%8);
		if (overlay[byte_idx] & bit_mask) {
			if (raw_buf[byte_idx] & bit_mask) {
				data[byte_idx] |= bit_mask;
				care[byte_idx] |= bit_mask;
			} else {
				data[byte_idx] &= ~bit_mask;
				care[byte_idx] &= ~bit_mask;
			}
		}
	}

	rs4_compress(ring, MAX_RING_BUF_SIZE, data, care, uncompressed_bit_size,
		     be32toh(ring->scan_addr), be16toh(ring->ring_id));
}

static void apply_overlays_to_gptr(struct tor_hdr *overlays_section,
				   struct ring_hdr *ring, uint8_t *rs4_buf,
				   uint8_t *raw_buf)
{
	if (get_overlays_ring(overlays_section, be16toh(ring->ring_id), rs4_buf,
			      raw_buf)) {
		/* raw_buf is passed from get_overlays_ring(), rs4_buf is just reused */
		apply_overlays_ring(ring, rs4_buf, raw_buf);
	}
}

static void tor_append_ring(struct tor_hdr *ring_section,
			    uint32_t *ring_section_size, uint16_t ring_id,
			    enum ppe_type ppe_type, uint8_t ring_variant,
			    uint8_t instance_id, struct ring_hdr *ring)
{
	uint16_t ring_offset;
	uint32_t ring_size;

	struct ring_put_info put_info;
	uint32_t put_info_size = sizeof(put_info);

	if (!tor_access_ring(ring_section, ring_id, ppe_type,
			     ring_variant, instance_id, &put_info,
			     &put_info_size, GET_RING_PUT_INFO))
		die("Failed to find where to put a ring!");

	if (*ring_section_size - put_info.chiplet_offset > MAX_TOR_RING_OFFSET)
		die("TOR section has reached its maximum size!");

	ring_offset = htobe16(*ring_section_size - put_info.chiplet_offset);
	ring_size = be16toh(ring->size);

	memcpy((uint8_t *)ring_section + put_info.ring_slot_offset,
	       &ring_offset, sizeof(ring_offset));
	memcpy((uint8_t *)ring_section + *ring_section_size, ring, ring_size);

	*ring_section_size = be32toh(ring_section->size) + ring_size;
	ring_section->size = htobe32(*ring_section_size);
}

/*
 * Extracts a ring from CP00 record of MVPD and appends it to the ring section
 * applying overlay if necessary.  All buffers must be be at least
 * MAX_RING_BUF_SIZE bytes in length.  Indicates result by setting *ring_status.
 */
static void tor_fetch_and_insert_vpd_ring(uint8_t chip,
					  struct tor_hdr *ring_section,
					  uint32_t *ring_section_size,
					  const struct ring_query *query,
					  uint32_t max_ring_section_size,
					  struct tor_hdr *overlays_section,
					  enum ppe_type ppe_type,
					  uint8_t chiplet_id,
					  uint8_t even_odd,
					  uint8_t *buf1,
					  uint8_t *buf2,
					  uint8_t *buf3,
					  enum ring_status *ring_status)
{

	bool success = false;
	uint8_t instance_id = 0;
	struct ring_hdr *ring = NULL;

	success = mvpd_extract_ring(chip, "CP00", query->kwd_name,
				    chiplet_id, even_odd,
				    query->ring_id, buf1, MAX_RING_BUF_SIZE);
	if (!success) {
		*ring_status = RING_NOT_FOUND;
		return;
	}

	ring = (struct ring_hdr *)buf1;

	if (query->ring_class == RING_CLASS_GPTR_NEST ||
	    query->ring_class == RING_CLASS_GPTR_EQ ||
	    query->ring_class == RING_CLASS_GPTR_EX ||
	    query->ring_class == RING_CLASS_GPTR_EC)
		apply_overlays_to_gptr(overlays_section, ring, buf2, buf3);

	if (ring->magic == htobe16(RS4_MAGIC)) {
		int redundant = 0;
		rs4_redundant(ring, &redundant);
		if (redundant) {
			*ring_status = RING_REDUNDANT;
			return;
		}
	}

	if (*ring_section_size + be16toh(ring->size) > max_ring_section_size)
		die("Not enough memory to append the ring: %d > %d",
		    *ring_section_size + be16toh(ring->size),
		    max_ring_section_size);

	instance_id = chiplet_id + even_odd;
	if (query->ring_class == RING_CLASS_EX_INS)
		instance_id += chiplet_id - query->min_instance_id;

	tor_append_ring(ring_section, ring_section_size, query->ring_id,
			ppe_type, RV_BASE, instance_id, ring);

	*ring_status = RING_FOUND;
}

void tor_fetch_and_insert_vpd_rings(uint8_t chip,
				    struct tor_hdr *ring_section,
				    uint32_t *ring_section_size,
				    uint32_t max_ring_section_size,
				    struct tor_hdr *overlays_section,
				    enum ppe_type ppe_type,
				    uint8_t *buf1, uint8_t *buf2, uint8_t *buf3)
{
	const size_t pdg_query_count =
		sizeof(RING_QUERIES_PDG) / sizeof(RING_QUERIES_PDG[0]);
	const size_t pdr_query_count =
		sizeof(RING_QUERIES_PDR) / sizeof(RING_QUERIES_PDR[0]);
	const size_t ring_query_count = pdg_query_count + pdr_query_count;

	size_t i = 0;
	uint8_t eq = 0;

	const struct ring_query *eq_query = NULL;
	const struct ring_query *ec_query = NULL;

	const struct ring_query *ex_queries[4];
	uint8_t ex_query_count = 0;

	/* Add all common rings */
	for (i = 0; i < ring_query_count; ++i) {
		uint8_t instance = 0;
		uint8_t max_instance_id = 0;
		const struct ring_query *query = NULL;

		if (i < pdg_query_count)
			query = &RING_QUERIES_PDG[i];
		else
			query = &RING_QUERIES_PDR[i - pdg_query_count];

		if (query->ring_class == RING_CLASS_EQ_INS ||
		    query->ring_class == RING_CLASS_EX_INS ||
		    query->ring_class == RING_CLASS_EC_INS)
			continue;

		max_instance_id = query->max_instance_id;
		/* 0xff meant multicast in Power8, but doesn't in Power9 */
		if (max_instance_id == 0xff)
			max_instance_id = query->min_instance_id;

		if (ppe_type == PT_CME &&
		    query->ring_class != RING_CLASS_EC &&
		    query->ring_class != RING_CLASS_GPTR_EC)
			continue;

		if (ppe_type == PT_SGPE &&
		    query->ring_class != RING_CLASS_EX &&
		    query->ring_class != RING_CLASS_EQ &&
		    query->ring_class != RING_CLASS_GPTR_EQ &&
		    query->ring_class != RING_CLASS_GPTR_EX)
			continue;

		for (instance = query->min_instance_id;
		     instance <= max_instance_id;
		     ++instance) {
			enum ring_status ring_status;
			tor_fetch_and_insert_vpd_ring(chip,
						      ring_section,
						      ring_section_size,
						      query,
						      max_ring_section_size,
						      overlays_section,
						      ppe_type,
						      instance,
						      /*even_odd=*/0,
						      buf1, buf2, buf3,
						      &ring_status);

			if (ring_status == RING_NOT_FOUND)
				die("Failed to insert a common ring.");
		}
	}

	/* Add all instance rings */

	for (i = 0; i < pdr_query_count; ++i) {
		const struct ring_query *query = &RING_QUERIES_PDR[i];
		const enum ring_class class = query->ring_class;
		if (class == RING_CLASS_EQ_INS && eq_query == NULL) {
			eq_query = query;
		} else if (class == RING_CLASS_EX_INS && ex_query_count < 4) {
			ex_queries[ex_query_count] = query;
			++ex_query_count;
		} else if (class == RING_CLASS_EC_INS && ec_query == NULL) {
			ec_query = query;
		}
	}

	for (eq = 0; eq < NUM_OF_QUADS; ++eq) {
		/* EQ instances */
		if ((ppe_type == PT_SBE || ppe_type == PT_SGPE) && eq_query != NULL) {
			const uint8_t instance = eq_query->min_instance_id + eq;

			enum ring_status ring_status;

			tor_fetch_and_insert_vpd_ring(chip,
						      ring_section,
						      ring_section_size,
						      eq_query,
						      max_ring_section_size,
						      overlays_section,
						      ppe_type,
						      instance,
						      /*even_odd=*/0,
						      buf1, buf2, buf3,
						      &ring_status);

			if (ring_status == RING_NOT_FOUND)
				die("Failed to insert an EQ ring.");
		}

		/* EX instances */
		if ((ppe_type == PT_SBE || ppe_type == PT_SGPE) && ex_query_count != 0) {
			uint8_t ex = 0;
			for (ex = 2 * eq; ex < 2 * (eq + 1); ++ex) {
				for (i = 0; i < ex_query_count; ++i) {
					const uint8_t instance = ex_queries[i]->min_instance_id + eq;

					enum ring_status ring_status;

					tor_fetch_and_insert_vpd_ring(chip,
								      ring_section,
								      ring_section_size,
								      ex_queries[i],
								      max_ring_section_size,
								      overlays_section,
								      ppe_type,
								      instance,
								      ex % 2,
								      buf1, buf2, buf3,
								      &ring_status);

					if (ring_status == RING_NOT_FOUND)
						die("Failed to insert an EC ring.");
				}
			}
		}

		/* EC instances */
		if ((ppe_type == PT_SBE || ppe_type == PT_CME) && ec_query != NULL) {
			uint8_t ec = 0;
			for (ec = 4 * eq; ec < 4 * (eq + 1); ++ec) {
				const uint8_t instance = ec_query->min_instance_id + ec;

				enum ring_status ring_status;

				tor_fetch_and_insert_vpd_ring(chip,
							      ring_section,
							      ring_section_size,
							      ec_query,
							      max_ring_section_size,
							      overlays_section,
							      ppe_type,
							      instance,
							      /*even_odd=*/0,
							      buf1, buf2, buf3,
							      &ring_status);

				if (ring_status == RING_NOT_FOUND)
					die("Failed to insert an EC ring.");
			}
		}
	}
}
