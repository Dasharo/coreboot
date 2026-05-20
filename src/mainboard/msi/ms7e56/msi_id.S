/* SPDX-License-Identifier: GPL-2.0-only */

#include <build.h>
#include <fmap_config.h>

.section .text, "a", @progbits

/* The following structures must reside in the last 64KiB of flash */

.align 16

build_date:
	.ascii "$MSESGN$"
	.ascii "E7E56AMSI.2"
	.byte  0x30 + DASHARO_MAJOR_VERSION
	.byte  0x30 + DASHARO_MINOR_VERSION
	.byte  0x30 + DASHARO_PATCH_VERSION
	.byte  ' '
	.asciz COREBOOT_DMI_DATE

.align 8

msi_sign_on:
	.ascii "$MS1E7E56AMSI2"
	.byte  0x30 + DASHARO_MAJOR_VERSION
	.byte  0x30 + DASHARO_MINOR_VERSION
	.byte  0x30 + DASHARO_PATCH_VERSION
	.byte  0x11
	.byte  0xe9
	.ascii "$MSVIDS$"
	.byte  0x11

.align 8

bpa:
	.ascii	"$BPA"
	.byte	bpa_end - bpa
	.byte	1
	.long	0x1000
	.long	6
	.byte	2
	.long	0x1be0000
	.long	0x20000
	.byte	3
	.long	0x1b90000
	.long	63
	.byte	4
	.byte	2
bpa_end:
	.zero	5

	.asciz	"@FC@"
	.byte	'2'
	.byte	0
	.byte	'1'
	.byte	0
	.asciz	"/P /fab /N /K"
	.byte	'2'
	.byte	0
	.ascii	"NULL"
	.ascii	"@@"
	.zero	58
