/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef __ROOT_BRIDGE_INFO_H_
#define __ROOT_BRIDGE_INFO_H_

#include <stdint.h>
#include <commonlib/bsd/compiler.h>
#include <commonlib/coreboot_tables.h>

/* Sourced from EDK2, Universal Payload's UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGES */

typedef struct {
  uint8_t     revision;
  uint8_t     reserved;
  uint16_t    length;
} root_bridges_info_header_t;

//
// (Base > Limit) indicates an aperture is not available.
//
typedef struct {
  //
  // Base and Limit are the device address instead of host address when
  // Translation is not zero
  //
  uint64_t    base;
  uint64_t    limit;
  //
  // According to UEFI 2.7, Device Address = Host Address + Translation,
  // so Translation = Device Address - Host Address.
  // On platforms where Translation is not zero, the subtraction is probably to
  // be performed with UINT64 wrap-around semantics, for we may translate an
  // above-4G host address into a below-4G device address for legacy PCIe device
  // compatibility.
  //
  // NOTE: The alignment of Translation is required to be larger than any BAR
  // alignment in the same root bridge, so that the same alignment can be
  // applied to both device address and host address, which simplifies the
  // situation and makes the current resource allocation code in generic PCI
  // host bridge driver still work.
  //
  uint64_t    translation;
} __packed root_bridge_aperture_t;

///
/// Payload PCI Root Bridge Information HOB
///
typedef struct {
  uint32_t                                      segment;               ///< Segment number.
  uint64_t                                      supports;              ///< Supported attributes.
                                                                       ///< Refer to EFI_PCI_ATTRIBUTE_xxx used by GetAttributes()
                                                                       ///< and SetAttributes() in EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL.
  uint64_t                                      attributes;            ///< Initial attributes.
                                                                       ///< Refer to EFI_PCI_ATTRIBUTE_xxx used by GetAttributes()
                                                                       ///< and SetAttributes() in EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL.
  bool                                          dma_above4g;           ///< DMA above 4GB memory.
                                                                       ///< Set to TRUE when root bridge supports DMA above 4GB memory.
  bool                                          no_ext_config_space;   ///< When FALSE, the root bridge supports
                                                                       ///< Extended (4096-byte) Configuration Space.
                                                                       ///< When TRUE, the root bridge supports
                                                                       ///< 256-byte Configuration Space only.
  uint64_t                                      allocation_attributes; ///< Allocation attributes.
                                                                       ///< Refer to EFI_PCI_HOST_BRIDGE_COMBINE_MEM_PMEM and
                                                                       ///< EFI_PCI_HOST_BRIDGE_MEM64_DECODE used by GetAllocAttributes()
                                                                       ///< in EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL.
  root_bridge_aperture_t                        bus;                   ///< Bus aperture which can be used by the root bridge.
  root_bridge_aperture_t                        io;                    ///< IO aperture which can be used by the root bridge.
  root_bridge_aperture_t                        mem;                   ///< MMIO aperture below 4GB which can be used by the root bridge.
  root_bridge_aperture_t                        mem_above4g;           ///< MMIO aperture above 4GB which can be used by the root bridge.
  root_bridge_aperture_t                        pmem;                  ///< Prefetchable MMIO aperture below 4GB which can be used by the root bridge.
  root_bridge_aperture_t                        pmem_above4g;          ///< Prefetchable MMIO aperture above 4GB which can be used by the root bridge.
  uint32_t                                      hid;                   ///< PnP hardware ID of the root bridge. This value must match the corresponding
                                                                       ///< _HID in the ACPI name space.
  uint32_t                                      uid;                   ///< Unique ID that is required by ACPI if two devices have the same _HID.
                                                                       ///< This value must also match the corresponding _UID/_HID pair in the ACPI name space.
} __packed pci_root_bridge_t;

typedef struct {
  root_bridges_info_header_t           header;
  bool                                 resource_assigned;
  uint8_t                              count;
  pci_root_bridge_t                    root_bridge[0];
} __packed pci_root_bridges_info_t;

#endif // __ROOT_BRIDGE_INFO_H_
