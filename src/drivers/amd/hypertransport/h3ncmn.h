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
 * u8 make_pci_device_from_node(u8 node);
 */
#define make_pci_device_from_node(node) \
	((u8)(24 + node))

/* Use a macro to convert a node number to a PCI bus.	If some future port of
 * this code needs to, this can easily be replaced by a function call:
 * u8 make_pci_bus_from_node(u8 node);
 */
#define make_pci_bus_from_node(node) \
	((u8)(0))

/* Use a macro to convert a node number to a PCI Segment.  If some future port of
 * this code needs to, this can easily be replaced by a function call:
 * u8 make_pci_segment_from_node(u8 node);
 */
#define make_pci_segment_from_node(node) \
	((u8)(0))

/* Macros to fix support issues that come up with early sample processors, which
 * sometimes do things like report capabilities that are actually unsupported.
 * Use the build flag, HT_BUILD_EARLY_SAMPLE_CPU_SUPPORT, to enable this support.
 *
 * It's not envisioned this would be replaced by an external function, but the prototype is
 * u16 fix_early_sample_freq_capability(u16 fc);
 */
#ifndef HT_BUILD_EARLY_SAMPLE_CPU_SUPPORT
#define fix_early_sample_freq_capability(fc) \
	((u16)fc)
#else
#define fix_early_sample_freq_capability(fc) \
	((u16)fc & HT_FREQUENCY_LIMIT_HT1_ONLY)
#endif

struct cNorthBridge
{
	/* Public data, clients of northbridge can access */
	u8 max_links;
	u8 max_nodes;
	u8 max_platform_links;

	/* Public Interfaces for northbridge clients, coherent init*/
	void (*write_routing_table)(u8 node, u8 target, u8 link, cNorthBridge *nb);
	void (*write_node_id)(u8 node, u8 node_id, cNorthBridge *nb);
	u8 (*read_def_lnk)(u8 node, cNorthBridge *nb);
	void (*enable_routing_tables)(u8 node, cNorthBridge *nb);
	BOOL (*verify_link_is_coherent)(u8 node, u8 link, cNorthBridge *nb);
	BOOL (*read_true_link_fail_status)(u8 node, u8 link, sMainData *p_dat,
					cNorthBridge *nb);
	u8 (*read_token)(u8 node, cNorthBridge *nb);
	void (*writeToken)(u8 node, u8 value, cNorthBridge *nb);
	u8 (*get_num_cores_on_node)(u8 node, cNorthBridge *nb);
	void (*set_total_nodes_and_cores)(u8 node, u8 total_nodes, u8 total_cores,
		cNorthBridge *nb);
	void (*limit_nodes)(u8 node, cNorthBridge *nb);
	void (*write_full_routing_table)(u8 node, u8 target, u8 req_link, u8 rsp_link,
					u32 bc_links, cNorthBridge *nb);
	BOOL (*is_compatible)(u8 node, cNorthBridge *nb);
	BOOL (*is_capable)(u8 node, sMainData *p_dat, cNorthBridge *nb);
	void (*is_link)(u8 node, u8 link, cNorthBridge *nb);
	BOOL (*handle_special_link_case)(u8 node, u8 link, sMainData *p_dat, cNorthBridge *nb);

	/* Public Interfaces for northbridge clients, noncoherent init */
	u8 (*read_sb_link)(cNorthBridge *nb);
	BOOL (*verify_link_is_non_coherent)(u8 node, u8 link, cNorthBridge *nb);
	void (*set_cfg_addr_map)(u8 cfg_map_index, u8 sec_bus, u8 sub_bus, u8 target_node,
				u8 target_link, sMainData *p_dat, cNorthBridge *nb);

	/* Public Interfaces for northbridge clients, Optimization */
	u8 (*convert_bits_to_width)(u8 value, cNorthBridge *nb);
	u8 (*convert_width_to_bits)(u8 value, cNorthBridge *nb);
	u32 (*northbridge_freq_mask)(u8 node, cNorthBridge *nb);
	void (*gather_link_data)(sMainData *p_dat, cNorthBridge *nb);
	void (*set_link_data)(sMainData *p_dat, cNorthBridge *nb);

	/* Public Interfaces for northbridge clients, System and performance Tuning. */
	void (*write_traffic_distribution)(u32 links01, u32 links10, cNorthBridge *nb);
	void (*buffer_optimizations)(u8 node, sMainData *p_dat, cNorthBridge *nb);

	/* Private Data for northbridge implementation use only */
	u32 self_route_request_mask;
	u32 self_route_response_mask;
	u8 broadcast_self_bit;
	u32 compatible_key;
};

void new_northbridge(u8 node, cNorthBridge *nb);

#endif	 /* H3NCMN_H */
