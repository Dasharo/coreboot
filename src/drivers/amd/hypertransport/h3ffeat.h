/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef H3FFEAT_H
#define H3FFEAT_H

#include "h3finit.h"

/*----------------------------------------------------------------------------
 *	Mixed (DEFINITIONS AND MACROS / TYPEDEFS, STRUCTURES, ENUMS)
 *
 *----------------------------------------------------------------------------
 */

/*-----------------------------------------------------------------------------
 *			DEFINITIONS AND MACROS
 *
 *-----------------------------------------------------------------------------
 */

#define MAX_NODES 8
#define MAX_LINKS 8
#define MAX_PLATFORM_LINKS 64 /* 8x8 fully connected (28) + 4 chains with two HT devices */

/* These following are internal definitions */
#define ROUTETOSELF 0x0F
#define INVALID_LINK 0xCC     /* Used in port list data structure to mark unused data entries.
				  Can also be used for no link found in a port list search */

/* definitions for working with the port list structure */
#define PORTLIST_TYPE_CPU 0
#define PORTLIST_TYPE_IO  1

/*
 * Hypertransport Capability definitions and macros
 *
 */

/* HT Host Capability */
/* bool isHTHostCapability(u32 reg) */
#define IS_HT_HOST_CAPABILITY(reg) \
	((reg & (u32)0xE00000FF) == (u32)0x20000008)

#define HT_HOST_CAP_SIZE			0x20

/* Host CapabilityRegisters */
#define HTHOST_LINK_CAPABILITY_REG		0x00
#define HTHOST_LINK_CONTROL_REG		0x04
#define HTHOST_FREQ_REV_REG			0x08
#define HTHOST_FREQ_REV_REG_2			0x1c
	#define HT_HOST_REV_REV3		0x60
#define HTHOST_FEATURE_CAP_REG			0x0C
#define HTHOST_BUFFER_COUNT_REG		0x10
#define HTHOST_ISOC_REG			0x14
#define HTHOST_LINK_TYPE_REG			0x18
	#define HTHOST_TYPE_COHERENT		3
	#define HTHOST_TYPE_NONCOHERENT	7
	#define HTHOST_TYPE_MASK		0x1F

/* HT Slave Capability (HT1 compat) */
#define IS_HT_SLAVE_CAPABILITY(reg) \
	((reg & (u32)0xE00000FF) == (u32)0x00000008)
#define HTSLAVE_LINK01_OFFSET			4
#define HTSLAVE_LINK_CONTROL_0_REG		4
#define HTSLAVE_FREQ_REV_0_REG			0xC
#define HTSLAVE_FEATURE_CAP_REG		0x10

/* HT3 gen Capability */
#define IS_HT_GEN3_CAPABILITY(reg) \
	((reg & (u32)0xF80000FF) == (u32)0xD0000008)
#define HTGEN3_LINK01_OFFSET			0x10
#define HTGEN3_LINK_TRAINING_0_REG		0x10

/* HT3 Retry Capability */
#define IS_HT_RETRY_CAPABILITY(reg) \
	((reg & (u32)0xF80000FF) == (u32)0xC0000008)

#define HTRETRY_CONTROL_REG			4

/* Unit ID Clumping Capability */
#define IS_HT_UNITID_CAPABILITY(reg) \
	((reg & (u32)0xF80000FF) == (u32)0x90000008)

#define HTUNIT_SUPPORT_REG			4
#define HTUNIT_ENABLE_REG			8

/*----------------------------------------------------------------------------
 *			    TYPEDEFS, STRUCTURES, ENUMS
 *
 *----------------------------------------------------------------------------
 */

typedef struct cNorthBridge cNorthBridge;

/* A pair consists of a source node, a link to the destination node, the
 * destination node, and its link back to source node.	 The even indices are
 * the source nodes and links, and the odd indices are for the destination
 * nodes and links.
 */
typedef struct
{
	/* This section is where the link is in the system and how to find it */
	/* 0 = CPU, 1 = Device, all others reserved */
	u8 type;
	/* 0-1 for devices, 0-7 for CPUs */
	u8 link;
	/* The node, or a pointer to the devices parent node */
	u8 node_id;
	/* Link of parent node + depth in chain.  Only used by devices */
	u8 host_link, host_depth;
	/* A pointer to the device's slave HT capability, so we don't have to keep searching */
	SBDFO pointer;

	/* This section is for the final settings, which are written to hardware */
	BOOL sel_regang; /* Only used for CPU->CPU links */
	u8 sel_width_in;
	u8 sel_width_out;
	u8 sel_frequency;
	uint8_t enable_isochronous_mode;

	/* This section is for keeping track of capabilities and possible configurations */
	BOOL regang_cap;
	uint32_t prv_frequency_cap;
	uint32_t prv_feature_cap;
	u8 prv_width_in_cap;
	u8 prv_width_out_cap;
	uint32_t composite_frequency_cap;

} sPortDescriptor;


/*
 * Our global state data structure
 */
typedef struct {
	AMD_HTBLOCK *ht_block;

	u8 nodes_discovered;	 /* One less than the number of nodes found in the system */
	u8 total_links;
	u8 sys_mp_cap;	/* The maximum number of nodes that all processors are capable of */

	/* Two ports for each link
	 * Note: The Port pair 2*N and 2*N+1 are connected together to form a link
	 * (e.g. 0,1 and 8,9 are ports on either end of an HT link) The lower number
	 * port (2*N) is the source port.	The device that owns the source port is
	 * always the device closer to the BSP. (i.e. nearer the CPU in a
	 * non-coherent chain, or the CPU with the lower node_id).
	 */
	sPortDescriptor port_list[MAX_PLATFORM_LINKS*2];

	/* The number of coherent links coming off of each node
	 * (i.e. the 'Degree' of the node)
	 */
	u8 sys_degree[MAX_NODES];
	/* The systems adjency (sys_matrix[i][j] is true if Node_i has a link to Node_j) */
	BOOL sys_matrix[MAX_NODES][MAX_NODES];

	/* Same as above, but for the currently selected database entry */
	u8 db_degree[MAX_NODES];
	BOOL db_matrix[MAX_NODES][MAX_NODES];

	u8 perm[MAX_NODES];	 /* The node mapping from the database to the system */
	u8 reverse_perm[MAX_NODES];	 /* The node mapping from the system to the database */

	/* Data for non-coherent initialization */
	u8 auto_bus_current;
	u8 used_cfg_map_entires;

	/* 'This' pointer for northbridge */
	cNorthBridge *nb;
} sMainData;

#endif	 /* H3FFEAT_H */
