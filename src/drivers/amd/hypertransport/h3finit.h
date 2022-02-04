/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef H3FINIT_H
#define H3FINIT_H

#include "comlib.h"

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

/* Width equates for call backs */
#define HT_WIDTH_8_BITS	8
#define HT_WIDTH_16_BITS	16
#define HT_WIDTH_4_BITS	4
#define HT_WIDTH_2_BITS	2

/* Frequency equates for call backs which take an actual frequency setting */
#define HT_FREQUENCY_200M	0
#define HT_FREQUENCY_400M	2
#define HT_FREQUENCY_600M	4
#define HT_FREQUENCY_800M	5
#define HT_FREQUENCY_1000M	6
#define HT_FREQUENCY_1200M	7
#define HT_FREQUENCY_1400M	8
#define HT_FREQUENCY_1600M	9
#define HT_FREQUENCY_1800M	10
#define HT_FREQUENCY_2000M	11
#define HT_FREQUENCY_2200M	12
#define HT_FREQUENCY_2400M	13
#define HT_FREQUENCY_2600M	14
#define HT_FREQUENCY_2800M	17
#define HT_FREQUENCY_3000M	18
#define HT_FREQUENCY_3200M	19

/* Frequency Limit equates for call backs which take a frequency supported mask. */
#define HT_FREQUENCY_LIMIT_200M	1
#define HT_FREQUENCY_LIMIT_400M	7
#define HT_FREQUENCY_LIMIT_600M	0x1F
#define HT_FREQUENCY_LIMIT_800M	0x3F
#define HT_FREQUENCY_LIMIT_1000M	0x7F
#define HT_FREQUENCY_LIMIT_HT1_ONLY	0x7F
#define HT_FREQUENCY_LIMIT_1200M	0xFF
#define HT_FREQUENCY_LIMIT_1400M	0x1FF
#define HT_FREQUENCY_LIMIT_1600M	0x3FF
#define HT_FREQUENCY_LIMIT_1800M	0x7FF
#define HT_FREQUENCY_LIMIT_2000M	0xFFF
#define HT_FREQUENCY_LIMIT_2200M	0x1FFF
#define HT_FREQUENCY_LIMIT_2400M	0x3FFF
#define HT_FREQUENCY_LIMIT_2600M	0x7FFF
#define HT_FREQUENCY_LIMIT_2800M	0x3FFFF
#define HT_FREQUENCY_LIMIT_3000M	0x7FFFF
#define HT_FREQUENCY_LIMIT_3200M	0xFFFFF

/*
 * Event Notify definitions
 */

/* Event Class definitions */
#define HT_EVENT_CLASS_CRITICAL	1
#define HT_EVENT_CLASS_ERROR		2
#define HT_EVENT_CLASS_HW_FAULT	3
#define HT_EVENT_CLASS_WARNING		4
#define HT_EVENT_CLASS_INFO		5

/* Event definitions. */

/* Coherent subfunction events */
#define HT_EVENT_COH_EVENTS		0x1000
#define HT_EVENT_COH_NO_TOPOLOGY	0x1001
#define HT_EVENT_COH_LINK_EXCEED	0x1002
#define HT_EVENT_COH_FAMILY_FEUD	0x1003
#define HT_EVENT_COH_NODE_DISCOVERED	0x1004
#define HT_EVENT_COH_MPCAP_MISMATCH	0x1005

/* Non-coherent subfunction events */
#define HT_EVENT_NCOH_EVENTS		0x2000
#define HT_EVENT_NCOH_BUID_EXCEED	0x2001
#define HT_EVENT_NCOH_LINK_EXCEED	0x2002
#define HT_EVENT_NCOH_BUS_MAX_EXCEED	0x2003
#define HT_EVENT_NCOH_CFG_MAP_EXCEED	0x2004
#define HT_EVENT_NCOH_DEVICE_FAILED	0x2005
#define HT_EVENT_NCOH_AUTO_DEPTH	0x2006

/* Optimization subfunction events */
#define HT_EVENT_OPT_EVENTS			0x3000
#define HT_EVENT_OPT_REQUIRED_CAP_RETRY	0x3001
#define HT_EVENT_OPT_REQUIRED_CAP_GEN3		0x3002

/* HW Fault events */
#define HT_EVENT_HW_EVENTS	0x4000
#define HT_EVENT_HW_SYNCHFLOOD	0x4001
#define HT_EVENT_HW_HTCRC	0x4002

/* The bbHT component (hb*) uses 0x5000 for events.
 * For consistency, we avoid that range here.
 */

/*----------------------------------------------------------------------------
 *			TYPEDEFS, STRUCTURES, ENUMS
 *
 *----------------------------------------------------------------------------
 */

typedef struct {
	u8 **topolist;
	u8 auto_bus_start;
	/* Note: This should always be the form auto_bus_current+N*auto_bus_increment, also bus
	 * 253-255 are reserved
	 */
	u8 auto_bus_max;
	u8 auto_bus_increment;

	/**-------------------------------------------------------------------------------------
	 *
	 * BOOL
	 * amd_cb_ignore_link(u8 node, u8 link)
	 *
	 * Description:
	 *	This routine is called every time a coherent link is found and then every
	 *	time a non-coherent link from a CPU is found.
	 *	Any coherent or non-coherent link from a CPU can be ignored and not used
	 *	for discovery or initialization.  Useful for connection based systems.
	 *	(Note: not called for IO device to IO Device links.)
	 *
	 * Parameters:
	 *	@param[in]   u8   node   = The node on which this link is located
	 *	@param[in]   u8   link   = The link about to be initialized
	 *	@param[out]  BOOL result = true to ignore this link and skip it
	 *				   false to initialize the link normally
	 *
	 * -------------------------------------------------------------------------------------
	 */
	BOOL (*amd_cb_ignore_link)(u8 node, u8 link);

	/**-------------------------------------------------------------------------------------
	 *
	 * BOOL
	 * amd_cb_override_bus_numbers(u8 node, u8 link, u8 *sec_bus, u8 *sub_bus)
	 *
	 * Description:
	 *	This routine is called every time a non-coherent chain is processed.
	 *	If a system can not use the auto bus numbering feature for non-coherent chain
	 *	bus assignments, this routine can provide explicit control. For each chain,
	 *	provide the bus number range to use.
	 *
	 * Parameters:
	 *	@param[in]  u8   node   = The node on which this chain is located
	 *	@param[in]  u8   link   = The link on the host for this chain
	 *	@param[out] u8   secBus = Secondary bus number for this non-coherent chain
	 *	@param[out] u8 *subBus = Subordinate bus number
	 *	@param[out] BOOL result = true this routine is supplying the bus numbers
	 *				  false use auto bus numbering
	 *
	 * -------------------------------------------------------------------------------------
	 */
	BOOL (*amd_cb_override_bus_numbers)(u8 node, u8 link, u8 *sec_bus, u8 *sub_bus);

	/**-------------------------------------------------------------------------------------
	 *
	 * BOOL
	 * amd_cb_manual_buid_swap_list(u8 node, u8 link, u8 **list)
	 *
	 * Description:
	 *	This routine is called every time a non-coherent chain is processed.
	 *	BUID assignment may be controlled explicitly on a non-coherent chain. Provide a
	 *	swap list. The first part of the list controls the BUID assignment and the
	 *	second part of the list provides the device to device linking. Device
	 *	orientation can be detected automatically, or explicitly.  See documentation for
	 *	more details.
	 *
	 *	Automatic non-coherent init assigns BUIDs starting at 1 and incrementing
	 *	sequentially based on each device's unit count.
	 *
	 * Parameters:
	 *	@param[in]  u8  node    = The node on which this chain is located
	 *	@param[in]  u8  link    = The link on the host for this chain
	 *	@param[out] u8 **list   = supply a pointer to a list
	 *	@param[out] BOOL result = true to use a manual list
	 *				  false to initialize the link automatically
	 *
	 * -------------------------------------------------------------------------------------
	 */
	BOOL (*amd_cb_manual_buid_swap_list)(u8 node, u8 link, const u8 **list);

	/**-------------------------------------------------------------------------------------
	 *
	 * void
	 * amd_cb_device_cap_override(u8 host_node, u8 host_link, u8 depth, u8 segment,
	 *				  u8 bus, u8 dev, u32 dev_ven_id, u8 link,
	 *				  u8 *link_width_in, u8 *link_width_out, u16 *freq_cap)
	 *
	 * Description:
	 *	This routine is called once for every link on every IO device.
	 *	Update the width and frequency capability if needed for this device.
	 *	This is used along with device capabilities, the limit call backs, and
	 *	northbridge limits to compute the default settings.  The components of the
	 *	device's PCI config address are provided, so its settings can be consulted if
	 *	need be. The input width and frequency are the reported device capabilities.
	 *
	 * Parameters:
	 *	@param[in]  u8  hostNode  = The node on which this chain is located
	 *	@param[in]  u8  hostLink  = The link on the host for this chain
	 *	@param[in]  u8  depth     = The depth in the I/O chain from the Host
	 *	@param[in]  u8  segment   = The Device's PCI bus segment number
	 *	@param[in]  u8  bus       = The Device's PCI bus number
	 *	@param[in]  u8  dev       = The Device's PCI device Number
	 *	@param[in]  u32 dev_ven_id  = The Device's PCI Vendor + Device ID (offset 0x00)
	 *	@param[in]  u8  link      = The Device's link number (0 or 1)
	 *	@param[in,out] u8 *link_width_in  = modify to change the link Witdh In
	 *	@param[in,out] u8 *link_width_out  = modify to change the link Witdh Out
	 *	@param[in,out] u32 *freq_cap = modify to change the link's frequency capability
	 *	@param[in,out] u32 *feature_cap = modify to change the link's feature capability
	 *
	 * -------------------------------------------------------------------------------------
	 */
	void (*amd_cb_device_cap_override)(
		u8 host_node,
		u8 host_link,
		u8 depth,
		u8 segment,
		u8 bus,
		u8 dev,
		u32 dev_ven_id,
		u8 link,
		u8 *link_width_in,
		u8 *link_width_out,
		u32 *freq_cap,
		u32 *feature_cap
	);

	/**-------------------------------------------------------------------------------------
	 *
	 * void
	 * amd_cb_cpu_2_cpu_pcb_limits(u8 node_a, u8 link_a, u8 node_b, u8 link_b,
	 *				u8 *ab_link_width_limit, u8 *ba_link_width_limit,
	 *				u16 *pcb_freq_cap)
	 *
	 * Description:
	 *	For each coherent connection this routine is called once.
	 *	Update the frequency and width if needed for this link (usually based on board
	 *	restriction).  This is used with CPU device capabilities and northbridge limits
	 *	to compute the default settings.  The input width and frequency are valid, but
	 *	do not necessarily reflect the minimum setting that will be chosen.
	 *
	 * Parameters:
	 *	@param[in]  u8  nodeA  = One node on which this link is located
	 *	@param[in]  u8  linkA  = The link on this node
	 *	@param[in]  u8  nodeB  = The other node on which this link is located
	 *	@param[in]  u8  linkB  = The link on that node
	 *	@param[in,out]  u8 *ab_link_width_limit = modify to change the link Witdh In
	 *	@param[in,out]  u8 *ba_link_width_limit = modify to change the link Witdh Out
	 *	@param[in,out]  u32 *pcb_freq_cap = modify to change the link's frequency
	 *						capability
	 *
	 * -------------------------------------------------------------------------------------
	 */
	void (*amd_cb_cpu_2_cpu_pcb_limits)(
		u8 node_a,
		u8 link_a,
		u8 node_b,
		u8 link_b,
		u8 *ab_link_width_limit,
		u8 *ba_link_width_limit,
		u32 *pcb_freq_cap
	);

	/**-------------------------------------------------------------------------------------
	 *
	 * void
	 * amd_cb_iop_cb_limits(u8 host_node, u8 host_link, u8 depth,
	 * 			u8 *downstream_link_width_limit,
	 *			u8 *upstream_link_width_limit, u16 *pcb_freq_cap)
	 *
	 * Description:
	 *	For each non-coherent connection this routine is called once.
	 *	Update the frequency and width if needed for this link (usually based on board
	 *	restriction).  This is used with device capabilities, device overrides, and
	 *	northbridge limits to compute the default settings. The input width and
	 *	frequency are valid, but do not necessarily reflect the minimum setting that
	 *	will be chosen.
	 *
	 * Parameters:
	 *	@param[in]  u8  hostNode  = The node on which this link is located
	 *	@param[in]  u8  hostLink  = The link about to be initialized
	 *	@param[in]  u8  depth  = The depth in the I/O chain from the Host
	 *	@param[in,out]  u8 *downstream_link_width_limit = modify to change the link
	 *			Witdh In
	 *	@param[in,out]  u8 *upstream_link_width_limit = modify to change the link Witdh
	 *			Out
	 *	@param[in,out]  u32 *pcb_freq_cap = modify to change the link's frequency
	 *			capability
	 *
	 * -------------------------------------------------------------------------------------
	 */
	void (*amd_cb_iop_cb_limits)(
		u8 host_node,
		u8 host_link,
		u8 depth,
		u8 *downstream_link_width_limit,
		u8 *upstream_link_width_limit,
		u32 *pcb_freq_cap
	);

	/**-------------------------------------------------------------------------------------
	 *
	 * BOOL
	 * amd_cb_skip_regang(u8 node_a, u8 link_a, u8 node_b, u8 link_b)
	 *
	 * Description:
	 *	This routine is called whenever two sublinks are both connected to the same
	 *	CPUs. Normally, unganged subsinks between the same two CPUs are reganged.
	 *	Return true from this routine to leave the links unganged.
	 *
	 * Parameters:
	 *	@param[in]  u8  nodeA   = One node on which this link is located
	 *	@param[in]  u8  linkA   = The link on this node
	 *	@param[in]  u8  nodeB   = The other node on which this link is located
	 *	@param[in]  u8  linkB   = The link on that node
	 *	@param[out] BOOL result = true to leave link unganged
	 *				  false to regang link automatically
	 *
	 * -------------------------------------------------------------------------------------
	 */
	BOOL (*amd_cb_skip_regang)(
		u8 node_a,
		u8 link_a,
		u8 node_b,
		u8 link_b
	);

	/**-------------------------------------------------------------------------------------
	 *
	 * BOOL
	 * amd_cb_customize_traffic_distribution()
	 *
	 * Description:
	 *	Near the end of HT initialization, this routine is called once.
	 *	If this routine will handle traffic distribution in a proprietary way,
	 *	after detecting which links to distribute traffic on and configuring the system,
	 *	return true. Return false to let the HT code detect and do traffic distribution
	 *	This routine can also be used to simply turn this feature off, or to pre-process
	 *	the system before normal traffic distribution.
	 *
	 * Parameters:
	 *	@param[out]  BOOL result  = true skip traffic distribution
	 *				    false do normal traffic distribution
	 *
	 * -------------------------------------------------------------------------------------
	 */
	BOOL (*amd_cb_customize_traffic_distribution)(void);


	/**-------------------------------------------------------------------------------------
	 *
	 * BOOL
	 * amd_cb_customize_buffers(u8 node)
	 *
	 * Description:
	 *	Near the end of HT initialization, this routine is called once per CPU node.
	 *	Implement proprietary buffer tuning and return true, or return false for normal
	 *	tuning. This routine can also be used to simply turn this feature off, or to
	 *	pre-process the system before normal tuning.
	 *
	 * Parameters:
	 *	@param[in] u8 node  = buffer allocation may apply to this node
	 *	@param[out] BOOL  result  = true skip buffer allocation on this node
	 *				    false tune buffers normally
	 *
	 * -------------------------------------------------------------------------------------
	 */
	BOOL (*amd_cb_customize_buffers)(u8 node);

	/**-------------------------------------------------------------------------------------
	 *
	 * void
	 * amd_cb_override_device_port(u8 host_node, u8 host_link, u8 depth, u8 *link_width_in,
	 *				   u8 *link_width_out, u16 *link_frequency)
	 *
	 * Description:
	 *	Called once for each active link on each IO device.
	 *	Provides an opportunity to directly control the frequency and width,
	 *	intended for test and debug.  The input frequency and width will be used
	 *	if not overridden.
	 *
	 * Parameters:
	 *	@param[in]  u8   hostNode  = The node on which this link is located
	 *	@param[in]  u8   hostLink  = The link about to be initialized
	 *	@param[in]  u8   depth     = The depth in the I/O chain from the Host
	 *	@param[in]  u8   link      = the link on the device (0 or 1)
	 *	@param[in,out]  u8 *link_width_in    = modify to change the link Witdh In
	 *	@param[in,out]  u8 *link_width_out   = modify to change the link Witdh Out
	 *	@param[in,out]  u16 *link_frequency = modify to change the link's frequency
	 *			capability
	 *
	 * -------------------------------------------------------------------------------------
	 */
	void (*amd_cb_override_device_port)(
		u8 host_node,
		u8 host_link,
		u8 depth,
		u8 link,
		u8 *link_width_in,
		u8 *link_width_out,
		u8 *link_frequency
	);

	/**-------------------------------------------------------------------------------------
	 *
	 * void
	 * amd_cb_override_cpu_port(u8 node, u8 link, u8 *link_width_in, u8 *link_width_out,
	 *				u16 *link_frequency)
	 *
	 * Description:
	 *	Called once for each active link on each CPU.
	 *	Provides an opportunity to directly control the frequency and width,
	 *	intended for test and debug. The input frequency and width will be used
	 *	if not overridden.
	 *
	 * Parameters:
	 *	@param[in]  u8  node  = One node on which this link is located
	 *	@param[in]  u8  link  = The link on this node
	 *	@param[in,out]  u8 *link_width_in = modify to change the link Witdh In
	 *	@param[in,out]  u8 *link_width_out = modify to change the link Witdh Out
	 *	@param[in,out]  u16 *link_frequency  = modify to change the link's frequency
	 *			capability
	 *
	 *--------------------------------------------------------------------------------------
	 */
	void (*amd_cb_override_cpu_port)(
		u8 node,
		u8 link,
		u8 *link_width_in,
		u8 *link_width_out,
		u8 *link_frequency
	);

	/**-------------------------------------------------------------------------------------
	 *
	 * void
	 * amd_cb_event_notify(u8 evt_class, u16 event, const u8 *p_event_data_0)
	 *
	 * Description:
	 *	Errors, events, faults, warnings, and useful information are provided by
	 *	calling this routine as often as necessary, once for each notification.
	 *	See elsewhere in this file for class, event, and event data definitions.
	 *	See the documentation for more details.
	 *
	 * Parameters:
	 *	@param[in]  u8  evt_class = What level event is this
	 *	@param[in]  u16 event = A unique ID of this event
	 *	@param[in]  u8 *p_event_data_0 = useful data associated with the event.
	 *
	 * -------------------------------------------------------------------------------------
	 */
	void (*amd_cb_event_notify) (
		u8 evt_class,
		u16 event,
		const u8 *p_event_data_0
	);

	const struct ht_link_config *ht_link_configuration;

} AMD_HTBLOCK;

/*
 * Event Notification Structures
 * These structures are passed to amd_cb_event_notify as *p_event_data_0.
 */

/* For event HT_EVENT_HW_SYNCHFLOOD */
typedef struct
{
	u8 e_size;
	u8 node;
	u8 link;
} sHtEventHWSynchFlood;

/* For event HT_EVENT_HW_HTCRC */
typedef struct
{
	u8 e_size;
	u8 node;
	u8 link;
	u8 lane_mask;
} sHtEventHWHtCrc;

/* For event HT_EVENT_NCOH_BUS_MAX_EXCEED */
typedef struct
{
	u8 e_size;
	u8 node;
	u8 link;
	u8 bus;
} sHTEventNcohBusMaxExceed;

/* For event HT_EVENT_NCOH_LINK_EXCEED */
typedef struct
{
	u8 e_size;
	u8 node;
	u8 link;
	u8 depth;
	u8 max_links;
} sHtEventNcohLinkExceed;

/* For event HT_EVENT_NCOH_CFG_MAP_EXCEED */
typedef struct
{
	u8 e_size;
	u8 node;
	u8 link;
} sHtEventNcohCfgMapExceed;

/* For event HT_EVENT_NCOH_BUID_EXCEED */
typedef struct
{
	u8 e_size;
	u8 node;
	u8 link;
	u8 depth;
	u8 current_buid;
	u8 unit_count;
} sHtEventNcohBuidExceed;

/* For event HT_EVENT_NCOH_DEVICE_FAILED */
typedef struct
{
	u8 e_size;
	u8 node;
	u8 link;
	u8 depth;
	u8 attempted_buid;
} sHtEventNcohDeviceFailed;

/* For event HT_EVENT_NCOH_AUTO_DEPTH */
typedef struct
{
	u8 e_size;
	u8 node;
	u8 link;
	u8 depth;
} sHtEventNcohAutoDepth;

/* For event HT_EVENT_OPT_REQUIRED_CAP_RETRY,
 *	      HT_EVENT_OPT_REQUIRED_CAP_GEN3
 */
typedef struct
{
	u8 e_size;
	u8 node;
	u8 link;
	u8 depth;
} sHtEventOptRequiredCap;

/* For event HT_EVENT_COH_NO_TOPOLOGY */
typedef struct
{
	u8 e_size;
	u8 total_nodes;
} sHtEventCohNoTopology;

/* For event HT_EVENT_COH_LINK_EXCEED */
typedef struct
{
	u8 e_size;
	u8 node;
	u8 link;
	u8 target_node;
	u8 total_nodes;
	u8 max_links;
} sHtEventCohLinkExceed;

/* For event HT_EVENT_COH_FAMILY_FEUD */
typedef struct
{
	u8 e_size;
	u8 node;
	u8 link;
	u8 total_nodes;
} sHtEventCohFamilyFeud;

/* For event HT_EVENT_COH_NODE_DISCOVERED */
typedef struct
{
	u8 e_size;
	u8 node;
	u8 link;
	u8 newNode;
} sHtEventCohNodeDiscovered;

/* For event HT_EVENT_COH_MPCAP_MISMATCH */
typedef struct
{
	u8 e_size;
	u8 node;
	u8 link;
	u8 sys_mp_cap;
	u8 total_nodes;
} sHtEventCohMpCapMismatch;

/*----------------------------------------------------------------------------
 *			FUNCTIONS PROTOTYPE
 *
 *----------------------------------------------------------------------------
 */
void amd_ht_initialize(AMD_HTBLOCK *p_block);


#endif	 /* H3FINIT_H */
