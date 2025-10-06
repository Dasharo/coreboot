/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef __ROOT_BRIDGE_INFO_H_
#define __ROOT_BRIDGE_INFO_H_

#include <stdint.h>
#include <commonlib/bsd/compiler.h>
#include <commonlib/coreboot_tables.h>

/* Sourced from EDK2, Universal Payload's UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGES */

typedef struct {
	uint8_t revision;
	uint8_t reserved;
	uint16_t length;
} root_bridges_info_header_t;

typedef struct {
	uint64_t base;
	uint64_t limit;
	uint64_t translation;
} __packed root_bridge_aperture_t;

typedef struct {
	uint32_t		segment;
	uint64_t		supports;
	uint64_t		attributes;
	bool			dma_above4g;
	bool			no_ext_config_space;
	uint64_t		allocation_attributes;
	root_bridge_aperture_t	bus;
	root_bridge_aperture_t	io;
	root_bridge_aperture_t	mem;
	root_bridge_aperture_t	mem_above4g;
	root_bridge_aperture_t	pmem;
	root_bridge_aperture_t	pmem_above4g;
	uint32_t		hid;
	uint32_t		uid;
} __packed pci_root_bridge_t;

typedef struct {
	root_bridges_info_header_t	header;
	bool				resource_assigned;
	uint8_t				count;
	pci_root_bridge_t		root_bridge[0];
} __packed pci_root_bridges_info_t;

#endif // __ROOT_BRIDGE_INFO_H_
