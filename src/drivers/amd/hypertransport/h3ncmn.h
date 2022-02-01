/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef H3NCMN_H
#define H3NCMN_H

#include <stdint.h>
#include <device/pci.h>
#include <cpu/amd/msr.h>

#include "comlib.h"
#include "h3finit.h"
#include "h3ffeat.h"

/* Use a macro to convert a node number to a PCI device.  If some future port of
 * this code needs to, this can easily be replaced by a function call:
 * u8 makePCIDeviceFromNode(u8 node);
 */
#define makePCIDeviceFromNode(node) \
	((u8)(24 + node))

/* Use a macro to convert a node number to a PCI bus.	If some future port of
 * this code needs to, this can easily be replaced by a function call:
 * u8 makePCIBusFromNode(u8 node);
 */
#define makePCIBusFromNode(node) \
	((u8)(0))

/* Use a macro to convert a node number to a PCI Segment.  If some future port of
 * this code needs to, this can easily be replaced by a function call:
 * u8 makePCISegmentFromNode(u8 node);
 */
#define makePCISegmentFromNode(node) \
	((u8)(0))

/* Macros to fix support issues that come up with early sample processors, which
 * sometimes do things like report capabilities that are actually unsupported.
 * Use the build flag, HT_BUILD_EARLY_SAMPLE_CPU_SUPPORT, to enable this support.
 *
 * It's not envisioned this would be replaced by an external function, but the prototype is
 * u16 fixEarlySampleFreqCapability(u16 fc);
 */
#ifndef HT_BUILD_EARLY_SAMPLE_CPU_SUPPORT
#define fixEarlySampleFreqCapability(fc) \
	((u16)fc)
#else
#define fixEarlySampleFreqCapability(fc) \
	((u16)fc & HT_FREQUENCY_LIMIT_HT1_ONLY)
#endif

struct cNorthBridge
{
	/* Public data, clients of northbridge can access */
	u8 maxLinks;
	u8 maxNodes;
	u8 maxPlatformLinks;

	/* Public Interfaces for northbridge clients, coherent init*/
	void (*write_routing_table)(u8 node, u8 target, u8 link, cNorthBridge *nb);
	void (*write_node_id)(u8 node, u8 nodeID, cNorthBridge *nb);
	u8 (*read_def_lnk)(u8 node, cNorthBridge *nb);
	void (*enable_routing_tables)(u8 node, cNorthBridge *nb);
	BOOL (*verify_link_is_coherent)(u8 node, u8 link, cNorthBridge *nb);
	BOOL (*read_true_link_fail_status)(u8 node, u8 link, sMainData *pDat, cNorthBridge *nb);
	u8 (*read_token)(u8 node, cNorthBridge *nb);
	void (*writeToken)(u8 node, u8 value, cNorthBridge *nb);
	u8 (*getNumCoresOnNode)(u8 node, cNorthBridge *nb);
	void (*set_total_nodes_and_cores)(u8 node, u8 totalNodes, u8 totalCores, cNorthBridge *nb);
	void (*limit_nodes)(u8 node, cNorthBridge *nb);
	void (*write_full_routing_table)(u8 node, u8 target, u8 reqLink, u8 rspLink, u32 bClinks, cNorthBridge *nb);
	BOOL (*is_compatible)(u8 node, cNorthBridge *nb);
	BOOL (*isCapable)(u8 node, sMainData *pDat, cNorthBridge *nb);
	void (*stopLink)(u8 node, u8 link, cNorthBridge *nb);
	BOOL (*handleSpecialLinkCase)(u8 node, u8 link, sMainData *pDat, cNorthBridge *nb);

	/* Public Interfaces for northbridge clients, noncoherent init */
	u8 (*read_sb_link)(cNorthBridge *nb);
	BOOL (*verify_link_is_non_coherent)(u8 node, u8 link, cNorthBridge *nb);
	void (*setCFGAddrMap)(u8 cfgMapIndex, u8 secBus, u8 subBus, u8 targetNode, u8 targetLink, sMainData *pDat, cNorthBridge *nb);

	/* Public Interfaces for northbridge clients, Optimization */
	u8 (*convert_bits_to_width)(u8 value, cNorthBridge *nb);
	u8 (*convert_width_to_bits)(u8 value, cNorthBridge *nb);
	u32 (*northBridgeFreqMask)(u8 node, cNorthBridge *nb);
	void (*gather_link_data)(sMainData *pDat, cNorthBridge *nb);
	void (*set_link_data)(sMainData *pDat, cNorthBridge *nb);

	/* Public Interfaces for northbridge clients, System and performance Tuning. */
	void (*writeTrafficDistribution)(u32 links01, u32 links10, cNorthBridge *nb);
	void (*bufferOptimizations)(u8 node, sMainData *pDat, cNorthBridge *nb);

	/* Private Data for northbridge implementation use only */
	u32 selfRouteRequestMask;
	u32 selfRouteResponseMask;
	u8 broadcastSelfBit;
	u32 compatibleKey;
} ;

void new_northbridge(u8 node, cNorthBridge *nb);

#endif	 /* H3NCMN_H */
