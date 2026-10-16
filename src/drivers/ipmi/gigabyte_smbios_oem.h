/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __DRIVERS_IPMI_GIGABYTE_SMBIOS_OEM_H__
#define __DRIVERS_IPMI_GIGABYTE_SMBIOS_OEM_H__

#include <types.h>

#include "ipmi_if.h"

/*
 * Gigabyte OEM IPMI protocol used by the MZ33-AR1 stock BIOS
 * (SendIpmiSystemInfo.efi) to hand the SMBIOS tables over to the AMI
 * MegaRAC based BMC firmware, reverse engineered from both the DXE driver
 * and the BMC side (libgbt.so / libgbtmisc.so).
 *
 * Every request is an OEM group (NetFn 0x2e) command whose data starts
 * with the Gigabyte IANA (15370 = 0x003c0a, LSB first) followed by a
 * sub-command byte. The BMC looks the sub-command up in a single table
 * holding a "set" and a "get" handler for each entry; the even command
 * number (0x20) selects the set handler and the odd one (0x21) the get
 * handler. Sub-commands used here:
 *
 *  - 0x0a, get only: enables the BMC's host shared memory window and
 *    returns its BMC (AHB) address. The host then writes the payload
 *    there through the ASPEED P2A bridge.
 *  - 0x0b, set only: tells the BMC the payload is in place. The BMC
 *    validates and stores the SMBIOS part of it, which is what the BMC
 *    web interface and Redfish use for the host BIOS/system information.
 *    Sending it as a "get" (0x21) is answered with 0xc5, as that table
 *    entry has no get handler.
 *  - 0x57, set only: POST is complete ("BIOS ready"), followed by one data
 *    byte 0. The BMC then starts its MCTP PCIe/I2C endpoint discovery and
 *    I3C device init. The vendor BIOS sends it at ReadyToBoot
 *    (GbtBiosReady.efi), right after driving its BIOS ready GPIO.
 *
 * Payload layout, all blocks back to back:
 *
 *   SMBIOS 3.0 entry point (24 bytes)
 *   SMBIOS structure table (entry point's struct_table_length bytes)
 *   _HD_    header (16 bytes) + 256 * n + 512 * m bytes of disk entries
 *   _CPU_   block (72 bytes)
 *   _MEM_   block (64 bytes)
 *   _LAN_   header (32 bytes) + 160 * n bytes of NIC entries
 *   _MDATA_ header (32 bytes) + 128 * n bytes of entries
 *   unnamed header (32 bytes) + 320 * n bytes of entries
 *   _HEADER_ directory (34 bytes)
 *
 * The BMC firmware (13.06.29) only parses the SMBIOS part: the entry
 * point checksum must be valid and the structures following it must end
 * with a type 127 record. The table address in the entry point may be a
 * host address; the BMC then looks for the table right after the entry
 * point. Everything after the SMBIOS table is not used by the BMC, but is
 * still sent in the layout the vendor BIOS uses, with no disk, NIC or
 * other entries.
 */
#define IPMI_NETFN_GIGABYTE_OEM			0x2e
#define IPMI_CMD_GIGABYTE_OEM_SET		0x20
#define IPMI_CMD_GIGABYTE_OEM_GET		0x21

#define GIGABYTE_IANA				{ 0x0a, 0x3c, 0x00 }

#define GIGABYTE_SUBCMD_GET_SHM_ADDR		0x0a
#define GIGABYTE_SUBCMD_SET_SMBIOS_READY	0x0b
#define GIGABYTE_SUBCMD_SET_BIOS_READY		0x57

struct gigabyte_oem_req {
	uint8_t iana[3];
	uint8_t subcmd;
} __packed;

struct gigabyte_set_bios_ready_req {
	struct gigabyte_oem_req hdr;
	uint8_t data;		/* 0: start the BMC's device discovery */
} __packed;

struct gigabyte_get_shm_addr_rsp {
	struct ipmi_rsp resp;
	uint8_t iana[3];
	uint32_t shm_address;
	uint8_t unknown;	/* Always 1 */
} __packed;

struct gigabyte_set_smbios_ready_req {
	struct gigabyte_oem_req hdr;
	uint16_t total_size;	/* Whole payload, including _HEADER_ */
	uint16_t data_size;	/* total_size without _HEADER_ */
	uint16_t reserved;
} __packed;

struct gigabyte_set_smbios_ready_rsp {
	struct ipmi_rsp resp;
	uint8_t iana[3];
	uint16_t smbios_size;	/* Entry point + table up to type 127 stored by the BMC */
} __packed;

#define GIGABYTE_SYSINFO_HD_SIGNATURE		{ '_', 'H', 'D', '_' }
#define GIGABYTE_SYSINFO_HD_REVISION		{ 'R', 'V', '1', '1' }

struct gigabyte_sysinfo_hd_block {
	uint8_t signature[4];
	uint8_t revision[4];
	uint8_t num_entries;		/* Number of 256-byte entries */
	uint8_t reserved0[5];
	uint8_t num_entries_dup;	/* Same as num_entries */
	uint8_t num_large_entries;	/* Number of 512-byte entries */
} __packed;

_Static_assert(sizeof(struct gigabyte_sysinfo_hd_block) == 16, "Wrong _HD_ block size");

#define GIGABYTE_SYSINFO_CPU_SIGNATURE		{ '_', 'C', 'P', 'U', '_' }
#define GIGABYTE_SYSINFO_CPU_SOCKETS		2
#define GIGABYTE_SYSINFO_CPU_FEATURE_ENABLED	0x0100

/* Each feature is GIGABYTE_SYSINFO_CPU_FEATURE_ENABLED or 0 */
struct gigabyte_sysinfo_cpu_entry {
	uint16_t smt;		/* CPUID 0xb.0 EBX[7:0] > 1 */
	uint16_t vmx;		/* CPUID 0x1 ECX[5] */
	uint16_t nx;		/* CPUID 0x80000001 EDX[20] */
	uint16_t turbo;		/* CPUID 0x6 EAX[1] */
	uint8_t reserved[24];
} __packed;

struct gigabyte_sysinfo_cpu_block {
	uint8_t signature[5];
	uint8_t reserved[3];
	struct gigabyte_sysinfo_cpu_entry cpu[GIGABYTE_SYSINFO_CPU_SOCKETS];
} __packed;

_Static_assert(sizeof(struct gigabyte_sysinfo_cpu_block) == 72, "Wrong _CPU_ block size");

#define GIGABYTE_SYSINFO_MEM_SIGNATURE		{ '_', 'M', 'E', 'M', '_' }
#define GIGABYTE_SYSINFO_MEM_MAX_SIZE		0x1800
#define GIGABYTE_SYSINFO_MEM_MAX_COUNT		0x18

struct gigabyte_sysinfo_mem_block {
	uint8_t signature[5];
	uint8_t reserved0[3];
	uint32_t max_size;	/* Always 0x1800 */
	uint32_t max_count;	/* Always 0x18 */
	uint8_t reserved1[48];
} __packed;

_Static_assert(sizeof(struct gigabyte_sysinfo_mem_block) == 64, "Wrong _MEM_ block size");

#define GIGABYTE_SYSINFO_LAN_SIGNATURE		{ '_', 'L', 'A', 'N', '_' }
#define GIGABYTE_SYSINFO_LAN_MAX_SIZE		1
struct gigabyte_sysinfo_lan_block {
	uint8_t signature[5];
	uint8_t reserved0[5];
	uint8_t max_size;	/* Always 1 */
	uint8_t num_entries;	/* Number of 160-byte entries */
	uint8_t reserved1[20];
} __packed;

_Static_assert(sizeof(struct gigabyte_sysinfo_lan_block) == 32, "Wrong _LAN_ block size");

#define GIGABYTE_SYSINFO_MDATA_SIGNATURE	{ '_', 'M', 'D', 'A', 'T', 'A', '_' }

struct gigabyte_sysinfo_mdata_block {
	uint8_t signature[7];
	uint8_t reserved0;
	uint8_t num_entries;	/* Number of 128-byte entries */
	uint8_t reserved1[23];
} __packed;

_Static_assert(sizeof(struct gigabyte_sysinfo_mdata_block) == 32, "Wrong _MDATA_ block size");

/* Block without a signature, the vendor BIOS always sends it empty */
struct gigabyte_sysinfo_unnamed_block {
	uint8_t reserved0[8];
	uint8_t num_entries;	/* Number of 320-byte entries */
	uint8_t reserved1[23];
} __packed;

_Static_assert(sizeof(struct gigabyte_sysinfo_unnamed_block) == 32, "Wrong unnamed block size");

#define GIGABYTE_SYSINFO_HEADER_SIGNATURE	{ '_', 'H', 'E', 'A', 'D', 'E', 'R', '_' }
#define GIGABYTE_SYSINFO_HEADER_NUM_BLOCKS	11

enum gigabyte_sysinfo_block {
	GIGABYTE_SYSINFO_BLOCK_SMBIOS_TABLE,	/* Without the entry point */
	GIGABYTE_SYSINFO_BLOCK_HD,		/* Each block including its entries */
	GIGABYTE_SYSINFO_BLOCK_CPU,
	GIGABYTE_SYSINFO_BLOCK_MEM,
	GIGABYTE_SYSINFO_BLOCK_LAN,
	GIGABYTE_SYSINFO_BLOCK_MDATA,
	GIGABYTE_SYSINFO_BLOCK_UNNAMED,
};

struct gigabyte_sysinfo_header {
	uint8_t signature[8];
	uint32_t num_blocks;
	uint16_t block_size[GIGABYTE_SYSINFO_HEADER_NUM_BLOCKS];
} __packed;

_Static_assert(sizeof(struct gigabyte_sysinfo_header) == 34, "Wrong _HEADER_ block size");

/* Everything following the SMBIOS table, with no variable-sized entries */
struct gigabyte_sysinfo_trailer {
	struct gigabyte_sysinfo_hd_block hd;
	struct gigabyte_sysinfo_cpu_block cpu;
	struct gigabyte_sysinfo_mem_block mem;
	struct gigabyte_sysinfo_lan_block lan;
	struct gigabyte_sysinfo_mdata_block mdata;
	struct gigabyte_sysinfo_unnamed_block unnamed;
	struct gigabyte_sysinfo_header header;
} __packed;

/*
 * Sends the SMBIOS 3.0 entry point and structure table built by
 * smbios_write_tables() to the BMC. Requires an ASPEED BMC graphics device
 * with its P2A bridge enabled (see ast_ahb.h).
 *
 * Returns CB_SUCCESS on success.
 */
enum cb_err ipmi_gigabyte_send_smbios(const int port);

#endif /* __DRIVERS_IPMI_GIGABYTE_SMBIOS_OEM_H__ */
