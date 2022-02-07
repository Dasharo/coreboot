/* SPDX-License-Identifier: GPL-2.0-only */

/*----------------------------------------------------------------------------
 *				MODULES USED
 *
 *----------------------------------------------------------------------------
 */

#include "h3ncmn.h"
#include "h3finit.h"
#include "h3ffeat.h"
#include "AsPsNb.h"

#include <arch/cpu.h>
#include <device/pci.h>
#include <cpu/amd/common/common.h>
#include <cpu/amd/msr.h>
#include <device/pci_def.h>
#include <northbridge/amd/amdfam10/raminit.h>
#include <northbridge/amd/amdfam10/amdfam10.h>

/*----------------------------------------------------------------------------
 *			DEFINITIONS AND MACROS
 *
 *----------------------------------------------------------------------------
 */

/* CPU Northbridge Functions */
#define CPU_HTNB_FUNC_00		0
#define CPU_HTNB_FUNC_04		4
#define CPU_ADDR_FUNC_01		1
#define CPU_NB_FUNC_03			3
#define CPU_NB_FUNC_05			5

/* Function 0 registers */
#define REG_ROUTE0_0X40		0x40
#define REG_ROUTE1_0X44		0x44
#define REG_NODE_ID_0X60		0x60
#define REG_UNIT_ID_0X64		0x64
#define REG_LINK_TRANS_CONTROL_0X68	0x68
#define REG_LINK_INIT_CONTROL_0X6C	0x6c
#define REG_HT_CAP_BASE_0X80		0x80
#define REG_NORTHBRIDGE_CFG_3X8C	0x8c
#define REG_HT_LINK_RETRY0_0X130	0x130
#define REG_HT_TRAFFIC_DIST_0X164	0x164
#define REG_HT_LINK_EXT_CONTROL0_0X170	0x170

#define HT_CONTROL_CLEAR_CRC		(~(3 << 8))

/* Function 1 registers */
#define REG_ADDR_CONFIG_MAP0_1XE0	0xE0
#define CPU_ADDR_NUM_CONFIG_MAPS	4

/* Function 3 registers */
#define REG_NB_SRI_XBAR_BUF_3X70	0x70
#define REG_NB_MCT_XBAR_BUF_3X78	0x78
#define REG_NB_FIFOPTR_3XDC		0xDC
#define REG_NB_CAPABILITY_3XE8		0xE8
#define REG_NB_CPUID_3XFC		0xFC
#define REG_NB_LINK_XCS_TOKEN0_3X148	0x148
#define REG_NB_DOWNCORE_3X190		0x190
#define REG_NB_CAPABILITY_5X84		0x84

/* Function 4 registers */


/*----------------------------------------------------------------------------
 *			TYPEDEFS AND STRUCTURES
 *
 *----------------------------------------------------------------------------
 */
/*----------------------------------------------------------------------------
 *			PROTOTYPES OF LOCAL FUNCTIONS
 *
 *----------------------------------------------------------------------------
 */

/***************************************************************************
 ***			FAMILY/NORTHBRIDGE SPECIFIC FUNCTIONS		***
 ***************************************************************************/

/***************************************************************************//**
 *
 * SBDFO
 * make_link_base(u8 current_node, u8 current_link)
 *
 *  Description:
 *	Private to northbridge implementation. Return the HT Host capability base
 *	PCI config address for a link.
 *
 *  Parameters:
 *	@param[in]  node    = the node this link is on
 *	@param[in]  link    = the link
 *
 *****************************************************************************/
static SBDFO make_link_base(u8 node, u8 link)
{
	SBDFO link_base;

	/* With rev F can not be called with a 4th link or with the sublinks */
	if (link < 4)
		link_base = MAKE_SBDFO(make_pci_segment_from_node(node),
				make_pci_bus_from_node(node),
				make_pci_device_from_node(node),
				CPU_HTNB_FUNC_00,
				REG_HT_CAP_BASE_0X80 + link*HT_HOST_CAP_SIZE);
	else
		link_base = MAKE_SBDFO(make_pci_segment_from_node(node),
				make_pci_bus_from_node(node),
				make_pci_device_from_node(node),
				CPU_HTNB_FUNC_04,
				REG_HT_CAP_BASE_0X80 + (link-4)*HT_HOST_CAP_SIZE);
	return link_base;
}

/***************************************************************************//**
 *
 * void
 * set_ht_control_register_bits(SBDFO reg, u8 hi_bit, u8 lo_bit, u32 *p_value)
 *
 *  Description:
 *	Private to northbridge implementation. Provide a common routine for accessing the
 *	HT Link Control registers (84, a4, c4, e4), to enforce not clearing the
 *	HT CRC error bits.  Replaces direct use of amd_pci_write_bits().
 *	NOTE: This routine is called for IO Devices as well as CPUs!
 *
 *  Parameters:
 *	@param[in]  reg    = the PCI config address the control register
 *	@param[in]  hi_bit  = the high bit number
 *	@param[in]  lo_bit  = the low bit number
 *	@param[in]  p_value = the value to write to that bit range. Bit 0 => lo_bit.
 *
 *****************************************************************************/
static void set_ht_control_register_bits(SBDFO reg, u8 hi_bit, u8 lo_bit, u32 *p_value)
{
	u32 temp, mask;

	ASSERT((hi_bit < 32) && (lo_bit < 32) && (hi_bit >= lo_bit) && ((reg & 0x3) == 0));
	ASSERT((hi_bit < 8) || (lo_bit > 9));

	/* A 1<<32 == 1<<0 due to x86 SHL instruction, so skip if that is the case */
	if ((hi_bit-lo_bit) != 31)
		mask = (((u32)1 << (hi_bit - lo_bit + 1)) - 1);
	else
		mask = (u32)0xFFFFFFFF;

	amd_pci_read(reg, &temp);
	temp &= ~(mask << lo_bit);
	temp |= (*p_value & mask) << lo_bit;
	temp &= (u32)HT_CONTROL_CLEAR_CRC;
	amd_pci_write(reg, &temp);
}

/***************************************************************************//**
 *
 * static void
 * write_routing_table(u8 node, u8 target, u8 Link, cNorthBridge *nb)
 *
 *  Description:
 *	 This routine will modify the routing tables on the
 *	 SourceNode to cause it to route both request and response traffic to the
 *	 target_node through the specified Link.
 *
 *	 NOTE: This routine is to be used for early discovery and initialization.  The
 *	 final routing tables must be loaded some other way because this
 *	 routine does not address the issue of probes, or independent request
 *	 response paths.
 *
 *  Parameters:
 *	@param[in]  node    = the node that will have it's routing tables modified.
 *	@param[in]  target  = For routing to node target
 *	@param[in]  link    =  Link from node to target
 *	@param[in]  *nb   = this northbridge
 *
 *****************************************************************************/

static void write_routing_table(u8 node, u8 target, u8 link, cNorthBridge *nb)
{
#ifndef HT_BUILD_NC_ONLY
	u32 temp = (nb->self_route_response_mask | nb->self_route_request_mask) << (link + 1);
	ASSERT((node < nb->max_nodes) && (target < nb->max_nodes) && (link < nb->max_links));
	amd_pci_write(MAKE_SBDFO(make_pci_segment_from_node(node),
			make_pci_bus_from_node(node),
			make_pci_device_from_node(node),
			CPU_HTNB_FUNC_00,
			REG_ROUTE0_0X40 + target * 4),
			&temp);
#else
	STOP_HERE;
#endif
}

/***************************************************************************//**
 *
 * static void
 * write_node_id(u8 node, u8 node_id, cNorthBridge *nb)
 *
 *  Description:
 *	Modifies the node_id register on the target node
 *
 *  Parameters:
 *	@param[in] node    = the node that will have its node_id altered.
 *	@param[in] node_id  = the new value for node_id
 *	@param[in] *nb     = this northbridge
 *
 *****************************************************************************/

static void write_node_id(u8 node, u8 node_id, cNorthBridge *nb)
{
	u32 temp;
	ASSERT((node < nb->max_nodes) && (node_id < nb->max_nodes));
	if (is_fam15h()) {
		temp = 1;
		amd_pci_write_bits(MAKE_SBDFO(make_pci_segment_from_node(node),
					make_pci_bus_from_node(node),
					make_pci_device_from_node(node),
					CPU_NB_FUNC_03,
					REG_NORTHBRIDGE_CFG_3X8C),
					22, 22, &temp);
	}
	temp = node_id;
	amd_pci_write_bits(MAKE_SBDFO(make_pci_segment_from_node(node),
				make_pci_bus_from_node(node),
				make_pci_device_from_node(node),
				CPU_HTNB_FUNC_00,
				REG_NODE_ID_0X60),
				2, 0, &temp);
}

/***************************************************************************//**
 *
 * static void
 * read_def_lnk(u8 node, cNorthBridge *nb)
 *
 *  Description:
 *	 Read the DefLnk (the source link of the current packet)
 *	 from node
 *
 *  Parameters:
 *	@param[in] node    = the node that will have its node_id altered.
 *	@param[in] *nb     = this northbridge
 *	@return                 The HyperTransport link where the request to
 *				read the default link came from.  Since this
 *				code is running on the BSP, this should be the link
 *				pointing back towards the BSP.
 *
 *****************************************************************************/

static u8 read_def_lnk(u8 node, cNorthBridge *nb)
{
	u32 deflink = 0;
	SBDFO licr;
	u32 temp;

	licr = MAKE_SBDFO(make_pci_segment_from_node(node),
			make_pci_bus_from_node(node),
			make_pci_device_from_node(node),
			CPU_HTNB_FUNC_00,
			REG_LINK_INIT_CONTROL_0X6C);

	ASSERT((node < nb->max_nodes));
	amd_pci_read_bits(licr, 3, 2, &deflink);
	amd_pci_read_bits(licr, 8, 8, &temp);	/* on rev F, this bit is reserved == 0 */
	deflink |= temp << 2;
	return (u8)deflink;
}

/***************************************************************************//**
 *
 * static void
 * enable_routing_tables(u8 node, cNorthBridge *nb)
 *
 *  Description:
 *	Turns routing tables on for a given node
 *
 *  Parameters:
 *	@param[in]  node = the node that will have it's routing tables enabled
 *	@param[in]  *nb  = this northbridge
 *
 *****************************************************************************/

static void enable_routing_tables(u8 node, cNorthBridge *nb)
{
	u32 temp = 0;
	ASSERT((node < nb->max_nodes));
	amd_pci_write_bits(MAKE_SBDFO(make_pci_segment_from_node(node),
				make_pci_bus_from_node(node),
				make_pci_device_from_node(node),
				CPU_HTNB_FUNC_00,
				REG_LINK_INIT_CONTROL_0X6C),
				0, 0, &temp);
}


/***************************************************************************//**
 *
 * static BOOL
 * verify_link_is_coherent(u8 node, u8 Link, cNorthBridge *nbk)
 *
 *  Description:
 *	Verify that the link is coherent, connected, and ready
 *
 *  Parameters:
 *	@param[in]   node      = the node that will be examined
 *	@param[in]   link      = the link on that Node to examine
 *	@param[in]   *nb       = this northbridge
 *	@return            true - The link has the following status
 *				  linkCon = 1,		Link is connected
 *				  INIT_COMPLETE = 1,	Link initialization is complete
 *				  NC = 0,		Link is coherent
 *				  UniP-cLDT = 0,	Link is not Uniprocessor cLDT
 *				  LinkConPend = 0	Link connection is not pending
 *				  false- The link has some other status
 *
 *****************************************************************************/

static BOOL verify_link_is_coherent(u8 node, u8 link, cNorthBridge *nb)
{
#ifndef HT_BUILD_NC_ONLY

	u32 link_type;
	SBDFO link_base;

	ASSERT((node < nb->max_nodes) && (link < nb->max_links));

	link_base = make_link_base(node, link);

	/*  FN0_98/A4/C4 = LDT Type Register */
	amd_pci_read(link_base + HTHOST_LINK_TYPE_REG, &link_type);

	/*  Verify LinkCon = 1, INIT_COMPLETE = 1, NC = 0, UniP-cLDT = 0, LinkConPend = 0 */
	return (link_type & HTHOST_TYPE_MASK) ==  HTHOST_TYPE_COHERENT;
#else
	return 0;
#endif /* HT_BUILD_NC_ONLY */
}

/***************************************************************************//**
 *
 * static bool
 * read_true_link_fail_status(u8 node, u8 link, sMainData *p_dat, cNorthBridge *nb)
 *
 *  Description:
 *	Return the LinkFailed status AFTER an attempt is made to clear the bit.
 *	Also, call event notify if a Hardware Fault caused a synch flood on a previous boot.
 *
 *	The table below summarizes correct responses of this routine.
 *	Family	  before    after    unconnected    Notify?    return
 *	  0F         0       0          0             No         0
 *	  0F         1       0          0             Yes        0
 *	  0F         1       1          X             No         1
 *	  10         0       0          0             No         0
 *	  10         1       0          0             Yes        0
 *	  10         1       0          3             No         1
 *
 *  Parameters:
 *	@param[in]    node      = the node that will be examined
 *	@param[in]    link      = the link on that node to examine
 *	@param[in]    *p_dat = access to call back routine
 *	@param[in]    *nb       = this northbridge
 *	@return                   true - the link is not connected or has hard error
 *	                          false- if the link is connected
 *
 *****************************************************************************/

static BOOL read_true_link_fail_status(u8 node, u8 link, sMainData *p_dat, cNorthBridge *nb)
{
	u32 before, after, unconnected, crc;
	SBDFO link_base;

	ASSERT((node < nb->max_nodes) && (link < nb->max_links));

	link_base = make_link_base(node, link);

	/* Save the CRC status before doing anything else.
	 * Read, Clear, the Re-read the error bits in the Link Control Register
	 * FN0_84/A4/C4[4] = LinkFail bit
	 * and the connection status, TransOff and EndOfChain
	 */
	amd_pci_read_bits(link_base + HTHOST_LINK_CONTROL_REG, 9, 8, &crc);
	amd_pci_read_bits(link_base + HTHOST_LINK_CONTROL_REG, 4, 4, &before);
	set_ht_control_register_bits(link_base + HTHOST_LINK_CONTROL_REG, 4, 4, &before);
	amd_pci_read_bits(link_base + HTHOST_LINK_CONTROL_REG, 4, 4, &after);
	amd_pci_read_bits(link_base + HTHOST_LINK_CONTROL_REG, 7, 6, &unconnected);

	if (before != after)
	{
		if (!unconnected)
		{
			if (crc != 0)
			{
				/* A synch flood occurred due to HT CRC */
				if (p_dat->ht_block->amd_cb_event_notify)
				{
					/* Pass the node and link on which the generic synch
					 * flood event occurred.
					 */
					sHtEventHWHtCrc evt;
					evt.e_size = sizeof(sHtEventHWHtCrc);
					evt.node = node;
					evt.link = link;
					evt.lane_mask = (u8)crc;

					p_dat->ht_block->amd_cb_event_notify(
								HT_EVENT_CLASS_HW_FAULT,
								HT_EVENT_HW_HTCRC,
								(u8 *)&evt);
				}
			}
			else
			{
				/* Some synch flood occurred */
				if (p_dat->ht_block->amd_cb_event_notify)
				{
					/* Pass the node and link on which the generic synch
					 * flood event occurred.
					 */
					sHtEventHWSynchFlood evt;
					evt.e_size = sizeof(sHtEventHWSynchFlood);
					evt.node = node;
					evt.link = link;

					p_dat->ht_block->amd_cb_event_notify(
								HT_EVENT_CLASS_HW_FAULT,
								HT_EVENT_HW_SYNCHFLOOD,
								(u8 *)&evt);
				}
			}
		}
	}
	return ((after != 0) || unconnected);
}


/***************************************************************************//**
 *
 * static u8
 * read_token(u8 node, cNorthBridge *nb)
 *
 *  Description:
 *	Read the token stored in the scratchpad register
 *	NOTE: The location used to store the token is arbitrary.  The only
 *	requirement is that the location warm resets to zero, and that
 *	using it will have no ill-effects during HyperTransport initialization.
 *
 *  Parameters:
 *	@param[in]  node      = the node that will be examined
 *	@param[in]  *nb       = this northbridge
 *	@return                the Token read from the node
 *
 *****************************************************************************/
static u8 read_token(u8 node, cNorthBridge *nb)
{
	u32 temp;

	ASSERT((node < nb->max_nodes));
	/* Use CpuCnt as a scratch register */
	/* Limiting use to 4 bits makes code GH to rev F compatible. */
	amd_pci_read_bits(MAKE_SBDFO(make_pci_segment_from_node(node),
				make_pci_bus_from_node(node),
				make_pci_device_from_node(node),
				CPU_HTNB_FUNC_00,
				REG_NODE_ID_0X60),
				19, 16, &temp);

	return (u8)temp;
}


/***************************************************************************//**
 *
 * static void
 * writeToken(u8 node, u8 Value, cNorthBridge *nb)
 *
 *  Description:
 *	Write the token stored in the scratchpad register
 *	NOTE: The location used to store the token is arbitrary.  The only
 *	requirement is that the location warm resets to zero, and that
 *	using it will have no ill-effects during HyperTransport initialization.
 *	Limiting use to 4 bits makes code GH to rev F compatible.
 *
 *  Parameters:
 *	@param[in]  node  = the node that will be examined
 *	@param      value
 *	@param[in] *nb  = this northbridge
 *
 *****************************************************************************/
static void writeToken(u8 node, u8 value, cNorthBridge *nb)
{
	u32 temp = value;
	ASSERT((node < nb->max_nodes));
	/* Use CpuCnt as a scratch register */
	/* Limiting use to 4 bits makes code GH to rev F compatible. */
	amd_pci_write_bits(MAKE_SBDFO(make_pci_segment_from_node(node),
					make_pci_bus_from_node(node),
					make_pci_device_from_node(node),
					CPU_HTNB_FUNC_00,
					REG_NODE_ID_0X60),
					19, 16, &temp);
}

/***************************************************************************//**
 *
 * static u8
 * fam_0f_get_num_cores_on_node(u8 node, cNorthBridge *nb)
 *
 *  Description:
 *	Return the number of cores (1 based count) on node.
 *
 *  Parameters:
 *	@param[in]  node      = the node that will be examined
 *	@param[in] *nb = this northbridge
 *	@return    = the number of cores
 *
 * ---------------------------------------------------------------------------------------
 */
static u8 fam_0f_get_num_cores_on_node(u8 node, cNorthBridge *nb)
{
	u32 temp;

	ASSERT((node < nb->max_nodes));
	/* Read CmpCap */
	amd_pci_read_bits(MAKE_SBDFO(make_pci_segment_from_node(node),
			make_pci_bus_from_node(node),
			make_pci_device_from_node(node),
			CPU_NB_FUNC_03,
			REG_NB_CAPABILITY_3XE8),
			13, 12, &temp);

	/* and add one */
	return (u8)(temp + 1);
}

/***************************************************************************//**
 *
 * static u8
 * fam_10_get_num_cores_on_node(u8 node, cNorthBridge *nb)
 *
 *  Description:
 *	Return the number of cores (1 based count) on node.
 *
 *  Parameters:
 *	@param[in]  node      = the node that will be examined
 *	@param[in] *nb = this northbridge
 *	@return    = the number of cores
 *
 *
 */
static u8 fam_10_get_num_cores_on_node(u8 node, cNorthBridge *nb)
{
	u32 temp, leveling, cores;
	u8 i;

	ASSERT((node < nb->max_nodes));
	/* Read CmpCap [2][1:0] */
	amd_pci_read_bits(MAKE_SBDFO(make_pci_segment_from_node(node),
				make_pci_bus_from_node(node),
				make_pci_device_from_node(node),
				CPU_NB_FUNC_03,
				REG_NB_CAPABILITY_3XE8),
				15, 12, &temp);

	/* bits[15,13,12] specify the cores */
	temp = ((temp & 8) >> 1) + (temp & 3);
	cores = temp + 1;

	/* Support Downcoring */
	amd_pci_read_bits (MAKE_SBDFO(make_pci_segment_from_node(node),
					make_pci_bus_from_node(node),
					make_pci_device_from_node(node),
					CPU_NB_FUNC_03,
					REG_NB_DOWNCORE_3X190),
					3, 0, &leveling);
	for (i = 0; i < cores; i++)
	{
		if (leveling & ((u32) 1 << i))
		{
			temp--;
		}
	}
	return (u8)(temp + 1);
}

/***************************************************************************//**
 *
 * static u8
 * fam_15_get_num_cores_on_node(u8 node, cNorthBridge *nb)
 *
 *  Description:
 *	Return the number of cores (1 based count) on node.
 *
 *  Parameters:
 *	@param[in]  node      = the node that will be examined
 *	@param[in] *nb = this northbridge
 *	@return    = the number of cores
 *
 *
 */
static u8 fam_15_get_num_cores_on_node(u8 node, cNorthBridge *nb)
{
	u32 temp, leveling, cores;
	u8 i;

	ASSERT((node < nb->max_nodes));
	/* Read CmpCap [7:0] */
	amd_pci_read_bits(MAKE_SBDFO(make_pci_segment_from_node(node),
				make_pci_bus_from_node(node),
				make_pci_device_from_node(node),
				CPU_NB_FUNC_05,
				REG_NB_CAPABILITY_5X84),
				7, 0, &temp);

	/* bits[7:0] specify the cores */
	temp = temp & 0xff;
	cores = temp + 1;

	/* Support Downcoring */
	amd_pci_read_bits (MAKE_SBDFO(make_pci_segment_from_node(node),
					make_pci_bus_from_node(node),
					make_pci_device_from_node(node),
					CPU_NB_FUNC_03,
					REG_NB_DOWNCORE_3X190),
					31, 0, &leveling);
	for (i = 0; i < cores; i++)
	{
		if (leveling & ((u32) 1 << i))
		{
			temp--;
		}
	}
	return (u8)(temp + 1);
}

/***************************************************************************//**
 *
 * static void
 * set_total_nodes_and_cores(u8 node, u8 total_nodes, u8 total_cores, cNorthBridge *nb)
 *
 *  Description:
 *	Write the total number of cores and nodes to the node
 *
 *  Parameters:
 *	@param[in]  node   = the node that will be examined
 *	@param[in]  total_nodes  = the total number of nodes
 *	@param[in]  total_cores  = the total number of cores
 *	@param[in] *nb   = this northbridge
 *
 * ---------------------------------------------------------------------------------------
 */
static void set_total_nodes_and_cores(u8 node, u8 total_nodes, u8 total_cores, cNorthBridge *nb)
{
	SBDFO node_id_reg;
	u32 temp;

	ASSERT((node < nb->max_nodes) && (total_nodes <= nb->max_nodes));
	node_id_reg = MAKE_SBDFO(make_pci_segment_from_node(node),
				make_pci_bus_from_node(node),
				make_pci_device_from_node(node),
				CPU_HTNB_FUNC_00,
				REG_NODE_ID_0X60);

	temp = total_cores-1;
	/* Rely on max number of nodes:cores for rev F and GH to make
	 * this code work, even though we write reserved bit 20 on rev F it will be
	 * zero in that case.
	 */
	amd_pci_write_bits(node_id_reg, 20, 16, &temp);
	temp = total_nodes-1;
	amd_pci_write_bits(node_id_reg, 6,  4, &temp);
}

/***************************************************************************//**
 *
 * static void
 * limit_nodes(u8 node, cNorthBridge *nb)
 *
 *  Description:
 *	Limit coherent config accesses to cpus as indicated by nodecnt.
 *
 *  Parameters:
 *	@param[in]  node  = the node that will be examined
 *	@param[in] *nb  = this northbridge
 *
 * ---------------------------------------------------------------------------------------
 */
static void limit_nodes(u8 node, cNorthBridge *nb)
{
	u32 temp = 1;
	ASSERT((node < nb->max_nodes));
	amd_pci_write_bits(MAKE_SBDFO(make_pci_segment_from_node(node),
				make_pci_bus_from_node(node),
				make_pci_device_from_node(node),
				CPU_HTNB_FUNC_00,
				REG_LINK_TRANS_CONTROL_0X68),
				15, 15, &temp);
}

/***************************************************************************//**
 *
 * static void
 * write_full_routing_table(u8 node, u8 target, u8 req_link, u8 rsp_link, u32 BClinks,
 * 				cNorthBridge *nb)
 *
 *  Description:
 *	Write the routing table entry for node to target, using the request link, response
 *	link, and broadcast links provided.
 *
 *  Parameters:
 *	@param[in]  node   = the node that will be examined
 *	@param[in]  target   = the target node for these routes
 *	@param[in]  req_link  = the link for requests to target
 *	@param[in]  rsp_link  = the link for responses to target
 *	@param[in]  bc_links  = the broadcast links
 *	@param[in] *nb  = this northbridge
 *
 * ---------------------------------------------------------------------------------------
 */
static void write_full_routing_table(u8 node, u8 target, u8 req_link, u8 rsp_link, u32 bc_links,
					cNorthBridge *nb)
{
#ifndef HT_BUILD_NC_ONLY
	u32 value = 0;

	ASSERT((node < nb->max_nodes) && (target < nb->max_nodes));
	if (req_link == ROUTETOSELF)
		value |= nb->self_route_request_mask;
	else
		value |= nb->self_route_request_mask << (req_link + 1);

	if (rsp_link == ROUTETOSELF)
		value |= nb->self_route_response_mask;
	else
		value |= nb->self_route_response_mask << (rsp_link + 1);

	/* Allow us to accept a Broadcast ourselves, then set broadcasts for routes */
	value |= (u32)1 << nb->broadcast_self_bit;
	value |= (u32)bc_links << (nb->broadcast_self_bit + 1);

	amd_pci_write(MAKE_SBDFO(make_pci_segment_from_node(node),
				make_pci_bus_from_node(node),
				make_pci_device_from_node(node),
				CPU_HTNB_FUNC_00,
				REG_ROUTE0_0X40 + target * 4), &value);
#else
	STOP_HERE;
#endif /* HT_BUILD_NC_ONLY */
}

/***************************************************************************//**
 *
 * static u32
 * make_key(u8 current_node)
 *
 *  Description:
 *	Private routine to northbridge code.
 *	Determine whether a node is compatible with the discovered configuration so
 *	far.  Currently, that means the family, extended family of the new node are the
 *	same as the BSP's.
 *
 *  Parameters:
 *	@param[in]   node   = the node
 *	@return = the key value
 *
 * ---------------------------------------------------------------------------------------
 */
static u32 make_key(u8 node)
{
	u32 ext_fam, base_fam;
	amd_pci_read_bits(MAKE_SBDFO(make_pci_segment_from_node(node),
				make_pci_bus_from_node(node),
				make_pci_device_from_node(node),
				CPU_NB_FUNC_03,
				REG_NB_CPUID_3XFC),
				27, 20, &ext_fam);
	amd_pci_read_bits(MAKE_SBDFO(make_pci_segment_from_node(node),
				make_pci_bus_from_node(node),
				make_pci_device_from_node(node),
				CPU_NB_FUNC_03,
				REG_NB_CPUID_3XFC),
				11, 8, &base_fam);
	return ((u32)(base_fam << 8) | ext_fam);
}


/***************************************************************************//**
 *
 * static BOOL
 * is_compatible(u8 current_node, cNorthBridge *nb)
 *
 *  Description:
 *	Determine whether a node is compatible with the discovered configuration so
 *	far.  Currently, that means the family, extended family of the new node are the
 *	same as the BSP's.
 *
 *  Parameters:
 *	@param[in]  node   = the node
 *	@param[in] *nb  = this northbridge
 *	@return = true: the new is compatible, false: it is not
 *
 * ---------------------------------------------------------------------------------------
 */
static BOOL is_compatible(u8 node, cNorthBridge *nb)
{
	return (make_key(node) == nb->compatible_key);
}

/***************************************************************************//**
 *
 * static BOOL
 * fam_0f_is_capable(u8 node, sMainData *p_dat, cNorthBridge *nb)
 *
 *  Description:
 *	Get node capability and update the minimum supported system capability.
 *	Return whether the current configuration exceeds the capability.
 *
 *  Parameters:
 *	@param[in]       node = the node
 *	@param[in,out]  *p_dat = sys_mp_cap (updated) and nodes_discovered
 *	@param[in]        *nb = this northbridge
 *	@return               true:  system is capable of current config.
 *			      false: system is not capable of current config.
 *
 * ---------------------------------------------------------------------------------------
 */
static BOOL fam_0f_is_capable(u8 node, sMainData *p_dat, cNorthBridge *nb)
{
#ifndef HT_BUILD_NC_ONLY
	u32 temp;
	u8 max_nodes;

	ASSERT(node < nb->max_nodes);

	amd_pci_read_bits(MAKE_SBDFO(make_pci_segment_from_node(node),
				make_pci_bus_from_node(node),
				make_pci_device_from_node(node),
				CPU_NB_FUNC_03,
				REG_NB_CAPABILITY_3XE8),
				2, 1, &temp);
	if (temp > 1)
	{
		max_nodes = 8;
	} else if (temp == 1) {
		max_nodes = 2;
	}
	else {
		max_nodes = 1;
	}

	if (p_dat->sys_mp_cap > max_nodes)
	{
		 p_dat->sys_mp_cap = max_nodes;
	}
	/* Note since sys_mp_cap is one based and nodes_discovered is zero based, equal is false
	*/
	return (p_dat->sys_mp_cap > p_dat->nodes_discovered);
#else
	return 1;
#endif
}

/***************************************************************************//**
 *
 * static BOOL
 * fam_10_is_capable(u8 node, sMainData *p_dat, cNorthBridge *nb)
 *
 *  Description:
 *	Get node capability and update the minimum supported system capability.
 *	Return whether the current configuration exceeds the capability.
 *
 *  Parameters:
 *	@param[in]  node   = the node
 *	@param[in,out] *p_dat = sys_mp_cap (updated) and nodes_discovered
 *	@param[in] *nb   = this northbridge
 *	@return             true: system is capable of current config.
 *			   false: system is not capable of current config.
 *
 * ---------------------------------------------------------------------------------------
 */
static BOOL fam_10_is_capable(u8 node, sMainData *p_dat, cNorthBridge *nb)
{
#ifndef HT_BUILD_NC_ONLY
	u32 temp;
	u8 max_nodes;

	ASSERT(node < nb->max_nodes);

	amd_pci_read_bits(MAKE_SBDFO(make_pci_segment_from_node(node),
				make_pci_bus_from_node(node),
				make_pci_device_from_node(node),
				CPU_NB_FUNC_03,
				REG_NB_CAPABILITY_3XE8),
				18, 16, &temp);

	if (temp != 0)
	{
		max_nodes = (1 << (~temp & 0x3));  /* That is, 1, 2, 4, or 8 */
	}
	else
	{
		max_nodes = 8;
	}

	if (p_dat->sys_mp_cap > max_nodes)
	{
		p_dat->sys_mp_cap = max_nodes;
	}
	/* Note since sys_mp_cap is one based and nodes_discovered is zero based, equal is false
	*/
	return (p_dat->sys_mp_cap > p_dat->nodes_discovered);
#else
	return 1;
#endif
}

/***************************************************************************//**
 *
 * static BOOL
 * fam_15_is_capable(u8 node, sMainData *p_dat, cNorthBridge *nb)
 *
 *  Description:
 *	Get node capability and update the minimum supported system capability.
 *	Return whether the current configuration exceeds the capability.
 *
 *  Parameters:
 *	@param[in]  node   = the node
 *	@param[in,out] *p_dat = sys_mp_cap (updated) and nodes_discovered
 *	@param[in] *nb   = this northbridge
 *	@return             true: system is capable of current config.
 *			   false: system is not capable of current config.
 *
 * ---------------------------------------------------------------------------------------
 */
static BOOL fam_15_is_capable(u8 node, sMainData *p_dat, cNorthBridge *nb)
{
#ifndef HT_BUILD_NC_ONLY
	u32 temp;
	u8 max_nodes;

	ASSERT(node < nb->max_nodes);

	amd_pci_read_bits(MAKE_SBDFO(make_pci_segment_from_node(node),
				make_pci_bus_from_node(node),
				make_pci_device_from_node(node),
				CPU_NB_FUNC_03,
				REG_NB_CAPABILITY_3XE8),
				18, 16, &temp);

	if (temp != 0)
	{
		max_nodes = (1 << (~temp & 0x3));  /* That is, 1, 2, 4, or 8 */
	}
	else
	{
		/* Check if CPU package is dual node */
		amd_pci_read_bits(MAKE_SBDFO(make_pci_segment_from_node(node),
					make_pci_bus_from_node(node),
					make_pci_device_from_node(node),
					CPU_NB_FUNC_03,
					REG_NB_CAPABILITY_3XE8),
					29, 29, &temp);
		if (temp)
			max_nodes = 4;
		else
			max_nodes = 8;
	}

	if (p_dat->sys_mp_cap > max_nodes)
	{
		p_dat->sys_mp_cap = max_nodes;
	}
	/* Note since sys_mp_cap is one based and nodes_discovered is zero based, equal is false
	*/
	return (p_dat->sys_mp_cap > p_dat->nodes_discovered);
#else
	return 1;
#endif
}

/***************************************************************************//**
 *
 * static void
 * fam_0f_stop_link(u8 current_node, u8 current_link, cNorthBridge *nb)
 *
 *  Description:
 *	Disable a cHT link on node by setting F0x[E4, C4, A4, 84][TransOff, EndOfChain]=1
 *
 *  Parameters:
 *	@param[in]  node      = the node this link is on
 *	@param[in]  link      = the link to stop
 *	@param[in] *nb = this northbridge
 *
 * ---------------------------------------------------------------------------------------
 */
static void fam_0f_stop_link(u8 node, u8 link, cNorthBridge *nb)
{
#ifndef HT_BUILD_NC_ONLY
	u32 temp;
	SBDFO link_base;

	ASSERT((node < nb->max_nodes) && (link < nb->max_links));

	link_base = make_link_base(node, link);

	/* Set TransOff, EndOfChain */
	temp = 3;
	set_ht_control_register_bits(link_base + HTHOST_LINK_CONTROL_REG, 7, 6, &temp);
#endif
}

/***************************************************************************//**
 *
 * static void
 * common_void()
 *
 *  Description:
 *	Nothing.
 *
 *  Parameters:
 *		None.
 *
 * ---------------------------------------------------------------------------------------
 */
static void common_void(void)
{
}

/***************************************************************************//**
 *
 * static BOOL
 * common_return_false()
 *
 *  Description:
 *	Return False.
 *
 *  Parameters:
 *	     @return	   = false
 *
 */
static BOOL common_return_false(void)
{
	return 0;
}

/***************************************************************************
 ***			Non-coherent init code				  ***
 ***			Northbridge access routines			  ***
 ***************************************************************************/

/***************************************************************************//**
 *
 * static u8
 * read_sb_link(cNorthBridge *nb)
 *
 *  Description:
 *	 Return the link to the Southbridge
 *
 *  Parameters:
 *	@param[in] *nb = this northbridge
 *	@return          the link to the southbridge
 *
 * ---------------------------------------------------------------------------------------
 */
static u8 read_sb_link(cNorthBridge *nb)
{
	u32 temp;
	amd_pci_read_bits(MAKE_SBDFO(make_pci_segment_from_node(0),
				make_pci_bus_from_node(0),
				make_pci_device_from_node(0),
				CPU_HTNB_FUNC_00,
				REG_UNIT_ID_0X64),
				10, 8, &temp);
	return (u8)temp;
}

/***************************************************************************//**
 *
 * static BOOL
 * verify_link_is_non_coherent(u8 node, u8 link, cNorthBridge *nb)
 *
 *  Description:
 *	 Verify that the link is non-coherent, connected, and ready
 *
 *  Parameters:
 *	@param[in]  node   = the node that will be examined
 *	@param[in]  link   = the Link on that node to examine
 *	@param[in] *nb = this northbridge
 *	@return   = true - The link has the following status
 *					LinkCon = 1,     Link is connected
 *					INIT_COMPLETE = 1,Link initialization is complete
 *					NC = 1,          Link is coherent
 *					UniP-cLDT = 0,   Link is not Uniprocessor cLDT
 *					LinkConPend = 0  Link connection is not pending
 *					false- The link has some other status
 *
 * ---------------------------------------------------------------------------------------
 */
static BOOL verify_link_is_non_coherent(u8 node, u8 link, cNorthBridge *nb)
{
	u32 link_type;
	SBDFO link_base;

	ASSERT((node < nb->max_nodes) && (link < nb->max_links));

	link_base = make_link_base(node, link);

	/* FN0_98/A4/C4 = LDT Type Register */
	amd_pci_read(link_base + HTHOST_LINK_TYPE_REG, &link_type);

	/* Verify linkCon = 1, INIT_COMPLETE = 1, NC = 0, UniP-cLDT = 0, LinkConPend = 0 */
	return (link_type & HTHOST_TYPE_MASK) ==  HTHOST_TYPE_NONCOHERENT;
}

/***************************************************************************//**
 *
 * static void
 * ht3_set_cfg_addr_map(u8 cfg_map_index, u8 sec_bus, u8 sub_bus, u8 target_node,
 * 			u8 target_link, sMainData *p_dat, cNorthBridge *nb)
 *
 *  Description:
 *	 Configure and enable config access to a non-coherent chain for the given bus range.
 *
 *  Parameters:
 *	@param[in] cfg_map_index = the map entry to set
 *	@param[in] sec_bus      = The secondary bus number to use
 *	@param[in] sub_bus      = The subordinate bus number to use
 *	@param[in] target_node  = The node  that shall be the recipient of the traffic
 *	@param[in] target_link  = The link that shall be the recipient of the traffic
 *	@param[in] p_dat   = our global state
 *	@param[in] *nb  = this northbridge
 *
 * ---------------------------------------------------------------------------------------
 */
static void  ht3_set_cfg_addr_map(u8 cfg_map_index, u8 sec_bus, u8 sub_bus, u8 target_node,
				u8 target_link, sMainData *p_dat, cNorthBridge *nb)
{
	u8 cur_node;
	SBDFO link_base;
	u32 temp;

	link_base = make_link_base(target_node, target_link);

	ASSERT(sec_bus <= sub_bus);
	temp = sec_bus;
	amd_pci_write_bits(link_base + HTHOST_ISOC_REG, 15, 8, &temp);

	/* For target link, note that rev F uses bits 9:8 and only with GH is bit 10
	 * set to indicate a sublink.  For node, we are currently not supporting Extended
	 * routing tables.
	 */
	temp = ((u32)sub_bus << 24) + ((u32)sec_bus << 16) + ((u32)target_link << 8)
		+ ((u32)target_node << 4) + (u32)3;
	for (cur_node = 0; cur_node < p_dat->nodes_discovered + 1; cur_node++)
		amd_pci_write(MAKE_SBDFO(make_pci_segment_from_node(cur_node),
					make_pci_bus_from_node(cur_node),
					make_pci_device_from_node(cur_node),
					CPU_ADDR_FUNC_01,
					REG_ADDR_CONFIG_MAP0_1XE0 + 4 * cfg_map_index),
					&temp);
}

/***************************************************************************//**
 *
 * static void
 * ht1_set_cfg_addr_map(u8 cfg_map_index, u8 sec_bus, u8 sub_bus, u8 target_node,
 * 			u8 target_link, sMainData *p_dat, cNorthBridge *nb)
 *
 *  Description:
 *	 Configure and enable config access to a non-coherent chain for the given bus range.
 *
 *  Parameters:
 *	@param[in]  cfg_map_index = the map entry to set
 *	@param[in]  sec_bus      = The secondary bus number to use
 *	@param[in]  sub_bus      = The subordinate bus number to use
 *	@param[in]  target_node  = The node  that shall be the recipient of the traffic
 *	@param[in]  target_link  = The link that shall be the recipient of the traffic
 *	@param[in] p_dat   = our global state
 *	@param[in] *nb   = this northbridge
 *
 ******************************************************************************/
static void ht1_set_cfg_addr_map(u8 cfg_map_index, u8 sec_bus, u8 sub_bus, u8 target_node,
				u8 target_link, sMainData *p_dat, cNorthBridge *nb)
{
	u8 cur_node;
	SBDFO link_base;
	u32 temp;

	link_base = make_link_base(target_node, target_link);

	ASSERT(sec_bus <= sub_bus);
	temp = sec_bus;
	amd_pci_write_bits(link_base + HTHOST_ISOC_REG, 15, 8, &temp);

	temp = sub_bus;
	amd_pci_write_bits(link_base + HTHOST_ISOC_REG, 23, 16, &temp);

	/* For target link, note that rev F uses bits 9:8 and only with GH is bit 10
	 * set to indicate a sublink.  For node, we are currently not supporting Extended
	 * routing tables.
	 */
	temp = ((u32)sub_bus << 24) + ((u32)sec_bus << 16) + ((u32)target_link << 8)
		+ ((u32)target_node << 4) + (u32)3;
	for (cur_node = 0; cur_node < p_dat->nodes_discovered + 1; cur_node++)
		 amd_pci_write(MAKE_SBDFO(make_pci_segment_from_node(cur_node),
					make_pci_bus_from_node(cur_node),
					make_pci_device_from_node(cur_node),
					CPU_ADDR_FUNC_01,
					REG_ADDR_CONFIG_MAP0_1XE0 + 4*cfg_map_index),
					&temp);
}

/***************************************************************************
 ***				 Link Optimization			  ***
 ***************************************************************************/

/**
 * static u8
 * convert_bits_to_width(u8 value, cNorthBridge *nb)
 *
 *  Description:
 *	 Given the bits set in the register field, return the width it represents
 *
 *  Parameters:
 *	@param[in]  value   = The bits for the register
 *	@param[in] *nb = this northbridge
 *	@return  The width
 *
 ******************************************************************************/
static u8 convert_bits_to_width(u8 value, cNorthBridge *nb)
{
	switch (value) {
	case 1: return 16;
	case 0: return 8;
	case 5: return 4;
	case 4: return 2;
	default: STOP_HERE; /*  This is an error internal condition */
	}
	return 0; // shut up GCC.
}

/***************************************************************************//**
 *
 * static u8
 * convert_width_to_bits(u8 value, cNorthBridge *nb)
 *
 *  Description:
 *	Translate a desired width setting to the bits to set in the register field
 *
 *  Parameters:
 *	@param[in]  value     = The width
 *	@param[in] *nb = this northbridge
 *	@return The bits for the register
 *
 ******************************************************************************/
static u8 convert_width_to_bits(u8 value, cNorthBridge *nb)
{
	switch (value) {
	case 16: return 1;
	case  8: return 0;
	case  4: return 5;
	case  2: return 4;
	default: STOP_HERE; /*  This is an internal error condition */
	}
	return 0; // shut up GCC
}

/***************************************************************************//**
 *
 * static u16
 * ht1_northbridge_freq_mask(u8 node_id, cNorthBridge *nb)
 *
 *  Description:
 *	Return a mask that eliminates HT frequencies that cannot be used due to a slow
 *	northbridge frequency.
 *
 *  Parameters:
 *	@param[in]  node      = Result could (later) be for a specific node
 *	@param[in] *nb = this northbridge
 *	@return  Frequency mask
 *
 ******************************************************************************/
static u32 ht1_northbridge_freq_mask(u8 node, cNorthBridge *nb)
{
	/* only up to HT1 speeds */
	return (HT_FREQUENCY_LIMIT_HT1_ONLY);
}

/***************************************************************************//**
 *
 * static u16
 * fam_10_northbridge_freq_mask(u8 node_id, cNorthBridge *nb)
 *
 *  Description:
 *	Return a mask that eliminates HT frequencies that cannot be used due to a slow
 *	northbridge frequency.
 *
 *  Parameters:
 *	@param[in]  node     = Result could (later) be for a specific node
 *	@param[in]  *nb      = this northbridge
 *	@return  = Frequency mask
 *
 ******************************************************************************/
static u32 fam_10_northbridge_freq_mask(u8 node, cNorthBridge *nb)
{
	u8 nb_cof;
	u32 supported;

	nb_cof = get_min_nb_cof();
	/*
	 * nb_cof is minimum northbridge speed in hundreds of MHz.
	 * HT can not go faster than the minimum speed of the northbridge.
	 */
	if ((nb_cof >= 6) && (nb_cof < 10))
	{
		/* Generation 1 HT link frequency */
		/* Convert frequency to bit and all less significant bits,
		 * by setting next power of 2 and subtracting 1.
		 */
		supported = ((u32)1 << ((nb_cof >> 1) + 2)) - 1;
	}
	else if ((nb_cof >= 10) && (nb_cof <= 32))
	{
		/* Generation 3 HT link frequency
		 * Assume error retry is enabled on all Gen 3 links
		 */
		if (is_gt_rev_d()) {
			nb_cof *= 2;
			if (nb_cof > 32)
				nb_cof = 32;
		}

		/* Convert frequency to bit and all less significant bits,
		 * by setting next power of 2 and subtracting 1.
		 */
		supported = ((u32)1 << ((nb_cof >> 1) + 2)) - 1;
	}
	else if (nb_cof > 32)
	{
		supported = HT_FREQUENCY_LIMIT_3200M;
	}
	/* unlikely cases, but include as a defensive measure, also avoid trick above */
	else if (nb_cof == 4)
	{
		supported = HT_FREQUENCY_LIMIT_400M;
	}
	else if (nb_cof == 2)
	{
		supported = HT_FREQUENCY_LIMIT_200M;
	}
	else
	{
		STOP_HERE;
		supported = HT_FREQUENCY_LIMIT_200M;
	}

	return (fix_early_sample_freq_capability(supported));
}

/***************************************************************************//**
 *
 * static u16
 * fam_15_northbridge_freq_mask(u8 node_id, cNorthBridge *nb)
 *
 *  Description:
 *	Return a mask that eliminates HT frequencies that cannot be used due to a slow
 *	northbridge frequency.
 *
 *  Parameters:
 *	@param[in]  node     = Result could (later) be for a specific node
 *	@param[in]  *nb      = this northbridge
 *	@return  = Frequency mask
 *
 ******************************************************************************/
static u32 fam_15_northbridge_freq_mask(u8 node, cNorthBridge *nb)
{
	u8 nb_cof;
	u32 supported;

	nb_cof = get_min_nb_cof();
	/*
	 * nb_cof is minimum northbridge speed in hundreds of MHz.
	 * HT can not go faster than the minimum speed of the northbridge.
	 */
	if ((nb_cof >= 6) && (nb_cof < 10))
	{
		/* Generation 1 HT link frequency */
		/* Convert frequency to bit and all less significant bits,
		 * by setting next power of 2 and subtracting 1.
		 */
		supported = ((u32)1 << ((nb_cof >> 1) + 2)) - 1;
	}
	else if ((nb_cof >= 10) && (nb_cof <= 32))
	{
		/* Generation 3 HT link frequency
		 * Assume error retry is enabled on all Gen 3 links
		 */
		nb_cof *= 2;
		if (nb_cof > 32)
			nb_cof = 32;

		/* Convert frequency to bit and all less significant bits,
		 * by setting next power of 2 and subtracting 1.
		 */
		supported = ((u32)1 << ((nb_cof >> 1) + 2)) - 1;
	}
	else if (nb_cof > 32)
	{
		supported = HT_FREQUENCY_LIMIT_3200M;
	}
	/* unlikely cases, but include as a defensive measure, also avoid trick above */
	else if (nb_cof == 4)
	{
		supported = HT_FREQUENCY_LIMIT_400M;
	}
	else if (nb_cof == 2)
	{
		supported = HT_FREQUENCY_LIMIT_200M;
	}
	else
	{
		STOP_HERE;
		supported = HT_FREQUENCY_LIMIT_200M;
	}

	return (fix_early_sample_freq_capability(supported));
}

/***************************************************************************//**
 *
 * static void
 * gather_link_data(sMainData *p_dat, cNorthBridge *nb)
 *
 *  Description:
 *	 For all discovered links, populate the port list with the frequency and width
 *	 capabilities.
 *
 *  Parameters:
 *	@param[in,out] p_dat = our global state, port list
 *	@param[in]     *nb = this northbridge
 *
 ******************************************************************************/
static void gather_link_data(sMainData *p_dat, cNorthBridge *nb)
{
	u8 i;
	SBDFO link_base;
	u32 temp;

	for (i = 0; i < p_dat->total_links*2; i++)
	{
		if (p_dat->port_list[i].type == PORTLIST_TYPE_CPU)
		{
			link_base = make_link_base(
				p_dat->port_list[i].node_id, p_dat->port_list[i].link);

			p_dat->port_list[i].pointer = link_base;

			amd_pci_read_bits(link_base + HTHOST_LINK_CONTROL_REG, 22, 20, &temp);
			p_dat->port_list[i].prv_width_out_cap
						= convert_bits_to_width((u8)temp, p_dat->nb);

			amd_pci_read_bits(link_base + HTHOST_LINK_CONTROL_REG, 18, 16, &temp);
			p_dat->port_list[i].prv_width_in_cap
						= convert_bits_to_width((u8)temp, p_dat->nb);

			amd_pci_read_bits(link_base + HTHOST_FREQ_REV_REG, 31, 16, &temp);
			p_dat->port_list[i].prv_frequency_cap
				= temp
				& 0x7FFF /*  Mask off bit 15, reserved value */
				& nb->northbridge_freq_mask(
					p_dat->port_list[i].node_id, p_dat->nb);
			if (is_gt_rev_d()) {
				amd_pci_read_bits(
					link_base + HTHOST_FREQ_REV_REG_2, 15, 1, &temp);
				temp &= 0x7;	/* Mask off reserved values */
				p_dat->port_list[i].prv_frequency_cap |= (temp << 17);
			}

			amd_pci_read_bits(link_base + HTHOST_FEATURE_CAP_REG, 9, 0, &temp);
			p_dat->port_list[i].prv_feature_cap = (u16)temp;
		}
		else
		{
			link_base = p_dat->port_list[i].pointer;
			if (p_dat->port_list[i].link == 1)
				link_base += HTSLAVE_LINK01_OFFSET;

			amd_pci_read_bits(
				link_base + HTSLAVE_LINK_CONTROL_0_REG, 22, 20, &temp);
			p_dat->port_list[i].prv_width_out_cap
						= convert_bits_to_width((u8)temp, p_dat->nb);

			amd_pci_read_bits(
				link_base + HTSLAVE_LINK_CONTROL_0_REG, 18, 16, &temp);
			p_dat->port_list[i].prv_width_in_cap
						= convert_bits_to_width((u8)temp, p_dat->nb);

			amd_pci_read_bits(link_base + HTSLAVE_FREQ_REV_0_REG, 31, 16, &temp);
			p_dat->port_list[i].prv_frequency_cap = (u16)temp;

			amd_pci_read_bits(link_base + HTSLAVE_FEATURE_CAP_REG, 7, 0, &temp);
			p_dat->port_list[i].prv_feature_cap = (u16)temp;

			if (p_dat->ht_block->amd_cb_device_cap_override)
			{
				link_base &= 0xFFFFF000;
				amd_pci_read(link_base, &temp);

				p_dat->ht_block->amd_cb_device_cap_override(
					p_dat->port_list[i].node_id,
					p_dat->port_list[i].host_link,
					p_dat->port_list[i].host_depth,
					(u8)SBDFO_SEG(p_dat->port_list[i].pointer),
					(u8)SBDFO_BUS(p_dat->port_list[i].pointer),
					(u8)SBDFO_DEV(p_dat->port_list[i].pointer),
					temp,
					p_dat->port_list[i].link,
					&(p_dat->port_list[i].prv_width_in_cap),
					&(p_dat->port_list[i].prv_width_out_cap),
					&(p_dat->port_list[i].prv_frequency_cap),
					&(p_dat->port_list[i].prv_feature_cap));
			}
		}
	}
}

/***************************************************************************//**
 *
 * static void
 * set_link_data(sMainData *p_dat, cNorthBridge *nb)
 *
 *  Description:
 *	 Change the hardware state for all links according to the now optimized data in the
 *	 port list data structure.
 *
 *  Parameters:
 *	  @param[in]  p_dat = our global state, port list
 *	  @param[in]  *nb   = this northbridge
 *
 ******************************************************************************/
static void set_link_data(sMainData *p_dat, cNorthBridge *nb)
{
	u8 i;
	SBDFO link_base;
	u32 temp, temp2, frequency_index, widthin, widthout, bits;

	for (i = 0; i < p_dat->total_links*2; i++)
	{

		ASSERT(p_dat->port_list[i & 0xFE].sel_width_out
			== p_dat->port_list[(i & 0xFE) + 1].sel_width_in);
		ASSERT(p_dat->port_list[i & 0xFE].sel_width_in
			== p_dat->port_list[(i & 0xFE) + 1].sel_width_out);
		ASSERT(p_dat->port_list[i & 0xFE].sel_frequency
			== p_dat->port_list[(i & 0xFE) + 1].sel_frequency);

		if (p_dat->port_list[i].sel_regang)
		{
			ASSERT(p_dat->port_list[i].type == PORTLIST_TYPE_CPU);
			ASSERT(p_dat->port_list[i].link < 4);
			temp = 1;
			amd_pci_write_bits(MAKE_SBDFO(
					make_pci_segment_from_node(p_dat->port_list[i].node_id),
					make_pci_bus_from_node(p_dat->port_list[i].node_id),
					make_pci_device_from_node(p_dat->port_list[i].node_id),
					CPU_HTNB_FUNC_00,
					REG_HT_LINK_EXT_CONTROL0_0X170
					+ 4 * p_dat->port_list[i].link),
				0, 0, &temp);
		}

		if (p_dat->port_list[i].type == PORTLIST_TYPE_CPU)
		{
			if (p_dat->ht_block->amd_cb_override_cpu_port)
				p_dat->ht_block->amd_cb_override_cpu_port(
						p_dat->port_list[i].node_id,
						p_dat->port_list[i].link,
						&(p_dat->port_list[i].sel_width_in),
						&(p_dat->port_list[i].sel_width_out),
						&(p_dat->port_list[i].sel_frequency));
		}
		else
		{
			if (p_dat->ht_block->amd_cb_override_device_port)
				p_dat->ht_block->amd_cb_override_device_port(
						p_dat->port_list[i].node_id,
						p_dat->port_list[i].host_link,
						p_dat->port_list[i].host_depth,
						p_dat->port_list[i].link,
						&(p_dat->port_list[i].sel_width_in),
						&(p_dat->port_list[i].sel_width_out),
						&(p_dat->port_list[i].sel_frequency));
		}

		link_base = p_dat->port_list[i].pointer;
		if ((p_dat->port_list[i].type == PORTLIST_TYPE_IO)
		&& (p_dat->port_list[i].link == 1))
			link_base += HTSLAVE_LINK01_OFFSET;

		/* Some IO devices don't work properly when setting widths, so write them in a
		 * single operation, rather than individually.
		 */
		widthout = convert_width_to_bits(p_dat->port_list[i].sel_width_out, p_dat->nb);
		ASSERT(widthout == 1 || widthout == 0 || widthout == 5 || widthout == 4);
		widthin = convert_width_to_bits(p_dat->port_list[i].sel_width_in, p_dat->nb);
		ASSERT(widthin == 1 || widthin == 0 || widthin == 5 || widthin == 4);

		temp = (widthin & 7) | ((widthout & 7) << 4);
		set_ht_control_register_bits(
				link_base + HTHOST_LINK_CONTROL_REG, 31, 24, &temp);

		temp = p_dat->port_list[i].sel_frequency;
		if (p_dat->port_list[i].type == PORTLIST_TYPE_CPU)
		{
			ASSERT((temp >= HT_FREQUENCY_600M && temp <= HT_FREQUENCY_3200M)
				|| (temp == HT_FREQUENCY_200M) || (temp == HT_FREQUENCY_400M));
			frequency_index = temp;
			if (temp > 0xf) {
				temp2 = (temp >> 4) & 0x1;
				temp &= 0xf;
			} else {
				temp2 = 0x0;
			}
			/* NOTE
			 * The Family 15h BKDG Rev. 3.14 is wrong
			 * Freq[4] must be set before Freq[3:0], otherwise the register writes
			 * will be ignored!
			 */
			if (is_gt_rev_d())
				amd_pci_write_bits(
					link_base + HTHOST_FREQ_REV_REG_2, 0, 0, &temp2);
			amd_pci_write_bits(link_base + HTHOST_FREQ_REV_REG, 11, 8, &temp);

			/* Enable isochronous flow control mode if supported by chipset */
			if (is_fam15h()) {
				if (p_dat->port_list[i].enable_isochronous_mode)
					temp = 1;
				else
					temp = 0;
				set_ht_control_register_bits(
						link_base + HTHOST_LINK_CONTROL_REG,
						12, 12, &temp);
			}

			/*  Gen1 = 200MHz -> 1000MHz, Gen3 = 1200MHz -> 3200MHz */
			if (frequency_index > HT_FREQUENCY_1000M)
			{
				/* Enable  for Gen3 frequencies */
				temp = 1;
			}
			else
			{
				/* Disable  for Gen1 frequencies */
				temp = 0;
			}
			/* HT3 retry mode enable / disable */
			amd_pci_write_bits(MAKE_SBDFO(
					make_pci_segment_from_node(p_dat->port_list[i].node_id),
					make_pci_bus_from_node(p_dat->port_list[i].node_id),
					make_pci_device_from_node(p_dat->port_list[i].node_id),
					CPU_HTNB_FUNC_00,
					REG_HT_LINK_RETRY0_0X130 + 4*p_dat->port_list[i].link),
				0, 0, &temp);

			/* and Scrambling enable / disable */
			amd_pci_write_bits(MAKE_SBDFO(
					make_pci_segment_from_node(p_dat->port_list[i].node_id),
					make_pci_bus_from_node(p_dat->port_list[i].node_id),
					make_pci_device_from_node(p_dat->port_list[i].node_id),
					CPU_HTNB_FUNC_00,
					REG_HT_LINK_EXT_CONTROL0_0X170
					+ 4 * p_dat->port_list[i].link),
				3, 3, &temp);
		}
		else
		{
			SBDFO current_ptr;
			BOOL is_found;

			ASSERT(temp <= HT_FREQUENCY_3200M);
			/* Write the frequency setting */
			amd_pci_write_bits(link_base + HTSLAVE_FREQ_REV_0_REG, 11, 8, &temp);

			/* Handle additional HT3 frequency requirements, if needed,
			 * or clear them if switching down to ht1 on a warm reset.
			 * Gen1 = 200MHz -> 1000MHz, Gen3 = 1200MHz -> 2600MHz
			 *
			 * Even though we assert if debugging, we need to check that the
			 * capability was found always, since this is an unknown hardware
			 * device, also we are taking unqualified frequency from the call backs
			 * (could be trying to do ht3 on an ht1 IO device).
			 */

			if (temp > HT_FREQUENCY_1000M)
			{
				/* Enabling features if gen 3 */
				bits = 1;
			}
			else
			{
				/* Disabling features if gen 1 */
				bits = 0;
			}

			/* Enable isochronous flow control mode if supported by chipset */
			if (is_fam15h()) {
				if (p_dat->port_list[i].enable_isochronous_mode)
					temp = 1;
				else
					temp = 0;
			}

			/* Retry Enable */
			is_found = FALSE;
			current_ptr = link_base & (u32)0xFFFFF000; /* Set PCI Offset to 0 */
			do
			{
				amd_pci_find_next_cap(&current_ptr);
				if (current_ptr != ILLEGAL_SBDFO)
				{
					amd_pci_read(current_ptr, &temp);
					/* HyperTransport Retry Capability? */
					if (IS_HT_RETRY_CAPABILITY(temp))
					{
						ASSERT(p_dat->port_list[i].link < 2);
						amd_pci_write_bits(
							current_ptr + HTRETRY_CONTROL_REG,
							p_dat->port_list[i].link*16,
							p_dat->port_list[i].link*16,
							&bits);
						is_found = TRUE;
					}
				/* Some other capability, keep looking */
				}
				else
				{
					/* If we are turning it off, that may mean the device
					 * was only ht1 capable, so don't complain that we can't
					 * do it.
					 */
					if (bits != 0)
					{
						if (p_dat->ht_block->amd_cb_event_notify)
						{
							sHtEventOptRequiredCap evt;
							evt.e_size = sizeof(
								sHtEventOptRequiredCap);
							evt.node = p_dat->port_list[i].node_id;
							evt.link = p_dat->port_list[i]
										.host_link;
							evt.depth = p_dat->port_list[i]
										.host_depth;

							p_dat->ht_block->amd_cb_event_notify(
								HT_EVENT_CLASS_WARNING,
								HT_EVENT_OPT_REQUIRED_CAP_RETRY,
								(u8 *)&evt);
						}
						STOP_HERE;
					}
					is_found = TRUE;
				}
			} while (!is_found);

			/* Scrambling enable */
			is_found = FALSE;
			current_ptr = link_base & (u32)0xFFFFF000; /* Set PCI Offset to 0 */
			do
			{
				amd_pci_find_next_cap(&current_ptr);
				if (current_ptr != ILLEGAL_SBDFO)
				{
					amd_pci_read(current_ptr, &temp);
					/* HyperTransport Gen3 Capability? */
					if (IS_HT_GEN3_CAPABILITY(temp))
					{
						ASSERT(p_dat->port_list[i].link < 2);
						amd_pci_write_bits(
							(current_ptr
							+ HTGEN3_LINK_TRAINING_0_REG
							+ p_dat->port_list[i].link
							* HTGEN3_LINK01_OFFSET),
							3, 3, &bits);
						is_found = TRUE;
					}
					/* Some other capability, keep looking */
					}
					else
					{
					/* If we are turning it off, that may mean the device
					 * was only ht1 capable, so don't complain that we can't
					 * do it.
					 */
					if (bits != 0)
					{
						if (p_dat->ht_block->amd_cb_event_notify)
						{
							sHtEventOptRequiredCap evt;
							evt.e_size = sizeof(
								sHtEventOptRequiredCap);
							evt.node = p_dat->port_list[i].node_id;
							evt.link = p_dat->port_list[i]
										.host_link;
							evt.depth = p_dat->port_list[i]
										.host_depth;

							p_dat->ht_block->amd_cb_event_notify(
								HT_EVENT_CLASS_WARNING,
								HT_EVENT_OPT_REQUIRED_CAP_GEN3,
								(u8 *)&evt);
						}
						STOP_HERE;
					}
					is_found = TRUE;
				}
			} while (!is_found);
		}
	}
}

/***************************************************************************//**
 *
 * void
 * fam_0f_write_ht_link_cmd_buffer_alloc(u8 node, u8 link, u8 req, u8 preq, u8 rsp, u8 prb)
 *
 *  Description:
 *	Set the command buffer allocations in the buffer count register for the node and link.
 *	The command buffer settings in the low 16 bits are the same on both
 *	family 10h and family 0fh northbridges.
 *
 *  Parameters:
 *	@param[in] node = The node to set allocations on
 *	@param[in] link = the link to set allocations on
 *	@param[in] req  = non-posted Request Command Buffers
 *	@param[in] preq = Posted Request Command Buffers
 *	@param[in] rsp  = Response Command Buffers
 *	@param[in] prb  = Probe Command Buffers
 *
 ******************************************************************************/
#ifndef HT_BUILD_NC_ONLY

static void fam_0f_write_ht_link_cmd_buffer_alloc(u8 node, u8 link, u8 req, u8 preq, u8 rsp,
						u8 prb)
{
	u32 temp;
	SBDFO current_ptr;

	current_ptr = make_link_base(node, link);
	current_ptr += HTHOST_BUFFER_COUNT_REG;

	/* non-posted Request Command Buffers */
	temp = req;
	amd_pci_write_bits(current_ptr, 3, 0, &temp);
	/* Posted Request Command Buffers */
	temp = preq;
	amd_pci_write_bits(current_ptr, 7, 4, &temp);
	/* Response Command Buffers */
	temp = rsp;
	amd_pci_write_bits(current_ptr, 11, 8, &temp);
	/* Probe Command Buffers */
	temp = prb;
	amd_pci_write_bits(current_ptr, 15, 12, &temp);
	/* LockBc */
	temp = 1;
	amd_pci_write_bits(current_ptr, 31, 31, &temp);
}
#endif /* HT_BUILD_NC_ONLY */

/***************************************************************************//**
 *
 * void
 * fam_0f_write_ht_link_dat_buffer_alloc(u8 node, u8 link, u8 req_d, u8 preq_d, u8 rsp_d)
 *
 *  Description:
 *	 Set the data buffer allocations in the buffer count register for the node and link.
 *	 The command buffer settings in the high 16 bits are not the same on both
 *	 family 10h and family 0fh northbridges.
 *
 *  Parameters:
 *	@param[in] node  = The node to set allocations on
 *	@param[in] link  = the link to set allocations on
 *	@param[in] req_d  = non-posted Request Data Buffers
 *	@param[in] preq_d = Posted Request Data Buffers
 *	@param[in] rsp_d  = Response Data Buffers
 *
 ******************************************************************************/
#ifndef HT_BUILD_NC_ONLY

static void fam_0f_write_ht_link_dat_buffer_alloc(u8 node, u8 link, u8 req_d, u8 preq_d,
						u8 rsp_d)
{
	u32 temp;
	SBDFO current_ptr;

	current_ptr = make_link_base(node, link);
	current_ptr += HTHOST_BUFFER_COUNT_REG;

	/* Request Data Buffers */
	temp = req_d;
	amd_pci_write_bits(current_ptr, 18, 16, &temp);
	/* Posted Request Data Buffers */
	temp = preq_d;
	amd_pci_write_bits(current_ptr, 22, 20, &temp);
	/* Response Data Buffers */
	temp = rsp_d;
	amd_pci_write_bits(current_ptr, 26, 24, &temp);
}
#endif /* HT_BUILD_NC_ONLY */

/***************************************************************************//**
 *
 * static void
 * ht3_write_traffic_distribution(u32 links01, u32 links10, cNorthBridge *nb)
 *
 *  Description:
 *	 Set the traffic distribution register for the links provided.
 *
 *  Parameters:
 *	@param[in]  links01   = coherent links from node 0 to 1
 *	@param[in]  links10   = coherent links from node 1 to 0
 *	@param[in]  nb = this northbridge
 *
 ******************************************************************************/
static void ht3_write_traffic_distribution(u32 links01, u32 links10, cNorthBridge *nb)
{
#ifndef HT_BUILD_NC_ONLY
	u32 temp;

	/* Node 0 */
	/* DstLnk */
	amd_pci_write_bits(MAKE_SBDFO(make_pci_segment_from_node(0),
			make_pci_bus_from_node(0),
			make_pci_device_from_node(0),
			CPU_HTNB_FUNC_00,
			REG_HT_TRAFFIC_DIST_0X164),
			23, 16, &links01);
	/* DstNode = 1, cHTPrbDistEn = 1, cHTRspDistEn = 1, cHTReqDistEn = 1 */
	temp = 0x0107;
	amd_pci_write_bits(MAKE_SBDFO(make_pci_segment_from_node(0),
			make_pci_bus_from_node(0),
			make_pci_device_from_node(0),
			CPU_HTNB_FUNC_00,
			REG_HT_TRAFFIC_DIST_0X164),
			15, 0, &temp);

	/* Node 1 */
	/* DstLnk */
	amd_pci_write_bits(MAKE_SBDFO(make_pci_segment_from_node(1),
			make_pci_bus_from_node(1),
			make_pci_device_from_node(1),
			CPU_HTNB_FUNC_00,
			REG_HT_TRAFFIC_DIST_0X164),
			23, 16, &links10);
	/* DstNode = 0, cHTPrbDistEn = 1, cHTRspDistEn = 1, cHTReqDistEn = 1 */
	temp = 0x0007;
	amd_pci_write_bits(MAKE_SBDFO(make_pci_segment_from_node(1),
			make_pci_bus_from_node(1),
			make_pci_device_from_node(1),
			CPU_HTNB_FUNC_00,
			REG_HT_TRAFFIC_DIST_0X164),
			15, 0, &temp);
#endif /* HT_BUILD_NC_ONLY */
}

/***************************************************************************//**
 *
 * static void
 * ht1_write_traffic_distribution(u32 links01, u32 links10, cNorthBridge *nb)
 *
 *  Description:
 *	 Traffic distribution is more complex in this case as the routing table must be
 *	 adjusted to use one link for requests and the other for responses.  Also,
 *	 perform the buffer tunings on the links required for this config.
 *
 *  Parameters:
 *	@param[in]  links01  = coherent links from node 0 to 1
 *	@param[in]  links10  = coherent links from node 1 to 0
 *	@param[in]  nb = this northbridge
 *
 ******************************************************************************/
static void ht1_write_traffic_distribution(u32 links01, u32 links10, cNorthBridge *nb)
{
#ifndef HT_BUILD_NC_ONLY
	u32 route01, route10;
	u8 req0, req1, rsp0, rsp1, nclink;

	/*
	 * Get the current request route for 0->1 and 1->0.  This will indicate which of the
	 * links in links01 are connected to which links in links10.  Since we have to route to
	 * distribute traffic, we need to know that. The link used by htinit will become the
	 * request, probe link. the other link will be used for responses.
	 */

	/* Get the routes, and hang on to them, we will write them back updated. */
	amd_pci_read(MAKE_SBDFO(make_pci_segment_from_node(0),
				make_pci_bus_from_node(0),
				make_pci_device_from_node(0),
				CPU_HTNB_FUNC_00,
				REG_ROUTE1_0X44),
				&route01);
	amd_pci_read(MAKE_SBDFO(make_pci_segment_from_node(1),
				make_pci_bus_from_node(1),
				make_pci_device_from_node(1),
				CPU_HTNB_FUNC_00,
				REG_ROUTE0_0X40),
				&route10);

	/* Convert the request routes to a link number.  Note "0xE" is ht1 nb specific.
	 * Find the response link numbers.
	*/
	ASSERT((route01 & 0xE) && (route10 & 0xE));    /* no route! error! */
	req0 = (u8)amd_bit_scan_reverse((route01 & 0xE)) - 1;
	req1 = (u8)amd_bit_scan_reverse((route10 & 0xE)) - 1;
	/* Now, find the other link for the responses */
	rsp0 = (u8)amd_bit_scan_reverse((links01 & ~((u32)1 << req0)));
	rsp1 = (u8)amd_bit_scan_reverse((links10 & ~((u32)1 << req1)));

	/* ht1 nb restriction, must have exactly two links */
	ASSERT(((((links01 & ~((u32)1 << req0)) & ~((u32)1 << rsp0))) == 0)
		&& ((((links10 & ~((u32)1 << req1)) & ~((u32)1 << rsp1))) == 0));

	route01 = (route01 & ~0x0E00) | ((u32)0x0100 << (rsp0 + 1));
	route10 = (route10 & ~0x0E00) | ((u32)0x0100 << (rsp1 + 1));

	amd_pci_write(MAKE_SBDFO(make_pci_segment_from_node(0),
				make_pci_bus_from_node(0),
				make_pci_device_from_node(0),
				CPU_HTNB_FUNC_00,
				REG_ROUTE1_0X44),
				&route01);

	amd_pci_write(MAKE_SBDFO(make_pci_segment_from_node(1),
				make_pci_bus_from_node(1),
				make_pci_device_from_node(1),
				CPU_HTNB_FUNC_00,
				REG_ROUTE0_0X40),
				&route10);

	/* While we otherwise do buffer tunings elsewhere, for the dual cHT DP case with
	 * ht1 northbridges like family 0Fh, do the tunings here where we have all the
	 * link and route info at hand and don't need to recalculate it.
	 */

	/* Node 0, Request / Probe Link (note family F only has links < 4) */
	fam_0f_write_ht_link_cmd_buffer_alloc(0, req0, 6, 3, 1, 6);
	fam_0f_write_ht_link_dat_buffer_alloc(0, req0, 4, 3, 1);
	/* Node 0, Response Link (note family F only has links < 4) */
	fam_0f_write_ht_link_cmd_buffer_alloc(0, rsp0, 1, 0, 15, 0);
	fam_0f_write_ht_link_dat_buffer_alloc(0, rsp0, 1, 1, 6);
	/* Node 1, Request / Probe Link (note family F only has links < 4) */
	fam_0f_write_ht_link_cmd_buffer_alloc(1, req1, 6, 3, 1, 6);
	fam_0f_write_ht_link_dat_buffer_alloc(1, req1, 4, 3, 1);
	/* Node 1, Response Link (note family F only has links < 4) */
	fam_0f_write_ht_link_cmd_buffer_alloc(1, rsp1, 1, 0, 15, 0);
	fam_0f_write_ht_link_dat_buffer_alloc(1, rsp1, 1, 1, 6);

	/* Node 0, is the third link non-coherent? */
	nclink = (u8)amd_bit_scan_reverse(((u8)0x07 & ~((u32)1 << req0) & ~((u32)1 << rsp0)));
	if (nb->verify_link_is_non_coherent(0, nclink, nb))
	{
		fam_0f_write_ht_link_cmd_buffer_alloc(0, nclink, 6, 5, 2, 0);
	}

	/* Node 1, is the third link non-coherent? */
	nclink = (u8)amd_bit_scan_reverse(((u8)0x07 & ~((u32)1 << req1) & ~((u32)1 << rsp1)));
	if (nb->verify_link_is_non_coherent(1, nclink, nb))
	{
		fam_0f_write_ht_link_cmd_buffer_alloc(1, nclink, 6, 5, 2, 0);
	}
#endif /* HT_BUILD_NC_ONLY */
}

/***************************************************************************//**
 *
 * static void
 * fam_0f_buffer_optimizations(u8 node, sMainData *p_dat, cNorthBridge *nb)
 *
 *  Description:
 *	 Buffer tunings are inherently northbridge specific. Check for specific configs
 *	 which require adjustments and apply any standard workarounds to this node.
 *
 *  Parameters:
 *	@param[in]  node      = the node to
 *	@param[in] *p_dat  = coherent links from node 0 to 1
 *	@param[in]  nb = this northbridge
 *
 ******************************************************************************/
static void fam_0f_buffer_optimizations(u8 node, sMainData *p_dat, cNorthBridge *nb)
{
#ifndef HT_BUILD_NC_ONLY
	u8 i;
	u32 temp;
	SBDFO current_ptr;

	ASSERT(node < nb->max_nodes);

	/* Fix the FIFO pointer register before changing speeds */
	current_ptr = MAKE_SBDFO(make_pci_segment_from_node(node),
				make_pci_bus_from_node(node),
				make_pci_device_from_node(node),
				CPU_NB_FUNC_03,
				REG_NB_FIFOPTR_3XDC);
	for (i = 0; i < nb->max_links; i++)
	{
		temp = 0;
		if (nb->verify_link_is_coherent(node, i, nb))
		{
			temp = 0x26;
			ASSERT(i < 3);
			amd_pci_write_bits(current_ptr, 8*i + 5, 8*i, &temp);
		}
		else
		{
			if (nb->verify_link_is_non_coherent(node, i, nb))
			{
				temp = 0x25;
				ASSERT(i < 3);
				amd_pci_write_bits(current_ptr, 8*i + 5, 8*i, &temp);
			}
		}
	}
	/*
	 * 8P Buffer tuning.
	 * Either apply the BKDG tunings or, if applicable, apply the more restrictive errata
	 * 153 workaround.
	 * If 8 nodes, Check this node for 'inner' or 'outer'.
	 * Tune each link based on coherent or non-coherent
	 */
	if (p_dat->nodes_discovered >= 6)
	{
		u8 j;
		BOOL is_outer;
		BOOL is_errata_153;

		/* This is for family 0Fh, so assuming dual core max then 7 or 8 nodes are
		 * required to be in the situation of 14 or more cores. We checked nodes above,
		 * cross check that the number of cores is 14 or more. We want both 14 cores
		 * with at least 7 or 8 nodes not one condition alone, to apply the errata 153
		 * workaround.  Otherwise, 7 or 8 rev F nodes use the BKDG tuning.
		 */

		is_errata_153 = 0;

		amd_pci_read_bits(MAKE_SBDFO(make_pci_segment_from_node(0),
					make_pci_bus_from_node(0),
					make_pci_device_from_node(0),
					CPU_HTNB_FUNC_00,
					REG_NODE_ID_0X60),
					19, 16, &temp);

		if (temp >= 14)
		{
			/* Check whether we need to do errata 153 tuning or BKDG tuning.
			 * Errata 153 applies to JH-1, JH-2 and older.  It is fixed in JH-3
			 * (and, one assumes, from there on).
			 */
			for (i = 0; i < (p_dat->nodes_discovered +1); i++)
			{
				amd_pci_read_bits(MAKE_SBDFO(make_pci_segment_from_node(i),
						make_pci_bus_from_node(i),
						make_pci_device_from_node(i),
						CPU_NB_FUNC_03,
						REG_NB_CPUID_3XFC),
						7, 0, &temp);
				if (((u8)temp & ~0x40) < 0x13)
				{
					is_errata_153 = 1;
					break;
				}
			}
		}

		for (i = 0; i < CPU_ADDR_NUM_CONFIG_MAPS; i++)
		{
			is_outer = FALSE;
			/* Check for outer node by scanning the config maps on node 0 for one
			 * which is assigned to this node.
			 */
			current_ptr = MAKE_SBDFO(make_pci_segment_from_node(0),
						make_pci_bus_from_node(0),
						make_pci_device_from_node(0),
						CPU_ADDR_FUNC_01,
						REG_ADDR_CONFIG_MAP0_1XE0 + (4 * i));
			amd_pci_read_bits (current_ptr, 1, 0, &temp);
			/* Make sure this config map is valid, if it is it will be enabled for
			 * read/write
			 */
			if (temp == 3)
			{
				/* It's valid, get the node (that node is an outer node) */
				amd_pci_read_bits (current_ptr, 6, 4, &temp);
				/* Is the node we're working on now? */
				if (node == (u8)temp)
				{
					/* This is an outer node. Tune it appropriately. */
					for (j = 0; j < nb->max_links; j++)
					{
						if (is_errata_153)
						{
							if (nb->verify_link_is_coherent(
								node, j, nb))
							{
								fam_0f_write_ht_link_cmd_buffer_alloc(
									node, j, 1, 1, 6, 4);
							}
							else
							{
								if (nb->verify_link_is_non_coherent(
									node, j, nb))
								{
									fam_0f_write_ht_link_cmd_buffer_alloc(
										node,
										j, 5, 4, 1, 0);
								}
							}
						}
						else
						{
							if (nb->verify_link_is_coherent(
								node, j, nb))
							{
								fam_0f_write_ht_link_cmd_buffer_alloc(
									node, j, 1, 1, 8, 5);
							}
						}
					}
					/*
					 * SRI to XBAR Buffer Counts are correct for outer node
					 * at power on defaults.
					 */
					is_outer = TRUE;
					break;
				}
			}
			/* We fill config maps in ascending order, so if we didn't use this one,
			 * we're done.
			 */
			else break;
		}
		if (!is_outer)
		{
			if (is_errata_153)
			{
				/* Tuning for inner node coherent links */
				for (j = 0; j < nb->max_links; j++)
				{
					if (nb->verify_link_is_coherent(node, j, nb))
					{
						fam_0f_write_ht_link_cmd_buffer_alloc(
								node, j, 2, 1, 5, 4);
					}

				}
				/* SRI to XBAR Buffer Count for inner nodes, zero DReq and DPReq
				*/
				temp = 0;
				amd_pci_write_bits (MAKE_SBDFO(make_pci_segment_from_node(node),
							make_pci_bus_from_node(node),
							make_pci_device_from_node(node),
							CPU_NB_FUNC_03,
							REG_NB_SRI_XBAR_BUF_3X70),
							31, 28, &temp);
			}
		}

		/*
		 * Tune MCT to XBAR Buffer Count the same an all nodes, 2 Probes, 5 Response
		 */
		if (is_errata_153)
		{
			temp = 0x25;
			amd_pci_write_bits (MAKE_SBDFO(make_pci_segment_from_node(node),
						make_pci_bus_from_node(node),
						make_pci_device_from_node(node),
						CPU_NB_FUNC_03,
						REG_NB_MCT_XBAR_BUF_3X78),
						14, 8, &temp);
		}
	}
#endif /* HT_BUILD_NC_ONLY */
}

/***************************************************************************//**
 *
 * static void
 * fam_10_buffer_optimizations(u8 node, sMainData *p_dat, cNorthBridge *nb)
 *
 *  Description:
 *	 Buffer tunings are inherently northbridge specific. Check for specific configs
 *	 which require adjustments and apply any standard workarounds to this node.
 *
 *  Parameters:
 *	@param[in] node       = the node to tune
 *	@param[in] *p_dat  = global state
 *	@param[in] nb = this northbridge
 *
 ******************************************************************************/
static void fam_10_buffer_optimizations(u8 node, sMainData *p_dat, cNorthBridge *nb)
{
	u32 temp;
	SBDFO current_ptr;
	u8 i;

	ASSERT(node < nb->max_nodes);

	/*
	 * Link to XCS Token Count Tuning
	 *
	 * For each active link that we reganged (so this unfortunately can't go into the PCI
	 * reg table), we have to switch the Link to XCS Token Counts to the ganged state.
	 * We do this here for the non-uma case, which is to write the values that would have
	 * been power on defaults if the link was ganged at cold reset.
	 */
	for (i = 0; i < p_dat->total_links*2; i++)
	{
		if ((p_dat->port_list[i].node_id == node)
		&& (p_dat->port_list[i].type == PORTLIST_TYPE_CPU))
		{
			/* If the link is greater than 4, this is a sublink 1, so it is not
			 * reganged.
			 */
			if (p_dat->port_list[i].link < 4)
			{
				current_ptr = MAKE_SBDFO(make_pci_segment_from_node(node),
						make_pci_bus_from_node(node),
						make_pci_device_from_node(node),
						CPU_NB_FUNC_03,
						REG_NB_LINK_XCS_TOKEN0_3X148
						+ 4 * p_dat->port_list[i].link);
				if (p_dat->port_list[i].sel_regang)
				{
					/* Handle all the regang Token count adjustments */

					/* Sublink 0:
					 * [Probe0tok] = 2
					 * [Rsp0tok] = 2
					 * [PReq0tok] = 2
					 * [Req0tok] = 2
					 */
					temp = 0xAA;
					amd_pci_write_bits(current_ptr, 7, 0, &temp);
					/* Sublink 1:
					 * [Probe1tok] = 0
					 * [Rsp1tok] = 0
					 * [PReq1tok] = 0
					 * [Req1tok] = 0
					 */
					temp = 0;
					amd_pci_write_bits(current_ptr, 23, 16, &temp);
					/* [FreeTok] = 3 */
					temp = 3;
					amd_pci_write_bits(current_ptr, 15, 14, &temp);
				}
				else
				{
					/* Read the regang bit in hardware */
					amd_pci_read_bits(MAKE_SBDFO(make_pci_segment_from_node(
							p_dat->port_list[i].node_id),
							make_pci_bus_from_node(
								p_dat->port_list[i].node_id),
							make_pci_device_from_node(
								p_dat->port_list[i].node_id),
							CPU_HTNB_FUNC_00,
							REG_HT_LINK_EXT_CONTROL0_0X170
							+ 4 * p_dat->port_list[i].link),
							0, 0, &temp);
					if (temp == 1)
					{
						/* handle a minor adjustment for stapped ganged
						 * links. If sel_regang is false we didn't do
						 * the regang, so if the bit is on then it's
						 * hardware strapped.
						 */

						/* [FreeTok] = 3 */
						temp = 3;
						amd_pci_write_bits(current_ptr, 15, 14, &temp);
					}
				}
			}
		}
	}
}

/***************************************************************************//**
 *
 * static void
 * fam_15_buffer_optimizations(u8 node, sMainData *p_dat, cNorthBridge *nb)
 *
 *  Description:
 *	 Buffer tunings are inherently northbridge specific. Check for specific configs
 *	 which require adjustments and apply any standard workarounds to this node.
 *
 *  Parameters:
 *	@param[in] node       = the node to tune
 *	@param[in] *p_dat  = global state
 *	@param[in] nb = this northbridge
 *
 ******************************************************************************/
static void fam_15_buffer_optimizations(u8 node, sMainData *p_dat, cNorthBridge *nb)
{
	/* Buffer count setup on Family 15h is currently handled in cpu_set_amd_pci */
}

/*
 * North Bridge 'constructor'.
 *
 */

/***************************************************************************//**
 *
 * void
 * new_northbridge(u8 node, cNorthBridge *nb)
 *
 *  Description:
 *	 Construct a new northbridge.  This routine encapsulates knowledge of how to tell
 *	 significant differences between families of supported northbridges and what routines
 *	 can be used in common and which are unique.  A fully populated northbridge interface
 *	 is provided by nb.
 *
 *  Parameters:
 *	  @param            node
 *	  @param[out]	    nb		 = the caller's northbridge structure to initialize.
 *
 ******************************************************************************/
void new_northbridge(u8 node, cNorthBridge *nb)
{
	u32 match;
	u32 ext_fam, base_fam, model;

	cNorthBridge fam15 =
	{
#ifdef HT_BUILD_NC_ONLY
		8,
		1,
		12,
#else
		8,
		8,
		64,
#endif /* HT_BUILD_NC_ONLY*/
		write_routing_table,
		write_node_id,
		read_def_lnk,
		enable_routing_tables,
		verify_link_is_coherent,
		read_true_link_fail_status,
		read_token,
		writeToken,
		fam_15_get_num_cores_on_node,
		set_total_nodes_and_cores,
		limit_nodes,
		write_full_routing_table,
		is_compatible,
		fam_15_is_capable,
		(void (*)(u8, u8, cNorthBridge*))common_void,
		(BOOL (*)(u8, u8, sMainData*, cNorthBridge*))common_return_false,
		read_sb_link,
		verify_link_is_non_coherent,
		ht3_set_cfg_addr_map,
		convert_bits_to_width,
		convert_width_to_bits,
		fam_15_northbridge_freq_mask,
		gather_link_data,
		set_link_data,
		ht3_write_traffic_distribution,
		fam_15_buffer_optimizations,
		0x00000001,
		0x00000200,
		18,
		0x00000f06
	};

	cNorthBridge fam10 =
	{
#ifdef HT_BUILD_NC_ONLY
		8,
		1,
		12,
#else
		8,
		8,
		64,
#endif /* HT_BUILD_NC_ONLY*/
		write_routing_table,
		write_node_id,
		read_def_lnk,
		enable_routing_tables,
		verify_link_is_coherent,
		read_true_link_fail_status,
		read_token,
		writeToken,
		fam_10_get_num_cores_on_node,
		set_total_nodes_and_cores,
		limit_nodes,
		write_full_routing_table,
		is_compatible,
		fam_10_is_capable,
		(void (*)(u8, u8, cNorthBridge*))common_void,
		(BOOL (*)(u8, u8, sMainData*, cNorthBridge*))common_return_false,
		read_sb_link,
		verify_link_is_non_coherent,
		ht3_set_cfg_addr_map,
		convert_bits_to_width,
		convert_width_to_bits,
		fam_10_northbridge_freq_mask,
		gather_link_data,
		set_link_data,
		ht3_write_traffic_distribution,
		fam_10_buffer_optimizations,
		0x00000001,
		0x00000200,
		18,
		0x00000f01
	};

	cNorthBridge fam0f =
	{
#ifdef HT_BUILD_NC_ONLY
		3,
		1,
		12,
#else
		3,
		8,
		32,
#endif /* HT_BUILD_NC_ONLY*/
		write_routing_table,
		write_node_id,
		read_def_lnk,
		enable_routing_tables,
		verify_link_is_coherent,
		read_true_link_fail_status,
		read_token,
		writeToken,
		fam_0f_get_num_cores_on_node,
		set_total_nodes_and_cores,
		limit_nodes,
		write_full_routing_table,
		is_compatible,
		fam_0f_is_capable,
		fam_0f_stop_link,
		(BOOL (*)(u8, u8, sMainData*, cNorthBridge*))common_return_false,
		read_sb_link,
		verify_link_is_non_coherent,
		ht1_set_cfg_addr_map,
		convert_bits_to_width,
		convert_width_to_bits,
		ht1_northbridge_freq_mask,
		gather_link_data,
		set_link_data,
		ht1_write_traffic_distribution,
		fam_0f_buffer_optimizations,
		0x00000001,
		0x00000100,
		16,
		0x00000f00
	};

	/* Start with enough of the key to identify the northbridge interface */
	amd_pci_read_bits(MAKE_SBDFO(make_pci_segment_from_node(node),
			make_pci_bus_from_node(node),
			make_pci_device_from_node(node),
			CPU_NB_FUNC_03,
			REG_NB_CPUID_3XFC),
			27, 20, &ext_fam);
	amd_pci_read_bits(MAKE_SBDFO(make_pci_segment_from_node(node),
			make_pci_bus_from_node(node),
			make_pci_device_from_node(node),
			CPU_NB_FUNC_03,
			REG_NB_CPUID_3XFC),
			11, 8, &base_fam);
	amd_pci_read_bits(MAKE_SBDFO(make_pci_segment_from_node(node),
			make_pci_bus_from_node(node),
			make_pci_device_from_node(node),
			CPU_NB_FUNC_03,
			REG_NB_CPUID_3XFC),
			7, 4, &model);
	match = (u32)((base_fam << 8) | ext_fam);

	/* Test each in turn looking for a match.
	 * Initialize the struct if found.
	 */
	if (match == fam15.compatible_key)
	{
		amd_memcpy((void *)nb, (const void *)&fam15, (u32) sizeof(cNorthBridge));
	}
	else if (match == fam10.compatible_key)
	{
		amd_memcpy((void *)nb, (const void *)&fam10, (u32) sizeof(cNorthBridge));
	}
	else
	{
		if (match == fam0f.compatible_key)
		{
			amd_memcpy(
				(void *)nb, (const void *)&fam0f, (u32) sizeof(cNorthBridge));
		}
		else
		{
		STOP_HERE;
		}
	}

	/* Update the initial limited key to the real one, which may include other matching info
	 */
	nb->compatible_key = make_key(node);
}
