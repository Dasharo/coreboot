/* SPDX-License-Identifier: GPL-2.0-only */

/*
 *----------------------------------------------------------------------------
 *				MODULES USED
 *
 *----------------------------------------------------------------------------
 */

#include "h3finit.h"
#include "h3ffeat.h"
#include "h3ncmn.h"
#include "h3gtopo.h"
#include "AsPsNb.h"

#include <arch/cpu.h>
#include <device/pci.h>
#include <device/pci_ops.h>
#include <console/console.h>
#include <cpu/x86/lapic_def.h>
#include <cpu/amd/common/common.h>
#include <cpu/amd/msr.h>
#include <device/pci_def.h>
#include <northbridge/amd/amdfam10/raminit.h>
#include <northbridge/amd/amdfam10/amdfam10.h>
#include <option.h>
#include <types.h>

/*----------------------------------------------------------------------------
 *			DEFINITIONS AND MACROS
 *
 *----------------------------------------------------------------------------
 */

#define NVRAM_LIMIT_HT_SPEED_200  0x12
#define NVRAM_LIMIT_HT_SPEED_300  0x11
#define NVRAM_LIMIT_HT_SPEED_400  0x10
#define NVRAM_LIMIT_HT_SPEED_500  0xf
#define NVRAM_LIMIT_HT_SPEED_600  0xe
#define NVRAM_LIMIT_HT_SPEED_800  0xd
#define NVRAM_LIMIT_HT_SPEED_1000 0xc
#define NVRAM_LIMIT_HT_SPEED_1200 0xb
#define NVRAM_LIMIT_HT_SPEED_1400 0xa
#define NVRAM_LIMIT_HT_SPEED_1600 0x9
#define NVRAM_LIMIT_HT_SPEED_1800 0x8
#define NVRAM_LIMIT_HT_SPEED_2000 0x7
#define NVRAM_LIMIT_HT_SPEED_2200 0x6
#define NVRAM_LIMIT_HT_SPEED_2400 0x5
#define NVRAM_LIMIT_HT_SPEED_2600 0x4
#define NVRAM_LIMIT_HT_SPEED_2800 0x3
#define NVRAM_LIMIT_HT_SPEED_3000 0x2
#define NVRAM_LIMIT_HT_SPEED_3200 0x1
#define NVRAM_LIMIT_HT_SPEED_AUTO 0x0

static const u32 ht_speed_limit[20] =
	{0xFFFFF, 0xFFFFF, 0x7FFFF, 0x3FFFF,
	 0x0FFFF, 0x07FFF, 0x03FFF, 0x01FFF,
	 0x00FFF, 0x007FF, 0x003FF, 0x001FF,
	 0x000FF, 0x0007F, 0x0003F, 0x0001F,
	 0x0000F, 0x00007, 0x00003, 0x00001};

static const struct ht_speed_limit_map_t {
	u16 mhz;
	u8 nvram;
} ht_speed_limit_map[] = {
	{0, NVRAM_LIMIT_HT_SPEED_AUTO},
	{200, NVRAM_LIMIT_HT_SPEED_200},
	{300, NVRAM_LIMIT_HT_SPEED_300},
	{400, NVRAM_LIMIT_HT_SPEED_400},
	{500, NVRAM_LIMIT_HT_SPEED_500},
	{600, NVRAM_LIMIT_HT_SPEED_600},
	{800, NVRAM_LIMIT_HT_SPEED_800},
	{1000, NVRAM_LIMIT_HT_SPEED_1000},
	{1200, NVRAM_LIMIT_HT_SPEED_1200},
	{1400, NVRAM_LIMIT_HT_SPEED_1400},
	{1600, NVRAM_LIMIT_HT_SPEED_1600},
	{1800, NVRAM_LIMIT_HT_SPEED_1800},
	{2000, NVRAM_LIMIT_HT_SPEED_2000},
	{2200, NVRAM_LIMIT_HT_SPEED_2200},
	{2400, NVRAM_LIMIT_HT_SPEED_2400},
	{2600, NVRAM_LIMIT_HT_SPEED_2600},
	{2800, NVRAM_LIMIT_HT_SPEED_2800},
	{3000, NVRAM_LIMIT_HT_SPEED_3000},
	{3200, NVRAM_LIMIT_HT_SPEED_3200},
};

static const u32 ht_speed_mhz_to_hw(u16 mhz)
{
	size_t i;
	for (i = 0; i < ARRAY_SIZE(ht_speed_limit_map); i++)
		if (ht_speed_limit_map[i].mhz == mhz)
			return ht_speed_limit[ht_speed_limit_map[i].nvram];

	printk(BIOS_WARNING,
		"WARNING: Invalid HT link limit frequency %d specified, ignoring...\n",
		 mhz);
	return ht_speed_limit[NVRAM_LIMIT_HT_SPEED_AUTO];
}

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

/*----------------------------------------------------------------------------
 *			EXPORTED FUNCTIONS
 *
 *----------------------------------------------------------------------------
 */

/*----------------------------------------------------------------------------
 *			LOCAL FUNCTIONS
 *
 *----------------------------------------------------------------------------
 */
#ifndef HT_BUILD_NC_ONLY
/*
 **************************************************************************
 *			Routing table decompressor
 **************************************************************************
 */

/*
 **************************************************************************
 *	Graph Support routines
 * These routines provide support for dealing with the graph representation
 * of the topologies, along with the routing table information for that topology.
 * The routing information is compressed and these routines currently decompress
 * 'on the fly'.  A graph is represented as a set of routes. All the edges in the
 * graph are routes; a direct route from node i to node j exists in the graph IFF
 * there is an edge directly connecting node i to node j.  All other routes designate
 * the edge which the route to that node initially takes, by designating a node
 * to which a direct connection exists.  That is, the route to non-adjacent node j
 * from node i specifies node k where node i directly connects to node k.
 *
 *
 * pseudo definition of compressed graph:
 * typedef struct
 * {
 *	BIT  broadcast[8];
 *	uint4 responseRoute;
 *	uint4 requestRoute;
 * } sRoute;
 * typedef struct
 * {
 *	u8 size;
 *	sRoute graph[size][size];
 * } sGraph;
 *
 **************************************************************************
 */

/*----------------------------------------------------------------------------------------
 * u8
 * graph_how_many_nodes(u8 *graph)
 *
 *  Description:
 *	 Returns the number of nodes in the compressed graph
 *
 *  Parameters:
 *	@param[in] graph = a compressed graph
 *	@param[out] results = the number of nodes in the graph
 * ---------------------------------------------------------------------------------------
 */
static u8 graph_how_many_nodes(u8 *graph)
{
	return graph[0];
}

/*----------------------------------------------------------------------------------------
 * BOOL
 * graph_is_adjacent(u8 *graph, u8 node_a, u8 node_b)
 *
 *  Description:
 * Returns true if NodeA is directly connected to NodeB, false otherwise
 * (if NodeA == NodeB also returns false)
 * Relies on rule that directly connected nodes always route requests directly.
 *
 *  Parameters:
 *	@param[in]   graph   = the graph to examine
 *	@param[in]   node_a   = the node number of the first node
 *	@param[in]   node_b   = the node number of the second node
 *	@param[out]    results  = true if node_a connects to node_b false if not
 * ---------------------------------------------------------------------------------------
 */
static BOOL graph_is_adjacent(u8 *graph, u8 node_a, u8 node_b)
{
	u8 size = graph[0];
	ASSERT(size <= MAX_NODES);
	ASSERT((node_a < size) && (node_b < size));
	return (graph[1 + (node_a * size + node_b) * 2 + 1] & 0x0F) == node_b;
}

/*----------------------------------------------------------------------------------------
 * u8
 * graph_get_rsp(u8 *graph, u8 node_a, u8 node_b)
 *
 *  Description:
 *	Returns the graph node used by node_a to route responses targeted at node_b.
 *	This will be a node directly connected to node_a (possibly node_b itself),
 *	or "Route to Self" if node_a and node_b are the same node.
 *	Note that all node numbers are abstract node numbers of the topology graph,
 *	it is the responsibility of the caller to apply any permutation needed.
 *
 *  Parameters:
 *	@param[in]    u8    graph   = the graph to examine
 *	@param[in]    u8    node_a   = the node number of the first node
 *	@param[in]    u8    node_b   = the node number of the second node
 *	@param[out]   u8    results = The response route node
 * ---------------------------------------------------------------------------------------
 */
static u8 graph_get_rsp(u8 *graph, u8 node_a, u8 node_b)
{
	u8 size = graph[0];
	ASSERT(size <= MAX_NODES);
	ASSERT((node_a < size) && (node_b < size));
	return (graph[1 + (node_a * size + node_b) * 2 + 1] & 0xF0) >> 4;
}

/*----------------------------------------------------------------------------------------
 * u8
 * graph_get_req(u8 *graph, u8 node_a, u8 node_b)
 *
 *  Description:
 *	 Returns the graph node used by node_a to route requests targeted at node_b.
 *	This will be a node directly connected to node_a (possibly node_b itself),
 *	or "Route to Self" if node_a and node_b are the same node.
 *	Note that all node numbers are abstract node numbers of the topology graph,
 *	it is the responsibility of the caller to apply any permutation needed.
 *
 *  Parameters:
 *	@param[in]   graph   = the graph to examine
 *	@param[in]   node_a   = the node number of the first node
 *	@param[in]   node_b   = the node number of the second node
 *	@param[out]  results = The request route node
 * ---------------------------------------------------------------------------------------
 */
static u8 graph_get_req(u8 *graph, u8 node_a, u8 node_b)
{
	u8 size = graph[0];
	ASSERT(size <= MAX_NODES);
	ASSERT((node_a < size) && (node_b < size));
	return (graph[1 + (node_a * size + node_b) * 2 + 1] & 0x0F);
}

/*----------------------------------------------------------------------------------------
 * u8
 * graph_get_bc(u8 *graph, u8 node_a, u8 node_b)
 *
 *  Description:
 *	 Returns a bit vector of nodes that node_a should forward a broadcast from
 *	 node_b towards
 *
 *  Parameters:
 *	@param[in]    graph   = the graph to examine
 *	@param[in]    node_a   = the node number of the first node
 *	@param[in]    node_b   = the node number of the second node
 *	OU    results = the broadcast routes for node_a from node_b
 * ---------------------------------------------------------------------------------------
 */
static u8 graph_get_bc(u8 *graph, u8 node_a, u8 node_b)
{
	u8 size = graph[0];
	ASSERT(size <= MAX_NODES);
	ASSERT((node_a < size) && (node_b < size));
	return graph[1 + (node_a * size + node_b) * 2];
}


/***************************************************************************
 ***		GENERIC HYPERTRANSPORT DISCOVERY CODE			***
 ***************************************************************************/

/*----------------------------------------------------------------------------------------
 * void
 * route_from_bsp(u8 target_node, u8 actual_target, sMainData *p_dat)
 *
 *  Description:
 *	 Ensure a request / response route from target node to bsp.  Since target node is
 *	 always a predecessor of actual target node, each node gets a route to actual target
 *	 on the link that goes to target.  The routing produced by this routine is adequate
 *	 for config access during discovery, but NOT for coherency.
 *
 *  Parameters:
 *	@param[in]    u8    target_node   = the path to actual target goes through target
 *	@param[in]    u8    actual_target = the ultimate target being routed to
 *	@param[in]    sMainData*  p_dat   = our global state, port config info
 * ---------------------------------------------------------------------------------------
 */
static void route_from_bsp(u8 target_node, u8 actual_target, sMainData *p_dat)
{
	u8 predecessor_node, predecessor_link, current_pair;

	if (target_node == 0)
		return;  /*  BSP has no predecessor, stop */

	/*  Search for the link that connects target_node to its predecessor */
	current_pair = 0;
	while (p_dat->port_list[current_pair * 2 + 1].node_id != target_node)
	{
		current_pair++;
		ASSERT(current_pair < p_dat->total_links);
	}

	predecessor_node = p_dat->port_list[current_pair * 2].node_id;
	predecessor_link = p_dat->port_list[current_pair * 2].Link;

	/*  Recursively call self to ensure the route from the BSP to the Predecessor */
	/*  Node is established */
	route_from_bsp(predecessor_node, actual_target, p_dat);

	p_dat->nb->write_routing_table(predecessor_node, actual_target, predecessor_link, p_dat->nb);
}

/*---------------------------------------------------------------------------*/

/**
 *  u8
 * convert_node_to_link(u8 src_node, u8 target_node, sMainData *p_dat)
 *
 *  Description:
 *	 Return the link on source node which connects to target node
 *
 *  Parameters:
 *	@param[in]    src_node    = the source node
 *	@param[in]    target_node = the target node to find the link to
 *	@param[in]    p_dat = our global state
 *	@return       the link on source which connects to target
 *
 */
static u8 convert_node_to_link(u8 src_node, u8 target_node, sMainData *p_dat)
{
	u8 target_link = INVALID_LINK;
	u8 k;

	for (k = 0; k < p_dat->total_links * 2; k += 2)
	{
		if ((p_dat->port_list[k + 0].node_id == src_node) && (p_dat->port_list[k + 1].node_id == target_node))
		{
			target_link = p_dat->port_list[k + 0].Link;
			break;
		}
		else if ((p_dat->port_list[k + 1].node_id == src_node) && (p_dat->port_list[k + 0].node_id == target_node))
		{
			target_link = p_dat->port_list[k + 1].Link;
			break;
		}
	}
	ASSERT(target_link != INVALID_LINK);

	return target_link;
}


/*----------------------------------------------------------------------------------------
 * void
 * ht_discovery_flood_fill(sMainData *p_dat)
 *
 *  Description:
 *	 Discover all coherent devices in the system, initializing some basics like node IDs
 *	 and total nodes found in the process. As we go we also build a representation of the
 *	 discovered system which we will use later to program the routing tables.  During this
 *	 step, the routing is via default link back to BSP and to each new node on the link it
 *	 was discovered on (no coherency is active yet).
 *
 *  Parameters:
 *	@param[in]    sMainData*  p_dat = our global state
 * ---------------------------------------------------------------------------------------
 */
static void ht_discovery_flood_fill(sMainData *p_dat)
{
	u8 current_node = 0;
	u8 current_link;
	u8 current_link_id;

	/* NOTE
	 * Each node inside a dual node (socket G34) processor must share
	 * an adjacent node ID.  Alter the link scan order such that the
	 * other internal node is always scanned first...
	 */
	u8 current_link_scan_order_default[8] = {0, 1, 2, 3, 4, 5, 6, 7};
	u8 current_link_scan_order_g34_fam10[8] = {1, 0, 2, 3, 4, 5, 6, 7};
	u8 current_link_scan_order_g34_fam15[8] = {2, 0, 1, 3, 4, 5, 6, 7};

	u8 fam_15h = 0;
	u8 rev_gte_d = 0;
	u8 dual_node = 0;
	u32 f3xe8;
	u32 family;
	u32 model;

	f3xe8 = pci_read_config32(NODE_PCI(0, 3), 0xe8);

	family = model = cpuid_eax(0x80000001);
	model = ((model & 0xf0000) >> 12) | ((model & 0xf0) >> 4);
	family = ((family & 0xf00000) >> 16) | ((family & 0xf00) >> 8);

	if (family >= 0x6f) {
		/* Family 15h or later */
		fam_15h = 1;
	}

	if ((model >= 0x8) || fam_15h)
		/* Revision D or later */
		rev_gte_d = 1;

	if (rev_gte_d)
		 /* Check for dual node capability */
		if (f3xe8 & 0x20000000)
			dual_node = 1;

	/* Entries are always added in pairs, the even indices are the 'source'
	 * side closest to the BSP, the odd indices are the 'destination' side
	 */
	while (current_node <= p_dat->nodes_discovered)
	{
		u32 temp;

		if (current_node != 0)
		{
			/* Set path from BSP to current_node */
			route_from_bsp(current_node, current_node, p_dat);

			/* Set path from BSP to current_node for current_node+1 if
			 * current_node+1 != MAX_NODES
			 */
			if (current_node + 1 != MAX_NODES)
				route_from_bsp(current_node, current_node + 1, p_dat);

			/* Configure current_node to route traffic to the BSP through its
			 * default link
			 */
			p_dat->nb->write_routing_table(current_node, 0, p_dat->nb->read_def_lnk(current_node, p_dat->nb), p_dat->nb);
		}

		/* Set current_node's node_id field to current_node */
		p_dat->nb->write_node_id(current_node, current_node, p_dat->nb);

		/* Enable routing tables on current_node */
		p_dat->nb->enable_routing_tables(current_node, p_dat->nb);

		for (current_link_id = 0; current_link_id < p_dat->nb->max_links; current_link_id++)
		{
			BOOL link_found;
			u8 token;

			if (current_link_id < 8) {
				if (dual_node) {
					if (fam_15h)
						current_link = current_link_scan_order_g34_fam15[current_link_id];
					else
						current_link = current_link_scan_order_g34_fam10[current_link_id];
				} else {
					current_link = current_link_scan_order_default[current_link_id];
				}
			} else {
				current_link = current_link_id;
			}

			if (p_dat->ht_block->amd_cb_ignore_link && p_dat->ht_block->amd_cb_ignore_link(current_node, current_link))
				continue;

			if (p_dat->nb->read_true_link_fail_status(current_node, current_link, p_dat, p_dat->nb))
				continue;

			/* Make sure that the link is connected, coherent, and ready */
			if (!p_dat->nb->verify_link_is_coherent(current_node, current_link, p_dat->nb))
				continue;


			/* Test to see if the current_link has already been explored */
			link_found = FALSE;
			for (temp = 0; temp < p_dat->total_links; temp++)
			{
				if ((p_dat->port_list[temp * 2 + 1].node_id == current_node) &&
				   (p_dat->port_list[temp * 2 + 1].Link == current_link))
				{
					link_found = TRUE;
					break;
				}
			}
			if (link_found)
			{
				/* We had already expored this link */
				continue;
			}

			if (p_dat->nb->handleSpecialLinkCase(current_node, current_link, p_dat, p_dat->nb))
			{
				continue;
			}

			/* Modify current_node's routing table to use current_link to send
			 * traffic to current_node+1
			 */
			p_dat->nb->write_routing_table(current_node, current_node + 1, current_link, p_dat->nb);

			/* Check the northbridge of the node we just found, to make sure it is compatible
			 * before doing anything else to it.
			 */
			if (!p_dat->nb->is_compatible(current_node + 1, p_dat->nb))
			{
				u8 node_to_kill;

				/* Notify BIOS of event (while variables are still the same) */
				if (p_dat->ht_block->amd_cb_event_notify)
				{
					sHtEventCohFamilyFeud evt;
					evt.e_size = sizeof(sHtEventCohFamilyFeud);
					evt.node = current_node;
					evt.link = current_link;
					evt.total_nodes = p_dat->nodes_discovered;

					p_dat->ht_block->amd_cb_event_notify(HT_EVENT_CLASS_ERROR,
									HT_EVENT_COH_FAMILY_FEUD,
									(u8 *)&evt);
				}

				/* If node is not compatible, force boot to 1P
				 * If they are not compatible stop cHT init and:
				 *	1. Disable all cHT links on the BSP
				 *	2. Configure the BSP routing tables as a UP.
				 *	3. Notify main BIOS.
				 */
				p_dat->nodes_discovered = 0;
				current_node = 0;
				p_dat->total_links = 0;
				/* Abandon our coherent link data structure.  At this point there may
				 * be coherent links on the BSP that are not yet in the portList, and
				 * we have to turn them off anyway.  So depend on the hardware to tell us.
				 */
				for (current_link = 0; current_link < p_dat->nb->max_links; current_link++)
				{
					/* Stop all links which are connected, coherent, and ready */
					if (p_dat->nb->verify_link_is_coherent(current_node, current_link, p_dat->nb))
						p_dat->nb->stopLink(current_node, current_link, p_dat->nb);
				}

				for (node_to_kill = 0; node_to_kill < p_dat->nb->maxNodes; node_to_kill++)
				{
					p_dat->nb->write_full_routing_table(0, node_to_kill, ROUTETOSELF, ROUTETOSELF, 0, p_dat->nb);
				}

				/* End Coherent Discovery */
				STOP_HERE;
				break;
			}

			/* Read token from Current+1 */
			token = p_dat->nb->read_token(current_node + 1, p_dat->nb);
			ASSERT(token <= p_dat->nodes_discovered);
			if (token == 0)
			{
				p_dat->nodes_discovered++;
				ASSERT(p_dat->nodes_discovered < p_dat->nb->maxNodes);
				/* Check the capability of northbridges against the currently known configuration */
				if (!p_dat->nb->isCapable(current_node + 1, p_dat, p_dat->nb))
				{
					u8 node_to_kill;

					/* Notify BIOS of event  */
					if (p_dat->ht_block->amd_cb_event_notify)
					{
						sHtEventCohMpCapMismatch evt;
						evt.e_size = sizeof(sHtEventCohMpCapMismatch);
						evt.node = current_node;
						evt.link = current_link;
						evt.sys_mp_cap = p_dat->sys_mp_cap;
						evt.total_nodes = p_dat->nodes_discovered;

						p_dat->ht_block->amd_cb_event_notify(HT_EVENT_CLASS_ERROR,
									HT_EVENT_COH_MPCAP_MISMATCH,
									(u8 *)&evt);
					}

					p_dat->nodes_discovered = 0;
					current_node = 0;
					p_dat->total_links = 0;

					for (node_to_kill = 0; node_to_kill < p_dat->nb->maxNodes; node_to_kill++)
					{
						p_dat->nb->write_full_routing_table(0, node_to_kill, ROUTETOSELF, ROUTETOSELF, 0, p_dat->nb);
					}

					/* End Coherent Discovery */
					STOP_HERE;
					break;
				}

				token = p_dat->nodes_discovered;
				p_dat->nb->writeToken(current_node + 1, token, p_dat->nb);
				/* Inform that we have discovered a node, so that logical id to
				 * socket mapping info can be recorded.
				 */
				if (p_dat->ht_block->amd_cb_event_notify)
				{
					sHtEventCohNodeDiscovered evt;
					evt.e_size = sizeof(sHtEventCohNodeDiscovered);
					evt.node = current_node;
					evt.link = current_link;
					evt.newNode = token;

					p_dat->ht_block->amd_cb_event_notify(HT_EVENT_CLASS_INFO,
								HT_EVENT_COH_NODE_DISCOVERED,
								(u8 *)&evt);
				}
			}

			if (p_dat->total_links == MAX_PLATFORM_LINKS)
			{
				/*
				 * Exceeded our capacity to describe all coherent links found in the system.
				 * Error strategy:
				 * Auto recovery is not possible because data space is already all used.
				 * If the callback is not implemented or returns we will continue to initialize
				 * the fabric we are capable of representing, adding no more nodes or links.
				 * This should yield a bootable topology, but likely not the one intended.
				 * We cannot continue discovery, there may not be any way to route a new
				 * node back to the BSP if we can't add links to our representation of the system.
				 */
				if (p_dat->ht_block->amd_cb_event_notify)
				{
					sHtEventCohLinkExceed evt;
					evt.e_size = sizeof(sHtEventCohLinkExceed);
					evt.node = current_node;
					evt.link = current_link;
					evt.target_node = token;
					evt.total_nodes = p_dat->nodes_discovered;
					evt.max_links = p_dat->nb->max_links;

					p_dat->ht_block->amd_cb_event_notify(HT_EVENT_CLASS_ERROR,
									HT_EVENT_COH_LINK_EXCEED,
									(u8 *)&evt);
				}
				/* Force link and node loops to halt */
				STOP_HERE;
				current_node = p_dat->nodes_discovered;
				break;
			}

			p_dat->port_list[p_dat->total_links * 2].Type = PORTLIST_TYPE_CPU;
			p_dat->port_list[p_dat->total_links * 2].Link = current_link;
			p_dat->port_list[p_dat->total_links * 2].node_id = current_node;

			p_dat->port_list[p_dat->total_links * 2 + 1].Type = PORTLIST_TYPE_CPU;
			p_dat->port_list[p_dat->total_links * 2 + 1].Link = p_dat->nb->read_def_lnk(current_node + 1, p_dat->nb);
			p_dat->port_list[p_dat->total_links * 2 + 1].node_id = token;

			p_dat->total_links++;

			if (!p_dat->sys_matrix[current_node][token])
			{
				p_dat->sys_degree[current_node]++;
				p_dat->sys_degree[token]++;
				p_dat->sys_matrix[current_node][token] = TRUE;
				p_dat->sys_matrix[token][current_node] = TRUE;
			}
		}
		current_node++;
	}
}


/***************************************************************************
 ***		ISOMORPHISM BASED ROUTING TABLE GENERATION CODE		***
 ***************************************************************************/

/*----------------------------------------------------------------------------------------
 * BOOL
 * iso_morph(u8 i, sMainData *p_dat)
 *
 *  Description:
 *	 Is graphA isomorphic to graphB?
 *	 if this function returns true, then perm will contain the permutation
 *	 required to transform graphB into graphA.
 *	 We also use the degree of each node, that is the number of connections it has, to
 *	 speed up rejection of non-isomorphic graphs (if there is a node in graphA with n
 *	 connections, there must be at least one unmatched in graphB with n connections).
 *
 *  Parameters:
 *	@param[in] u8 i   = the discovered node which we are trying to match
 *		    with a permutation the topology
 *	@param[in]/@param[out] sMainData* p_dat  = our global state, degree and adjacency matrix,
 *				  output a permutation if successful
 *	@param[out] BOOL results = the graphs are (or are not) isomorphic
 * ---------------------------------------------------------------------------------------
 */
static BOOL iso_morph(u8 i, sMainData *p_dat)
{
	u8 j, k;
	u8 nodecnt;

	/* We have only been called if nodecnt == p_selected->size ! */
	nodecnt = p_dat->nodes_discovered + 1;

	if (i != nodecnt)
	{
		/*  Keep building the permutation */
		for (j = 0; j < nodecnt; j++)
		{
			/*  Make sure the degree matches */
			if (p_dat->sys_degree[i] != p_dat->db_degree[j])
				continue;

			/*  Make sure that j hasn't been used yet (ought to use a "used" */
			/*  array instead, might be faster) */
			for (k = 0; k < i; k++)
			{
				if (p_dat->perm[k] == j)
					break;
			}
			if (k != i)
				continue;
			p_dat->perm[i] = j;
			if (iso_morph(i + 1, p_dat))
				return TRUE;
		}
		return FALSE;
	} else {
		/*  Test to see if the permutation is isomorphic */
		for (j = 0; j < nodecnt; j++)
		{
			for (k = 0; k < nodecnt; k++)
			{
				if (p_dat->sys_matrix[j][k] !=
				   p_dat->db_matrix[p_dat->perm[j]][p_dat->perm[k]])
					return FALSE;
			}
		}
		return TRUE;
	}
}


/*----------------------------------------------------------------------------------------
 * void
 * lookup_compute_and_load_routing_tables(sMainData *p_dat)
 *
 *  Description:
 *	 Using the description of the fabric topology we discovered, try to find a match
 *	 among the supported topologies.  A supported topology description matches
 *	 the discovered fabric if the nodes can be matched in such a way that all the nodes connected
 *	 in one set are exactly the nodes connected in the other (formally, that the graphs are
 *	 isomorphic).  Which links are used is not really important to matching.  If the graphs
 *	 match, then there is a permutation of one that translates the node positions and linkages
 *	 to the other.
 *
 *	 In order to make the isomorphism test efficient, we test for matched number of nodes
 *	 (a 4 node fabric is not isomorphic to a 2 node topology), and provide degrees of nodes
 *	 to the isomorphism test.
 *
 *	 The generic routing table solution for any topology is predetermined and represented
 *	 as part of the topology.  The permutation we computed tells us how to interpret the
 *	 routing onto the fabric we discovered.	 We do this working backward from the last
 *	 node discovered to the BSP, writing the routing tables as we go.
 *
 *  Parameters:
 *	@param[in]    sMainData* p_dat = our global state, the discovered fabric,
 *	@param[out]			degree matrix, permutation
 * ---------------------------------------------------------------------------------------
 */
static void lookup_compute_and_load_routing_tables(sMainData *p_dat)
{
	u8 **p_topology_list;
	u8 *p_selected;

	int i, j, k, size;

	size = p_dat->nodes_discovered + 1;
	/* Use the provided topology list or the internal, default one. */
	p_topology_list = p_dat->ht_block->topolist;
	if (p_topology_list == NULL)
	{
		get_amd_topolist(&p_topology_list);
	}

	p_selected = *p_topology_list;
	while (p_selected != NULL)
	{
		if (graph_how_many_nodes(p_selected) == size)
		{
			/*  Build Degree vector and Adjency Matrix for this entry */
			for (i = 0; i < size; i++)
			{
				p_dat->db_degree[i] = 0;
				for (j = 0; j < size; j++)
				{
					if (graph_is_adjacent(p_selected, i, j))
					{
						p_dat->db_matrix[i][j] = 1;
						p_dat->db_degree[i]++;
					}
					else
					{
						p_dat->db_matrix[i][j] = 0;
					}
				}
			}
			if (iso_morph(0, p_dat))
				break;  /*  A matching topology was found */
		}

		p_topology_list++;
		p_selected = *p_topology_list;
	}

	if (p_selected != NULL)
	{
		/*  Compute the reverse Permutation */
		for (i = 0; i < size; i++)
		{
			p_dat->reverse_perm[p_dat->perm[i]] = i;
		}

		/*  Start with the last discovered node, and move towards the BSP */
		for (i = size-1; i >= 0; i--)
		{
			for (j = 0; j < size; j++)
			{
				u8 req_target_link, rsp_target_link;
				u8 req_target_node, rsp_target_node;

				u8 abstract_bc_target_nodes = graph_get_bc(p_selected, p_dat->perm[i], p_dat->perm[j]);
				u32 bc_target_links = 0;

				for (k = 0; k < MAX_NODES; k++)
				{
					if (abstract_bc_target_nodes & ((u32)1 << k))
					{
						bc_target_links |= (u32)1 << convert_node_to_link(i, p_dat->reverse_perm[k], p_dat);
					}
				}

				if (i == j)
				{
					req_target_link = ROUTETOSELF;
					rsp_target_link = ROUTETOSELF;
				}
				else
				{
					req_target_node = graph_get_req(p_selected, p_dat->perm[i], p_dat->perm[j]);
					req_target_link = convert_node_to_link(i, p_dat->reverse_perm[req_target_node], p_dat);

					rsp_target_node = graph_get_rsp(p_selected, p_dat->perm[i], p_dat->perm[j]);
					rsp_target_link = convert_node_to_link(i, p_dat->reverse_perm[rsp_target_node], p_dat);
				}

				p_dat->nb->write_full_routing_table(i, j, req_target_link, rsp_target_link, bc_target_links, p_dat->nb);
			}
			/* Clean up discovery 'footprint' that otherwise remains in the routing table.  It didn't hurt
			 * anything, but might cause confusion during debug and validation.  Do this by setting the
			 * route back to all self routes. Since it's the node that would be one more than actually installed,
			 * this only applies if less than maxNodes were found.
			 */
			if (size < p_dat->nb->maxNodes)
			{
				p_dat->nb->write_full_routing_table(i, size, ROUTETOSELF, ROUTETOSELF, 0, p_dat->nb);
			}
		}

	}
	else
	{
		/*
		 * No Matching Topology was found
		 * Error Strategy:
		 * Auto recovery doesn't seem likely, Force boot as 1P.
		 * For reporting, logging, provide number of nodes
		 * If not implemented or returns, boot as BSP uniprocessor.
		 */
		if (p_dat->ht_block->amd_cb_event_notify)
		{
			sHtEventCohNoTopology evt;
			evt.e_size = sizeof(sHtEventCohNoTopology);
			evt.total_nodes = p_dat->nodes_discovered;

			p_dat->ht_block->amd_cb_event_notify(HT_EVENT_CLASS_ERROR,
						HT_EVENT_COH_NO_TOPOLOGY,
						(u8 *)&evt);
		}
		STOP_HERE;
		/* Force 1P */
		p_dat->nodes_discovered = 0;
		p_dat->total_links = 0;
		p_dat->nb->enable_routing_tables(0, p_dat->nb);
	}
}
#endif /* HT_BUILD_NC_ONLY */


/*----------------------------------------------------------------------------------------
 * void
 * finialize_coherent_init(sMainData *p_dat)
 *
 *  Description:
 *	 Find the total number of cores and update the number of nodes and cores in all cpus.
 *	 Limit CPU config access to installed cpus.
 *
 *  Parameters:
 *	@param[in] sMainData* p_dat = our global state, number of nodes discovered.
 * ---------------------------------------------------------------------------------------
 */
static void finialize_coherent_init(sMainData *p_dat)
{
	u8 cur_node;

	u8 total_cores = 0;
	for (cur_node = 0; cur_node < p_dat->nodes_discovered + 1; cur_node++)
	{
		total_cores += p_dat->nb->getNumCoresOnNode(cur_node, p_dat->nb);
	}

	for (cur_node = 0; cur_node < p_dat->nodes_discovered + 1; cur_node++)
	{
		p_dat->nb->set_total_nodes_and_cores(cur_node, p_dat->nodes_discovered + 1, total_cores, p_dat->nb);
	}

	for (cur_node = 0; cur_node < p_dat->nodes_discovered + 1; cur_node++)
	{
		p_dat->nb->limit_nodes(cur_node, p_dat->nb);
	}

}

/*----------------------------------------------------------------------------------------
 * void
 * coherent_init(sMainData *p_dat)
 *
 *  Description:
 *	 Perform discovery and initialization of the coherent fabric.
 *
 *  Parameters:
 *	@param[in] sMainData* p_dat = our global state
 * ---------------------------------------------------------------------------------------
 */
static void coherent_init(sMainData *p_dat)
{
#ifdef HT_BUILD_NC_ONLY
	/* Replace discovery process with:
	 * No other nodes, no coherent links
	 * Enable routing tables on current_node, for power on self route
	 */
	p_dat->nodes_discovered = 0;
	p_dat->total_links = 0;
	p_dat->nb->enable_routing_tables(0, p_dat->nb);
#else
	u8 i, j;

	p_dat->nodes_discovered = 0;
	p_dat->total_links = 0;
	for (i = 0; i < MAX_NODES; i++)
	{
		p_dat->sys_degree[i] = 0;
		for (j = 0; j < MAX_NODES; j++)
		{
			p_dat->sys_matrix[i][j] = 0;
		}
	}

	ht_discovery_flood_fill(p_dat);
	lookup_compute_and_load_routing_tables(p_dat);
#endif
	finialize_coherent_init(p_dat);
}

/***************************************************************************
 ***			    Non-coherent init code			  ***
 ***				  Algorithms				  ***
 ***************************************************************************/
/*----------------------------------------------------------------------------------------
 * void
 * process_link(u8 node, u8 link, sMainData *p_dat)
 *
 *  Description:
 *	 Process a non-coherent link, enabling a range of bus numbers, and setting the device
 *	 ID for all devices found
 *
 *  Parameters:
 *	@param[in] u8 node = Node on which to process nc init
 *	@param[in] u8 link = The non-coherent link on that node
 *	@param[in] sMainData* p_dat = our global state
 * ---------------------------------------------------------------------------------------
 */
static void process_link(u8 node, u8 link, sMainData *p_dat)
{
	u8 sec_bus, sub_bus;
	u32 current_buid;
	u32 temp;
	u32 unit_id_cnt;
	SBDFO current_ptr;
	u8 depth;
	const u8 *p_swap_ptr;

	SBDFO last_sbdfo = ILLEGAL_SBDFO;
	u8 last_link = 0;

	ASSERT(node < p_dat->nb->maxNodes && link < p_dat->nb->max_links);

	if ((p_dat->ht_block->amd_cb_override_bus_numbers == NULL)
	   || !p_dat->ht_block->amd_cb_override_bus_numbers(node, link, &sec_bus, &sub_bus))
	{
		/* Assign Bus numbers */
		if (p_dat->auto_bus_current >= p_dat->ht_block->auto_bus_max)
		{
			/* If we run out of Bus Numbers notify, if call back unimplemented or if it
			 * returns, skip this chain
			 */
			if (p_dat->ht_block->amd_cb_event_notify)
			{
				sHTEventNcohBusMaxExceed evt;
				evt.e_size = sizeof(sHTEventNcohBusMaxExceed);
				evt.node = node;
				evt.link = link;
				evt.bus = p_dat->auto_bus_current;

				p_dat->ht_block->amd_cb_event_notify(HT_EVENT_CLASS_ERROR,HT_EVENT_NCOH_BUS_MAX_EXCEED,(u8 *)&evt);
			}
			STOP_HERE;
			return;
		}

		if (p_dat->used_cfg_map_entires >= 4)
		{
			/* If we have used all the PCI Config maps we can't add another chain.
			 * Notify and if call back is unimplemented or returns, skip this chain.
			 */
			if (p_dat->ht_block->amd_cb_event_notify)
			{
				sHtEventNcohCfgMapExceed evt;
				evt.e_size = sizeof(sHtEventNcohCfgMapExceed);
				evt.node = node;
				evt.link = link;

				p_dat->ht_block->amd_cb_event_notify(HT_EVENT_CLASS_ERROR,
							HT_EVENT_NCOH_CFG_MAP_EXCEED,
							(u8 *)&evt);
			}
			STOP_HERE;
			return;
		}

		sec_bus = p_dat->auto_bus_current;
		sub_bus = sec_bus + p_dat->ht_block->auto_bus_increment-1;
		p_dat->auto_bus_current += p_dat->ht_block->auto_bus_increment;
	}

	p_dat->nb->setCFGAddrMap(p_dat->used_cfg_map_entires, sec_bus, sub_bus, node, link, p_dat, p_dat->nb);
	p_dat->used_cfg_map_entires++;

	if ((p_dat->ht_block->amd_cb_manual_buid_swap_list != NULL)
	 && p_dat->ht_block->amd_cb_manual_buid_swap_list(node, link, &p_swap_ptr))
	{
		/* Manual non-coherent BUID assignment */
		current_buid = 1;

		/* Assign BUID's per manual override */
		while (*p_swap_ptr != 0xFF)
		{
			current_ptr = MAKE_SBDFO(0, sec_bus, *p_swap_ptr, 0, 0);
			p_swap_ptr++;

			do
			{
				amd_pci_find_next_cap(&current_ptr);
				ASSERT(current_ptr != ILLEGAL_SBDFO);
				AmdPCIRead(current_ptr, &temp);
			} while (!IS_HT_SLAVE_CAPABILITY(temp));

			current_buid = *p_swap_ptr;
			p_swap_ptr++;
			amd_pci_write_bits(current_ptr, 20, 16, &current_buid);
		}

		/* Build chain of devices */
		depth = 0;
		p_swap_ptr++;
		while (*p_swap_ptr != 0xFF)
		{
			p_dat->port_list[p_dat->total_links * 2].node_id = node;
			if (depth == 0)
			{
				p_dat->port_list[p_dat->total_links * 2].Type = PORTLIST_TYPE_CPU;
				p_dat->port_list[p_dat->total_links * 2].Link = link;
			}
			else
			{
				p_dat->port_list[p_dat->total_links * 2].Type = PORTLIST_TYPE_IO;
				p_dat->port_list[p_dat->total_links * 2].Link = 1 - last_link;
				p_dat->port_list[p_dat->total_links * 2].host_link = link;
				p_dat->port_list[p_dat->total_links * 2].host_depth = depth - 1;
				p_dat->port_list[p_dat->total_links * 2].pointer = last_sbdfo;
			}

			p_dat->port_list[p_dat->total_links * 2 + 1].Type = PORTLIST_TYPE_IO;
			p_dat->port_list[p_dat->total_links * 2 + 1].node_id = node;
			p_dat->port_list[p_dat->total_links * 2 + 1].host_link = link;
			p_dat->port_list[p_dat->total_links * 2 + 1].host_depth = depth;

			current_ptr = MAKE_SBDFO(0, sec_bus, (*p_swap_ptr & 0x3F), 0, 0);
			do
			{
				amd_pci_find_next_cap(&current_ptr);
				ASSERT(current_ptr != ILLEGAL_SBDFO);
				AmdPCIRead(current_ptr, &temp);
			} while (!IS_HT_SLAVE_CAPABILITY(temp));
			p_dat->port_list[p_dat->total_links * 2 + 1].pointer = current_ptr;
			last_sbdfo = current_ptr;

			/* Bit 6 indicates whether orientation override is desired.
			 * Bit 7 indicates the upstream link if overriding.
			 */
			/* assert catches at least the one known incorrect setting */
			ASSERT ((*p_swap_ptr & 0x40) || (!(*p_swap_ptr & 0x80)));
			if (*p_swap_ptr & 0x40)
			{
				/* Override the device's orientation */
				last_link = *p_swap_ptr >> 7;
			}
			else
			{
				/* Detect the device's orientation */
				amd_pci_read_bits(current_ptr, 26, 26, &temp);
				last_link = (u8)temp;
			}
			p_dat->port_list[p_dat->total_links * 2 + 1].Link = last_link;

			depth++;
			p_dat->total_links++;
			p_swap_ptr++;
		}
	}
	else
	{
		/* Automatic non-coherent device detection */
		depth = 0;
		current_buid = 1;
		while (1)
		{
			current_ptr = MAKE_SBDFO(0, sec_bus, 0, 0, 0);

			AmdPCIRead(current_ptr, &temp);
			if (temp == 0xFFFFFFFF)
				/* No device found at current_ptr */
				break;

			if (p_dat->total_links == MAX_PLATFORM_LINKS)
			{
				/*
				 * Exceeded our capacity to describe all non-coherent links found in the system.
				 * Error strategy:
				 * Auto recovery is not possible because data space is already all used.
				 */
				if (p_dat->ht_block->amd_cb_event_notify)
				{
					sHtEventNcohLinkExceed evt;
					evt.e_size = sizeof(sHtEventNcohLinkExceed);
					evt.node = node;
					evt.link = link;
					evt.depth = depth;
					evt.max_links = p_dat->nb->max_links;

					p_dat->ht_block->amd_cb_event_notify(HT_EVENT_CLASS_ERROR,
								HT_EVENT_NCOH_LINK_EXCEED,
								(u8 *)&evt);
				}
				/* Force link loop to halt */
				STOP_HERE;
				break;
			}

			p_dat->port_list[p_dat->total_links*2].node_id = node;
			if (depth == 0)
			{
				p_dat->port_list[p_dat->total_links * 2].Type = PORTLIST_TYPE_CPU;
				p_dat->port_list[p_dat->total_links * 2].Link = link;
			}
			else
			{
				p_dat->port_list[p_dat->total_links * 2].Type = PORTLIST_TYPE_IO;
				p_dat->port_list[p_dat->total_links * 2].Link = 1 - last_link;
				p_dat->port_list[p_dat->total_links * 2].host_link = link;
				p_dat->port_list[p_dat->total_links * 2].host_depth = depth - 1;
				p_dat->port_list[p_dat->total_links * 2].pointer = last_sbdfo;
			}

			p_dat->port_list[p_dat->total_links * 2 + 1].Type = PORTLIST_TYPE_IO;
			p_dat->port_list[p_dat->total_links * 2 + 1].node_id = node;
			p_dat->port_list[p_dat->total_links * 2 + 1].host_link = link;
			p_dat->port_list[p_dat->total_links * 2 + 1].host_depth = depth;

			do
			{
				amd_pci_find_next_cap(&current_ptr);
				ASSERT(current_ptr != ILLEGAL_SBDFO);
				AmdPCIRead(current_ptr, &temp);
			} while (!IS_HT_SLAVE_CAPABILITY(temp));

			amd_pci_read_bits(current_ptr, 25, 21, &unit_id_cnt);
			if ((unit_id_cnt + current_buid > 31) || ((sec_bus == 0) && (unit_id_cnt + current_buid > 24)))
			{
				/* An error handler for the case where we run out of BUID's on a chain */
				if (p_dat->ht_block->amd_cb_event_notify)
				{
					sHtEventNcohBuidExceed evt;
					evt.e_size = sizeof(sHtEventNcohBuidExceed);
					evt.node = node;
					evt.link = link;
					evt.depth = depth;
					evt.current_buid = (uint8)current_buid;
					evt.unit_count = (uint8)unit_id_cnt;

					p_dat->ht_block->amd_cb_event_notify(HT_EVENT_CLASS_ERROR,HT_EVENT_NCOH_BUID_EXCEED,(u8 *)&evt);
				}
				STOP_HERE;
				break;
			}
			amd_pci_write_bits(current_ptr, 20, 16, &current_buid);


			current_ptr += MAKE_SBDFO(0, 0, current_buid, 0, 0);
			amd_pci_read_bits(current_ptr, 20, 16, &temp);
			if (temp != current_buid)
			{
				/* An error handler for this critical error */
				if (p_dat->ht_block->amd_cb_event_notify)
				{
					sHtEventNcohDeviceFailed evt;
					evt.e_size = sizeof(sHtEventNcohDeviceFailed);
					evt.node = node;
					evt.link = link;
					evt.depth = depth;
					evt.attempted_buid = (uint8)current_buid;

					p_dat->ht_block->amd_cb_event_notify(HT_EVENT_CLASS_ERROR,HT_EVENT_NCOH_DEVICE_FAILED,(u8 *)&evt);
				}
				STOP_HERE;
				break;
			}

			amd_pci_read_bits(current_ptr, 26, 26, &temp);
			p_dat->port_list[p_dat->total_links * 2 + 1].Link = (u8)temp;
			p_dat->port_list[p_dat->total_links * 2 + 1].pointer = current_ptr;

			last_link = (u8)temp;
			last_sbdfo = current_ptr;

			depth++;
			p_dat->total_links++;
			current_buid += unit_id_cnt;
		}
		if (p_dat->ht_block->amd_cb_event_notify)
		{
			/* Provide information on automatic device results */
			sHtEventNcohAutoDepth evt;
			evt.e_size = sizeof(sHtEventNcohAutoDepth);
			evt.node = node;
			evt.link = link;
			evt.depth = (depth - 1);

			p_dat->ht_block->amd_cb_event_notify(HT_EVENT_CLASS_INFO,HT_EVENT_NCOH_AUTO_DEPTH,(u8 *)&evt);
		}
	}
}


/*----------------------------------------------------------------------------------------
 * void
 * nc_init(sMainData *p_dat)
 *
 *  Description:
 *	 Initialize the non-coherent fabric. Begin with the compat link on the BSP, then
 *	 find and initialize all other non-coherent chains.
 *
 *  Parameters:
 *	@param[in]  sMainData*  p_dat = our global state
 * ---------------------------------------------------------------------------------------
 */
static void nc_init(sMainData *p_dat)
{
	u8 node, link;
	u8 compat_link;

	compat_link = p_dat->nb->read_sb_link(p_dat->nb);
	process_link(0, compat_link, p_dat);

	for (node = 0; node <= p_dat->nodes_discovered; node++)
	{
		for (link = 0; link < p_dat->nb->max_links; link++)
		{
			if (p_dat->ht_block->amd_cb_ignore_link && p_dat->ht_block->amd_cb_ignore_link(node, link))
				continue;   /*  Skip the link */

			if (node == 0 && link == compat_link)
				continue;

			if (p_dat->nb->read_true_link_fail_status(node, link, p_dat, p_dat->nb))
				continue;

			if (p_dat->nb->verify_link_is_non_coherent(node, link, p_dat->nb))
				process_link(node, link, p_dat);
		}
	}
}

/***************************************************************************
 ***				Link Optimization			  ***
 ***************************************************************************/

/*----------------------------------------------------------------------------------------
 * void
 * regang_links(sMainData *p_dat)
 *
 *  Description:
 *	 Test the sublinks of a link to see if they qualify to be reganged.  If they do,
 *	 update the port list data to indicate that this should be done.  Note that no
 *	 actual hardware state is changed in this routine.
 *
 *  Parameters:
 *	@param[in,out] sMainData*  p_dat = our global state
 * ---------------------------------------------------------------------------------------
 */
static void regang_links(sMainData *p_dat)
{
#ifndef HT_BUILD_NC_ONLY
	u8 i, j;
	for (i = 0; i < p_dat->total_links * 2; i += 2)
	{
		ASSERT(p_dat->port_list[i].Type < 2 && p_dat->port_list[i].Link < p_dat->nb->max_links);  /*  Data validation */
		ASSERT(p_dat->port_list[i + 1].Type < 2 && p_dat->port_list[i + 1].Link < p_dat->nb->max_links); /*  data validation */
		ASSERT(!(p_dat->port_list[i].Type == PORTLIST_TYPE_IO && p_dat->port_list[i + 1].Type == PORTLIST_TYPE_CPU));  /*  ensure src is closer to the bsp than dst */

		/* Regang is false unless we pass all conditions below */
		p_dat->port_list[i].sel_regang = FALSE;
		p_dat->port_list[i + 1].sel_regang = FALSE;

		if ((p_dat->port_list[i].Type != PORTLIST_TYPE_CPU) || (p_dat->port_list[i + 1].Type != PORTLIST_TYPE_CPU))
			continue;   /*  Only process CPU to CPU links */

		for (j = i + 2; j < p_dat->total_links * 2; j += 2)
		{
			if ((p_dat->port_list[j].Type != PORTLIST_TYPE_CPU) || (p_dat->port_list[j + 1].Type != PORTLIST_TYPE_CPU))
				continue;   /*  Only process CPU to CPU links */

			if (p_dat->port_list[i].node_id != p_dat->port_list[j].node_id)
				continue;   /*  Links must be from the same source */

			if (p_dat->port_list[i + 1].node_id != p_dat->port_list[j + 1].node_id)
				continue;   /*  Link must be to the same target */

			if ((p_dat->port_list[i].Link & 3) != (p_dat->port_list[j].Link & 3))
				continue;   /*  Ensure same source base port */

			if ((p_dat->port_list[i + 1].Link & 3) != (p_dat->port_list[j + 1].Link & 3))
				continue;   /*  Ensure same destination base port */

			if ((p_dat->port_list[i].Link & 4) != (p_dat->port_list[i + 1].Link & 4))
				continue;   /*  Ensure sublink0 routes to sublink0 */

			ASSERT((p_dat->port_list[j].Link & 4) == (p_dat->port_list[j + 1].Link & 4)); /*  (therefore sublink1 routes to sublink1) */

			if (p_dat->ht_block->amd_cb_skip_regang &&
				p_dat->ht_block->amd_cb_skip_regang(p_dat->port_list[i].node_id,
							p_dat->port_list[i].Link & 0x03,
							p_dat->port_list[i + 1].node_id,
							p_dat->port_list[i + 1].Link & 0x03))
			{
				continue;   /*  Skip regang */
			}


			p_dat->port_list[i].Link &= 0x03; /*  Force to point to sublink0 */
			p_dat->port_list[i + 1].Link &= 0x03;
			p_dat->port_list[i].sel_regang = TRUE; /*  Enable link reganging */
			p_dat->port_list[i + 1].sel_regang = TRUE;
			p_dat->port_list[i].prv_width_out_cap = HT_WIDTH_16_BITS;
			p_dat->port_list[i + 1].prv_width_out_cap = HT_WIDTH_16_BITS;
			p_dat->port_list[i].prv_width_in_cap = HT_WIDTH_16_BITS;
			p_dat->port_list[i + 1].prv_width_in_cap = HT_WIDTH_16_BITS;

			/*  Delete port_list[j, j+1], slow but easy to debug implementation */
			p_dat->total_links--;
			amd_memcpy(&(p_dat->port_list[j]), &(p_dat->port_list[j + 2]), sizeof(sPortDescriptor)*(p_dat->total_links * 2 - j));
			amd_memset(&(p_dat->port_list[p_dat->total_links * 2]), INVALID_LINK, sizeof(sPortDescriptor) * 2);

			/* //High performance, but would make debuging harder due to 'shuffling' of the records */
			/* //amd_memcpy(port_list[TotalPorts-2], port_list[j], SIZEOF(sPortDescriptor)*2); */
			/* //TotalPorts -=2; */

			break; /*  Exit loop, advance to port_list[i+2] */
		}
	}
#endif /* HT_BUILD_NC_ONLY */
}

static void detect_io_link_isochronous_capable(sMainData *p_dat)
{
	u8 i;
	u8 isochronous_capable = 0;
	u8 iommu = get_uint_option("iommu", 1);

	for (i = 0; i < p_dat->total_links * 2; i += 2) {
		if ((p_dat->port_list[i].Type == PORTLIST_TYPE_CPU) && (p_dat->port_list[i + 1].Type == PORTLIST_TYPE_IO)) {
			if ((p_dat->port_list[i].prv_feature_cap & 0x1) && (p_dat->port_list[i + 1].prv_feature_cap & 0x1)) {
				p_dat->port_list[i].enable_isochronous_mode = 1;
				p_dat->port_list[i + 1].enable_isochronous_mode = 1;
				isochronous_capable = 1;
			} else {
				p_dat->port_list[i].enable_isochronous_mode = 0;
				p_dat->port_list[i + 1].enable_isochronous_mode = 0;
			}
		}
	}

	if (isochronous_capable && iommu) {
		printk(BIOS_DEBUG, "Forcing HT links to isochronous mode due to enabled IOMMU\n");
		/* Isochronous mode must be set on all links if the IOMMU is enabled */
		for (i = 0; i < p_dat->total_links * 2; i += 2) {
			p_dat->port_list[i].enable_isochronous_mode = 1;
			p_dat->port_list[i + 1].enable_isochronous_mode = 1;
		}
	}
}

/*----------------------------------------------------------------------------------------
 * void
 * select_optimal_width_and_frequency(sMainData *p_dat)
 *
 *  Description:
 *	 For all links:
 *	 Examine both sides of a link and determine the optimal frequency and width,
 *	 taking into account externally provided limits and enforcing any other limit
 *	 or matching rules as applicable except sublink balancing.   Update the port
 *	 list date with the optimal settings.
 *	 Note no hardware state changes in this routine.
 *
 *  Parameters:
 *	@param[in,out]  sMainData*  p_dat = our global state, port list data
 * ---------------------------------------------------------------------------------------
 */
static void select_optimal_width_and_frequency(sMainData *p_dat)
{
	u8 i, j;
	u32 temp;
	u32 cb_pcb_freq_limit;
	u32 cb_pcb_freq_limit_nvram;
	u8 cb_pcb_ab_downstream_width;
	u8 cb_pcb_ba_upstream_width;

	cb_pcb_freq_limit_nvram = 0xfffff;
	temp = get_uint_option("hypertransport_speed_limit", 0);

	cb_pcb_freq_limit_nvram = ht_speed_limit[temp & 0xf];

	if (!is_fam_15h()) {
		/* FIXME
		 * By default limit frequency to 2.6 GHz as there are residual
		 * problems with HT v3.1 implementation on at least some Socket G34
		 * mainboards / Fam10h CPUs.
		 * Debug the issues and reenable this...
		 */
		if (cb_pcb_freq_limit_nvram > 0xffff)
			cb_pcb_freq_limit_nvram = 0xffff;
	}

	for (i = 0; i < p_dat->total_links*2; i += 2)
	{
		cb_pcb_freq_limit = 0xfffff;		// Maximum allowed by autoconfiguration
		if (p_dat->ht_block->ht_link_configuration)
			cb_pcb_freq_limit = ht_speed_mhz_to_hw(p_dat->ht_block->ht_link_configuration->ht_speed_limit);
		cb_pcb_freq_limit = MIN(cb_pcb_freq_limit, cb_pcb_freq_limit_nvram);

#if CONFIG(LIMIT_HT_DOWN_WIDTH_8)
		cb_pcb_ab_downstream_width = 8;
#else
		cb_pcb_ab_downstream_width = 16;
#endif

#if CONFIG(LIMIT_HT_UP_WIDTH_8)
		cb_pcb_ba_upstream_width = 8;
#else
		cb_pcb_ba_upstream_width = 16;
#endif

		if ((p_dat->port_list[i].Type == PORTLIST_TYPE_CPU) && (p_dat->port_list[i + 1].Type == PORTLIST_TYPE_CPU))
		{
			if (p_dat->ht_block->amd_cb_cpu_2_cpu_pcb_limits)
			{
				p_dat->ht_block->amd_cb_cpu_2_cpu_pcb_limits(
						p_dat->port_list[i].node_id,
						p_dat->port_list[i].Link,
						p_dat->port_list[i + 1].node_id,
						p_dat->port_list[i + 1].Link,
						&cb_pcb_ab_downstream_width,
						&cb_pcb_ba_upstream_width, &cb_pcb_freq_limit
						);
			}
		}
		else
		{
			if (p_dat->ht_block->amd_cb_iop_cb_limits)
			{
				p_dat->ht_block->amd_cb_iop_cb_limits(
						p_dat->port_list[i + 1].node_id,
						p_dat->port_list[i + 1].host_link,
						p_dat->port_list[i + 1].host_depth,
						&cb_pcb_ab_downstream_width,
						 &cb_pcb_ba_upstream_width, &cb_pcb_freq_limit
						);
			}
		}

		temp = p_dat->port_list[i].prv_frequency_cap;
		temp &= p_dat->port_list[i + 1].prv_frequency_cap;
		temp &= cb_pcb_freq_limit;
		p_dat->port_list[i].composite_frequency_cap = temp;
		p_dat->port_list[i + 1].composite_frequency_cap = temp;

		ASSERT (temp != 0);
		for (j = 19;; j--)
		{
			if ((j == 16) || (j == 15))
				continue;
			if (temp & ((u32)1 << j))
				break;
		}

		p_dat->port_list[i].sel_frequency = j;
		p_dat->port_list[i + 1].sel_frequency = j;

		temp = p_dat->port_list[i].prv_width_out_cap;
		if (p_dat->port_list[i + 1].prv_width_in_cap < temp)
			temp = p_dat->port_list[i + 1].prv_width_in_cap;
		if (cb_pcb_ab_downstream_width < temp)
			temp = cb_pcb_ab_downstream_width;
		p_dat->port_list[i].sel_width_out = (u8)temp;
		p_dat->port_list[i + 1].sel_width_in = (u8)temp;

		temp = p_dat->port_list[i].prv_width_in_cap;
		if (p_dat->port_list[i + 1].prv_width_out_cap < temp)
			temp = p_dat->port_list[i + 1].prv_width_out_cap;
		if (cb_pcb_ba_upstream_width < temp)
			temp = cb_pcb_ba_upstream_width;
		p_dat->port_list[i].sel_width_in = (u8)temp;
		p_dat->port_list[i + 1].sel_width_out = (u8)temp;
	}
}

/*----------------------------------------------------------------------------------------
 * void
 * hammer_sublink_fixup(sMainData *p_dat)
 *
 *  Description:
 *	 Iterate through all links, checking the frequency of each sublink pair.  Make the
 *	 adjustment to the port list data so that the frequencies are at a valid ratio,
 *	 reducing frequency as needed to achieve this. (All links support the minimum 200 MHz
 *	 frequency.)  Repeat the above until no adjustments are needed.
 *	 Note no hardware state changes in this routine.
 *
 *  Parameters:
 *	@param[in,out] sMainData* p_dat = our global state, link state and port list
 * ---------------------------------------------------------------------------------------
 */
static void hammer_sublink_fixup(sMainData *p_dat)
{
#ifndef HT_BUILD_NC_ONLY
	u8 i, j, k;
	BOOL changes, downgrade;

	u8 hi_index;
	u8 hi_freq, lo_freq;

	u32 temp;

	do
	{
		changes = FALSE;
		for (i = 0; i < p_dat->total_links*2; i++)
		{
			if (p_dat->port_list[i].Type != PORTLIST_TYPE_CPU) /*  Must be a CPU link */
				continue;
			if (p_dat->port_list[i].Link < 4) /*  Only look for sublink1's */
				continue;

			for (j = 0; j < p_dat->total_links*2; j++)
			{
				/*  Step 1. Find the matching sublink0 */
				if (p_dat->port_list[j].Type != PORTLIST_TYPE_CPU)
					continue;
				if (p_dat->port_list[j].node_id != p_dat->port_list[i].node_id)
					continue;
				if (p_dat->port_list[j].Link != (p_dat->port_list[i].Link & 0x03))
					continue;

				/*  Step 2. Check for an illegal frequency ratio */
				if (p_dat->port_list[i].sel_frequency >= p_dat->port_list[j].sel_frequency)
				{
					hi_index = i;
					hi_freq = p_dat->port_list[i].sel_frequency;
					lo_freq = p_dat->port_list[j].sel_frequency;
				}
				else
				{
					hi_index = j;
					hi_freq = p_dat->port_list[j].sel_frequency;
					lo_freq = p_dat->port_list[i].sel_frequency;
				}

				if (hi_freq == lo_freq)
					break; /*  The frequencies are 1:1, no need to do anything */

				downgrade = FALSE;

				if (hi_freq == 13)
				{
					if ((lo_freq != 7) &&  /* {13, 7} 2400MHz / 1200MHz 2:1 */
						(lo_freq != 4) &&  /* {13, 4} 2400MHz /  600MHz 4:1 */
						(lo_freq != 2))   /* {13, 2} 2400MHz /  400MHz 6:1 */
						downgrade = TRUE;
				}
				else if (hi_freq == 11)
				{
					if ((lo_freq != 6))    /* {11, 6} 2000MHz / 1000MHz 2:1 */
						downgrade = TRUE;
				}
				else if (hi_freq == 9)
				{
					if ((lo_freq != 5) &&  /* { 9, 5} 1600MHz /  800MHz 2:1 */
						(lo_freq != 2) &&  /* { 9, 2} 1600MHz /  400MHz 4:1 */
						(lo_freq != 0))   /* { 9, 0} 1600MHz /  200MHz 8:1 */
						downgrade = TRUE;
				}
				else if (hi_freq == 7)
				{
					if ((lo_freq != 4) &&  /* { 7, 4} 1200MHz /  600MHz 2:1 */
						(lo_freq != 0))   /* { 7, 0} 1200MHz /  200MHz 6:1 */
						downgrade = TRUE;
				}
				else if (hi_freq == 5)
				{
					if ((lo_freq != 2) &&  /* { 5, 2}  800MHz /  400MHz 2:1 */
						(lo_freq != 0))   /* { 5, 0}  800MHz /  200MHz 4:1 */
						downgrade = TRUE;
				}
				else if (hi_freq == 2)
				{
					if ((lo_freq != 0))    /* { 2, 0}  400MHz /  200MHz 2:1 */
						downgrade = TRUE;
				}
				else
				{
					downgrade = TRUE; /*  no legal ratios for hi_freq */
				}

				/*  Step 3. Downgrade the higher of the two frequencies, and set nochanges to FALSE */
				if (downgrade)
				{
					/*  Although the problem was with the port specified by hi_index, we need to */
					/*  downgrade both ends of the link. */
					hi_index = hi_index & 0xFE; /*  Select the 'upstream' (i.e. even) port */

					temp = p_dat->port_list[hi_index].composite_frequency_cap;

					/*  Remove hi_freq from the list of valid frequencies */
					temp = temp & ~((uint32)1 << hi_freq);
					ASSERT (temp != 0);
					p_dat->port_list[hi_index].composite_frequency_cap = temp;
					p_dat->port_list[hi_index + 1].composite_frequency_cap = temp;

					for (k = 19;; k--)
					{
						if ((j == 16) || (j == 15))
							continue;
						if (temp & ((u32)1 << k))
							break;
					}

					p_dat->port_list[hi_index].sel_frequency = k;
					p_dat->port_list[hi_index + 1].sel_frequency = k;

					changes = TRUE;
				}
			}
		}
	} while (changes); /*  Repeat until a valid configuration is reached */
#endif /* HT_BUILD_NC_ONLY */
}

/*----------------------------------------------------------------------------------------
 * void
 * link_optimization(sMainData *p_dat)
 *
 *  Description:
 *	 Based on link capabilities, apply optimization rules to come up with the real best
 *	 settings, including several external limit decision from call backs. This includes
 *	 handling of sublinks.	Finally, after the port list data is updated, set the hardware
 *	 state for all links.
 *
 *  Parameters:
 *	@param[in]  sMainData* p_dat = our global state
 * ---------------------------------------------------------------------------------------
 */
static void link_optimization(sMainData *p_dat)
{
	p_dat->nb->gather_link_data(p_dat, p_dat->nb);
	regang_links(p_dat);
	if (is_fam_15h())
		detect_io_link_isochronous_capable(p_dat);
	select_optimal_width_and_frequency(p_dat);
	hammer_sublink_fixup(p_dat);
	p_dat->nb->set_link_data(p_dat, p_dat->nb);
}


/*----------------------------------------------------------------------------------------
 * void
 * traffic_distribution(sMainData *p_dat)
 *
 *  Description:
 *	 In the case of a two node system with both sublinks used, enable the traffic
 *	 distribution feature.
 *
 *  Parameters:
 *	  @param[in]	    sMainData*	  p_dat		 = our global state, port list data
 * ---------------------------------------------------------------------------------------
 */
static void traffic_distribution(sMainData *p_dat)
{
#ifndef HT_BUILD_NC_ONLY
	u32 links_01, links_10;
	u8 link_count;
	u8 i;

	/*  Traffic Distribution is only used when there are exactly two nodes in the system */
	if (p_dat->nodes_discovered + 1 != 2)
		return;

	links_01 = 0;
	links_10 = 0;
	link_count = 0;
	for (i = 0; i < p_dat->total_links*2; i += 2)
	{
		if ((p_dat->port_list[i].Type == PORTLIST_TYPE_CPU) && (p_dat->port_list[i + 1].Type == PORTLIST_TYPE_CPU))
		{
			links_01 |= (u32)1 << p_dat->port_list[i].Link;
			links_10 |= (u32)1 << p_dat->port_list[i + 1].Link;
			link_count++;
		}
	}
	ASSERT(link_count != 0);
	if (link_count == 1)
		return; /*  Don't setup Traffic Distribution if only one link is being used */

	p_dat->nb->writeTrafficDistribution(links_01, links_10, p_dat->nb);
#endif /* HT_BUILD_NC_ONLY */
}

/*----------------------------------------------------------------------------------------
 * void
 * tuning(sMainData *p_dat)
 *
 *  Description:
 *	 Handle system and performance tunings, such as traffic distribution, fifo and
 *	 buffer tuning, and special config tunings.
 *
 *  Parameters:
 *	@param[in] sMainData* p_dat = our global state, port list data
 * ---------------------------------------------------------------------------------------
 */
static void tuning(sMainData *p_dat)
{
	u8 i;

	/* See if traffic distribution can be done and do it if so
	 * or allow system specific customization
	 */
	if ((p_dat->ht_block->amd_cb_customize_traffic_distribution == NULL)
		|| !p_dat->ht_block->amd_cb_customize_traffic_distribution())
	{
		traffic_distribution(p_dat);
	}

	/* For each node, invoke northbridge specific buffer tunings or
	 * system specific customizations.
	 */
	for (i = 0; i < p_dat->nodes_discovered + 1; i++)
	{
		if ((p_dat->ht_block->amd_cb_customize_buffers == NULL)
		   || !p_dat->ht_block->amd_cb_customize_buffers(i))
		{
			p_dat->nb->bufferOptimizations(i, p_dat, p_dat->nb);
		}
	}
}

/*----------------------------------------------------------------------------------------
 * BOOL
 * is_sanity_check_ok()
 *
 *  Description:
 *	 Perform any general sanity checks which should prevent HT from running if they fail.
 *	 Currently only the "Must run on BSP only" check.
 *
 *  Parameters:
 *	@param[out] result BOOL  = true if check is ok, false if it failed
 * ---------------------------------------------------------------------------------------
 */
static BOOL is_sanity_check_ok(void)
{
	uint64 q_value;

	AmdMSRRead(LAPIC_BASE_MSR, &q_value);

	return ((q_value.lo & LAPIC_BASE_MSR_BOOTSTRAP_PROCESSOR) != 0);
}

/***************************************************************************
 ***				 HT Initialize				   ***
 ***************************************************************************/

/*----------------------------------------------------------------------------------------
 * void
 * htInitialize(AMD_HTBLOCK *p_block)
 *
 *  Description:
 *	 This is the top level external interface for Hypertransport Initialization.
 *	 Create our initial internal state, initialize the coherent fabric,
 *	 initialize the non-coherent chains, and perform any required fabric tuning or
 *	 optimization.
 *
 *  Parameters:
 *	@param[in] AMD_HTBLOCK*  p_block = Our Initial State including possible
 *				  topologies and routings, non coherent bus
 *				  assignment info, and actual
 *				  wrapper or OEM call back routines.
 * ---------------------------------------------------------------------------------------
 */
void amd_ht_initialize(AMD_HTBLOCK *p_block)
{
	sMainData p_dat;
	cNorthBridge nb;

	if (is_sanity_check_ok())
	{
		new_northbridge(0, &nb);

		p_dat.ht_block = p_block;
		p_dat.nb = &nb;
		p_dat.sys_mp_cap = nb.maxNodes;
		nb.isCapable(0, &p_dat, p_dat.nb);
		coherent_init(&p_dat);

		p_dat.auto_bus_current = p_block->auto_bus_start;
		p_dat.used_cfg_map_entires = 0;
		nc_init(&p_dat);
		link_optimization(&p_dat);
		tuning(&p_dat);
	}
}
