/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <fcntl.h>
#include <getopt.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/io.h>
#include <sys/mman.h>

#define STR(x)			_STR(x)
#define _STR(x)			#x

#define APM_CNT			0xb2
#define SMMSTORE_CMD		0xed
#define SMMSTORE_PARAMS_SIZE	0x1000
#define SMMSTORE_COMBUF_SIZE	0x10000

/* Size is doubled to account for page alignment required for mmap() */
static unsigned char tmpbuf[2 * SMMSTORE_PARAMS_SIZE];

static inline unsigned int call_smm (unsigned int cmd, unsigned int arg)
{
	unsigned int res = 0;
	asm volatile (
		"outb %b0, $" STR(APM_CNT) "\n\t"
		"nop; nop; nop\n\t"
		: "=a" (res)
		: "a" (cmd),
		  "b" (arg)
		: "memory");
	return res;
}

static int usage (char *name)
{
	printf ("Usage:\n"
		"\t%s --tmp_buf <addr> --com_buf <addr> --command <cmd>\n"
		"\t%*s [--in_file <path>] [--out_file <path>] param1 ...\n"
		"\t%s -t <addr> -b <addr> -c <cmd>\n"
		"\t%*s [-i <path>] [-o <path>] param1 ...\n\n",
		name, (int)strlen(name), "", name, (int)strlen(name), "");
	printf ("--tmp_buf, -t  - physical address of a buffer in low 4GB of RAM. Up to 8KB\n"
		"                 after this address will be overwritten with params and then\n"
		"                 restored. Point it to something that won't break because of\n"
		"                 such temporary modification (e.g. part of CONSOLE in CBMEM,\n"
		"                 **after** the header - so SMI handler can still print to\n"
		"                 the CBMEM console).\n"
		"--com_buf, -b  - physical address of SMM COMBUFFER, can be read from\n"
		"                 `cbmem -l`. It must be aligned to a page size.\n"
		"--command, -c  - command to execute, see `coreboot/src/include/smmstore.h`.\n"
		"                 For SMMSTOREv2 this is 5 for read, 6 for write, 7 for\n"
		"                 clear.\n"
		"param1 ...     - params passed to SMI handler, as set of uint32_t values.\n"
		"                 Numbers are parsed as hexadecimal values, even without '0x'\n"
		"                 prefix. Expected structure depends on the command, refer to\n"
		"                 `smmstore_params_raw*` structure definitions in\n"
		"                 `coreboot/src/include/smmstore.h` file. At least one\n"
		"                 parameter must be specified.\n"
		"--in_file, -i  - optional, binary file which content is written to com_buf\n"
		"                 before SMM call.\n"
		"--out_file, -o - optional, path to binary file (it is created if doesn't exist)\n"
		"                 to which the content of com_buf after SMM call is saved.\n\n"
		"Some of the combinations don't make sense (e.g. writing to COMBUFFER before\n"
		"read command), but the tool doesn't check this. It is supposed to help\n"
		"with testing and debugging, hence it allows for sending garbage to SMMSTORE\n"
		"handler, with the assumption that the user knows what he's doing.\n\n"
		"The utility must be run with CAP_SYS_RAWIO privilege (e.g. as root)\n"
		"and with no kernel_lockdown in place (i.e. without SecureBoot).\n");
	return -1;
}

static struct option long_options[] = {
	{"tmp_buf",  required_argument, 0, 't'},
	{"com_buf",  required_argument, 0, 'b'},
	{"command",  required_argument, 0, 'c'},
	{"out_file", required_argument, 0, 'o'},
	{"in_file",  required_argument, 0, 'i'},
	{0,          0,                 0,  0 }
};

int main (int argc, char *argv[])
{
	int fd;
	void *combuf;
	unsigned long combuf_addr = 0;
	unsigned char *params;
	unsigned long params_phys = 0;
	unsigned long params_phys_aligned;
	unsigned int command = (unsigned int)-1;
	char *in_file = NULL;
	char *out_file = NULL;
	char *end;

	while (1) {
		int idx = 0;
		int opt;

		opt = getopt_long (argc, argv, "t:b:c:o:i:", long_options, &idx);

		if (opt == -1)
			break;

		switch (opt) {
		case 't':
			params_phys = strtoul (optarg, &end, 16);
			if (params_phys == 0 || params_phys == ULONG_MAX) return usage(argv[0]);
			break;
		case 'b':
			combuf_addr = strtoul (optarg, &end, 16);
			if (combuf_addr == 0 || combuf_addr == ULONG_MAX) return usage(argv[0]);
			break;
		case 'c':
			command = strtoul (optarg, &end, 16);
			if (command == 0 || command == UINT_MAX) return usage(argv[0]);
			break;
		case 'o':
			out_file = optarg;
			break;
		case 'i':
			in_file = optarg;
			break;
		default:
			return usage(argv[0]);
		}
	}

	/* Check if all required options were given */
	if (!combuf_addr || !params_phys || command == (unsigned int)-1 || optind == argc)
		return usage(argv[0]);

	params_phys_aligned = params_phys & ~((unsigned long)(getpagesize() - 1));

	/* Map COMBUF and temporary buffer for params */
	fd = open ("/dev/mem", O_SYNC | O_RDWR);
	if (fd < 0) {
		perror ("Can't access physical memory");
		printf ("This tool must be run as root with SecureBoot disabled.\n");
		return -1;
	}
	combuf = mmap (NULL, SMMSTORE_COMBUF_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_LOCKED | MAP_POPULATE, fd, combuf_addr);
	params = mmap (NULL, 2 * SMMSTORE_PARAMS_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_LOCKED | MAP_POPULATE, fd, params_phys_aligned);
	close (fd);

	/* Fill COMBUF from in_file */
	if (in_file) {
		fd = open (in_file, O_RDONLY);
		if (fd < 0) {
			perror ("Error opening in_file");
			return -1;
		}
		read (fd, combuf, SMMSTORE_COMBUF_SIZE);
		close (fd);
	}

	/* Request IO access to APM_CNT port */
	if (ioperm (APM_CNT, 1, 1) < 0) {
		perror ("Can't access IO port");
		printf ("This tool must be run as root with SecureBoot disabled.\n");
		return -1;
	}

	/* Backup temporary buffer and fill params */
	memcpy (tmpbuf, params, sizeof(tmpbuf));
	for (int i = 0; i < (argc - optind) && i < (int)(SMMSTORE_PARAMS_SIZE / sizeof (unsigned int)); i++) {
		unsigned int *param = (unsigned int *)(params + (params_phys - params_phys_aligned));
		param[i] = strtoul (argv[i + optind], &end, 16);
	}

	printf ("SMI handler returned %#x\n", call_smm ((command << 8) | SMMSTORE_CMD, params_phys));

	/* Restore original contents of memory used for params */
	memcpy (params, tmpbuf, sizeof(tmpbuf));

	/* Store COMBUF to out_file */
	if (out_file) {
		fd = creat (out_file, 0666);
		if (fd < 0) {
			perror ("Error opening out_file");
			return -1;
		}
		write (fd, combuf, SMMSTORE_COMBUF_SIZE);
		close (fd);
	}

	return 0;
}
