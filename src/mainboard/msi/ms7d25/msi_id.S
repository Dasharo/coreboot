/* SPDX-License-Identifier: GPL-2.0-only */

#include <build.h>
#include <fmap_config.h>

.section ".msi_id", "a", @progbits

/* The following structures must reside in the last 64KiB of flash */

.align 16

build_date:
	.ascii "$MSESGN$"
	.ascii "E7D25IMS."
#if CONFIG(BOARD_MSI_Z690_A_PRO_WIFI_DDR5)
	.byte 'A'
#endif
#if CONFIG(BOARD_MSI_Z690_A_PRO_WIFI_DDR4)
	.byte 0x30 + DASHARO_MAJOR_VERSION
#endif
	.byte 0x30 + DASHARO_MINOR_VERSION
	.byte 0x30 + DASHARO_PATCH_VERSION
	.byte ' '
	.asciz COREBOOT_DMI_DATE

.align 8

msi_sign_on:
	.ascii "$MS1E7D25IMS"
#if CONFIG(BOARD_MSI_Z690_A_PRO_WIFI_DDR5)
	.byte	'A'
#endif
#if CONFIG(BOARD_MSI_Z690_A_PRO_WIFI_DDR4)
	.byte	0x30 + DASHARO_MAJOR_VERSION
#endif
	.byte	0x30 + DASHARO_MINOR_VERSION
	.byte	0x30 + DASHARO_PATCH_VERSION
	.byte	2

.align 8

bpa:
	.ascii	"$BPA"
	.byte	bpa_end - bpa
	.byte	1
	.long	0x1000
	.long	6
	.byte	2
	.long	FMAP_SECTION_ROMHOLE_START
	.long	FMAP_SECTION_ROMHOLE_SIZE
	.byte	3
	.long	FMAP_SECTION_BPA_START
	.long	64
	.byte	4
	.long	2
bpa_end:
	.byte	0, 0, 0, 0, 0

	.asciz	"@FC@"
	.byte	2
	.byte	0
	.byte	'1'
	.asciz	"/P /B /N"
	.byte	'2'
	.byte	0
	.ascii	"NULL"
	.ascii	"@@"
	.fill	10, 1, 0
