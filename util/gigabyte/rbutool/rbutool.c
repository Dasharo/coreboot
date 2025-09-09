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

static const char *optstring  = "i:o:h";

static struct option long_options[] = {
	{"input",              required_argument, 0, 'i' },
	{"help",                     no_argument, 0, 'h' },
};

static void usage(void)
{
	printf("rbutool: Create RBU image for Gigabyte BMC BIOS updates\n");
	printf("Usage: rbutool -i coreboot.rom\n");
	printf("-i | --input <FILE>              The input coreboot ROM file\n");
	printf("-h | --help                      Print this help\n");
}

int main(int argc, char **argv)
{
	int c;
	int ret = 1;
	char *filename = NULL;
	char *inputfilename = NULL;
	uint8_t b;
	uint16_t checksum = 0;

	while (1) {
		int optindex = 0;

		c = getopt_long(argc, argv, optstring, long_options, &optindex);

		if (c == -1)
			break;

		switch (c) {
		case 'i':
			inputfilename = strdup(optarg);
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

	if (!inputfilename) {
		fprintf(stderr, "E: Must specify coreboot ROM filename\n");
		goto out;
	}

	FILE *fd = fopen(inputfilename, "r+");
	if (!fd) {
		fprintf(stderr, "E: %s open failed: %s\n", inputfilename, strerror(errno));
		goto out;
	}

	// Calculate checksum
	while (fread(&b, 1, 1, fd) == 1) {
		checksum += b;
	}

	fseek(fd, 0L, SEEK_END);
	// Write magic string
	fputs("#GBT#ROM", fd);
	// Write checksum in BE
	fputc(checksum >> 8, fd);
	fputc(checksum & 0xff, fd);
	// Pad with zeros to 16 bytes
	fputc(0, fd);
	fputc(0, fd);
	fputc(0, fd);
	fputc(0, fd);
	fputc(0, fd);
	fputc(0, fd);

	if (fclose(fd)) {
		fprintf(stderr, "E: %s close failed: %s\n", filename, strerror(errno));
		goto out;
	}

	ret = 0;
out:
	if (ret > 0)
		fprintf(stderr, "E: Error creating '%s'\n", filename);

	if (inputfilename)
		free(inputfilename);

	exit(ret);

	return 0;
}
