/* SPDX-License-Identifier: GPL-2.0-only */

#include <version.h>
#include <build.h>

#ifndef COREBOOT_VERSION
#error COREBOOT_VERSION not defined
#endif
#ifndef COREBOOT_BUILD
#error COREBOOT_BUILD not defined
#endif

#ifndef COREBOOT_COMPILE_TIME
#error COREBOOT_COMPILE_TIME not defined
#endif

#ifndef COREBOOT_EXTRA_VERSION
#define COREBOOT_EXTRA_VERSION ""
#endif

#ifndef DASHARO_VERSION
#define DASHARO_VERSION "*INVALID*"
#endif

#ifndef DASHARO_MAJOR_VERSION
#define DASHARO_MAJOR_VERSION 0
#endif

#ifndef DASHARO_MINOR_VERSION
#define DASHARO_MINOR_VERSION 0
#endif

#ifndef DASHARO_PATCH_VERSION
#define DASHARO_PATCH_VERSION 0
#endif

const char coreboot_version[] = COREBOOT_VERSION;
const char coreboot_extra_version[] = COREBOOT_EXTRA_VERSION;
const char coreboot_build[] = COREBOOT_BUILD;
const unsigned int coreboot_version_timestamp = COREBOOT_VERSION_TIMESTAMP;
const unsigned int coreboot_major_revision = COREBOOT_MAJOR_VERSION;
const unsigned int coreboot_minor_revision = COREBOOT_MINOR_VERSION;

const char dasharo_version[] = DASHARO_VERSION;
const unsigned int dasharo_major_revision = DASHARO_MAJOR_VERSION;
const unsigned int dasharo_minor_revision = DASHARO_MINOR_VERSION;
const unsigned int dasharo_patch_revision = DASHARO_PATCH_VERSION;

const char coreboot_compile_time[] = COREBOOT_COMPILE_TIME;
const char coreboot_dmi_date[] = COREBOOT_DMI_DATE;

const struct bcd_date coreboot_build_date = {
	.century = 0x20,
	.year = COREBOOT_BUILD_YEAR_BCD,
	.month = COREBOOT_BUILD_MONTH_BCD,
	.day = COREBOOT_BUILD_DAY_BCD,
	.weekday = COREBOOT_BUILD_WEEKDAY_BCD,
};

const unsigned int asl_revision = ASL_VERSION;
