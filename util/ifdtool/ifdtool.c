/* ifdtool - dump Intel Firmware Descriptor information */
/* SPDX-License-Identifier: GPL-2.0-only */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <commonlib/helpers.h>
#include <fmap.h>
#include "ifdtool.h"

#ifndef O_BINARY
#define O_BINARY 0
#endif

/**
 * PTR_IN_RANGE - examine whether a pointer falls in [base, base + limit)
 * @param ptr:    the non-void* pointer to a single arbitrary-sized object.
 * @param base:   base address represented with char* type.
 * @param limit:  upper limit of the legal address.
 *
 */
#define PTR_IN_RANGE(ptr, base, limit)			\
	((const char *)(ptr) >= (base) &&		\
	 (const char *)&(ptr)[1] <= (base) + (limit))

/**
 * PLATFORM_HAS_GBE_REGION - some platforms do not support the PCH GbE LAN region
 */
#define PLATFORM_HAS_GBE_REGION (platform != PLATFORM_DNV)

/*
 * PLATFORM_HAS_EC_REGION - some platforms do not support the EC region
 */
#define PLATFORM_HAS_EC_REGION (ifd_version >= IFD_VERSION_2 && platform != PLATFORM_DNV)

/*
 * PLATFORM_HAS_10GBE_X_REGION - some platforms have 1 or more 10GbE LAN regions
 */
#define PLATFORM_HAS_10GBE_0_REGION (platform == PLATFORM_DNV)
#define PLATFORM_HAS_10GBE_1_REGION (platform == PLATFORM_DNV)

static int max_regions_from_fdbar(const struct fdbar *fdb);

static int ifd_version;
static int chipset;
static unsigned int max_regions = 0;
static int selected_chip = 0;
static int platform = -1;

static const struct region_name region_names[MAX_REGIONS] = {
	{ "Flash Descriptor", "fd", "flashregion_0_flashdescriptor.bin", "SI_DESC" },
	{ "BIOS", "bios", "flashregion_1_bios.bin", "SI_BIOS" },
	{ "Intel ME", "me", "flashregion_2_intel_me.bin", "SI_ME" },
	{ "GbE", "gbe", "flashregion_3_gbe.bin", "SI_GBE" },
	{ "Platform Data", "pd", "flashregion_4_platform_data.bin", "SI_PDR" },
	{ "Device Exp1", "devexp", "flashregion_5_device_exp.bin", "SI_DEVICEEXT" },
	{ "Secondary BIOS", "bios2", "flashregion_6_bios2.bin", "SI_BIOS2" },
	{ "Reserved", "res7", "flashregion_7_reserved.bin", NULL },
	{ "EC", "ec", "flashregion_8_ec.bin", "SI_EC" },
	{ "Device Exp2", "devexp2", "flashregion_9_device_exp.bin", "SI_DEVICEEXT2" },
	{ "IE", "ie", "flashregion_10_ie.bin", "SI_IE" },
	{ "10GbE_0", "10gbe_0", "flashregion_11_10gbe0.bin", "SI_10GBE0" },
	{ "10GbE_1", "10gbe_1", "flashregion_12_10gbe1.bin", "SI_10GBE1" },
	{ "Reserved", "res13", "flashregion_13_reserved.bin", NULL },
	{ "Reserved", "res14", "flashregion_14_reserved.bin", NULL },
	{ "PTT", "ptt", "flashregion_15_ptt.bin", "SI_PTT" },
};

/* port from flashrom */
static const char *const ich_chipset_names[] = {
	"Unknown ICH",
	"ICH8",
	"ICH9",
	"ICH10",
	"Unknown PCH",
	"5 series Ibex Peak",
	"6 series Cougar Point",
	"7 series Panther Point",
	"8 series Lynx Point",
	"Baytrail",
	"8 series Lynx Point LP",
	"8 series Wellsburg",
	"9 series Wildcat Point",
	"9 series Wildcat Point LP",
	"Apollo Lake: N3xxx, J3xxx",
	"Gemini Lake: N5xxx, J5xxx, N4xxx, J4xxx",
	"Jasper Lake: N6xxx, N51xx, N45xx",
	"Elkhart Lake: x6000 series Atom",
	"100/200 series Sunrise Point",
	"300 series Cannon Point",
	"400 series Ice Point",
	"500 series Tiger Point/ 600 series Alder Point",
	"800 series Meteor Lake",
	"C620 series Lewisburg",
	"Denverton: C39xx",
	NULL
};

static struct fdbar *find_fd(char *image, int size)
{
	int i, found = 0;

	/* Scan for FD signature */
	for (i = 0; i < (size - 4); i += 4) {
		if (*(uint32_t *) (image + i) == 0x0FF0A55A) {
			found = 1;
			break;	// signature found.
		}
	}

	if (!found) {
		printf("No Flash Descriptor found in this image\n");
		return NULL;
	}

	struct fdbar *fdb = (struct fdbar *) (image + i);
	return PTR_IN_RANGE(fdb, image, size) ? fdb : NULL;
}

static char *find_flumap(char *image, int size)
{
	/* The upper map is located in the word before the 256B-long OEM section
	 * at the end of the 4kB-long flash descriptor. In the official
	 * documentation this is defined as FDBAR + 0xEFC. However, starting
	 * with B-Step of Ibex Peak (5 series) the signature (and thus FDBAR)
	 * has moved 16 bytes back to offset 0x10 of the image. Although
	 * official documentation still maintains the offset relative to FDBAR
	 * this is wrong and a simple fixed offset from the start of the image
	 * works.
	 */
	char *flumap = image + 4096 - 256 - 4;
	return PTR_IN_RANGE(flumap, image, size) ? flumap : NULL;
}

static struct fcba *find_fcba(char *image, int size)
{
	struct fdbar *fdb = find_fd(image, size);
	if (!fdb)
		return NULL;
	struct fcba *fcba = (struct fcba *) (image + ((fdb->flmap0 & 0xff) << 4));
	return PTR_IN_RANGE(fcba, image, size) ? fcba : NULL;

}

static struct fmba *find_fmba(char *image, int size)
{
	struct fdbar *fdb = find_fd(image, size);
	if (!fdb)
		return NULL;
	struct fmba *fmba = (struct fmba *) (image + ((fdb->flmap1 & 0xff) << 4));
	return PTR_IN_RANGE(fmba, image, size) ? fmba : NULL;
}

static struct frba *find_frba(char *image, int size)
{
	struct fdbar *fdb = find_fd(image, size);
	if (!fdb)
		return NULL;
	struct frba *frba =
		(struct frba *) (image + (((fdb->flmap0 >> 16) & 0xff) << 4));
	return PTR_IN_RANGE(frba, image, size) ? frba : NULL;
}

static struct fpsba *find_fpsba(char *image, int size)
{
	struct fdbar *fdb = find_fd(image, size);
	if (!fdb)
		return NULL;
	struct fpsba *fpsba =
		(struct fpsba *) (image + (((fdb->flmap1 >> 16) & 0xff) << 4));

	int SSL = ((fdb->flmap1 >> 24) & 0xff) * sizeof(uint32_t);
	if ((((char *)fpsba) + SSL) >= (image + size))
		return NULL;
	return fpsba;
}

static struct fmsba *find_fmsba(char *image, int size)
{
	struct fdbar *fdb = find_fd(image, size);
	if (!fdb)
		return NULL;
	struct fmsba *fmsba = (struct fmsba *) (image + ((fdb->flmap2 & 0xff) << 4));
	return PTR_IN_RANGE(fmsba, image, size) ? fmsba : NULL;
}

/* port from flashrom */
static enum ich_chipset ifd1_guess_chipset(char *image, int size)
{
	const struct fdbar *fdb = find_fd(image, size);
	if (!fdb)
		exit(EXIT_FAILURE);
	uint32_t iccriba = (fdb->flmap2 >> 16) & 0xff;
	uint32_t msl = (fdb->flmap2 >> 8) & 0xff;
	uint32_t isl = (fdb->flmap1 >> 24);

	/* Rest for IFD1 chipset type */
	if (iccriba == 0x00) {
		if (msl == 0 && isl <= 2)
			return CHIPSET_ICH8;
		else if (isl <= 2)
			return CHIPSET_ICH9;
		else if (isl <= 10)
			return CHIPSET_ICH10;
		else if (isl <= 16)
			return CHIPSET_5_SERIES_IBEX_PEAK;
		printf("Peculiar firmware descriptor, assuming Ibex Peak compatibility.\n");
		return CHIPSET_5_SERIES_IBEX_PEAK;
	} else if (iccriba < 0x31 && (fdb->flmap2 & 0xff) < 0x30) {
		if (msl == 0 && isl <= 17)
			return CHIPSET_BAYTRAIL;
		else if (msl <= 1 && isl <= 18)
			return CHIPSET_6_SERIES_COUGAR_POINT;
		else if (msl <= 1 && isl <= 21)
			return CHIPSET_8_SERIES_LYNX_POINT;
		printf("Peculiar firmware descriptor, assuming Wildcat Point compatibility.\n");
		return CHIPSET_9_SERIES_WILDCAT_POINT;
	}
	return CHIPSET_PCH_UNKNOWN;
}

static enum ich_chipset ifd2_platform_to_chipset(const int pindex)
{
	switch (pindex) {
	case PLATFORM_APL:
		return CHIPSET_N_J_SERIES_APOLLO_LAKE;
	case PLATFORM_GLK:
		return CHIPSET_N_J_SERIES_GEMINI_LAKE;
	case PLATFORM_JSL:
		return CHIPSET_N_SERIES_JASPER_LAKE;
	case PLATFORM_EHL:
		return CHIPSET_x6000_SERIES_ELKHART_LAKE;
	case PLATFORM_SKLKBL:
		return CHIPSET_100_200_SERIES_SUNRISE_POINT;
	case PLATFORM_CNL:
		return CHIPSET_300_SERIES_CANNON_POINT;
	case PLATFORM_TGL:
	case PLATFORM_ADL:
	case PLATFORM_IFD2:
		return CHIPSET_500_600_SERIES_TIGER_ALDER_POINT;
	case PLATFORM_MTL:
		return CHIPSET_800_SERIES_METEOR_LAKE;
	case PLATFORM_ICL:
		return CHIPSET_400_SERIES_ICE_POINT;
	case PLATFORM_LBG:
		return CHIPSET_C620_SERIES_LEWISBURG;
	case PLATFORM_DNV:
		return CHIPSET_DENVERTON;
	case PLATFORM_WBG:
		return CHIPSET_8_SERIES_WELLSBURG;
	default:
		return CHIPSET_PCH_UNKNOWN;
	}
}

/*
 * Some newer platforms have re-defined the FCBA field that was used to
 * distinguish IFD v1 v/s v2. Define a list of platforms that we know do not
 * have the required FCBA field, but are IFD v2 and return true if current
 * platform is one of them.
 */
static int is_platform_ifd_2(void)
{
	static const int ifd_2_platforms[] = {
		PLATFORM_APL,
		PLATFORM_GLK,
		PLATFORM_CNL,
		PLATFORM_LBG,
		PLATFORM_DNV,
		PLATFORM_ICL,
		PLATFORM_TGL,
		PLATFORM_JSL,
		PLATFORM_EHL,
		PLATFORM_ADL,
		PLATFORM_SKLKBL,
		PLATFORM_IFD2,
		PLATFORM_MTL,
		PLATFORM_WBG,
	};
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(ifd_2_platforms); i++) {
		if (platform == ifd_2_platforms[i])
			return 1;
	}

	return 0;
}

static void check_ifd_version(char *image, int size)
{
	const struct fdbar *fdb = find_fd(image, size);

	if (is_platform_ifd_2()) {
		chipset = ifd2_platform_to_chipset(platform);
		if (chipset == CHIPSET_8_SERIES_WELLSBURG)
			ifd_version = IFD_VERSION_1_5;
		else
			ifd_version = IFD_VERSION_2;
		max_regions = MIN(max_regions_from_fdbar(fdb), MAX_REGIONS);
	} else {
		ifd_version = IFD_VERSION_1;
		chipset = ifd1_guess_chipset(image, size);
		max_regions = MIN(max_regions_from_fdbar(fdb), MAX_REGIONS_OLD);
	}
}

static struct region get_region(const struct frba *frba, unsigned int region_type)
{
	int base_mask;
	int limit_mask;
	uint32_t flreg;
	struct region region;

	if (ifd_version >= IFD_VERSION_2)
		base_mask = 0x7fff;
	else
		base_mask = 0xfff;

	limit_mask = base_mask << 16;

	if (region_type >= max_regions) {
		fprintf(stderr, "Invalid region type %d.\n", region_type);
		exit (EXIT_FAILURE);
	}

	flreg = frba->flreg[region_type];
	region.base = (flreg & base_mask) << 12;
	region.limit = ((flreg & limit_mask) >> 4) | 0xfff;
	region.size = region.limit - region.base + 1;
	region.type = region_type;

	if (region.size < 0)
		region.size = 0;

	return region;
}

static void set_region(struct frba *frba, unsigned int region_type,
		       const struct region *region)
{
	if (region_type >= max_regions) {
		fprintf(stderr, "Invalid region type %u.\n", region_type);
		exit (EXIT_FAILURE);
	}

	frba->flreg[region_type] =
		(((region->limit >> 12) & 0x7fff) << 16) |
		((region->base >> 12) & 0x7fff);
}

static const char *region_name(unsigned int region_type)
{
	if (region_type >= max_regions) {
		fprintf(stderr, "Invalid region type.\n");
		exit (EXIT_FAILURE);
	}

	return region_names[region_type].pretty;
}

static int region_num(const char *name)
{
	unsigned int i;

	for (i = 0; i < max_regions; i++) {
		if (strcasecmp(name, region_names[i].pretty) == 0)
			return i;
		if (strcasecmp(name, region_names[i].terse) == 0)
			return i;
	}

	return -1;
}

static void dump_region(unsigned int num, const struct frba *frba)
{
	struct region region = get_region(frba, num);
	printf("  Flash Region %d (%s): %08x - %08x %s\n",
		num, region_name(num), region.base, region.limit,
		region.size < 1 ? "(unused)" : "");
}

static int sort_compare(const void *a, const void *b)
{
	return *(size_t *)a - *(size_t *)b;
}

/*
 * IFDv1 always has 8 regions, while IFDv2 always has 16 regions.
 *
 * It's platform specific which regions are used or are reserved.
 * The 'SPI programming guide' as the name says is a guide only,
 * not a specification what the hardware actually does.
 * The best to do is not to rely on the guide, but detect how many
 * regions are present in the IFD and expose them all.
 *
 * Very early IFDv2 chipsets, sometimes unofficially referred to as
 * IFDv1.5 platforms, only have 8 regions. To not corrupt the IFD when
 * operating on an IFDv1.5 detect how much space is actually present
 * in the IFD.
 */
static int max_regions_from_fdbar(const struct fdbar *fdb)
{
	const size_t fcba = (fdb->flmap0 & 0xff) << 4;
	const size_t fmba = (fdb->flmap1 & 0xff) << 4;
	const size_t frba = ((fdb->flmap0 >> 16) & 0xff) << 4;
	const size_t fpsba = ((fdb->flmap1 >> 16) & 0xff) << 4;
	const size_t flumap = 4096 - 256 - 4;
	size_t sorted[5] = {fcba, fmba, frba, fpsba, flumap};

	qsort(sorted, ARRAY_SIZE(sorted), sizeof(size_t), sort_compare);

	for (size_t i = 0; i < 4; i++) {
		/*
		 * Find FRBA in the sorted array and determine the size of the
		 * region by the start of the next region. Every region requires
		 * 4 bytes of space.
		 */
		if (sorted[i] == frba)
			return MIN((sorted[i+1] - sorted[i])/4, MAX_REGIONS);
	}
	/* Never reaches this point */
	return 0;
}

static void dump_frba(const struct frba *frba)
{
	unsigned int i;
	struct region region;
	printf("Found Region Section\n");
	for (i = 0; i < max_regions; i++) {
		region = get_region(frba, i);
		/* Skip unused & reserved Flash Region */
		if (region.size < 1 && !strcmp(region_name(i), "Reserved"))
			continue;

		printf("FLREG%u:    0x%08x\n", i, frba->flreg[i]);
		dump_region(i, frba);
	}
}

static void dump_flashrom_layout(char *image, int size, const char *layout_fname)
{
	const struct frba *frba = find_frba(image, size);
	if (!frba)
		exit(EXIT_FAILURE);

	int layout_fd = open(layout_fname, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (layout_fd == -1) {
		perror("Could not open file");
		exit(EXIT_FAILURE);
	}

	for (unsigned int i = 0; i < max_regions; i++) {
		struct region region = get_region(frba, i);

		/* A region limit of 0 is an indicator of an unused region
		 * A region base of 7FFFh is an indicator of a reserved region
		 */
		if (region.limit == 0 || region.base == 0x07FFF000)
			continue;

		char buf[LAYOUT_LINELEN];
		snprintf(buf, LAYOUT_LINELEN, "%08x:%08x %s\n", region.base, region.limit, region_names[i].terse);
		if (write(layout_fd, buf, strlen(buf)) < 0) {
			perror("Could not write to file");
			exit(EXIT_FAILURE);
		}
	}
	close(layout_fd);
	printf("Wrote layout to %s\n", layout_fname);
}

static void _decode_spi_frequency(unsigned int freq)
{
	switch (freq) {
	case SPI_FREQUENCY_20MHZ:
		printf("20MHz");
		break;
	case SPI_FREQUENCY_33MHZ:
		printf("33MHz");
		break;
	case SPI_FREQUENCY_48MHZ:
		printf("48MHz");
		break;
	case SPI_FREQUENCY_50MHZ_30MHZ:
		switch (ifd_version) {
		case IFD_VERSION_1:
		case IFD_VERSION_1_5:
			printf("50MHz");
			break;
		case IFD_VERSION_2:
			printf("30MHz");
			break;
		}
		break;
	case SPI_FREQUENCY_17MHZ:
		printf("17MHz");
		break;
	default:
		printf("unknown<%x>MHz", freq);
	}
}

static void _decode_spi_frequency_500_series(unsigned int freq)
{
	switch (freq) {
	case SPI_FREQUENCY_100MHZ:
		printf("100MHz");
		break;
	case SPI_FREQUENCY_50MHZ:
		printf("50MHz");
		break;
	case SPI_FREQUENCY_500SERIES_33MHZ:
		printf("33MHz");
		break;
	case SPI_FREQUENCY_25MHZ:
		printf("25MHz");
		break;
	case SPI_FREQUENCY_14MHZ:
		printf("14MHz");
		break;
	default:
		printf("unknown<%x>MHz", freq);
	}
}

static void decode_spi_frequency(unsigned int freq)
{
	switch (chipset) {
	case CHIPSET_500_600_SERIES_TIGER_ALDER_POINT:
	case CHIPSET_800_SERIES_METEOR_LAKE:
		_decode_spi_frequency_500_series(freq);
		break;
	default:
		_decode_spi_frequency(freq);
	}
}

static void _decode_espi_frequency(unsigned int freq)
{
	switch (freq) {
	case ESPI_FREQUENCY_20MHZ:
		printf("20MHz");
		break;
	case ESPI_FREQUENCY_24MHZ:
		printf("24MHz");
		break;
	case ESPI_FREQUENCY_30MHZ:
		printf("30MHz");
		break;
	case ESPI_FREQUENCY_48MHZ:
		printf("48MHz");
		break;
	case ESPI_FREQUENCY_60MHZ:
		printf("60MHz");
		break;
	case ESPI_FREQUENCY_17MHZ:
		printf("17MHz");
		break;
	default:
		printf("unknown<%x>MHz", freq);
	}
}

static void _decode_espi_frequency_500_series(unsigned int freq)
{
	switch (freq) {
	case ESPI_FREQUENCY_500SERIES_20MHZ:
		printf("20MHz");
		break;
	case ESPI_FREQUENCY_500SERIES_24MHZ:
		printf("24MHz");
		break;
	case ESPI_FREQUENCY_500SERIES_25MHZ:
		printf("25MHz");
		break;
	case ESPI_FREQUENCY_500SERIES_48MHZ:
		printf("48MHz");
		break;
	case ESPI_FREQUENCY_500SERIES_60MHZ:
		printf("60MHz");
		break;
	default:
		printf("unknown<%x>MHz", freq);
	}
}

static void _decode_espi_frequency_800_series(unsigned int freq)
{
	switch (freq) {
	case ESPI_FREQUENCY_800SERIES_20MHZ:
		printf("20MHz");
		break;
	case ESPI_FREQUENCY_800SERIES_25MHZ:
		printf("25MHz");
		break;
	case ESPI_FREQUENCY_800SERIES_33MHZ:
		printf("33MHz");
		break;
	case ESPI_FREQUENCY_800SERIES_50MHZ:
		printf("50MHz");
		break;
	default:
		printf("unknown<%x>MHz", freq);
	}
}

static void decode_espi_frequency(unsigned int freq)
{
	if (chipset == CHIPSET_500_600_SERIES_TIGER_ALDER_POINT)
		_decode_espi_frequency_500_series(freq);
	else if (chipset == CHIPSET_800_SERIES_METEOR_LAKE)
		_decode_espi_frequency_800_series(freq);
	else
		_decode_espi_frequency(freq);
}

static void decode_component_density(unsigned int density)
{
	switch (density) {
	case COMPONENT_DENSITY_512KB:
		printf("512KB");
		break;
	case COMPONENT_DENSITY_1MB:
		printf("1MB");
		break;
	case COMPONENT_DENSITY_2MB:
		printf("2MB");
		break;
	case COMPONENT_DENSITY_4MB:
		printf("4MB");
		break;
	case COMPONENT_DENSITY_8MB:
		printf("8MB");
		break;
	case COMPONENT_DENSITY_16MB:
		printf("16MB");
		break;
	case COMPONENT_DENSITY_32MB:
		printf("32MB");
		break;
	case COMPONENT_DENSITY_64MB:
		printf("64MB");
		break;
	case COMPONENT_DENSITY_UNUSED:
		printf("UNUSED");
		break;
	default:
		printf("unknown<%x>MB", density);
	}
}

static int is_platform_with_pch(void)
{
	if (chipset >= CHIPSET_5_SERIES_IBEX_PEAK)
		return 1;

	return 0;
}

/* FLMAP0 register bit 24 onwards are reserved from SPT PCH */
static int is_platform_with_100x_series_pch(void)
{
	if (chipset >= CHIPSET_100_200_SERIES_SUNRISE_POINT &&
			chipset <= CHIPSET_800_SERIES_METEOR_LAKE)
		return 1;

	return 0;
}

static void dump_fcba(const struct fcba *fcba, const struct fpsba *fpsba)
{
	unsigned int freq;

	printf("\nFound Component Section\n");
	printf("FLCOMP     0x%08x\n", fcba->flcomp);
	printf("  Dual Output Fast Read Support:       %ssupported\n",
		(fcba->flcomp & (1 << 30))?"":"not ");
	printf("  Read ID/Read Status Clock Frequency: ");
	decode_spi_frequency((fcba->flcomp >> 27) & 7);
	printf("\n  Write/Erase Clock Frequency:         ");
	decode_spi_frequency((fcba->flcomp >> 24) & 7);
	printf("\n  Fast Read Clock Frequency:           ");
	decode_spi_frequency((fcba->flcomp >> 21) & 7);
	printf("\n  Fast Read Support:                   %ssupported",
		(fcba->flcomp & (1 << 20))?"":"not ");
	if (is_platform_with_100x_series_pch() &&
			chipset != CHIPSET_100_200_SERIES_SUNRISE_POINT) {
		printf("\n  Read eSPI/EC Bus Frequency:          ");
		if (chipset == CHIPSET_500_600_SERIES_TIGER_ALDER_POINT)
			freq = (fpsba->pchstrp[22] & 0x38) >> 3;
		else if (chipset == CHIPSET_800_SERIES_METEOR_LAKE)
			freq = (fpsba->pchstrp[65] & 0x38) >> 3;
		else
			freq = (fcba->flcomp >> 17) & 7;
		decode_espi_frequency(freq);
	} else {
		printf("\n  Read Clock Frequency:                ");
		decode_spi_frequency((fcba->flcomp >> 17) & 7);
	}

	switch (ifd_version) {
	case IFD_VERSION_1:
		printf("\n  Component 2 Density:                 ");
		decode_component_density((fcba->flcomp >> 3) & 7);
		printf("\n  Component 1 Density:                 ");
		decode_component_density(fcba->flcomp & 7);
		break;
	case IFD_VERSION_1_5:
	case IFD_VERSION_2:
		printf("\n  Component 2 Density:                 ");
		decode_component_density((fcba->flcomp >> 4) & 0xf);
		printf("\n  Component 1 Density:                 ");
		decode_component_density(fcba->flcomp & 0xf);
		break;
	}

	printf("\n");
	printf("FLILL      0x%08x\n", fcba->flill);
	printf("  Invalid Instruction 3: 0x%02x\n",
		(fcba->flill >> 24) & 0xff);
	printf("  Invalid Instruction 2: 0x%02x\n",
		(fcba->flill >> 16) & 0xff);
	printf("  Invalid Instruction 1: 0x%02x\n",
		(fcba->flill >> 8) & 0xff);
	printf("  Invalid Instruction 0: 0x%02x\n",
		fcba->flill & 0xff);
	if (is_platform_with_100x_series_pch()) {
		printf("FLILL1     0x%08x\n", fcba->flpb);
		printf("  Invalid Instruction 7: 0x%02x\n",
			(fcba->flpb >> 24) & 0xff);
		printf("  Invalid Instruction 6: 0x%02x\n",
			(fcba->flpb >> 16) & 0xff);
		printf("  Invalid Instruction 5: 0x%02x\n",
			(fcba->flpb >> 8) & 0xff);
		printf("  Invalid Instruction 4: 0x%02x\n",
			fcba->flpb & 0xff);
	} else {
		printf("FLPB       0x%08x\n", fcba->flpb);
		printf("  Flash Partition Boundary Address: 0x%06x\n\n",
				(fcba->flpb & 0xfff) << 12);
	}
}

static void dump_fpsba(const struct fdbar *fdb, const struct fpsba *fpsba)
{
	unsigned int i;
	/* SoC Straps, aka PSL, aka ISL */
	unsigned int SS = (fdb->flmap1 >> 24) & 0xff;

	printf("Found PCH Strap Section\n");
	for (i = 0; i < SS; i++)
		printf("PCHSTRP%-3u: 0x%08x\n", i, fpsba->pchstrp[i]);

	if (ifd_version >= IFD_VERSION_2) {
		printf("HAP bit is %sset\n",
		       fpsba->pchstrp[0] & (1 << 16) ? "" : "not ");
	} else if (chipset >= CHIPSET_ICH8 && chipset <= CHIPSET_ICH10) {
		printf("ICH_MeDisable bit is %sset\n",
		       fpsba->pchstrp[0] & 1 ? "" : "not ");
	} else {
		printf("AltMeDisable bit is %sset\n",
		       fpsba->pchstrp[10] & (1 << 7) ? "" : "not ");
	}

	printf("\n");
}

static void decode_flmstr(uint32_t flmstr)
{
	int wr_shift, rd_shift;
	if (ifd_version >= IFD_VERSION_2) {
		wr_shift = FLMSTR_WR_SHIFT_V2;
		rd_shift = FLMSTR_RD_SHIFT_V2;
	} else {
		wr_shift = FLMSTR_WR_SHIFT_V1;
		rd_shift = FLMSTR_RD_SHIFT_V1;
	}

	/* EC region access only available on v2+ */
	if (PLATFORM_HAS_EC_REGION)
		printf("  EC Region Write Access:            %s\n",
			(flmstr & (1 << (wr_shift + 8))) ?
			"enabled" : "disabled");
	printf("  Platform Data Region Write Access: %s\n",
			(flmstr & (1 << (wr_shift + 4))) ? "enabled" : "disabled");
	if (PLATFORM_HAS_GBE_REGION) {
		printf("  GbE Region Write Access:           %s\n",
			(flmstr & (1 << (wr_shift + 3))) ? "enabled" : "disabled");
	}
	printf("  Intel ME Region Write Access:      %s\n",
		(flmstr & (1 << (wr_shift + 2))) ? "enabled" : "disabled");
	printf("  Host CPU/BIOS Region Write Access: %s\n",
		(flmstr & (1 << (wr_shift + 1))) ? "enabled" : "disabled");
	printf("  Flash Descriptor Write Access:     %s\n",
		(flmstr & (1 << wr_shift)) ? "enabled" : "disabled");
	if (PLATFORM_HAS_10GBE_0_REGION) {
		printf("  10GbE_0 Write Access:               %s\n",
			(flmstr & (1 << (wr_shift + 11))) ? "enabled" : "disabled");
	}
	if (PLATFORM_HAS_10GBE_1_REGION) {
		printf("  10GbE_1 Write Access:               %s\n",
			(flmstr & (1 << 4)) ? "enabled" : "disabled");
	}

	if (PLATFORM_HAS_EC_REGION)
		printf("  EC Region Read Access:             %s\n",
			(flmstr & (1 << (rd_shift + 8))) ?
			"enabled" : "disabled");
	printf("  Platform Data Region Read Access:  %s\n",
		(flmstr & (1 << (rd_shift + 4))) ? "enabled" : "disabled");
	if (PLATFORM_HAS_GBE_REGION) {
		printf("  GbE Region Read Access:            %s\n",
			(flmstr & (1 << (rd_shift + 3))) ? "enabled" : "disabled");
	}
	printf("  Intel ME Region Read Access:       %s\n",
		(flmstr & (1 << (rd_shift + 2))) ? "enabled" : "disabled");
	printf("  Host CPU/BIOS Region Read Access:  %s\n",
		(flmstr & (1 << (rd_shift + 1))) ? "enabled" : "disabled");
	printf("  Flash Descriptor Read Access:      %s\n",
		(flmstr & (1 << rd_shift)) ? "enabled" : "disabled");
	if (PLATFORM_HAS_10GBE_0_REGION) {
		printf("  10GbE_0 Read Access:                %s\n",
			(flmstr & (1 << (rd_shift + 11))) ? "enabled" : "disabled");
	}
	if (PLATFORM_HAS_10GBE_1_REGION) {
		printf("  10GbE_1 Read Access:                %s\n",
			(flmstr & (1 << 0)) ? "enabled" : "disabled");
	}

	/* Requestor ID doesn't exist for ifd 2 */
	if (ifd_version < IFD_VERSION_2)
		printf("  Requester ID:                      0x%04x\n\n",
			flmstr & 0xffff);
}

static void dump_fmba(const struct fmba *fmba)
{
	printf("Found Master Section\n");
	printf("FLMSTR1:   0x%08x (Host CPU/BIOS)\n", fmba->flmstr1);
	decode_flmstr(fmba->flmstr1);
	printf("FLMSTR2:   0x%08x (Intel ME)\n", fmba->flmstr2);
	decode_flmstr(fmba->flmstr2);
	if (PLATFORM_HAS_GBE_REGION) {
		printf("FLMSTR3:   0x%08x (GbE)\n", fmba->flmstr3);
		decode_flmstr(fmba->flmstr3);
		if (ifd_version >= IFD_VERSION_2) {
			printf("FLMSTR5:   0x%08x (EC)\n", fmba->flmstr5);
			decode_flmstr(fmba->flmstr5);
		}
	} else {
		printf("FLMSTR6:  0x%08x (IE)\n", fmba->flmstr6);
		decode_flmstr(fmba->flmstr6);
	}
}

static void dump_fmsba(const struct fmsba *fmsba)
{
	unsigned int i;
	printf("Found Processor Strap Section\n");
	for (i = 0; i < ARRAY_SIZE(fmsba->data); i++)
		printf("????:      0x%08x\n", fmsba->data[i]);

	if (chipset >= CHIPSET_ICH8 && chipset <= CHIPSET_ICH10) {
		printf("MCH_MeDisable bit is %sset\n",
			fmsba->data[0] & 1 ? "" : "not ");
		printf("MCH_AltMeDisable bit is %sset\n",
			fmsba->data[0] & (1 << 7) ? "" : "not ");
	}
}

static void dump_jid(uint32_t jid)
{
	printf("    SPI Component Vendor ID:            0x%02x\n",
		jid & 0xff);
	printf("    SPI Component Device ID 0:          0x%02x\n",
		(jid >> 8) & 0xff);
	printf("    SPI Component Device ID 1:          0x%02x\n",
		(jid >> 16) & 0xff);
}

static void dump_vscc(uint32_t vscc)
{
	printf("    Lower Erase Opcode:                 0x%02x\n",
		vscc >> 24);
	printf("    Lower Write Enable on Write Status: 0x%02x\n",
		vscc & (1 << 20) ? 0x06 : 0x50);
	printf("    Lower Write Status Required:        %s\n",
		vscc & (1 << 19) ? "Yes" : "No");
	printf("    Lower Write Granularity:            %d bytes\n",
		vscc & (1 << 18) ? 64 : 1);
	printf("    Lower Block / Sector Erase Size:    ");
	switch ((vscc >> 16) & 0x3) {
	case 0:
		printf("256 Byte\n");
		break;
	case 1:
		printf("4KB\n");
		break;
	case 2:
		printf("8KB\n");
		break;
	case 3:
		printf("64KB\n");
		break;
	}

	printf("    Upper Erase Opcode:                 0x%02x\n",
		(vscc >> 8) & 0xff);
	printf("    Upper Write Enable on Write Status: 0x%02x\n",
		vscc & (1 << 4) ? 0x06 : 0x50);
	printf("    Upper Write Status Required:        %s\n",
		vscc & (1 << 3) ? "Yes" : "No");
	printf("    Upper Write Granularity:            %d bytes\n",
		vscc & (1 << 2) ? 64 : 1);
	printf("    Upper Block / Sector Erase Size:    ");
	switch (vscc & 0x3) {
	case 0:
		printf("256 Byte\n");
		break;
	case 1:
		printf("4KB\n");
		break;
	case 2:
		printf("8KB\n");
		break;
	case 3:
		printf("64KB\n");
		break;
	}
}

static void dump_vtba(const struct vtba *vtba, int vtl)
{
	int i;
	int max_len = sizeof(struct vtba)/sizeof(struct vscc);
	int num = (vtl >> 1) < max_len ? (vtl >> 1) : max_len;

	printf("ME VSCC table:\n");
	for (i = 0; i < num; i++) {
		printf("  JID%d:  0x%08x\n", i, vtba->entry[i].jid);
		dump_jid(vtba->entry[i].jid);
		printf("  VSCC%d: 0x%08x\n", i, vtba->entry[i].vscc);
		dump_vscc(vtba->entry[i].vscc);
	}
	printf("\n");
}

static void dump_oem(const uint8_t *oem)
{
	int i, j;
	printf("OEM Section:\n");
	for (i = 0; i < 4; i++) {
		printf("%02x:", i << 4);
		for (j = 0; j < 16; j++)
			printf(" %02x", oem[(i<<4)+j]);
		printf ("\n");
	}
	printf ("\n");
}

static void dump_fd(char *image, int size)
{
	const struct fdbar *fdb = find_fd(image, size);
	if (!fdb)
		exit(EXIT_FAILURE);

	printf("%s", is_platform_with_pch() ? "PCH" : "ICH");
	printf(" Revision: %s\n", ich_chipset_names[chipset]);
	printf("FLMAP0:    0x%08x\n", fdb->flmap0);
	if (!is_platform_with_100x_series_pch())
		printf("  NR:      %d\n", (fdb->flmap0 >> 24) & 7);
	printf("  FRBA:    0x%x\n", ((fdb->flmap0 >> 16) & 0xff) << 4);
	printf("  NC:      %d\n", ((fdb->flmap0 >> 8) & 3) + 1);
	printf("  FCBA:    0x%x\n", ((fdb->flmap0) & 0xff) << 4);

	printf("FLMAP1:    0x%08x\n", fdb->flmap1);
	printf("  %s:     ", is_platform_with_100x_series_pch() ? "PSL" : "ISL");
	printf("0x%02x\n", (fdb->flmap1 >> 24) & 0xff);
	printf("  FPSBA:   0x%x\n", ((fdb->flmap1 >> 16) & 0xff) << 4);
	printf("  NM:      %d\n", (fdb->flmap1 >> 8) & 3);
	printf("  FMBA:    0x%x\n", ((fdb->flmap1) & 0xff) << 4);

	if (!is_platform_with_100x_series_pch()) {
		printf("FLMAP2:    0x%08x\n", fdb->flmap2);
		printf("  PSL:     0x%04x\n", (fdb->flmap2 >> 8) & 0xffff);
		printf("  FMSBA:   0x%x\n", ((fdb->flmap2) & 0xff) << 4);
	}

	if (chipset == CHIPSET_500_600_SERIES_TIGER_ALDER_POINT ||
		 chipset == CHIPSET_800_SERIES_METEOR_LAKE) {
		printf("FLMAP3:    0x%08x\n", fdb->flmap3);
		printf("  Minor Revision ID:     0x%04x\n", (fdb->flmap3 >> 14) & 0x7f);
		printf("  Major Revision ID:     0x%04x\n", (fdb->flmap3 >> 21) & 0x7ff);
	}

	char *flumap = find_flumap(image, size);
	uint32_t flumap1 = *(uint32_t *)flumap;
	printf("FLUMAP1:   0x%08x\n", flumap1);
	printf("  Intel ME VSCC Table Length (VTL):        %d\n",
		(flumap1 >> 8) & 0xff);
	printf("  Intel ME VSCC Table Base Address (VTBA): 0x%06x\n\n",
		(flumap1 & 0xff) << 4);
	dump_vtba((struct vtba *)
			(image + ((flumap1 & 0xff) << 4)),
			(flumap1 >> 8) & 0xff);
	dump_oem((const uint8_t *)image + 0xf00);

	const struct frba *frba = find_frba(image, size);
	const struct fcba *fcba = find_fcba(image, size);
	const struct fpsba *fpsba = find_fpsba(image, size);
	const struct fmba *fmba = find_fmba(image, size);
	const struct fmsba *fmsba = find_fmsba(image, size);

	if (frba && fcba && fpsba && fmba && fmsba) {
		dump_frba(frba);
		dump_fcba(fcba, fpsba);
		dump_fpsba(fdb, fpsba);
		dump_fmba(fmba);
		dump_fmsba(fmsba);
	} else {
		printf("FD is corrupted!\n");
	}
}

/* Takes an image containing an IFD and creates a Flashmap .fmd file template.
 * This flashmap will contain all IFD regions except the BIOS region.
 * The BIOS region is created by coreboot itself and 'should' match the IFD region
 * anyway (CONFIG_VALIDATE_INTEL_DESCRIPTOR should make sure). coreboot built system will use
 * this template to generate the final Flashmap file.
 */
static void create_fmap_template(char *image, int size, const char *layout_fname)
{
	const struct frba *frba = find_frba(image, size);
	if (!frba)
		exit(EXIT_FAILURE);

	int layout_fd = open(layout_fname, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (layout_fd == -1) {
		perror("Could not open file");
		exit(EXIT_FAILURE);
	}

	char *bbuf = "FLASH@##ROM_BASE## ##ROM_SIZE## {\n";
	if (write(layout_fd, bbuf, strlen(bbuf)) < 0) {
		perror("Could not write to file");
		exit(EXIT_FAILURE);
	}

	/* fmaptool requires regions in .fmd to be sorted.
	 * => We need to sort the regions by base address before writing them in .fmd File
	 */
	int count_regions = 0;
	struct region sorted_regions[MAX_REGIONS] = { 0 };
	for (unsigned int i = 0; i < max_regions; i++) {
		struct region region = get_region(frba, i);

		/* A region limit of 0 is an indicator of an unused region
		 * A region base of 7FFFh is an indicator of a reserved region
		 */
		if (region.limit == 0 || region.base == 0x07FFF000)
			continue;

		/* Here we decide to use the coreboot generated FMAP BIOS region, instead of
		 * the one specified in the IFD. The case when IFD and FMAP BIOS region do not
		 * match cannot be caught here, therefore one should still validate IFD and
		 * FMAP via CONFIG_VALIDATE_INTEL_DESCRIPTOR
		 */
		if (i == REGION_BIOS)
			continue;

		sorted_regions[count_regions] = region;
		// basically insertion sort
		for (int i = count_regions-1; i >= 0 ; i--) {
			if (sorted_regions[i].base > sorted_regions[i+1].base) {
				struct region tmp = sorted_regions[i];
				sorted_regions[i] = sorted_regions[i+1];
				sorted_regions[i+1] = tmp;
			}
		}
		count_regions++;
	}

	// Now write regions sorted by base address in the fmap file
	for (int i = 0; i < count_regions; i++) {
		struct region region = sorted_regions[i];
		char buf[LAYOUT_LINELEN];
		snprintf(buf, LAYOUT_LINELEN, "\t%s@0x%X 0x%X\n", region_names[region.type].fmapname, region.base, region.size);
		if (write(layout_fd, buf, strlen(buf)) < 0) {
			perror("Could not write to file");
			exit(EXIT_FAILURE);
		}
	}

	char *ebuf = "\tSI_BIOS@##BIOS_BASE## ##BIOS_SIZE## {\n"
		     "\t\t##CONSOLE_ENTRY##\n"
		     "\t\t##MRC_CACHE_ENTRY##\n"
		     "\t\t##SMMSTORE_ENTRY##\n"
		     "\t\t##SPD_CACHE_ENTRY##\n"
		     "\t\t##VPD_ENTRY##\n"
		     "\t\tFMAP@##FMAP_BASE## ##FMAP_SIZE##\n"
		     "\t\tCOREBOOT(CBFS)@##CBFS_BASE## ##CBFS_SIZE##\n"
		     "\t}\n"
		     "}\n";
	if (write(layout_fd, ebuf, strlen(ebuf)) < 0) {
		perror("Could not write to file");
		exit(EXIT_FAILURE);
	}

	close(layout_fd);
	printf("Wrote layout to %s\n", layout_fname);
}

static void write_regions(char *image, int size)
{
	unsigned int i;
	const struct frba *frba = find_frba(image, size);

	if (!frba)
		exit(EXIT_FAILURE);

	for (i = 0; i < max_regions; i++) {
		struct region region = get_region(frba, i);
		dump_region(i, frba);
		if (region.size > 0) {
			int region_fd;
			region_fd = open(region_names[i].filename,
					 O_WRONLY | O_CREAT | O_TRUNC | O_BINARY,
					 S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
			if (region_fd < 0) {
				perror("Error while trying to open file");
				exit(EXIT_FAILURE);
			}
			if (write(region_fd, image + region.base, region.size) != region.size)
				perror("Error while writing");
			close(region_fd);
		}
	}
}

static void validate_layout(char *image, int size)
{
	uint i, errors = 0;
	struct fmap *fmap;
	long int fmap_loc = fmap_find((uint8_t *)image, size);
	const struct frba *frba = find_frba(image, size);

	if (fmap_loc < 0 || !frba) {
		printf("Could not find FMAP (%p) or Intel Flash Descriptor (%p)\n",
			(void *)fmap_loc, frba);
		exit(EXIT_FAILURE);
	}

	fmap = (struct fmap *)(image + fmap_loc);

	int matches = 0;
	for (i = 0; i < max_regions; i++) {
		struct region region = get_region(frba, i);
		if (region.size == 0)
			continue;

		const struct fmap_area *area = fmap_find_area(fmap, region_names[i].fmapname);
		if (!area)
			continue;

		matches++; // found a match between FMAP and IFD region

		if ((uint)region.base != area->offset || (uint)region.size != area->size) {
			printf("Region mismatch between %s and %s\n", region_names[i].terse, area->name);
			printf(" Descriptor region %s:\n", region_names[i].terse);
			printf("  offset: 0x%08x\n", region.base);
			printf("  length: 0x%08x\n", region.size);
			printf(" FMAP area %s:\n", area->name);
			printf("  offset: 0x%08x\n", area->offset);
			printf("  length: 0x%08x\n", area->size);
			errors++;
		}
	}

	if (!matches) {
		// At least a BIOS region should be present in both IFD and FMAP
		fprintf(stderr, "Warning: Not a single IFD region found in FMAP\n");
	}

	if (errors > 0)
		exit(EXIT_FAILURE);
}

static void write_image(const char *filename, char *image, int size)
{
	int new_fd;
	printf("Writing new image to %s\n", filename);

	// Now write out new image
	new_fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, 0644);
	if (new_fd < 0) {
		perror("Error while trying to open file");
		exit(EXIT_FAILURE);
	}
	if (write(new_fd, image, size) != size)
		perror("Error while writing");
	close(new_fd);
}

static void set_spi_frequency(const char *filename, char *image, int size,
			      enum spi_frequency freq)
{
	struct fcba *fcba = find_fcba(image, size);
	if (!fcba)
		exit(EXIT_FAILURE);

	/* clear bits 21-29 */
	fcba->flcomp &= ~0x3fe00000;
	/* Read ID and Read Status Clock Frequency */
	fcba->flcomp |= freq << 27;
	/* Write and Erase Clock Frequency */
	fcba->flcomp |= freq << 24;
	/* Fast Read Clock Frequency */
	fcba->flcomp |= freq << 21;

	write_image(filename, image, size);
}

static void set_em100_mode(const char *filename, char *image, int size)
{
	struct fcba *fcba = find_fcba(image, size);
	if (!fcba)
		exit(EXIT_FAILURE);

	int freq;

	switch (ifd_version) {
	case IFD_VERSION_1:
	case IFD_VERSION_1_5:
		freq = SPI_FREQUENCY_20MHZ;
		break;
	case IFD_VERSION_2:
		freq = SPI_FREQUENCY_17MHZ;
		break;
	default:
		freq = SPI_FREQUENCY_17MHZ;
		break;
	}

	fcba->flcomp &= ~(1 << 30);
	set_spi_frequency(filename, image, size, freq);
}

static void set_chipdensity(const char *filename, char *image, int size,
			    unsigned int density)
{
	struct fcba *fcba = find_fcba(image, size);
	uint8_t mask, chip2_offset;
	if (!fcba)
		exit(EXIT_FAILURE);

	printf("Setting chip density to ");
	decode_component_density(density);
	printf("\n");

	switch (ifd_version) {
	case IFD_VERSION_1:
		/* fail if selected density is not supported by this version */
		if ( (density == COMPONENT_DENSITY_32MB) ||
		     (density == COMPONENT_DENSITY_64MB) ||
		     (density == COMPONENT_DENSITY_UNUSED) ) {
			printf("error: Selected density not supported in IFD version 1.\n");
			exit(EXIT_FAILURE);
		}
		mask = 0x7;
		chip2_offset = 3;
		break;
	case IFD_VERSION_1_5:
	case IFD_VERSION_2:
		mask = 0xf;
		chip2_offset = 4;
		break;
	default:
		printf("error: Unknown IFD version\n");
		exit(EXIT_FAILURE);
		break;
	}

	/* clear chip density for corresponding chip */
	switch (selected_chip) {
	case 1:
		fcba->flcomp &= ~mask;
		break;
	case 2:
		fcba->flcomp &= ~(mask << chip2_offset);
		break;
	default: /*both chips*/
		fcba->flcomp &= ~(mask | (mask << chip2_offset));
		break;
	}

	/* set the new density */
	if (selected_chip == 1 || selected_chip == 0)
		fcba->flcomp |= (density); /* first chip */
	if (selected_chip == 2 || selected_chip == 0)
		fcba->flcomp |= (density << chip2_offset); /* second chip */

	write_image(filename, image, size);
}

static int check_region(const struct frba *frba, unsigned int region_type)
{
	struct region region;

	if (!frba)
		return 0;

	region = get_region(frba, region_type);
	return !!((region.base < region.limit) && (region.size > 0));
}

static void lock_descriptor(const char *filename, char *image, int size)
{
	int wr_shift, rd_shift;
	struct fmba *fmba = find_fmba(image, size);
	const struct frba *frba = find_frba(image, size);
	if (!fmba)
		exit(EXIT_FAILURE);

	if (ifd_version >= IFD_VERSION_2) {
		wr_shift = FLMSTR_WR_SHIFT_V2;
		rd_shift = FLMSTR_RD_SHIFT_V2;

		/* Clear non-reserved bits */
		fmba->flmstr1 &= 0xff;
		fmba->flmstr2 &= 0xff;
		fmba->flmstr3 &= 0xff;
		fmba->flmstr5 &= 0xff;
	} else {
		wr_shift = FLMSTR_WR_SHIFT_V1;
		rd_shift = FLMSTR_RD_SHIFT_V1;

		fmba->flmstr1 = 0;
		fmba->flmstr2 = 0;
		/* Requestor ID */
		fmba->flmstr3 = 0x118;
	}

	switch (platform) {
	case PLATFORM_APL:
	case PLATFORM_GLK:
		/* CPU/BIOS can read descriptor and BIOS */
		fmba->flmstr1 |= 0x3 << rd_shift;
		/* CPU/BIOS can write BIOS */
		fmba->flmstr1 |= 0x2 << wr_shift;
		/* TXE can read descriptor, BIOS and Device Expansion */
		fmba->flmstr2 |= 0x23 << rd_shift;
		/* TXE can only write Device Expansion */
		fmba->flmstr2 |= 0x20 << wr_shift;
		break;
	case PLATFORM_CNL:
	case PLATFORM_ICL:
	case PLATFORM_SKLKBL:
	case PLATFORM_TGL:
	case PLATFORM_JSL:
	case PLATFORM_EHL:
	case PLATFORM_ADL:
	case PLATFORM_IFD2:
	case PLATFORM_MTL:
		/* CPU/BIOS can read descriptor and BIOS. */
		fmba->flmstr1 |= (1 << REGION_DESC) << rd_shift;
		fmba->flmstr1 |= (1 << REGION_BIOS) << rd_shift;
		/* CPU/BIOS can write BIOS. */
		fmba->flmstr1 |= (1 << REGION_BIOS) << wr_shift;
		/* ME can read descriptor and ME. */
		fmba->flmstr2 |= (1 << REGION_DESC) << rd_shift;
		fmba->flmstr2 |= (1 << REGION_ME) << rd_shift;
		/* ME can write ME. */
		fmba->flmstr2 |= (1 << REGION_ME) << wr_shift;
		if (check_region(frba, REGION_GBE)) {
			/* BIOS can read/write GbE. */
			fmba->flmstr1 |= (1 << REGION_GBE) << rd_shift;
			fmba->flmstr1 |= (1 << REGION_GBE) << wr_shift;
			/* ME can read GbE. */
			fmba->flmstr2 |= (1 << REGION_GBE) << rd_shift;
			/* GbE can read descriptor and read/write GbE.. */
			fmba->flmstr3 |= (1 << REGION_DESC) << rd_shift;
			fmba->flmstr3 |= (1 << REGION_GBE) << rd_shift;
			fmba->flmstr3 |= (1 << REGION_GBE) << wr_shift;
		}
		if (check_region(frba, REGION_PDR)) {
			/* BIOS can read/write PDR. */
			fmba->flmstr1 |= (1 << REGION_PDR) << rd_shift;
			fmba->flmstr1 |= (1 << REGION_PDR) << wr_shift;
		}
		if (check_region(frba, REGION_EC)) {
			/* BIOS can read EC. */
			fmba->flmstr1 |= (1 << REGION_EC) << rd_shift;
			/* EC can read descriptor and read/write EC. */
			fmba->flmstr5 |= (1 << REGION_DESC) << rd_shift;
			fmba->flmstr5 |= (1 << REGION_EC) << rd_shift;
			fmba->flmstr5 |= (1 << REGION_EC) << wr_shift;
		}
		break;
	case PLATFORM_DNV:
	case PLATFORM_WBG:
		/* CPU/BIOS can read descriptor and BIOS. */
		fmba->flmstr1 |= (1 << REGION_DESC) << rd_shift;
		fmba->flmstr1 |= (1 << REGION_BIOS) << rd_shift;
		/* CPU/BIOS can write BIOS. */
		fmba->flmstr1 |= (1 << REGION_BIOS) << wr_shift;
		/* ME can read descriptor and ME. */
		fmba->flmstr2 |= (1 << REGION_DESC) << rd_shift;
		fmba->flmstr2 |= (1 << REGION_ME) << rd_shift;
		/* ME can write ME. */
		fmba->flmstr2 |= (1 << REGION_ME) << wr_shift;
		break;
	default:
		/* CPU/BIOS can read descriptor and BIOS. */
		fmba->flmstr1 |= (1 << REGION_DESC) << rd_shift;
		fmba->flmstr1 |= (1 << REGION_BIOS) << rd_shift;
		/* CPU/BIOS can write BIOS. */
		fmba->flmstr1 |= (1 << REGION_BIOS) << wr_shift;
		/* ME can read descriptor and ME. */
		fmba->flmstr2 |= (1 << REGION_DESC) << rd_shift;
		fmba->flmstr2 |= (1 << REGION_ME) << rd_shift;
		/* ME can write ME. */
		fmba->flmstr2 |= (1 << REGION_ME) << wr_shift;
		if (check_region(frba, REGION_GBE)) {
			/* BIOS can read GbE. */
			fmba->flmstr1 |= (1 << REGION_GBE) << rd_shift;
			/* BIOS can write GbE. */
			fmba->flmstr1 |= (1 << REGION_GBE) << wr_shift;
			/* ME can read GbE. */
			fmba->flmstr2 |= (1 << REGION_GBE) << rd_shift;
			/* ME can write GbE. */
			fmba->flmstr2 |= (1 << REGION_GBE) << wr_shift;
			/* GbE can write GbE. */
			fmba->flmstr3 |= (1 << REGION_GBE) << rd_shift;
			/* GbE can read GbE. */
			fmba->flmstr3 |= (1 << REGION_GBE) << wr_shift;
		}
		break;
	}

	write_image(filename, image, size);
}

static void enable_cpu_read_me(const char *filename, char *image, int size)
{
	int rd_shift;
	struct fmba *fmba = find_fmba(image, size);

	if (!fmba)
		exit(EXIT_FAILURE);

	if (ifd_version >= IFD_VERSION_2)
		rd_shift = FLMSTR_RD_SHIFT_V2;
	else
		rd_shift = FLMSTR_RD_SHIFT_V1;

	/* CPU/BIOS can read ME. */
	fmba->flmstr1 |= (1 << REGION_ME) << rd_shift;

	write_image(filename, image, size);
}

static void unlock_descriptor(const char *filename, char *image, int size)
{
	struct fmba *fmba = find_fmba(image, size);
	if (!fmba)
		exit(EXIT_FAILURE);

	if (ifd_version >= IFD_VERSION_2) {
		/* Access bits for each region are read: 19:8 write: 31:20 */
		fmba->flmstr1 = 0xffffff00 | (fmba->flmstr1 & 0xff);
		fmba->flmstr2 = 0xffffff00 | (fmba->flmstr2 & 0xff);
		fmba->flmstr3 = 0xffffff00 | (fmba->flmstr3 & 0xff);
		fmba->flmstr5 = 0xffffff00 | (fmba->flmstr5 & 0xff);
	} else {
		fmba->flmstr1 = 0xffff0000;
		fmba->flmstr2 = 0xffff0000;
		/* Keep chipset specific Requester ID */
		fmba->flmstr3 = 0x08080000 | (fmba->flmstr3 & 0xffff);
	}

	write_image(filename, image, size);
}

static void set_pchstrap(struct fpsba *fpsba, const struct fdbar *fdb, const int strap,
			const unsigned int value)
{
	if (!fpsba || !fdb) {
		fprintf(stderr, "Internal error\n");
		exit(EXIT_FAILURE);
	}

	/* SoC Strap, aka PSL, aka ISL */
	int SS = (fdb->flmap1 >> 24) & 0xff;
	if (strap >= SS) {
		fprintf(stderr, "Strap index %d out of range (max: %d)\n", strap, SS);
		exit(EXIT_FAILURE);
	}
	fpsba->pchstrp[strap] = value;
}

static int ifd2_platform_get_hap_location(struct fpsba *fpsba)
{
	switch (platform) {
	case PLATFORM_SKLKBL:
		/* No exceptions, always strap 0 */
		return 0;
	case PLATFORM_IFD2:
		printf("To change the AltMeDisable/HAP bit please provide the platform as parameter\n");
		return -1;
	case PLATFORM_CNL:
		/*
		 * This gets little tricky here.
		 * 1. CNL/CFL/CML-LP strap 106 (FPSBA + 0x70), but
		 *    FSPBA + 0x70 bits 15:4 set to 0x1b
		 *    FPSBA + 0x80 strap has configurable bits,
		 *    FPSBA + 0x82/0x83 bytes are set to zeros,
		 *    FPSBA + 0x00 is set to zeros,
		 * 2. CML-V strap 0 (FPSBA + 0x00) with bits 15:2 set to 0x00, but
		 *    FSPBA + 0x00 bits 15:2 must be set to 0,
		 *    FPSBA + 0x00 bits 23:22 must be set to 0x2,
		 *    FPSBA + 0x70 is set to all zeros,
		 *    FPSBA + 0x80 byte has configurable bits,
		 *    FPSBA + 0x81/0x82 bytes are set to zeros.
		 * 3. CML-H/S strap 123 (FPSBA + 0x80), but
		 *    FSPBA + 0x80 bits 15:4 set to 0x1b
		 *    FPSBA + 0x00/0x70 is set to all zeros.
		 *
		 * So we do like this:
		 *
		 * 1. Prioritize CML-V detection as strap 0 cannot be all zeros.
		 * 2. Then heck if FSPBA + 0x70 is zero for CML-H/S, CNL/CFL/CML-LP
		 *    cannot have FPSBA + 0x70 all zeros.
		 */
		if (fpsba->pchstrp[0] != 0) {
			/* CML-V, one more safetycheck ot exclude other SKUs */
			if (fpsba->pchstrp[0x70 / 4] == 0) {
				printf("Detected Comet Lake V platform flash descriptor\n");
				return 0;
			}
		} else {
			/* Do safety check for CML-H/S to be sure */
			if (fpsba->pchstrp[0x70 / 4] == 0) {
				printf("Detected Comet Lake H/S platform flash descriptor\n");
				return 0x80 / 4;
			}
			/* Do safety check for CNL/CFL/CML-LP to be sure */
			if (((fpsba->pchstrp[0x70 / 4] >> 4) & 0xfff) == 0x1b) {
				printf("Detected Cannon Lake/Coffee Lake/Comet Lake LP "
				       "platform flash descriptor\n");
				return 0x70 / 4;
			}
		}
		printf("Unusual Cannon Lake/Coffee Lake/Comet Lake flash descriptor detected\n"
		       "Not modifying flash descriptor\n");
		return -1;
	case PLATFORM_TGL:
		/*
		 * RKL-H/TGL-H strap 141 (FPSBA + 0x94), but
		 * FPSBA + 0x94 bits 8:1 set to 0x18.
		 * FPSBA + 0x7c must be zero.
		 * TGL-LP has non-zero FPSBA + 0x7c.
		 */
		if ((((fpsba->pchstrp[0x94 / 4] >> 1) & 0xff) == 0x18) &&
		    (fpsba->pchstrp[0x7c / 4] == 0)) {
			printf("Detected Rocket Lake H/Tiger Lake H platform "
			       "flash descriptor\n");
			return 0x94 / 4;
		}
		/*
		 * TGL-LP strap 114 (FPSBA + 0x7c), but
		 * FSPBA + 0x7c bits 8:1 set to 0x18
		 * FPSBA + 0x92/0x93/0x94/0x95 must be all zeros.
		 * TGL-H has non-zero FPSBA + 0x94/0x95.
		 */
		if ((((fpsba->pchstrp[0x7c / 4] >> 1) & 0xff) == 0x18) &&
		    (((fpsba->pchstrp[0x94 / 4] >> 16) & 0xffff) == 0)) {
			printf("Detected Tiger Lake LP platform flash descriptor\n");
			return 0x7c / 4;
		}

		printf("Unusual Tiger Lake flash descriptor detected\n"
		       "Not modifying flash descriptor\n");
		return -1;
	case PLATFORM_ADL:
		/*
		 * This gets little tricky here.
		 * 1. RPL-P/ADL-N/P strap 114 (FSPBA + 0x7c), but
		 *    FPSBA + 0x7c bits 8:1 set to 0x18,
		 *    FPSBA + 0xdc strap has configurable bits,
		 *    FPSBA + 0xd0 must be set to 0x00000300.
		 * 2. ADL-S B0 step/RPL-S strap 193 (FSPBA + 0xdc), but
		 *    FPSBA + 0xdc bits 8:1 set to 0x58,
		 *    FPSBA + 0x7c strap has configurable bits,
		 *    FPSBA + 0xd0 must be set to 0x10081008.
		 * 3. ADL-S A0 step strap 183 (FSPBA + 0xd0), but
		 *    FPSBA + 0xd0 bits 8:1 set to 0x58,
		 *    FPSBA + 0x7c/0xdc straps have configurable bits, however
		 *    analogically to 0xdc vs. 0xd0, the FPSBA + (0xd0 - 0x0c)
		 *    must be 0x10081008.
		 *
		 * So we do like this:
		 *
		 * 1. Prioritize ADL-S detection by checking 0xd0/0xdc bits 8:1
		 *    and 0xd0/0xc4 straps are set to 0x10081008.
		 * 2. ADL-S and ADL-N/P is mutually exclusive due to 0xd0
		 *    reserved values. SO 0xd0 can be used to distinguish all
		 *    SKUs.
		 */
		if (fpsba->pchstrp[0xd0 / 4] == 0x10081008) {
			/* ADL-S B0 step/RPL-S, do safety check to be sure */
			if (((fpsba->pchstrp[0xdc / 4] >> 1) & 0xff) == 0x58) {
				printf("Detected Raptor Lake S/Alder Lake S B0"
				       " step platform flash descriptor\n");
				return 0xdc / 4;
			}
		} else if (fpsba->pchstrp[0xd0 / 4] == 0x00000300) {
			/* RPL-P/ADL-N/P, do safety check to be sure */
			if (((fpsba->pchstrp[0x7c / 4] >> 1) & 0xff) == 0x18) {
				printf("Detected Raptor Lake P/Alder Lake P/N"
				       " platform flash descriptor\n");
				return 0x7c / 4;
			}
		} else if (((fpsba->pchstrp[0xd0 / 4] >> 1) & 0xff) == 0x58) {
			/* ADL-S A0 step, do safety check to be sure */
			if (fpsba->pchstrp[0xc4 / 4] == 0x10081008) {
				printf("Detected Alder Lake S A0 step platform"
				       " flash descriptor\n");
				return 0xd0 / 4;
			}
		}
		printf("Unusual Alder Lake flash descriptor detected\n"
		       "Not modifying flash descriptor\n");
		return -1;
	case PLATFORM_MTL:
		/* MTL-U/H strap 225 (FPSBA + 0x11c) */
		return 0x11c / 4;
	case PLATFORM_ICL:
		/*
		 * ICX-D strap 56 (FPSBA + 0xe0), unconfirmed.
		 * ICX-D FPSBA + 0x78 must be zero.
		 */
		if ((((fpsba->pchstrp[0xe0 / 4] >> 1) & 0xff) == 0x98) &&
		    (fpsba->pchstrp[0x78 / 4] == 0)) {
			printf("Detected Ice Lake D platform flash descriptor\n");
			return 0xe0 / 4;
		}
		/*
		 * ICL-LP strap 113 (FPSBA + 0x78), but
		 * FSPBA + 0x78 bits 9:1 set to 0x98.
		 * FPSBA + 0xe0 must be 0x68070000.
		 */
		if ((((fpsba->pchstrp[0x78 / 4] >> 1) & 0xff) == 0x98) &&
		    (fpsba->pchstrp[0xe0 / 4] == 0x68070000)) {
			printf("Detected Ice Lake LP platform flash descriptor\n");
			return 0x94 / 4;
		}

		printf("Unusual Ice Lake flash descriptor detected\n"
		       "Not modifying flash descriptor\n");
		return -1;
	case PLATFORM_LBG:
		/* LBG strap 53 (FPSBA + 0xd4) */
		return 0xd4 / 4;
	case PLATFORM_DNV:
		/* DNV strap 14 (FPSBA + 0x38) */
		return 0x38 / 4;
	case PLATFORM_APL:
	case PLATFORM_GLK:
		/*
		 * Strap 10 (FPSBA + 0x2c), but can be only set through SMIP?
		 * Couldn't get it to work on GLK.
		 */
		return 0x2c / 4;
	case PLATFORM_JSL:
		/* JSL strap 113 (FSPBA + 0x68) */
		return 0x68 / 4;
	case PLATFORM_EHL:
		/* EHL strap 122 (FSPBA + 0x84) */
		return 0x84 / 4;
	default:
		printf("HAP bit location unknown for selected platform\n"
		       "Not modifying flash descriptor\n");
		return -1;
	}
}

/* Set the AltMeDisable (or HAP for >= IFD_VERSION_2) */
static void fpsba_set_altmedisable(struct fpsba *fpsba, struct fmsba *fmsba, bool altmedisable)
{
	int loc;

	if (ifd_version >= IFD_VERSION_2) {
		loc = ifd2_platform_get_hap_location(fpsba);

		if (loc == -1)
			return;

		printf("%sting the HAP bit to %s Intel ME...\n",
			altmedisable?"Set":"Unset",
			altmedisable?"disable":"enable");
		/* Special case for APL/GLK */
		if (platform == PLATFORM_APL || platform == PLATFORM_GLK) {
			if (altmedisable)
				fpsba->pchstrp[loc] |= (1 << 7);
			else
				fpsba->pchstrp[loc] &= ~(1 << 7);
		} else {
			if (altmedisable)
				fpsba->pchstrp[loc] |= (1 << 16);
			else
				fpsba->pchstrp[loc] &= ~(1 << 16);
		}

	} else {
		if (chipset >= CHIPSET_ICH8 && chipset <= CHIPSET_ICH10) {
			printf("%sting the ICH_MeDisable, MCH_MeDisable, "
			       "and MCH_AltMeDisable to %s Intel ME...\n",
			       altmedisable?"Set":"Unset",
			       altmedisable?"disable":"enable");
			if (altmedisable) {
				/* MCH_MeDisable */
				fmsba->data[0] |= 1;
				/* MCH_AltMeDisable */
				fmsba->data[0] |= (1 << 7);
				/* ICH_MeDisable */
				fpsba->pchstrp[0] |= 1;
			} else {
				fmsba->data[0] &= ~1;
				fmsba->data[0] &= ~(1 << 7);
				fpsba->pchstrp[0] &= ~1;
			}
		} else {
			printf("%sting the AltMeDisable to %s Intel ME...\n",
				altmedisable?"Set":"Unset",
				altmedisable?"disable":"enable");
			if (altmedisable)
				fpsba->pchstrp[10] |= (1 << 7);
			else
				fpsba->pchstrp[10] &= ~(1 << 7);
		}
	}
}

static void inject_region(const char *filename, char *image, int size,
			  unsigned int region_type, const char *region_fname)
{
	struct frba *frba = find_frba(image, size);
	if (!frba)
		exit(EXIT_FAILURE);

	struct region region = get_region(frba, region_type);
	if (region.size <= 0xfff) {
		fprintf(stderr, "Region %s is disabled in target. Not injecting.\n",
				region_name(region_type));
		exit(EXIT_FAILURE);
	}

	int region_fd = open(region_fname, O_RDONLY | O_BINARY);
	if (region_fd == -1) {
		perror("Could not open file");
		exit(EXIT_FAILURE);
	}
	struct stat buf;
	if (fstat(region_fd, &buf) == -1) {
		perror("Could not stat file");
		exit(EXIT_FAILURE);
	}
	int region_size = buf.st_size;

	printf("File %s is %d bytes\n", region_fname, region_size);

	if (region_size > region.size) {
		fprintf(stderr, "Region %s is %d(0x%x) bytes. File is %d(0x%x)"
				" bytes. Not injecting.\n",
				region_name(region_type), region.size,
				region.size, region_size, region_size);
		exit(EXIT_FAILURE);
	}

	int offset = 0;
	if ((region_type == 1) && (region_size < region.size)) {
		fprintf(stderr, "Region %s is %d(0x%x) bytes. File is %d(0x%x)"
				" bytes. Padding before injecting.\n",
				region_name(region_type), region.size,
				region.size, region_size, region_size);
		offset = region.size - region_size;
		memset(image + region.base, 0xff, offset);
	}

	if (size < region.base + offset + region_size) {
		fprintf(stderr, "Output file is too small. (%d < %d)\n",
			size, region.base + offset + region_size);
		exit(EXIT_FAILURE);
	}

	if (read(region_fd, image + region.base + offset, region_size) != region_size) {
		perror("Could not read file");
		exit(EXIT_FAILURE);
	}

	close(region_fd);

	printf("Adding %s as the %s section of %s\n",
	       region_fname, region_name(region_type), filename);
	write_image(filename, image, size);
}

static unsigned int next_pow2(unsigned int x)
{
	unsigned int y = 1;
	if (x == 0)
		return 0;
	while (y <= x)
		y = y << 1;

	return y;
}

/**
 * Determine if two memory regions overlap.
 *
 * @param r1, r2 Memory regions to compare.
 * @return 0 if the two regions are separate
 * @return 1 if the two regions overlap
 */
static int regions_collide(const struct region *r1, const struct region *r2)
{
	if ((r1->size == 0) || (r2->size == 0))
		return 0;

	/* r1 should be either completely below or completely above r2 */
	return !(r1->limit < r2->base || r1->base > r2->limit);
}

static void new_layout(const char *filename, char *image, int size,
		const char *layout_fname)
{
	FILE *romlayout;
	char tempstr[256];
	char layout_region_name[256];
	unsigned int i, j;
	int region_number;
	struct region current_regions[MAX_REGIONS];
	struct region new_regions[MAX_REGIONS];
	int new_extent = 0;
	char *new_image;

	/* load current descriptor map and regions */
	struct frba *frba = find_frba(image, size);
	if (!frba)
		exit(EXIT_FAILURE);

	for (i = 0; i < max_regions; i++) {
		current_regions[i] = get_region(frba, i);
		new_regions[i] = get_region(frba, i);
	}

	/* read new layout */
	romlayout = fopen(layout_fname, "r");

	if (!romlayout) {
		perror("Could not read layout file.\n");
		exit(EXIT_FAILURE);
	}

	while (!feof(romlayout)) {
		char *tstr1, *tstr2;

		if (2 != fscanf(romlayout, "%255s %255s\n", tempstr,
					layout_region_name))
			continue;

		region_number = region_num(layout_region_name);
		if (region_number < 0)
			continue;

		tstr1 = strtok(tempstr, ":");
		tstr2 = strtok(NULL, ":");
		if (!tstr1 || !tstr2) {
			fprintf(stderr, "Could not parse layout file.\n");
			exit(EXIT_FAILURE);
		}
		new_regions[region_number].base = strtol(tstr1,
				(char **)NULL, 16);
		new_regions[region_number].limit = strtol(tstr2,
				(char **)NULL, 16);
		new_regions[region_number].size =
			new_regions[region_number].limit -
			new_regions[region_number].base + 1;

		if (new_regions[region_number].size < 0)
			new_regions[region_number].size = 0;
	}
	fclose(romlayout);

	/* check new layout */
	for (i = 0; i < max_regions; i++) {
		if (new_regions[i].size == 0)
			continue;

		if (new_regions[i].size < current_regions[i].size) {
			printf("DANGER: Region %s is shrinking.\n",
					region_name(i));
			printf("    The region will be truncated to fit.\n");
			printf("    This may result in an unusable image.\n");
		}

		for (j = i + 1; j < max_regions; j++) {
			if (regions_collide(&new_regions[i], &new_regions[j])) {
				fprintf(stderr, "Regions would overlap.\n");
				exit(EXIT_FAILURE);
			}
		}

		/* detect if the image size should grow */
		if (new_extent < new_regions[i].limit)
			new_extent = new_regions[i].limit;
	}

	/* check if the image is actually a Flash Descriptor region */
	if (size == new_regions[0].size) {
		printf("The image is a single Flash Descriptor:\n");
		printf("    Only the descriptor will be modified\n");
		new_extent = size;
	} else {
		new_extent = next_pow2(new_extent - 1);
		if (new_extent != size) {
			printf("The image has changed in size.\n");
			printf("The old image is %d bytes.\n", size);
			printf("The new image is %d bytes.\n", new_extent);
		}
	}

	/* copy regions to a new image */
	new_image = malloc(new_extent);
	memset(new_image, 0xff, new_extent);
	for (i = 0; i < max_regions; i++) {
		int copy_size = new_regions[i].size;
		int offset_current = 0, offset_new = 0;
		const struct region *current = &current_regions[i];
		const struct region *new = &new_regions[i];

		if (new->size == 0)
			continue;

		if (new->size > current->size) {
			/* copy from the end of the current region */
			copy_size = current->size;
			if (i == REGION_BIOS)
				offset_new = new->size - current->size;
		}

		if ((i == REGION_BIOS) && (new->size < current->size)) {
			/* copy BIOS region to the end of the new region */
			offset_current = current->size - new->size;
		}

		if (size < current->base + offset_current + copy_size) {
			printf("Skip descriptor %d (%s) (region missing in the old image)\n", i,
				region_name(i));
			continue;
		};

		printf("Copy Descriptor %d (%s) (%d bytes)\n", i,
				region_name(i), copy_size);
		printf("   from %08x+%08x:%08x (%10d)\n", current->base,
				offset_current, current->limit, current->size);
		printf("     to %08x+%08x:%08x (%10d)\n", new->base,
				offset_new, new->limit, new->size);

		memcpy(new_image + new->base + offset_new,
				image + current->base + offset_current,
				copy_size);
	}

	/* update new descriptor regions */
	frba = find_frba(new_image, new_extent);
	if (!frba)
		exit(EXIT_FAILURE);

	printf("Modify Flash Descriptor regions\n");
	for (i = 1; i < max_regions; i++)
		set_region(frba, i, &new_regions[i]);

	write_image(filename, new_image, new_extent);
	free(new_image);
}

static void print_version(void)
{
	printf("ifdtool v%s -- ", IFDTOOL_VERSION);
	printf("Copyright (C) 2011 Google Inc.\n\n");
	printf
	    ("This program is free software: you can redistribute it and/or modify\n"
	     "it under the terms of the GNU General Public License as published by\n"
	     "the Free Software Foundation, version 2 of the License.\n\n"
	     "This program is distributed in the hope that it will be useful,\n"
	     "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
	     "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
	     "GNU General Public License for more details.\n\n");
}

static void print_usage(const char *name)
{
	printf("usage: %s [-vhdix?] <filename>\n", name);
	printf("\n"
	       "   -d | --dump:                          dump intel firmware descriptor\n"
	       "   -f | --layout <filename>              dump regions into a flashrom layout file\n"
	       "   -F | --fmap-layout <filename>         dump IFD regions into a fmap layout template (.fmd) file\n"
	       "   -t | --validate                       Validate that the firmware descriptor layout matches the fmap layout\n"
	       "   -x | --extract:                       extract intel fd modules\n"
	       "   -i | --inject <region>:<module>       inject file <module> into region <region>\n"
	       "   -n | --newlayout <filename>           update regions using a flashrom layout file\n"
	       "   -O | --output <filename>              output filename\n"
	       "   -s | --spifreq <17|20|30|33|48|50>    set the SPI frequency\n"
	       "   -D | --density <512|1|2|4|8|16|32|64> set chip density (512 in KByte, others in MByte)\n"
	       "   -C | --chip <0|1|2>                   select spi chip on which to operate\n"
	       "                                         can only be used once per run:\n"
	       "                                         0 - both chips (default), 1 - first chip, 2 - second chip\n"
	       "   -e | --em100                          set SPI frequency to 20MHz and disable\n"
	       "                                         Dual Output Fast Read Support\n"
	       "   -l | --lock                           Lock firmware descriptor and ME region\n"
	       "   -r | --read				 Enable CPU/BIOS read access for ME region\n"
	       "   -u | --unlock                         Unlock firmware descriptor and ME region\n"
	       "   -M | --altmedisable <0|1>             Set the MeDisable and AltMeDisable (or HAP for skylake or newer platform)\n"
	       "                                         bits to disable ME\n"
	       "   -p | --platform                       Add platform-specific quirks\n"
	       "                                         adl    - Alder Lake\n"
	       "                                         aplk   - Apollo Lake\n"
	       "                                         cnl    - Cannon Lake\n"
	       "                                         lbg    - Lewisburg PCH\n"
	       "                                         dnv    - Denverton\n"
	       "                                         ehl    - Elkhart Lake\n"
	       "                                         glk    - Gemini Lake\n"
	       "                                         icl    - Ice Lake\n"
	       "                                         ifd2   - IFDv2 Platform\n"
	       "                                         jsl    - Jasper Lake\n"
	       "                                         mtl    - Meteor Lake\n"
	       "                                         sklkbl - Sky Lake/Kaby Lake\n"
	       "                                         tgl    - Tiger Lake\n"
	       "                                         wbg    - Wellsburg\n"
	       "   -S | --setpchstrap                    Write a PCH strap\n"
	       "   -V | --newvalue                       The new value to write into PCH strap specified by -S\n"
	       "   -v | --version:                       print the version\n"
	       "   -h | --help:                          print this help\n\n"
	       "<region> is one of Descriptor, BIOS, ME, GbE, Platform Data, Secondary BIOS, "
	       "Device Exp1, EC, Device Exp2, IE, 10GbE_0, 10GbE_1, PTT\n"
	       "\n");
}

int main(int argc, char *argv[])
{
	int opt, option_index = 0;
	int mode_dump = 0, mode_extract = 0, mode_inject = 0, mode_spifreq = 0;
	int mode_em100 = 0, mode_locked = 0, mode_unlocked = 0, mode_validate = 0;
	int mode_layout = 0, mode_newlayout = 0, mode_density = 0, mode_setstrap = 0;
	int mode_read = 0, mode_altmedisable = 0, altmedisable = 0, mode_fmap_template = 0;
	char *region_type_string = NULL, *region_fname = NULL;
	const char *layout_fname = NULL;
	char *new_filename = NULL;
	int region_type = -1, inputfreq = 0;
	unsigned int value = 0;
	unsigned int pchstrap = 0;
	unsigned int new_density = 0;
	enum spi_frequency spifreq = SPI_FREQUENCY_20MHZ;

	static const struct option long_options[] = {
		{"dump", 0, NULL, 'd'},
		{"layout", 1, NULL, 'f'},
		{"fmap-template", 1, NULL, 'F'},
		{"extract", 0, NULL, 'x'},
		{"inject", 1, NULL, 'i'},
		{"newlayout", 1, NULL, 'n'},
		{"output", 1, NULL, 'O'},
		{"spifreq", 1, NULL, 's'},
		{"density", 1, NULL, 'D'},
		{"chip", 1, NULL, 'C'},
		{"altmedisable", 1, NULL, 'M'},
		{"em100", 0, NULL, 'e'},
		{"lock", 0, NULL, 'l'},
		{"read", 0, NULL, 'r'},
		{"unlock", 0, NULL, 'u'},
		{"version", 0, NULL, 'v'},
		{"help", 0, NULL, 'h'},
		{"platform", 0, NULL, 'p'},
		{"validate", 0, NULL, 't'},
		{"setpchstrap", 1, NULL, 'S'},
		{"newvalue", 1, NULL, 'V'},
		{0, 0, 0, 0}
	};

	while ((opt = getopt_long(argc, argv, "S:V:df:F:D:C:M:xi:n:O:s:p:elruvth?",
					long_options, &option_index)) != EOF) {
		switch (opt) {
		case 'd':
			mode_dump = 1;
			break;
		case 'S':
			mode_setstrap = 1;
			pchstrap = strtoul(optarg, NULL, 0);
			break;
		case 'V':
			value = strtoul(optarg, NULL, 0);
			break;
		case 'f':
			mode_layout = 1;
			layout_fname = strdup(optarg);
			if (!layout_fname) {
				fprintf(stderr, "No layout file specified\n");
				fprintf(stderr, "run '%s -h' for usage\n", argv[0]);
				exit(EXIT_FAILURE);
			}
			break;
		case 'F':
			mode_fmap_template = 1;
			layout_fname = strdup(optarg);
			if (!layout_fname) {
				fprintf(stderr, "No layout file specified\n");
				fprintf(stderr, "run '%s -h' for usage\n", argv[0]);
				exit(EXIT_FAILURE);
			}
			break;
		case 'x':
			mode_extract = 1;
			break;
		case 'i':
			// separate type and file name
			region_type_string = strdup(optarg);
			region_fname = strchr(region_type_string, ':');
			if (!region_fname) {
				fprintf(stderr, "run '%s -h' for usage\n", argv[0]);
				exit(EXIT_FAILURE);
			}
			region_fname[0] = '\0';
			region_fname++;
			// Descriptor, BIOS, ME, GbE, Platform
			// valid type?
			if (!strcasecmp("Descriptor", region_type_string))
				region_type = 0;
			else if (!strcasecmp("BIOS", region_type_string))
				region_type = 1;
			else if (!strcasecmp("ME", region_type_string))
				region_type = 2;
			else if (!strcasecmp("GbE", region_type_string))
				region_type = 3;
			else if (!strcasecmp("Platform Data", region_type_string))
				region_type = 4;
			else if (!strcasecmp("Device Exp1", region_type_string))
				region_type = 5;
			else if (!strcasecmp("Secondary BIOS", region_type_string))
				region_type = 6;
			else if (!strcasecmp("Reserved", region_type_string))
				region_type = 7;
			else if (!strcasecmp("EC", region_type_string))
				region_type = 8;
			else if (!strcasecmp("Device Exp2", region_type_string))
				region_type = 9;
			else if (!strcasecmp("IE", region_type_string))
				region_type = 10;
			else if (!strcasecmp("10GbE_0", region_type_string))
				region_type = 11;
			else if (!strcasecmp("10GbE_1", region_type_string))
				region_type = 12;
			else if (!strcasecmp("PTT", region_type_string))
				region_type = 15;
			if (region_type == -1) {
				fprintf(stderr, "No such region type: '%s'\n\n",
					region_type_string);
				fprintf(stderr, "run '%s -h' for usage\n", argv[0]);
				exit(EXIT_FAILURE);
			}
			mode_inject = 1;
			break;
		case 'n':
			mode_newlayout = 1;
			layout_fname = strdup(optarg);
			if (!layout_fname) {
				fprintf(stderr, "No layout file specified\n");
				fprintf(stderr, "run '%s -h' for usage\n", argv[0]);
				exit(EXIT_FAILURE);
			}
			break;
		case 'O':
			new_filename = strdup(optarg);
			if (!new_filename) {
				fprintf(stderr, "No output filename specified\n");
				fprintf(stderr, "run '%s -h' for usage\n", argv[0]);
				exit(EXIT_FAILURE);
			}
			break;
		case 'D':
			mode_density = 1;
			new_density = strtoul(optarg, NULL, 0);
			switch (new_density) {
			case 512:
				new_density = COMPONENT_DENSITY_512KB;
				break;
			case 1:
				new_density = COMPONENT_DENSITY_1MB;
				break;
			case 2:
				new_density = COMPONENT_DENSITY_2MB;
				break;
			case 4:
				new_density = COMPONENT_DENSITY_4MB;
				break;
			case 8:
				new_density = COMPONENT_DENSITY_8MB;
				break;
			case 16:
				new_density = COMPONENT_DENSITY_16MB;
				break;
			case 32:
				new_density = COMPONENT_DENSITY_32MB;
				break;
			case 64:
				new_density = COMPONENT_DENSITY_64MB;
				break;
			case 0:
				new_density = COMPONENT_DENSITY_UNUSED;
				break;
			default:
				printf("error: Unknown density\n");
				fprintf(stderr, "run '%s -h' for usage\n", argv[0]);
				exit(EXIT_FAILURE);
			}
			break;
		case 'C':
			selected_chip = strtol(optarg, NULL, 0);
			if (selected_chip > 2) {
				fprintf(stderr, "error: Invalid chip selection\n");
				fprintf(stderr, "run '%s -h' for usage\n", argv[0]);
				exit(EXIT_FAILURE);
			}
			break;
		case 'M':
			mode_altmedisable = 1;
			altmedisable = strtol(optarg, NULL, 0);
			if (altmedisable > 1) {
				fprintf(stderr, "error: Illegal value\n");
				fprintf(stderr, "run '%s -h' for usage\n", argv[0]);
				exit(EXIT_FAILURE);
			}
			break;
		case 's':
			// Parse the requested SPI frequency
			inputfreq = strtol(optarg, NULL, 0);
			switch (inputfreq) {
			case 17:
				spifreq = SPI_FREQUENCY_17MHZ;
				break;
			case 20:
				spifreq = SPI_FREQUENCY_20MHZ;
				break;
			case 30:
				spifreq = SPI_FREQUENCY_50MHZ_30MHZ;
				break;
			case 33:
				spifreq = SPI_FREQUENCY_33MHZ;
				break;
			case 48:
				spifreq = SPI_FREQUENCY_48MHZ;
				break;
			case 50:
				spifreq = SPI_FREQUENCY_50MHZ_30MHZ;
				break;
			default:
				fprintf(stderr, "Invalid SPI Frequency: %d\n",
					inputfreq);
				fprintf(stderr, "run '%s -h' for usage\n", argv[0]);
				exit(EXIT_FAILURE);
			}
			mode_spifreq = 1;
			break;
		case 'e':
			mode_em100 = 1;
			break;
		case 'l':
			mode_locked = 1;
			if (mode_unlocked == 1) {
				fprintf(stderr, "Locking/Unlocking FD and ME are mutually exclusive\n");
				exit(EXIT_FAILURE);
			}
			break;
		case 'r':
			mode_read = 1;
			break;
		case 'u':
			mode_unlocked = 1;
			if (mode_locked == 1) {
				fprintf(stderr, "Locking/Unlocking FD and ME are mutually exclusive\n");
				exit(EXIT_FAILURE);
			}
			break;
		case 'p':
			if (!strcmp(optarg, "aplk")) {
				platform = PLATFORM_APL;
			} else if (!strcmp(optarg, "cnl")) {
				platform = PLATFORM_CNL;
			} else if (!strcmp(optarg, "lbg")) {
				platform = PLATFORM_LBG;
			} else if (!strcmp(optarg, "dnv")) {
				platform = PLATFORM_DNV;
			} else if (!strcmp(optarg, "ehl")) {
				platform = PLATFORM_EHL;
			} else if (!strcmp(optarg, "glk")) {
				platform = PLATFORM_GLK;
			} else if (!strcmp(optarg, "icl")) {
				platform = PLATFORM_ICL;
			} else if (!strcmp(optarg, "jsl")) {
				platform = PLATFORM_JSL;
			} else if (!strcmp(optarg, "sklkbl")) {
				platform = PLATFORM_SKLKBL;
			} else if (!strcmp(optarg, "tgl")) {
				platform = PLATFORM_TGL;
			} else if (!strcmp(optarg, "adl")) {
				platform = PLATFORM_ADL;
			} else if (!strcmp(optarg, "ifd2")) {
				platform = PLATFORM_IFD2;
			} else if (!strcmp(optarg, "mtl")) {
				platform = PLATFORM_MTL;
			} else if (!strcmp(optarg, "wbg")) {
				platform = PLATFORM_WBG;
			} else {
				fprintf(stderr, "Unknown platform: %s\n", optarg);
				exit(EXIT_FAILURE);
			}
			break;
		case 't':
			mode_validate = 1;
			break;
		case 'v':
			print_version();
			exit(EXIT_SUCCESS);
			break;
		case 'h':
		case '?':
		default:
			print_usage(argv[0]);
			exit(EXIT_SUCCESS);
			break;
		}
	}

	if ((mode_dump + mode_layout + mode_fmap_template + mode_extract + mode_inject +
			mode_setstrap + mode_newlayout + (mode_spifreq | mode_em100 |
			mode_unlocked | mode_locked) + mode_altmedisable + mode_validate) > 1) {
		fprintf(stderr, "You may not specify more than one mode.\n\n");
		fprintf(stderr, "run '%s -h' for usage\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	if ((mode_dump + mode_layout + mode_fmap_template + mode_extract + mode_inject +
			mode_setstrap + mode_newlayout + mode_spifreq + mode_em100 +
			mode_locked + mode_unlocked + mode_density + mode_altmedisable + mode_validate) == 0) {
		fprintf(stderr, "You need to specify a mode.\n\n");
		fprintf(stderr, "run '%s -h' for usage\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	if (optind + 1 != argc) {
		fprintf(stderr, "You need to specify a file.\n\n");
		fprintf(stderr, "run '%s -h' for usage\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	if (platform == -1)
		fprintf(stderr, "Warning: No platform specified. Output may be incomplete\n");

	char *filename = argv[optind];
	int bios_fd = open(filename, O_RDONLY | O_BINARY);
	if (bios_fd == -1) {
		perror("Could not open file");
		exit(EXIT_FAILURE);
	}
	struct stat buf;
	if (fstat(bios_fd, &buf) == -1) {
		perror("Could not stat file");
		exit(EXIT_FAILURE);
	}
	int size = buf.st_size;

	printf("File %s is %d bytes\n", filename, size);

	char *image = malloc(size);
	if (!image) {
		printf("Out of memory.\n");
		exit(EXIT_FAILURE);
	}

	if (read(bios_fd, image, size) != size) {
		perror("Could not read file");
		exit(EXIT_FAILURE);
	}

	close(bios_fd);

	// generate new filename
	if (new_filename == NULL) {
		new_filename = (char *) malloc((strlen(filename) + 5) * sizeof(char));
		if (!new_filename) {
			printf("Out of memory.\n");
			exit(EXIT_FAILURE);
		}
		// - 5: leave room for ".new\0"
		strcpy(new_filename, filename);
		strcat(new_filename, ".new");
	}

	check_ifd_version(image, size);

	if (mode_dump)
		dump_fd(image, size);

	if (mode_layout)
		dump_flashrom_layout(image, size, layout_fname);

	if (mode_fmap_template)
		create_fmap_template(image, size, layout_fname);

	if (mode_extract)
		write_regions(image, size);

	if (mode_validate)
		validate_layout(image, size);

	if (mode_inject)
		inject_region(new_filename, image, size, region_type,
				region_fname);

	if (mode_newlayout)
		new_layout(new_filename, image, size, layout_fname);

	if (mode_spifreq)
		set_spi_frequency(new_filename, image, size, spifreq);

	if (mode_density)
		set_chipdensity(new_filename, image, size, new_density);

	if (mode_em100)
		set_em100_mode(new_filename, image, size);

	if (mode_locked)
		lock_descriptor(new_filename, image, size);

	if (mode_read)
		enable_cpu_read_me(new_filename, image, size);

	if (mode_unlocked)
		unlock_descriptor(new_filename, image, size);

	if (mode_setstrap) {
		struct fpsba *fpsba = find_fpsba(image, size);
		const struct fdbar *fdb = find_fd(image, size);
		set_pchstrap(fpsba, fdb, pchstrap, value);
		write_image(new_filename, image, size);
	}

	if (mode_altmedisable) {
		struct fpsba *fpsba = find_fpsba(image, size);
		struct fmsba *fmsba = find_fmsba(image, size);
		fpsba_set_altmedisable(fpsba, fmsba, altmedisable);
		write_image(new_filename, image, size);
	}

	free(new_filename);
	free(image);

	return 0;
}
