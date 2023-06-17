/* SPDX-License-Identifier: GPL-2.0-only */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <getopt.h>
#include <errno.h>
#include <stdarg.h>

#define MiB	(1024*1024)

// TODO: take it as an input
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

static const char *optstring  = "i:a:s:l:o:h";

static struct option long_options[] = {
	{"layout",             required_argument, 0, 'l' },
	{"address",            required_argument, 0, 'a' },
	{"size",               required_argument, 0, 's' },
	{"output",             required_argument, 0, 'o' },
	{"help",                     no_argument, 0, 'h' },
};

static void usage(void)
{
	printf("romholetool: Create ROMHOLE for MSI BIOS updates compatibility\n");
	printf("Usage: romholetool [options] -i build.h -l fmap_config.h -o <filename>\n");
	printf("       romholetool [options] -i build.h -a 0xff7c0000 -s 0x20000 -o <filename>\n");
	printf("-l | --layout  <FILE>             The fmap_config.h file to parse\n");
	printf("-a | --address <HEX>              The address of ROMHOLE in CBFS\n");
	printf("-s | --size    <HEX>              The size of ROMHOLE in CBFS\n");
	printf("-o | --output  <FILE>             The file to generate\n");
	printf("-h | --help                       Print this help\n");
}

static int bcd2int(int hex)
{
	if (hex > 0xff)
		return -1;
	return ((hex & 0xF0) >> 4) * 10 + (hex & 0x0F);
}

static char *get_line(char *fn, char *match)
{
	ssize_t read;
	char *line = NULL;
	char *ret = NULL;
	size_t len = 0;

	FILE *fp = fopen(fn, "r");
	if (fp == NULL) {
		fprintf(stderr, "E: Couldn't open file '%s'\n", fn);
		return NULL;
	}

	while ((read = getline(&line, &len, fp)) != -1) {
		if (strstr(line, match) != NULL) {
			ret = strdup(strstr(line, match) + strlen(match));
			break;
		}
	}

	if (!ret)
		fprintf(stderr, "E: %s not found in %s\n", match, fn);

	fclose(fp);
	return ret;
}

static int get_line_as_int(char *fn, char *match, int bcd)
{
	int ret = -1;
	char *s = get_line(fn, match);
	if (s && strlen(s) > 0) {
		char *endptr;
		ret = strtol(s, &endptr, 0);
		if (*endptr != '\0' && *endptr != '\n') {
			fprintf(stderr, "E: Couldn't parse number for key '%s'\n", match);
			return -1;
		}
		if (bcd)
			ret = bcd2int(ret);
		free(s);
	} else {
		fprintf(stderr, "E: Got invalid line for key '%s'\n", match);
	}

	return ret;
}

int main(int argc, char **argv)
{
	int c, i;
	int ret = 1;
	char *filename = NULL;
	char *layoutfile = NULL;
	char *addr_param = NULL;
	char *size_param = NULL;
	uint32_t flash_start, flash_size, romhole_addr, romhole_size;
	struct msi_romhole *romhole = NULL;

	while (1) {
		int optindex = 0;

		c = getopt_long(argc, argv, optstring, long_options, &optindex);

		if (c == -1)
			break;

		switch (c) {
		case 'a':
			addr_param = strdup(optarg);
			break;
		case 's':
			size_param = strdup(optarg);
			break;
		case 'l':
			layoutfile = strdup(optarg);
			break;
		case 'o':
			filename = strdup(optarg);
			break;
		case 'h':
			ret = 0; /* fallthrough */
		case '?':
			usage();
			goto out;
		default:
			break;
		}
	}
	if (!layoutfile && (!addr_param || !size_param)) {
		fprintf(stderr, "E: Must specify either a fmap_config.h filename or address and size\n");
		goto out;
	}
	if (!filename) {
		fprintf(stderr, "E: Must specify a destination filename\n");
		goto out;
	}

	if (layoutfile) {
		flash_start = get_line_as_int(layoutfile, "FMAP_SECTION_FLASH_START", 0);
		flash_size = get_line_as_int(layoutfile, "FMAP_SECTION_FLASH_SIZE", 0);
		romhole_addr = get_line_as_int(layoutfile, "FMAP_SECTION_ROMHOLE_START", 0);
		romhole_size = get_line_as_int(layoutfile, "FMAP_SECTION_ROMHOLE_SIZE", 0);

		// If flash is greater than 16 MiB, we have to adjust the start offset.
		// Only 16MiB long BIOS region is mapped under 4G.
		romhole_addr -= flash_start;
		if ((flash_size > 16*MiB) && (romhole_addr > 16*MiB))
			romhole_addr -= 16*MiB;

		// The ROMHOLE structure needs the 32bit flash base address in host memory
		romhole_addr += 0xff000000;
	} else if (addr_param && size_param) {
		romhole_addr = strtol(addr_param, NULL, 16);
		romhole_size = strtol(size_param, NULL, 16);
	} else {
		fprintf(stderr, "E: Incorrect input parameters\n");
		goto out;
	}

	if (romhole_size < 0x20000) {
		fprintf(stderr, "E: The romhole flash region should be at least 128K\n");
		goto out;
	}

	romhole = (struct msi_romhole *)malloc(romhole_size);
	if (!romhole) {
		fprintf(stderr, "E: Failed to allocate memory for the romhole\n");
		goto out;
	}

	// Generate the table

	// Header
	romhole->magic0 = ROMHOLE_MAGIC;
	strncpy(romhole->magic1, "SYSD", 4);
	romhole->address = romhole_addr;
	romhole->size = romhole_size;
	memset(romhole->pad0, 0xff, 48);
	romhole->length = (uint16_t)sizeof(*romhole);

	// For some reason the actual data size is reduced by 64K
	if (romhole_size > 0x10000)
		romhole->length -= 0x10000;

	// Structures

	// UUID
	strncpy(romhole->uuid.magic0, "sSiD", 4);
	romhole->uuid.length = (uint16_t)sizeof(struct uuid_structure);
	strncpy(romhole->uuid.magic1, "$uD", 3);
	romhole->uuid.space = 1;
	memset(romhole->uuid.uuid, 0xff, 16);

	// PWD
	strncpy(romhole->pwd.magic, "spWd", 4);
	romhole->pwd.length = (uint16_t)sizeof(struct pwd_structure);
	for (i = 0; i < NUM_PWD_ENTRIES; i++) {
		strncpy(romhole->pwd.entries[i].magic, "$s$", 3);
		romhole->pwd.entries[i].number = i + 1;
		romhole->pwd.entries[i].length = 0;
		memset(romhole->pwd.entries[i].pwd, 0, 20);
	}

	// GPDT
	strncpy(romhole->gpdt.magic, "gPdT", 4);
	romhole->gpdt.length = (uint16_t)sizeof(struct gpdt_structure);
	romhole->gpdt.num_entries = NUM_GD_ENTRIES;
	romhole->gpdt.space = 0;
	for (i = 0; i < NUM_GD_ENTRIES; i++) {
		strncpy(romhole->gpdt.entries[i].magic, "$gD", 3);
		romhole->gpdt.entries[i].number = i + 1;
		romhole->gpdt.entries[i].length = GD_SIZE;
		memset(romhole->gpdt.entries[i].gd, 0xff, GD_SIZE);
	}

	// UCSC
	strncpy(romhole->ucsc.magic, "uCsC", 4);
	romhole->ucsc.length = (uint16_t)sizeof(struct ucsc_structure);
	for (i = 0; i < NUM_US_ENTRIES; i++) {
		strncpy(romhole->ucsc.entries[i].magic, "$uS", 3);
		// The first 6 entries are in order
		if (i <= 5)
			romhole->ucsc.entries[i].number = i + 1;
		// The 7th entry and later start numeration from 'A' ascii
		else
			romhole->ucsc.entries[i].number = 0x41 + i - 6;

		romhole->ucsc.entries[i].length = US_SIZE;
		memset(romhole->ucsc.entries[i].us, 0xff, US_SIZE);
	}

	// T2RW
	strncpy(romhole->t2rw.magic0, "t2Rw", 4);
	romhole->t2rw.length = (uint16_t)sizeof(struct type2_rw);
	strncpy(romhole->t2rw.magic1, "$tW", 3);
	romhole->t2rw.space = 1;
	memset(romhole->t2rw.smbios_type2, 0xff, TYPE2_SIZE);

	// FTB
	strncpy(romhole->ftb.magic0, "$FTB", 4);
	romhole->ftb.length = (uint16_t)sizeof(struct ftb_structure);
	strncpy(romhole->ftb.magic1, "$DIB", 4);
	romhole->ftb.space0 = 2;
	romhole->ftb.size = FTB_SIZE;
	romhole->ftb.space1 = 0;
	strncpy(romhole->ftb.magic2, "MFTD", 4);
	memset(romhole->ftb.data, 0xfe, FTB_SIZE);

	// FIB
	strncpy(romhole->fib.magic0, "$FIB", 4);
	romhole->fib.length = (uint16_t)sizeof(struct fib_structure);
	romhole->fib.size = FIB_SIZE;
	strncpy(romhole->fib.magic1, "$MFX", 4);
	memset(romhole->fib.data, 0xfa, FIB_SIZE);

	// FDP
	strncpy(romhole->fdp.magic0, "$FDP", 4);
	romhole->fdp.length = (uint16_t)sizeof(struct fdp_structure);
	strncpy(romhole->fdp.magic1, "$Fd", 3);
	romhole->fdp.number = 1;
	memset(romhole->fdp.data, 0xff, FDP_SIZE);

	// BUV
	strncpy(romhole->buv.magic0, "$BuV", 4);
	romhole->buv.length = (uint16_t)sizeof(struct buv_structure);
	strncpy(romhole->buv.magic1, "$Bv", 3);
	romhole->buv.number = 1;
	memset(romhole->buv.data, 0xff, BUV_SIZE);

	// RIS
	strncpy(romhole->ris.magic0, "$RiS", 4);
	romhole->ris.length = (uint16_t)sizeof(struct ris_structure);
	strncpy(romhole->ris.magic1, "$RiT", 4);
	// We don't need to cary much about the UEFI volumes. The MSI BIOS update
	// utilities will just copy the right values from the currently running BIOS.
	memset(romhole->ris.volumes, 0xff, sizeof(romhole->ris.volumes));
	for (i = 0; i < NUM_VOLUMES; i++) {
		// It seems there can be up to 8 volumes in total depending on the UEFI image
		// layout, but last 8th entry did not match any of the UEFI volume addresses
		// and additionally 7th entry is skipped.
		if (i == NUM_VOLUMES - 1)
			romhole->ris.volumes[i].number = 0x31 + NUM_VOLUMES; // ascii
		else
			romhole->ris.volumes[i].number = 0x31 + i; // ascii
		romhole->ris.volumes[i].space = 0xff;
	}

	// write the table
	FILE *fd = fopen(filename, "wb");
	if (!fd) {
		fprintf(stderr, "E: %s open failed: %s\n", filename, strerror(errno));
		goto out;
	}

	if (fwrite(romhole, 1, sizeof(*romhole), fd) != sizeof(*romhole)) {
		fprintf(stderr, "E: %s write failed: %s\n", filename, strerror(errno));
		fclose(fd);
		goto out;
	}

	if (fclose(fd)) {
		fprintf(stderr, "E: %s close failed: %s\n", filename, strerror(errno));
		goto out;
	}

	ret = 0;
out:
	if (ret > 0)
		fprintf(stderr, "E: Error creating '%s'\n", filename);

	free(filename);

	if (romhole)
		free(romhole);

	exit(ret);

	return 0;
}
