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
 * graphHowManyNodes(u8 *graph)
 *
 *  Description:
 *	 Returns the number of nodes in the compressed graph
 *
 *  Parameters:
 *	@param[in] graph = a compressed graph
 *	@param[out] results = the number of nodes in the graph
 * ---------------------------------------------------------------------------------------
 */
static u8 graphHowManyNodes(u8 *graph)
{
	return graph[0];
}

/*----------------------------------------------------------------------------------------
 * BOOL
 * graphIsAdjacent(u8 *graph, u8 nodeA, u8 nodeB)
 *
 *  Description:
 * Returns true if NodeA is directly connected to NodeB, false otherwise
 * (if NodeA == NodeB also returns false)
 * Relies on rule that directly connected nodes always route requests directly.
 *
 *  Parameters:
 *	@param[in]   graph   = the graph to examine
 *	@param[in]   nodeA   = the node number of the first node
 *	@param[in]   nodeB   = the node number of the second node
 *	@param[out]    results  = true if nodeA connects to nodeB false if not
 * ---------------------------------------------------------------------------------------
 */
static BOOL graphIsAdjacent(u8 *graph, u8 nodeA, u8 nodeB)
{
	u8 size = graph[0];
	ASSERT(size <= MAX_NODES);
	ASSERT((nodeA < size) && (nodeB < size));
	return (graph[1 + (nodeA * size + nodeB) * 2 + 1] & 0x0F) == nodeB;
}

/*----------------------------------------------------------------------------------------
 * u8
 * graphGetRsp(u8 *graph, u8 nodeA, u8 nodeB)
 *
 *  Description:
 *	Returns the graph node used by nodeA to route responses targeted at nodeB.
 *	This will be a node directly connected to nodeA (possibly nodeB itself),
 *	or "Route to Self" if nodeA and nodeB are the same node.
 *	Note that all node numbers are abstract node numbers of the topology graph,
 *	it is the responsibility of the caller to apply any permutation needed.
 *
 *  Parameters:
 *	@param[in]    u8    graph   = the graph to examine
 *	@param[in]    u8    nodeA   = the node number of the first node
 *	@param[in]    u8    nodeB   = the node number of the second node
 *	@param[out]   u8    results = The response route node
 * ---------------------------------------------------------------------------------------
 */
static u8 graphGetRsp(u8 *graph, u8 nodeA, u8 nodeB)
{
	u8 size = graph[0];
	ASSERT(size <= MAX_NODES);
	ASSERT((nodeA < size) && (nodeB < size));
	return (graph[1 + (nodeA * size + nodeB) * 2 + 1] & 0xF0) >> 4;
}

/*----------------------------------------------------------------------------------------
 * u8
 * graphGetReq(u8 *graph, u8 nodeA, u8 nodeB)
 *
 *  Description:
 *	 Returns the graph node used by nodeA to route requests targeted at nodeB.
 *	This will be a node directly connected to nodeA (possibly nodeB itself),
 *	or "Route to Self" if nodeA and nodeB are the same node.
 *	Note that all node numbers are abstract node numbers of the topology graph,
 *	it is the responsibility of the caller to apply any permutation needed.
 *
 *  Parameters:
 *	@param[in]   graph   = the graph to examine
 *	@param[in]   nodeA   = the node number of the first node
 *	@param[in]   nodeB   = the node number of the second node
 *	@param[out]  results = The request route node
 * ---------------------------------------------------------------------------------------
 */
static u8 graphGetReq(u8 *graph, u8 nodeA, u8 nodeB)
{
	u8 size = graph[0];
	ASSERT(size <= MAX_NODES);
	ASSERT((nodeA < size) && (nodeB < size));
	return (graph[1 + (nodeA * size + nodeB) * 2 + 1] & 0x0F);
}

/*----------------------------------------------------------------------------------------
 * u8
 * graphGetBc(u8 *graph, u8 nodeA, u8 nodeB)
 *
 *  Description:
 *	 Returns a bit vector of nodes that nodeA should forward a broadcast from
 *	 nodeB towards
 *
 *  Parameters:
 *	@param[in]    graph   = the graph to examine
 *	@param[in]    nodeA   = the node number of the first node
 *	@param[in]    nodeB   = the node number of the second node
 *	OU    results = the broadcast routes for nodeA from nodeB
 * ---------------------------------------------------------------------------------------
 */
static u8 graphGetBc(u8 *graph, u8 nodeA, u8 nodeB)
{
	u8 size = graph[0];
	ASSERT(size <= MAX_NODES);
	ASSERT((nodeA < size) && (nodeB < size));
	return graph[1 + (nodeA * size + nodeB) * 2];
}


/***************************************************************************
 ***		GENERIC HYPERTRANSPORT DISCOVERY CODE			***
 ***************************************************************************/

/*----------------------------------------------------------------------------------------
 * void
 * routeFromBSP(u8 target_node, u8 actualTarget, sMainData *pDat)
 *
 *  Description:
 *	 Ensure a request / response route from target node to bsp.  Since target node is
 *	 always a predecessor of actual target node, each node gets a route to actual target
 *	 on the link that goes to target.  The routing produced by this routine is adequate
 *	 for config access during discovery, but NOT for coherency.
 *
 *  Parameters:
 *	@param[in]    u8    target_node   = the path to actual target goes through target
 *	@param[in]    u8    actualTarget = the ultimate target being routed to
 *	@param[in]    sMainData*  pDat   = our global state, port config info
 * ---------------------------------------------------------------------------------------
 */
static void routeFromBSP(u8 target_node, u8 actualTarget, sMainData *pDat)
{
	u8 predecessorNode, predecessorLink, currentPair;

	if (target_node == 0)
		return;  /*  BSP has no predecessor, stop */

	/*  Search for the link that connects target_node to its predecessor */
	currentPair = 0;
	while (pDat->port_list[currentPair * 2 + 1].node_id != target_node)
	{
		currentPair++;
		ASSERT(currentPair < pDat->total_links);
	}

	predecessorNode = pDat->port_list[currentPair * 2].node_id;
	predecessorLink = pDat->port_list[currentPair * 2].Link;

	/*  Recursively call self to ensure the route from the BSP to the Predecessor */
	/*  Node is established */
	routeFromBSP(predecessorNode, actualTarget, pDat);

	pDat->nb->writeRoutingTable(predecessorNode, actualTarget, predecessorLink, pDat->nb);
}

/*---------------------------------------------------------------------------*/

/**
 *  u8
 * convertNodeToLink(u8 srcNode, u8 target_node, sMainData *pDat)
 *
 *  Description:
 *	 Return the link on source node which connects to target node
 *
 *  Parameters:
 *	@param[in]    srcNode    = the source node
 *	@param[in]    target_node = the target node to find the link to
 *	@param[in]    pDat = our global state
 *	@return       the link on source which connects to target
 *
 */
static u8 convertNodeToLink(u8 srcNode, u8 target_node, sMainData *pDat)
{
	u8 targetlink = INVALID_LINK;
	u8 k;

	for (k = 0; k < pDat->total_links * 2; k += 2)
	{
		if ((pDat->port_list[k + 0].node_id == srcNode) && (pDat->port_list[k + 1].node_id == target_node))
		{
			targetlink = pDat->port_list[k + 0].Link;
			break;
		}
		else if ((pDat->port_list[k + 1].node_id == srcNode) && (pDat->port_list[k + 0].node_id == target_node))
		{
			targetlink = pDat->port_list[k + 1].Link;
			break;
		}
	}
	ASSERT(targetlink != INVALID_LINK);

	return targetlink;
}


/*----------------------------------------------------------------------------------------
 * void
 * htDiscoveryFloodFill(sMainData *pDat)
 *
 *  Description:
 *	 Discover all coherent devices in the system, initializing some basics like node IDs
 *	 and total nodes found in the process. As we go we also build a representation of the
 *	 discovered system which we will use later to program the routing tables.  During this
 *	 step, the routing is via default link back to BSP and to each new node on the link it
 *	 was discovered on (no coherency is active yet).
 *
 *  Parameters:
 *	@param[in]    sMainData*  pDat = our global state
 * ---------------------------------------------------------------------------------------
 */
static void htDiscoveryFloodFill(sMainData *pDat)
{
	u8 currentNode = 0;
	u8 currentLink;
	u8 currentLinkID;

	/* NOTE
	 * Each node inside a dual node (socket G34) processor must share
	 * an adjacent node ID.  Alter the link scan order such that the
	 * other internal node is always scanned first...
	 */
	u8 currentLinkScanOrder_Default[8] = {0, 1, 2, 3, 4, 5, 6, 7};
	u8 currentLinkScanOrder_G34_Fam10[8] = {1, 0, 2, 3, 4, 5, 6, 7};
	u8 currentLinkScanOrder_G34_Fam15[8] = {2, 0, 1, 3, 4, 5, 6, 7};

	u8 fam15h = 0;
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
		fam15h = 1;
	}

	if ((model >= 0x8) || fam15h)
		/* Revision D or later */
		rev_gte_d = 1;

	if (rev_gte_d)
		 /* Check for dual node capability */
		if (f3xe8 & 0x20000000)
			dual_node = 1;

	/* Entries are always added in pairs, the even indices are the 'source'
	 * side closest to the BSP, the odd indices are the 'destination' side
	 */
	while (currentNode <= pDat->nodes_discovered)
	{
		u32 temp;

		if (currentNode != 0)
		{
			/* Set path from BSP to currentNode */
			routeFromBSP(currentNode, currentNode, pDat);

			/* Set path from BSP to currentNode for currentNode+1 if
			 * currentNode+1 != MAX_NODES
			 */
			if (currentNode + 1 != MAX_NODES)
				routeFromBSP(currentNode, currentNode + 1, pDat);

			/* Configure currentNode to route traffic to the BSP through its
			 * default link
			 */
			pDat->nb->writeRoutingTable(currentNode, 0, pDat->nb->readDefLnk(currentNode, pDat->nb), pDat->nb);
		}

		/* Set currentNode's node_id field to currentNode */
		pDat->nb->writeNodeID(currentNode, currentNode, pDat->nb);

		/* Enable routing tables on currentNode */
		pDat->nb->enableRoutingTables(currentNode, pDat->nb);

		for (currentLinkID = 0; currentLinkID < pDat->nb->max_links; currentLinkID++)
		{
			BOOL linkfound;
			u8 token;

			if (currentLinkID < 8) {
				if (dual_node) {
					if (fam15h)
						currentLink = currentLinkScanOrder_G34_Fam15[currentLinkID];
					else
						currentLink = currentLinkScanOrder_G34_Fam10[currentLinkID];
				} else {
					currentLink = currentLinkScanOrder_Default[currentLinkID];
				}
			} else {
				currentLink = currentLinkID;
			}

			if (pDat->ht_block->amd_cb_ignore_link && pDat->ht_block->amd_cb_ignore_link(currentNode, currentLink))
				continue;

			if (pDat->nb->readTrueLinkFailStatus(currentNode, currentLink, pDat, pDat->nb))
				continue;

			/* Make sure that the link is connected, coherent, and ready */
			if (!pDat->nb->verifyLinkIsCoherent(currentNode, currentLink, pDat->nb))
				continue;


			/* Test to see if the currentLink has already been explored */
			linkfound = FALSE;
			for (temp = 0; temp < pDat->total_links; temp++)
			{
				if ((pDat->port_list[temp * 2 + 1].node_id == currentNode) &&
				   (pDat->port_list[temp * 2 + 1].Link == currentLink))
				{
					linkfound = TRUE;
					break;
				}
			}
			if (linkfound)
			{
				/* We had already expored this link */
				continue;
			}

			if (pDat->nb->handleSpecialLinkCase(currentNode, currentLink, pDat, pDat->nb))
			{
				continue;
			}

			/* Modify currentNode's routing table to use currentLink to send
			 * traffic to currentNode+1
			 */
			pDat->nb->writeRoutingTable(currentNode, currentNode + 1, currentLink, pDat->nb);

			/* Check the northbridge of the node we just found, to make sure it is compatible
			 * before doing anything else to it.
			 */
			if (!pDat->nb->isCompatible(currentNode + 1, pDat->nb))
			{
				u8 nodeToKill;

				/* Notify BIOS of event (while variables are still the same) */
				if (pDat->ht_block->amd_cb_event_notify)
				{
					sHtEventCohFamilyFeud evt;
					evt.e_size = sizeof(sHtEventCohFamilyFeud);
					evt.node = currentNode;
					evt.link = currentLink;
					evt.total_nodes = pDat->nodes_discovered;

					pDat->ht_block->amd_cb_event_notify(HT_EVENT_CLASS_ERROR,
									HT_EVENT_COH_FAMILY_FEUD,
									(u8 *)&evt);
				}

				/* If node is not compatible, force boot to 1P
				 * If they are not compatible stop cHT init and:
				 *	1. Disable all cHT links on the BSP
				 *	2. Configure the BSP routing tables as a UP.
				 *	3. Notify main BIOS.
				 */
				pDat->nodes_discovered = 0;
				currentNode = 0;
				pDat->total_links = 0;
				/* Abandon our coherent link data structure.  At this point there may
				 * be coherent links on the BSP that are not yet in the portList, and
				 * we have to turn them off anyway.  So depend on the hardware to tell us.
				 */
				for (currentLink = 0; currentLink < pDat->nb->max_links; currentLink++)
				{
					/* Stop all links which are connected, coherent, and ready */
					if (pDat->nb->verifyLinkIsCoherent(currentNode, currentLink, pDat->nb))
						pDat->nb->stopLink(currentNode, currentLink, pDat->nb);
				}

				for (nodeToKill = 0; nodeToKill < pDat->nb->maxNodes; nodeToKill++)
				{
					pDat->nb->writeFullRoutingTable(0, nodeToKill, ROUTETOSELF, ROUTETOSELF, 0, pDat->nb);
				}

				/* End Coherent Discovery */
				STOP_HERE;
				break;
			}

			/* Read token from Current+1 */
			token = pDat->nb->readToken(currentNode + 1, pDat->nb);
			ASSERT(token <= pDat->nodes_discovered);
			if (token == 0)
			{
				pDat->nodes_discovered++;
				ASSERT(pDat->nodes_discovered < pDat->nb->maxNodes);
				/* Check the capability of northbridges against the currently known configuration */
				if (!pDat->nb->isCapable(currentNode + 1, pDat, pDat->nb))
				{
					u8 nodeToKill;

					/* Notify BIOS of event  */
					if (pDat->ht_block->amd_cb_event_notify)
					{
						sHtEventCohMpCapMismatch evt;
						evt.e_size = sizeof(sHtEventCohMpCapMismatch);
						evt.node = currentNode;
						evt.link = currentLink;
						evt.sys_mp_cap = pDat->sys_mp_cap;
						evt.total_nodes = pDat->nodes_discovered;

						pDat->ht_block->amd_cb_event_notify(HT_EVENT_CLASS_ERROR,
									HT_EVENT_COH_MPCAP_MISMATCH,
									(u8 *)&evt);
					}

					pDat->nodes_discovered = 0;
					currentNode = 0;
					pDat->total_links = 0;

					for (nodeToKill = 0; nodeToKill < pDat->nb->maxNodes; nodeToKill++)
					{
						pDat->nb->writeFullRoutingTable(0, nodeToKill, ROUTETOSELF, ROUTETOSELF, 0, pDat->nb);
					}

					/* End Coherent Discovery */
					STOP_HERE;
					break;
				}

				token = pDat->nodes_discovered;
				pDat->nb->writeToken(currentNode + 1, token, pDat->nb);
				/* Inform that we have discovered a node, so that logical id to
				 * socket mapping info can be recorded.
				 */
				if (pDat->ht_block->amd_cb_event_notify)
				{
					sHtEventCohNodeDiscovered evt;
					evt.e_size = sizeof(sHtEventCohNodeDiscovered);
					evt.node = currentNode;
					evt.link = currentLink;
					evt.newNode = token;

					pDat->ht_block->amd_cb_event_notify(HT_EVENT_CLASS_INFO,
								HT_EVENT_COH_NODE_DISCOVERED,
								(u8 *)&evt);
				}
			}

			if (pDat->total_links == MAX_PLATFORM_LINKS)
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
				if (pDat->ht_block->amd_cb_event_notify)
				{
					sHtEventCohLinkExceed evt;
					evt.e_size = sizeof(sHtEventCohLinkExceed);
					evt.node = currentNode;
					evt.link = currentLink;
					evt.target_node = token;
					evt.total_nodes = pDat->nodes_discovered;
					evt.max_links = pDat->nb->max_links;

					pDat->ht_block->amd_cb_event_notify(HT_EVENT_CLASS_ERROR,
									HT_EVENT_COH_LINK_EXCEED,
									(u8 *)&evt);
				}
				/* Force link and node loops to halt */
				STOP_HERE;
				currentNode = pDat->nodes_discovered;
				break;
			}

			pDat->port_list[pDat->total_links * 2].Type = PORTLIST_TYPE_CPU;
			pDat->port_list[pDat->total_links * 2].Link = currentLink;
			pDat->port_list[pDat->total_links * 2].node_id = currentNode;

			pDat->port_list[pDat->total_links * 2 + 1].Type = PORTLIST_TYPE_CPU;
			pDat->port_list[pDat->total_links * 2 + 1].Link = pDat->nb->readDefLnk(currentNode + 1, pDat->nb);
			pDat->port_list[pDat->total_links * 2 + 1].node_id = token;

			pDat->total_links++;

			if (!pDat->sys_matrix[currentNode][token])
			{
				pDat->sys_degree[currentNode]++;
				pDat->sys_degree[token]++;
				pDat->sys_matrix[currentNode][token] = TRUE;
				pDat->sys_matrix[token][currentNode] = TRUE;
			}
		}
		currentNode++;
	}
}


/***************************************************************************
 ***		ISOMORPHISM BASED ROUTING TABLE GENERATION CODE		***
 ***************************************************************************/

/*----------------------------------------------------------------------------------------
 * BOOL
 * isoMorph(u8 i, sMainData *pDat)
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
 *	@param[in]/@param[out] sMainData* pDat  = our global state, degree and adjacency matrix,
 *				  output a permutation if successful
 *	@param[out] BOOL results = the graphs are (or are not) isomorphic
 * ---------------------------------------------------------------------------------------
 */
static BOOL isoMorph(u8 i, sMainData *pDat)
{
	u8 j, k;
	u8 nodecnt;

	/* We have only been called if nodecnt == pSelected->size ! */
	nodecnt = pDat->nodes_discovered + 1;

	if (i != nodecnt)
	{
		/*  Keep building the permutation */
		for (j = 0; j < nodecnt; j++)
		{
			/*  Make sure the degree matches */
			if (pDat->sys_degree[i] != pDat->db_degree[j])
				continue;

			/*  Make sure that j hasn't been used yet (ought to use a "used" */
			/*  array instead, might be faster) */
			for (k = 0; k < i; k++)
			{
				if (pDat->perm[k] == j)
					break;
			}
			if (k != i)
				continue;
			pDat->perm[i] = j;
			if (isoMorph(i + 1, pDat))
				return TRUE;
		}
		return FALSE;
	} else {
		/*  Test to see if the permutation is isomorphic */
		for (j = 0; j < nodecnt; j++)
		{
			for (k = 0; k < nodecnt; k++)
			{
				if (pDat->sys_matrix[j][k] !=
				   pDat->db_matrix[pDat->perm[j]][pDat->perm[k]])
					return FALSE;
			}
		}
		return TRUE;
	}
}


/*----------------------------------------------------------------------------------------
 * void
 * lookupComputeAndLoadRoutingTables(sMainData *pDat)
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
 *	@param[in]    sMainData* pDat = our global state, the discovered fabric,
 *	@param[out]			degree matrix, permutation
 * ---------------------------------------------------------------------------------------
 */
static void lookupComputeAndLoadRoutingTables(sMainData *pDat)
{
	u8 **pTopologyList;
	u8 *pSelected;

	int i, j, k, size;

	size = pDat->nodes_discovered + 1;
	/* Use the provided topology list or the internal, default one. */
	pTopologyList = pDat->ht_block->topolist;
	if (pTopologyList == NULL)
	{
		getAmdTopolist(&pTopologyList);
	}

	pSelected = *pTopologyList;
	while (pSelected != NULL)
	{
		if (graphHowManyNodes(pSelected) == size)
		{
			/*  Build Degree vector and Adjency Matrix for this entry */
			for (i = 0; i < size; i++)
			{
				pDat->db_degree[i] = 0;
				for (j = 0; j < size; j++)
				{
					if (graphIsAdjacent(pSelected, i, j))
					{
						pDat->db_matrix[i][j] = 1;
						pDat->db_degree[i]++;
					}
					else
					{
						pDat->db_matrix[i][j] = 0;
					}
				}
			}
			if (isoMorph(0, pDat))
				break;  /*  A matching topology was found */
		}

		pTopologyList++;
		pSelected = *pTopologyList;
	}

	if (pSelected != NULL)
	{
		/*  Compute the reverse Permutation */
		for (i = 0; i < size; i++)
		{
			pDat->reverse_perm[pDat->perm[i]] = i;
		}

		/*  Start with the last discovered node, and move towards the BSP */
		for (i = size-1; i >= 0; i--)
		{
			for (j = 0; j < size; j++)
			{
				u8 ReqTargetLink, RspTargetLink;
				u8 ReqTargetNode, RspTargetNode;

				u8 AbstractBcTargetNodes = graphGetBc(pSelected, pDat->perm[i], pDat->perm[j]);
				u32 BcTargetLinks = 0;

				for (k = 0; k < MAX_NODES; k++)
				{
					if (AbstractBcTargetNodes & ((u32)1 << k))
					{
						BcTargetLinks |= (u32)1 << convertNodeToLink(i, pDat->reverse_perm[k], pDat);
					}
				}

				if (i == j)
				{
					ReqTargetLink = ROUTETOSELF;
					RspTargetLink = ROUTETOSELF;
				}
				else
				{
					ReqTargetNode = graphGetReq(pSelected, pDat->perm[i], pDat->perm[j]);
					ReqTargetLink = convertNodeToLink(i, pDat->reverse_perm[ReqTargetNode], pDat);

					RspTargetNode = graphGetRsp(pSelected, pDat->perm[i], pDat->perm[j]);
					RspTargetLink = convertNodeToLink(i, pDat->reverse_perm[RspTargetNode], pDat);
				}

				pDat->nb->writeFullRoutingTable(i, j, ReqTargetLink, RspTargetLink, BcTargetLinks, pDat->nb);
			}
			/* Clean up discovery 'footprint' that otherwise remains in the routing table.  It didn't hurt
			 * anything, but might cause confusion during debug and validation.  Do this by setting the
			 * route back to all self routes. Since it's the node that would be one more than actually installed,
			 * this only applies if less than maxNodes were found.
			 */
			if (size < pDat->nb->maxNodes)
			{
				pDat->nb->writeFullRoutingTable(i, size, ROUTETOSELF, ROUTETOSELF, 0, pDat->nb);
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
		if (pDat->ht_block->amd_cb_event_notify)
		{
			sHtEventCohNoTopology evt;
			evt.e_size = sizeof(sHtEventCohNoTopology);
			evt.total_nodes = pDat->nodes_discovered;

			pDat->ht_block->amd_cb_event_notify(HT_EVENT_CLASS_ERROR,
						HT_EVENT_COH_NO_TOPOLOGY,
						(u8 *)&evt);
		}
		STOP_HERE;
		/* Force 1P */
		pDat->nodes_discovered = 0;
		pDat->total_links = 0;
		pDat->nb->enableRoutingTables(0, pDat->nb);
	}
}
#endif /* HT_BUILD_NC_ONLY */


/*----------------------------------------------------------------------------------------
 * void
 * finializeCoherentInit(sMainData *pDat)
 *
 *  Description:
 *	 Find the total number of cores and update the number of nodes and cores in all cpus.
 *	 Limit CPU config access to installed cpus.
 *
 *  Parameters:
 *	@param[in] sMainData* pDat = our global state, number of nodes discovered.
 * ---------------------------------------------------------------------------------------
 */
static void finializeCoherentInit(sMainData *pDat)
{
	u8 curNode;

	u8 totalCores = 0;
	for (curNode = 0; curNode < pDat->nodes_discovered + 1; curNode++)
	{
		totalCores += pDat->nb->getNumCoresOnNode(curNode, pDat->nb);
	}

	for (curNode = 0; curNode < pDat->nodes_discovered + 1; curNode++)
	{
		pDat->nb->setTotalNodesAndCores(curNode, pDat->nodes_discovered + 1, totalCores, pDat->nb);
	}

	for (curNode = 0; curNode < pDat->nodes_discovered + 1; curNode++)
	{
		pDat->nb->limitNodes(curNode, pDat->nb);
	}

}

/*----------------------------------------------------------------------------------------
 * void
 * coherentInit(sMainData *pDat)
 *
 *  Description:
 *	 Perform discovery and initialization of the coherent fabric.
 *
 *  Parameters:
 *	@param[in] sMainData* pDat = our global state
 * ---------------------------------------------------------------------------------------
 */
static void coherentInit(sMainData *pDat)
{
#ifdef HT_BUILD_NC_ONLY
	/* Replace discovery process with:
	 * No other nodes, no coherent links
	 * Enable routing tables on currentNode, for power on self route
	 */
	pDat->nodes_discovered = 0;
	pDat->total_links = 0;
	pDat->nb->enableRoutingTables(0, pDat->nb);
#else
	u8 i, j;

	pDat->nodes_discovered = 0;
	pDat->total_links = 0;
	for (i = 0; i < MAX_NODES; i++)
	{
		pDat->sys_degree[i] = 0;
		for (j = 0; j < MAX_NODES; j++)
		{
			pDat->sys_matrix[i][j] = 0;
		}
	}

	htDiscoveryFloodFill(pDat);
	lookupComputeAndLoadRoutingTables(pDat);
#endif
	finializeCoherentInit(pDat);
}

/***************************************************************************
 ***			    Non-coherent init code			  ***
 ***				  Algorithms				  ***
 ***************************************************************************/
/*----------------------------------------------------------------------------------------
 * void
 * processLink(u8 node, u8 link, sMainData *pDat)
 *
 *  Description:
 *	 Process a non-coherent link, enabling a range of bus numbers, and setting the device
 *	 ID for all devices found
 *
 *  Parameters:
 *	@param[in] u8 node = Node on which to process nc init
 *	@param[in] u8 link = The non-coherent link on that node
 *	@param[in] sMainData* pDat = our global state
 * ---------------------------------------------------------------------------------------
 */
static void processLink(u8 node, u8 link, sMainData *pDat)
{
	u8 secBus, subBus;
	u32 current_buid;
	u32 temp;
	u32 unitIDcnt;
	SBDFO currentPtr;
	u8 depth;
	const u8 *pSwapPtr;

	SBDFO lastSBDFO = ILLEGAL_SBDFO;
	u8 lastLink = 0;

	ASSERT(node < pDat->nb->maxNodes && link < pDat->nb->max_links);

	if ((pDat->ht_block->amd_cb_override_bus_numbers == NULL)
	   || !pDat->ht_block->amd_cb_override_bus_numbers(node, link, &secBus, &subBus))
	{
		/* Assign Bus numbers */
		if (pDat->auto_bus_current >= pDat->ht_block->auto_bus_max)
		{
			/* If we run out of Bus Numbers notify, if call back unimplemented or if it
			 * returns, skip this chain
			 */
			if (pDat->ht_block->amd_cb_event_notify)
			{
				sHTEventNcohBusMaxExceed evt;
				evt.e_size = sizeof(sHTEventNcohBusMaxExceed);
				evt.node = node;
				evt.link = link;
				evt.bus = pDat->auto_bus_current;

				pDat->ht_block->amd_cb_event_notify(HT_EVENT_CLASS_ERROR,HT_EVENT_NCOH_BUS_MAX_EXCEED,(u8 *)&evt);
			}
			STOP_HERE;
			return;
		}

		if (pDat->used_cfg_map_entires >= 4)
		{
			/* If we have used all the PCI Config maps we can't add another chain.
			 * Notify and if call back is unimplemented or returns, skip this chain.
			 */
			if (pDat->ht_block->amd_cb_event_notify)
			{
				sHtEventNcohCfgMapExceed evt;
				evt.e_size = sizeof(sHtEventNcohCfgMapExceed);
				evt.node = node;
				evt.link = link;

				pDat->ht_block->amd_cb_event_notify(HT_EVENT_CLASS_ERROR,
							HT_EVENT_NCOH_CFG_MAP_EXCEED,
							(u8 *)&evt);
			}
			STOP_HERE;
			return;
		}

		secBus = pDat->auto_bus_current;
		subBus = secBus + pDat->ht_block->auto_bus_increment-1;
		pDat->auto_bus_current += pDat->ht_block->auto_bus_increment;
	}

	pDat->nb->setCFGAddrMap(pDat->used_cfg_map_entires, secBus, subBus, node, link, pDat, pDat->nb);
	pDat->used_cfg_map_entires++;

	if ((pDat->ht_block->amd_cb_manual_buid_swap_list != NULL)
	 && pDat->ht_block->amd_cb_manual_buid_swap_list(node, link, &pSwapPtr))
	{
		/* Manual non-coherent BUID assignment */
		current_buid = 1;

		/* Assign BUID's per manual override */
		while (*pSwapPtr != 0xFF)
		{
			currentPtr = MAKE_SBDFO(0, secBus, *pSwapPtr, 0, 0);
			pSwapPtr++;

			do
			{
				amd_pci_find_next_cap(&currentPtr);
				ASSERT(currentPtr != ILLEGAL_SBDFO);
				AmdPCIRead(currentPtr, &temp);
			} while (!IS_HT_SLAVE_CAPABILITY(temp));

			current_buid = *pSwapPtr;
			pSwapPtr++;
			amd_pci_write_bits(currentPtr, 20, 16, &current_buid);
		}

		/* Build chain of devices */
		depth = 0;
		pSwapPtr++;
		while (*pSwapPtr != 0xFF)
		{
			pDat->port_list[pDat->total_links * 2].node_id = node;
			if (depth == 0)
			{
				pDat->port_list[pDat->total_links * 2].Type = PORTLIST_TYPE_CPU;
				pDat->port_list[pDat->total_links * 2].Link = link;
			}
			else
			{
				pDat->port_list[pDat->total_links * 2].Type = PORTLIST_TYPE_IO;
				pDat->port_list[pDat->total_links * 2].Link = 1 - lastLink;
				pDat->port_list[pDat->total_links * 2].host_link = link;
				pDat->port_list[pDat->total_links * 2].host_depth = depth - 1;
				pDat->port_list[pDat->total_links * 2].pointer = lastSBDFO;
			}

			pDat->port_list[pDat->total_links * 2 + 1].Type = PORTLIST_TYPE_IO;
			pDat->port_list[pDat->total_links * 2 + 1].node_id = node;
			pDat->port_list[pDat->total_links * 2 + 1].host_link = link;
			pDat->port_list[pDat->total_links * 2 + 1].host_depth = depth;

			currentPtr = MAKE_SBDFO(0, secBus, (*pSwapPtr & 0x3F), 0, 0);
			do
			{
				amd_pci_find_next_cap(&currentPtr);
				ASSERT(currentPtr != ILLEGAL_SBDFO);
				AmdPCIRead(currentPtr, &temp);
			} while (!IS_HT_SLAVE_CAPABILITY(temp));
			pDat->port_list[pDat->total_links * 2 + 1].pointer = currentPtr;
			lastSBDFO = currentPtr;

			/* Bit 6 indicates whether orientation override is desired.
			 * Bit 7 indicates the upstream link if overriding.
			 */
			/* assert catches at least the one known incorrect setting */
			ASSERT ((*pSwapPtr & 0x40) || (!(*pSwapPtr & 0x80)));
			if (*pSwapPtr & 0x40)
			{
				/* Override the device's orientation */
				lastLink = *pSwapPtr >> 7;
			}
			else
			{
				/* Detect the device's orientation */
				amd_pci_read_bits(currentPtr, 26, 26, &temp);
				lastLink = (u8)temp;
			}
			pDat->port_list[pDat->total_links * 2 + 1].Link = lastLink;

			depth++;
			pDat->total_links++;
			pSwapPtr++;
		}
	}
	else
	{
		/* Automatic non-coherent device detection */
		depth = 0;
		current_buid = 1;
		while (1)
		{
			currentPtr = MAKE_SBDFO(0, secBus, 0, 0, 0);

			AmdPCIRead(currentPtr, &temp);
			if (temp == 0xFFFFFFFF)
				/* No device found at currentPtr */
				break;

			if (pDat->total_links == MAX_PLATFORM_LINKS)
			{
				/*
				 * Exceeded our capacity to describe all non-coherent links found in the system.
				 * Error strategy:
				 * Auto recovery is not possible because data space is already all used.
				 */
				if (pDat->ht_block->amd_cb_event_notify)
				{
					sHtEventNcohLinkExceed evt;
					evt.e_size = sizeof(sHtEventNcohLinkExceed);
					evt.node = node;
					evt.link = link;
					evt.depth = depth;
					evt.max_links = pDat->nb->max_links;

					pDat->ht_block->amd_cb_event_notify(HT_EVENT_CLASS_ERROR,
								HT_EVENT_NCOH_LINK_EXCEED,
								(u8 *)&evt);
				}
				/* Force link loop to halt */
				STOP_HERE;
				break;
			}

			pDat->port_list[pDat->total_links*2].node_id = node;
			if (depth == 0)
			{
				pDat->port_list[pDat->total_links * 2].Type = PORTLIST_TYPE_CPU;
				pDat->port_list[pDat->total_links * 2].Link = link;
			}
			else
			{
				pDat->port_list[pDat->total_links * 2].Type = PORTLIST_TYPE_IO;
				pDat->port_list[pDat->total_links * 2].Link = 1 - lastLink;
				pDat->port_list[pDat->total_links * 2].host_link = link;
				pDat->port_list[pDat->total_links * 2].host_depth = depth - 1;
				pDat->port_list[pDat->total_links * 2].pointer = lastSBDFO;
			}

			pDat->port_list[pDat->total_links * 2 + 1].Type = PORTLIST_TYPE_IO;
			pDat->port_list[pDat->total_links * 2 + 1].node_id = node;
			pDat->port_list[pDat->total_links * 2 + 1].host_link = link;
			pDat->port_list[pDat->total_links * 2 + 1].host_depth = depth;

			do
			{
				amd_pci_find_next_cap(&currentPtr);
				ASSERT(currentPtr != ILLEGAL_SBDFO);
				AmdPCIRead(currentPtr, &temp);
			} while (!IS_HT_SLAVE_CAPABILITY(temp));

			amd_pci_read_bits(currentPtr, 25, 21, &unitIDcnt);
			if ((unitIDcnt + current_buid > 31) || ((secBus == 0) && (unitIDcnt + current_buid > 24)))
			{
				/* An error handler for the case where we run out of BUID's on a chain */
				if (pDat->ht_block->amd_cb_event_notify)
				{
					sHtEventNcohBuidExceed evt;
					evt.e_size = sizeof(sHtEventNcohBuidExceed);
					evt.node = node;
					evt.link = link;
					evt.depth = depth;
					evt.current_buid = (uint8)current_buid;
					evt.unit_count = (uint8)unitIDcnt;

					pDat->ht_block->amd_cb_event_notify(HT_EVENT_CLASS_ERROR,HT_EVENT_NCOH_BUID_EXCEED,(u8 *)&evt);
				}
				STOP_HERE;
				break;
			}
			amd_pci_write_bits(currentPtr, 20, 16, &current_buid);


			currentPtr += MAKE_SBDFO(0, 0, current_buid, 0, 0);
			amd_pci_read_bits(currentPtr, 20, 16, &temp);
			if (temp != current_buid)
			{
				/* An error handler for this critical error */
				if (pDat->ht_block->amd_cb_event_notify)
				{
					sHtEventNcohDeviceFailed evt;
					evt.e_size = sizeof(sHtEventNcohDeviceFailed);
					evt.node = node;
					evt.link = link;
					evt.depth = depth;
					evt.attempted_buid = (uint8)current_buid;

					pDat->ht_block->amd_cb_event_notify(HT_EVENT_CLASS_ERROR,HT_EVENT_NCOH_DEVICE_FAILED,(u8 *)&evt);
				}
				STOP_HERE;
				break;
			}

			amd_pci_read_bits(currentPtr, 26, 26, &temp);
			pDat->port_list[pDat->total_links * 2 + 1].Link = (u8)temp;
			pDat->port_list[pDat->total_links * 2 + 1].pointer = currentPtr;

			lastLink = (u8)temp;
			lastSBDFO = currentPtr;

			depth++;
			pDat->total_links++;
			current_buid += unitIDcnt;
		}
		if (pDat->ht_block->amd_cb_event_notify)
		{
			/* Provide information on automatic device results */
			sHtEventNcohAutoDepth evt;
			evt.e_size = sizeof(sHtEventNcohAutoDepth);
			evt.node = node;
			evt.link = link;
			evt.depth = (depth - 1);

			pDat->ht_block->amd_cb_event_notify(HT_EVENT_CLASS_INFO,HT_EVENT_NCOH_AUTO_DEPTH,(u8 *)&evt);
		}
	}
}


/*----------------------------------------------------------------------------------------
 * void
 * ncInit(sMainData *pDat)
 *
 *  Description:
 *	 Initialize the non-coherent fabric. Begin with the compat link on the BSP, then
 *	 find and initialize all other non-coherent chains.
 *
 *  Parameters:
 *	@param[in]  sMainData*  pDat = our global state
 * ---------------------------------------------------------------------------------------
 */
static void ncInit(sMainData *pDat)
{
	u8 node, link;
	u8 compatLink;

	compatLink = pDat->nb->readSbLink(pDat->nb);
	processLink(0, compatLink, pDat);

	for (node = 0; node <= pDat->nodes_discovered; node++)
	{
		for (link = 0; link < pDat->nb->max_links; link++)
		{
			if (pDat->ht_block->amd_cb_ignore_link && pDat->ht_block->amd_cb_ignore_link(node, link))
				continue;   /*  Skip the link */

			if (node == 0 && link == compatLink)
				continue;

			if (pDat->nb->readTrueLinkFailStatus(node, link, pDat, pDat->nb))
				continue;

			if (pDat->nb->verifyLinkIsNonCoherent(node, link, pDat->nb))
				processLink(node, link, pDat);
		}
	}
}

/***************************************************************************
 ***				Link Optimization			  ***
 ***************************************************************************/

/*----------------------------------------------------------------------------------------
 * void
 * regangLinks(sMainData *pDat)
 *
 *  Description:
 *	 Test the sublinks of a link to see if they qualify to be reganged.  If they do,
 *	 update the port list data to indicate that this should be done.  Note that no
 *	 actual hardware state is changed in this routine.
 *
 *  Parameters:
 *	@param[in,out] sMainData*  pDat = our global state
 * ---------------------------------------------------------------------------------------
 */
static void regangLinks(sMainData *pDat)
{
#ifndef HT_BUILD_NC_ONLY
	u8 i, j;
	for (i = 0; i < pDat->total_links * 2; i += 2)
	{
		ASSERT(pDat->port_list[i].Type < 2 && pDat->port_list[i].Link < pDat->nb->max_links);  /*  Data validation */
		ASSERT(pDat->port_list[i + 1].Type < 2 && pDat->port_list[i + 1].Link < pDat->nb->max_links); /*  data validation */
		ASSERT(!(pDat->port_list[i].Type == PORTLIST_TYPE_IO && pDat->port_list[i + 1].Type == PORTLIST_TYPE_CPU));  /*  ensure src is closer to the bsp than dst */

		/* Regang is false unless we pass all conditions below */
		pDat->port_list[i].sel_regang = FALSE;
		pDat->port_list[i + 1].sel_regang = FALSE;

		if ((pDat->port_list[i].Type != PORTLIST_TYPE_CPU) || (pDat->port_list[i + 1].Type != PORTLIST_TYPE_CPU))
			continue;   /*  Only process CPU to CPU links */

		for (j = i + 2; j < pDat->total_links * 2; j += 2)
		{
			if ((pDat->port_list[j].Type != PORTLIST_TYPE_CPU) || (pDat->port_list[j + 1].Type != PORTLIST_TYPE_CPU))
				continue;   /*  Only process CPU to CPU links */

			if (pDat->port_list[i].node_id != pDat->port_list[j].node_id)
				continue;   /*  Links must be from the same source */

			if (pDat->port_list[i + 1].node_id != pDat->port_list[j + 1].node_id)
				continue;   /*  Link must be to the same target */

			if ((pDat->port_list[i].Link & 3) != (pDat->port_list[j].Link & 3))
				continue;   /*  Ensure same source base port */

			if ((pDat->port_list[i + 1].Link & 3) != (pDat->port_list[j + 1].Link & 3))
				continue;   /*  Ensure same destination base port */

			if ((pDat->port_list[i].Link & 4) != (pDat->port_list[i + 1].Link & 4))
				continue;   /*  Ensure sublink0 routes to sublink0 */

			ASSERT((pDat->port_list[j].Link & 4) == (pDat->port_list[j + 1].Link & 4)); /*  (therefore sublink1 routes to sublink1) */

			if (pDat->ht_block->amd_cb_skip_regang &&
				pDat->ht_block->amd_cb_skip_regang(pDat->port_list[i].node_id,
							pDat->port_list[i].Link & 0x03,
							pDat->port_list[i + 1].node_id,
							pDat->port_list[i + 1].Link & 0x03))
			{
				continue;   /*  Skip regang */
			}


			pDat->port_list[i].Link &= 0x03; /*  Force to point to sublink0 */
			pDat->port_list[i + 1].Link &= 0x03;
			pDat->port_list[i].sel_regang = TRUE; /*  Enable link reganging */
			pDat->port_list[i + 1].sel_regang = TRUE;
			pDat->port_list[i].prv_width_out_cap = HT_WIDTH_16_BITS;
			pDat->port_list[i + 1].prv_width_out_cap = HT_WIDTH_16_BITS;
			pDat->port_list[i].prv_width_in_cap = HT_WIDTH_16_BITS;
			pDat->port_list[i + 1].prv_width_in_cap = HT_WIDTH_16_BITS;

			/*  Delete port_list[j, j+1], slow but easy to debug implementation */
			pDat->total_links--;
			amd_memcpy(&(pDat->port_list[j]), &(pDat->port_list[j + 2]), sizeof(sPortDescriptor)*(pDat->total_links * 2 - j));
			amd_memset(&(pDat->port_list[pDat->total_links * 2]), INVALID_LINK, sizeof(sPortDescriptor) * 2);

			/* //High performance, but would make debuging harder due to 'shuffling' of the records */
			/* //amd_memcpy(port_list[TotalPorts-2], port_list[j], SIZEOF(sPortDescriptor)*2); */
			/* //TotalPorts -=2; */

			break; /*  Exit loop, advance to port_list[i+2] */
		}
	}
#endif /* HT_BUILD_NC_ONLY */
}

static void detectIoLinkIsochronousCapable(sMainData *pDat)
{
	u8 i;
	u8 isochronous_capable = 0;
	u8 iommu = get_uint_option("iommu", 1);

	for (i = 0; i < pDat->total_links * 2; i += 2) {
		if ((pDat->port_list[i].Type == PORTLIST_TYPE_CPU) && (pDat->port_list[i + 1].Type == PORTLIST_TYPE_IO)) {
			if ((pDat->port_list[i].prv_feature_cap & 0x1) && (pDat->port_list[i + 1].prv_feature_cap & 0x1)) {
				pDat->port_list[i].enable_isochronous_mode = 1;
				pDat->port_list[i + 1].enable_isochronous_mode = 1;
				isochronous_capable = 1;
			} else {
				pDat->port_list[i].enable_isochronous_mode = 0;
				pDat->port_list[i + 1].enable_isochronous_mode = 0;
			}
		}
	}

	if (isochronous_capable && iommu) {
		printk(BIOS_DEBUG, "Forcing HT links to isochronous mode due to enabled IOMMU\n");
		/* Isochronous mode must be set on all links if the IOMMU is enabled */
		for (i = 0; i < pDat->total_links * 2; i += 2) {
			pDat->port_list[i].enable_isochronous_mode = 1;
			pDat->port_list[i + 1].enable_isochronous_mode = 1;
		}
	}
}

/*----------------------------------------------------------------------------------------
 * void
 * selectOptimalWidthAndFrequency(sMainData *pDat)
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
 *	@param[in,out]  sMainData*  pDat = our global state, port list data
 * ---------------------------------------------------------------------------------------
 */
static void selectOptimalWidthAndFrequency(sMainData *pDat)
{
	u8 i, j;
	u32 temp;
	u32 cbPCBFreqLimit;
	u32 cbPCBFreqLimit_NVRAM;
	u8 cbPCBABDownstreamWidth;
	u8 cbPCBBAUpstreamWidth;

	cbPCBFreqLimit_NVRAM = 0xfffff;
	temp = get_uint_option("hypertransport_speed_limit", 0);

	cbPCBFreqLimit_NVRAM = ht_speed_limit[temp & 0xf];

	if (!is_fam15h()) {
		/* FIXME
		 * By default limit frequency to 2.6 GHz as there are residual
		 * problems with HT v3.1 implementation on at least some Socket G34
		 * mainboards / Fam10h CPUs.
		 * Debug the issues and reenable this...
		 */
		if (cbPCBFreqLimit_NVRAM > 0xffff)
			cbPCBFreqLimit_NVRAM = 0xffff;
	}

	for (i = 0; i < pDat->total_links*2; i += 2)
	{
		cbPCBFreqLimit = 0xfffff;		// Maximum allowed by autoconfiguration
		if (pDat->ht_block->ht_link_configuration)
			cbPCBFreqLimit = ht_speed_mhz_to_hw(pDat->ht_block->ht_link_configuration->ht_speed_limit);
		cbPCBFreqLimit = MIN(cbPCBFreqLimit, cbPCBFreqLimit_NVRAM);

#if CONFIG(LIMIT_HT_DOWN_WIDTH_8)
		cbPCBABDownstreamWidth = 8;
#else
		cbPCBABDownstreamWidth = 16;
#endif

#if CONFIG(LIMIT_HT_UP_WIDTH_8)
		cbPCBBAUpstreamWidth = 8;
#else
		cbPCBBAUpstreamWidth = 16;
#endif

		if ((pDat->port_list[i].Type == PORTLIST_TYPE_CPU) && (pDat->port_list[i + 1].Type == PORTLIST_TYPE_CPU))
		{
			if (pDat->ht_block->amd_cb_cpu_2_cpu_pcb_limits)
			{
				pDat->ht_block->amd_cb_cpu_2_cpu_pcb_limits(
						pDat->port_list[i].node_id,
						pDat->port_list[i].Link,
						pDat->port_list[i + 1].node_id,
						pDat->port_list[i + 1].Link,
						&cbPCBABDownstreamWidth,
						&cbPCBBAUpstreamWidth, &cbPCBFreqLimit
						);
			}
		}
		else
		{
			if (pDat->ht_block->amd_cb_iop_cb_limits)
			{
				pDat->ht_block->amd_cb_iop_cb_limits(
						pDat->port_list[i + 1].node_id,
						pDat->port_list[i + 1].host_link,
						pDat->port_list[i + 1].host_depth,
						&cbPCBABDownstreamWidth,
						 &cbPCBBAUpstreamWidth, &cbPCBFreqLimit
						);
			}
		}

		temp = pDat->port_list[i].prv_frequency_cap;
		temp &= pDat->port_list[i + 1].prv_frequency_cap;
		temp &= cbPCBFreqLimit;
		pDat->port_list[i].composite_frequency_cap = temp;
		pDat->port_list[i + 1].composite_frequency_cap = temp;

		ASSERT (temp != 0);
		for (j = 19;; j--)
		{
			if ((j == 16) || (j == 15))
				continue;
			if (temp & ((u32)1 << j))
				break;
		}

		pDat->port_list[i].sel_frequency = j;
		pDat->port_list[i + 1].sel_frequency = j;

		temp = pDat->port_list[i].prv_width_out_cap;
		if (pDat->port_list[i + 1].prv_width_in_cap < temp)
			temp = pDat->port_list[i + 1].prv_width_in_cap;
		if (cbPCBABDownstreamWidth < temp)
			temp = cbPCBABDownstreamWidth;
		pDat->port_list[i].sel_width_out = (u8)temp;
		pDat->port_list[i + 1].sel_width_in = (u8)temp;

		temp = pDat->port_list[i].prv_width_in_cap;
		if (pDat->port_list[i + 1].prv_width_out_cap < temp)
			temp = pDat->port_list[i + 1].prv_width_out_cap;
		if (cbPCBBAUpstreamWidth < temp)
			temp = cbPCBBAUpstreamWidth;
		pDat->port_list[i].sel_width_in = (u8)temp;
		pDat->port_list[i + 1].sel_width_out = (u8)temp;
	}
}

/*----------------------------------------------------------------------------------------
 * void
 * hammerSublinkFixup(sMainData *pDat)
 *
 *  Description:
 *	 Iterate through all links, checking the frequency of each sublink pair.  Make the
 *	 adjustment to the port list data so that the frequencies are at a valid ratio,
 *	 reducing frequency as needed to achieve this. (All links support the minimum 200 MHz
 *	 frequency.)  Repeat the above until no adjustments are needed.
 *	 Note no hardware state changes in this routine.
 *
 *  Parameters:
 *	@param[in,out] sMainData* pDat = our global state, link state and port list
 * ---------------------------------------------------------------------------------------
 */
static void hammerSublinkFixup(sMainData *pDat)
{
#ifndef HT_BUILD_NC_ONLY
	u8 i, j, k;
	BOOL changes, downgrade;

	u8 hiIndex;
	u8 hiFreq, loFreq;

	u32 temp;

	do
	{
		changes = FALSE;
		for (i = 0; i < pDat->total_links*2; i++)
		{
			if (pDat->port_list[i].Type != PORTLIST_TYPE_CPU) /*  Must be a CPU link */
				continue;
			if (pDat->port_list[i].Link < 4) /*  Only look for sublink1's */
				continue;

			for (j = 0; j < pDat->total_links*2; j++)
			{
				/*  Step 1. Find the matching sublink0 */
				if (pDat->port_list[j].Type != PORTLIST_TYPE_CPU)
					continue;
				if (pDat->port_list[j].node_id != pDat->port_list[i].node_id)
					continue;
				if (pDat->port_list[j].Link != (pDat->port_list[i].Link & 0x03))
					continue;

				/*  Step 2. Check for an illegal frequency ratio */
				if (pDat->port_list[i].sel_frequency >= pDat->port_list[j].sel_frequency)
				{
					hiIndex = i;
					hiFreq = pDat->port_list[i].sel_frequency;
					loFreq = pDat->port_list[j].sel_frequency;
				}
				else
				{
					hiIndex = j;
					hiFreq = pDat->port_list[j].sel_frequency;
					loFreq = pDat->port_list[i].sel_frequency;
				}

				if (hiFreq == loFreq)
					break; /*  The frequencies are 1:1, no need to do anything */

				downgrade = FALSE;

				if (hiFreq == 13)
				{
					if ((loFreq != 7) &&  /* {13, 7} 2400MHz / 1200MHz 2:1 */
						(loFreq != 4) &&  /* {13, 4} 2400MHz /  600MHz 4:1 */
						(loFreq != 2))   /* {13, 2} 2400MHz /  400MHz 6:1 */
						downgrade = TRUE;
				}
				else if (hiFreq == 11)
				{
					if ((loFreq != 6))    /* {11, 6} 2000MHz / 1000MHz 2:1 */
						downgrade = TRUE;
				}
				else if (hiFreq == 9)
				{
					if ((loFreq != 5) &&  /* { 9, 5} 1600MHz /  800MHz 2:1 */
						(loFreq != 2) &&  /* { 9, 2} 1600MHz /  400MHz 4:1 */
						(loFreq != 0))   /* { 9, 0} 1600MHz /  200MHz 8:1 */
						downgrade = TRUE;
				}
				else if (hiFreq == 7)
				{
					if ((loFreq != 4) &&  /* { 7, 4} 1200MHz /  600MHz 2:1 */
						(loFreq != 0))   /* { 7, 0} 1200MHz /  200MHz 6:1 */
						downgrade = TRUE;
				}
				else if (hiFreq == 5)
				{
					if ((loFreq != 2) &&  /* { 5, 2}  800MHz /  400MHz 2:1 */
						(loFreq != 0))   /* { 5, 0}  800MHz /  200MHz 4:1 */
						downgrade = TRUE;
				}
				else if (hiFreq == 2)
				{
					if ((loFreq != 0))    /* { 2, 0}  400MHz /  200MHz 2:1 */
						downgrade = TRUE;
				}
				else
				{
					downgrade = TRUE; /*  no legal ratios for hiFreq */
				}

				/*  Step 3. Downgrade the higher of the two frequencies, and set nochanges to FALSE */
				if (downgrade)
				{
					/*  Although the problem was with the port specified by hiIndex, we need to */
					/*  downgrade both ends of the link. */
					hiIndex = hiIndex & 0xFE; /*  Select the 'upstream' (i.e. even) port */

					temp = pDat->port_list[hiIndex].composite_frequency_cap;

					/*  Remove hiFreq from the list of valid frequencies */
					temp = temp & ~((uint32)1 << hiFreq);
					ASSERT (temp != 0);
					pDat->port_list[hiIndex].composite_frequency_cap = temp;
					pDat->port_list[hiIndex + 1].composite_frequency_cap = temp;

					for (k = 19;; k--)
					{
						if ((j == 16) || (j == 15))
							continue;
						if (temp & ((u32)1 << k))
							break;
					}

					pDat->port_list[hiIndex].sel_frequency = k;
					pDat->port_list[hiIndex + 1].sel_frequency = k;

					changes = TRUE;
				}
			}
		}
	} while (changes); /*  Repeat until a valid configuration is reached */
#endif /* HT_BUILD_NC_ONLY */
}

/*----------------------------------------------------------------------------------------
 * void
 * linkOptimization(sMainData *pDat)
 *
 *  Description:
 *	 Based on link capabilities, apply optimization rules to come up with the real best
 *	 settings, including several external limit decision from call backs. This includes
 *	 handling of sublinks.	Finally, after the port list data is updated, set the hardware
 *	 state for all links.
 *
 *  Parameters:
 *	@param[in]  sMainData* pDat = our global state
 * ---------------------------------------------------------------------------------------
 */
static void linkOptimization(sMainData *pDat)
{
	pDat->nb->gatherLinkData(pDat, pDat->nb);
	regangLinks(pDat);
	if (is_fam15h())
		detectIoLinkIsochronousCapable(pDat);
	selectOptimalWidthAndFrequency(pDat);
	hammerSublinkFixup(pDat);
	pDat->nb->setLinkData(pDat, pDat->nb);
}


/*----------------------------------------------------------------------------------------
 * void
 * trafficDistribution(sMainData *pDat)
 *
 *  Description:
 *	 In the case of a two node system with both sublinks used, enable the traffic
 *	 distribution feature.
 *
 *  Parameters:
 *	  @param[in]	    sMainData*	  pDat		 = our global state, port list data
 * ---------------------------------------------------------------------------------------
 */
static void trafficDistribution(sMainData *pDat)
{
#ifndef HT_BUILD_NC_ONLY
	u32 links01, links10;
	u8 linkCount;
	u8 i;

	/*  Traffic Distribution is only used when there are exactly two nodes in the system */
	if (pDat->nodes_discovered + 1 != 2)
		return;

	links01 = 0;
	links10 = 0;
	linkCount = 0;
	for (i = 0; i < pDat->total_links*2; i += 2)
	{
		if ((pDat->port_list[i].Type == PORTLIST_TYPE_CPU) && (pDat->port_list[i + 1].Type == PORTLIST_TYPE_CPU))
		{
			links01 |= (u32)1 << pDat->port_list[i].Link;
			links10 |= (u32)1 << pDat->port_list[i + 1].Link;
			linkCount++;
		}
	}
	ASSERT(linkCount != 0);
	if (linkCount == 1)
		return; /*  Don't setup Traffic Distribution if only one link is being used */

	pDat->nb->writeTrafficDistribution(links01, links10, pDat->nb);
#endif /* HT_BUILD_NC_ONLY */
}

/*----------------------------------------------------------------------------------------
 * void
 * tuning(sMainData *pDat)
 *
 *  Description:
 *	 Handle system and performance tunings, such as traffic distribution, fifo and
 *	 buffer tuning, and special config tunings.
 *
 *  Parameters:
 *	@param[in] sMainData* pDat = our global state, port list data
 * ---------------------------------------------------------------------------------------
 */
static void tuning(sMainData *pDat)
{
	u8 i;

	/* See if traffic distribution can be done and do it if so
	 * or allow system specific customization
	 */
	if ((pDat->ht_block->amd_cb_customize_traffic_distribution == NULL)
		|| !pDat->ht_block->amd_cb_customize_traffic_distribution())
	{
		trafficDistribution(pDat);
	}

	/* For each node, invoke northbridge specific buffer tunings or
	 * system specific customizations.
	 */
	for (i = 0; i < pDat->nodes_discovered + 1; i++)
	{
		if ((pDat->ht_block->amd_cb_customize_buffers == NULL)
		   || !pDat->ht_block->amd_cb_customize_buffers(i))
		{
			pDat->nb->bufferOptimizations(i, pDat, pDat->nb);
		}
	}
}

/*----------------------------------------------------------------------------------------
 * BOOL
 * isSanityCheckOk()
 *
 *  Description:
 *	 Perform any general sanity checks which should prevent HT from running if they fail.
 *	 Currently only the "Must run on BSP only" check.
 *
 *  Parameters:
 *	@param[out] result BOOL  = true if check is ok, false if it failed
 * ---------------------------------------------------------------------------------------
 */
static BOOL isSanityCheckOk(void)
{
	uint64 qValue;

	AmdMSRRead(LAPIC_BASE_MSR, &qValue);

	return ((qValue.lo & LAPIC_BASE_MSR_BOOTSTRAP_PROCESSOR) != 0);
}

/***************************************************************************
 ***				 HT Initialize				   ***
 ***************************************************************************/

/*----------------------------------------------------------------------------------------
 * void
 * htInitialize(AMD_HTBLOCK *pBlock)
 *
 *  Description:
 *	 This is the top level external interface for Hypertransport Initialization.
 *	 Create our initial internal state, initialize the coherent fabric,
 *	 initialize the non-coherent chains, and perform any required fabric tuning or
 *	 optimization.
 *
 *  Parameters:
 *	@param[in] AMD_HTBLOCK*  pBlock = Our Initial State including possible
 *				  topologies and routings, non coherent bus
 *				  assignment info, and actual
 *				  wrapper or OEM call back routines.
 * ---------------------------------------------------------------------------------------
 */
void amd_ht_initialize(AMD_HTBLOCK *pBlock)
{
	sMainData pDat;
	cNorthBridge nb;

	if (isSanityCheckOk())
	{
		newNorthBridge(0, &nb);

		pDat.ht_block = pBlock;
		pDat.nb = &nb;
		pDat.sys_mp_cap = nb.maxNodes;
		nb.isCapable(0, &pDat, pDat.nb);
		coherentInit(&pDat);

		pDat.auto_bus_current = pBlock->auto_bus_start;
		pDat.used_cfg_map_entires = 0;
		ncInit(&pDat);
		linkOptimization(&pDat);
		tuning(&pDat);
	}
}
