/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef _ROMHOLE_H_
#define _ROMHOLE_H_

#define ROMHOLE_MAGIC 0x10012501
#define NUM_PWD_ENTRIES 2
#define PWD_SIZE 40
#define NUM_GD_ENTRIES 8
#define GD_SIZE 48
#define NUM_US_ENTRIES 8
#define US_SIZE 0x3851
#define TYPE2_SIZE 256
#define FTB_SIZE 105
#define FIB_SIZE 512
#define FDP_SIZE 1024
#define BUV_SIZE 1024
#define NUM_VOLUMES 7

struct uuid_structure {
	uint8_t			magic0[4];	// always sSiD
	uint16_t		length;		// structure length, always 0x1a
	uint8_t			magic1[3];	// awlays $uD
	uint8_t			space;		// always 0x1
	uint8_t			uuid[16];	// SMBIOS type 1 UUID
} __packed;

struct pwd_entry {
	uint8_t			magic[3];	// always $s$
	uint8_t			number;		// ordinal number
	uint8_t			length;		// length of pwd
	uint8_t			pwd[PWD_SIZE];	// password?
} __packed;

struct pwd_structure {
	uint8_t			magic[4];	// always spWd
	uint16_t		length;		// structure length
	struct pwd_entry	entries[NUM_PWD_ENTRIES];
} __packed;

struct gd_entry {
	uint8_t			magic[3];	// always $gD
	uint8_t			number;		// ordinal number
	uint16_t		length;		// length of gd
	uint8_t			gd[GD_SIZE];	// global data?
} __packed;

struct gpdt_structure {
	uint8_t			magic[4];	// always gPdT
	uint16_t		length;		// structure length
	uint8_t			num_entries;	// number of entries
	uint8_t			space;		// awlays zero
	struct gd_entry		entries[NUM_GD_ENTRIES];
} __packed;

struct us_entry {
	uint8_t			magic[3];	// always $uS
	uint8_t			number;		// ordinal number
	uint16_t		length;		// length of us
	uint8_t			us[US_SIZE];	// user data?
} __packed;

struct ucsc_structure {
	uint8_t			magic[4];	// always uCsC
	uint16_t		length;		// structure length
	struct us_entry		entries[NUM_US_ENTRIES];
} __packed;

struct type2_rw {
	uint8_t			magic0[4];	// always t2Rw
	uint16_t		length;		// structure length
	uint8_t			magic1[3];	// always $tW
	uint8_t			space;		// always 0x1
	uint8_t			smbios_type2[TYPE2_SIZE]; // SMBIOS type2 table
} __packed;

struct ftb_structure {
	uint8_t			magic0[4];	// always $FTB
	uint16_t		length;		// structure length
	uint8_t			magic1[4];	// always $DIB
	uint16_t		space0;		// always 0x2
	uint16_t		size;		// size of data field, always 105
	uint16_t		space1;		// always 0x0
	uint8_t			magic2[4];	// always MFTD
	uint8_t			data[FTB_SIZE];	// default 0xfe
} __packed;

struct fib_structure {
	uint8_t			magic0[4];	// always $FIB
	uint16_t		length;		// structure length
	uint16_t		size;		// size of data field, always 512
	uint8_t			magic1[4];	// always $MFX
	uint8_t			data[FIB_SIZE];	// default 0xfa
} __packed;

/* Structure present in newer releases of MSI PRO Z790-P since A.40/1.40 */
struct fdp_structure {
	uint8_t			magic0[4];	// always $FDP
	uint16_t		length;		// structure length
	uint8_t			magic1[3];	// always $Fd
	uint8_t			number;		// always 0x1
	uint8_t			data[FDP_SIZE];	// default 0xff
} __packed;

/* Structure present in newer releases of MSI PRO Z790-P A.40/1.40 */
struct buv_structure {
	uint8_t			magic0[4];	// always $BuV
	uint16_t		length;		// structure length
	uint8_t			magic1[3];	// always $Bv
	uint8_t			number;		// always 0x1
	uint8_t			data[BUV_SIZE];	// default 0xff
} __packed;

struct volume_entry {
	uint8_t			number;		// ordinal number in ascii
	uint32_t		address;	// volume base address
	uint32_t		size;		// volume size
	uint8_t			space;		// always 0xff
} __packed;

struct ris_structure {
	uint8_t			magic0[4];	// always $RiS
	uint16_t		length;		// structure length
	uint8_t			magic1[4];	// always $RiT
	/* Array below keep the addresses and sizes of UEFI Firmware Volumes */
	struct volume_entry	volumes[NUM_VOLUMES];
} __packed;

/* Place the following struct in the ROMHOLE flash region with 64K/128K alignment */
struct msi_romhole {
	uint32_t		magic0;		// 0x10012501
	uint16_t		length;		// length of the structure
	char			magic1[4];	// always SYSD
	uint32_t		address;	// romhole address in flash
	uint32_t		size;		// romhole total size
	uint8_t			pad0[48];	// always 0xff
	struct uuid_structure	uuid;
	struct pwd_structure	pwd;
	struct gpdt_structure	gpdt;
	struct ucsc_structure	ucsc;
	struct type2_rw 	t2rw;
	struct ftb_structure	ftb;
	struct fib_structure	fib;
	struct fdp_structure	fdp;
	struct buv_structure	buv;
	struct ris_structure	ris;
	// the rest is padded with 0xff by cbfstool
} __packed;

#endif
