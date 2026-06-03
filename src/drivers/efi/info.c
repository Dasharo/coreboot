/* SPDX-License-Identifier: GPL-2.0-only */

#include <boot/coreboot_tables.h>
#include <commonlib/bsd/cb_err.h>
#include <console/console.h>
#include <ctype.h>
#include <ec/dasharo/ec/commands.h>
#include <stdint.h>
#include <string.h>
#include <uuid.h>

static void add_main_fw_info(struct lb_header *header)
{
	uint8_t guid[16];
	struct lb_efi_fw_info *fw_info;

	if (parse_uuid(guid, CONFIG_DRIVERS_EFI_MAIN_FW_GUID)) {
		printk(BIOS_ERR, "%s(): failed to parse firmware's GUID: '%s'\n", __func__,
		       CONFIG_DRIVERS_EFI_MAIN_FW_GUID);
		return;
	}

	fw_info = (struct lb_efi_fw_info *)lb_new_record(header);
	fw_info->tag = LB_TAG_EFI_FW_INFO;
	fw_info->size = sizeof(*fw_info);

	memcpy(fw_info->guid, guid, sizeof(guid));
	fw_info->version = CONFIG_DRIVERS_EFI_MAIN_FW_VERSION;
	fw_info->lowest_supported_version = CONFIG_DRIVERS_EFI_MAIN_FW_LSV;
	fw_info->fw_size = CONFIG_ROM_SIZE;
}

static enum cb_err parse_int(const char **str, int *value)
{
	const char *s = *str;

	if (!isdigit(*s)) {
		printk(BIOS_WARNING, "%s(): not a digit: '%c'\n", __func__, *s);
		return CB_ERR;
	}

	*value = 0;
	while (isdigit(*s)) {
		*value *= 10;
		*value += *s++ - '0';
	}

	*str = s;
	return CB_SUCCESS;
}

static enum cb_err parse_char(const char **str, char ch)
{
	if (**str != ch) {
		printk(BIOS_WARNING, "%s(): not a '%c': '%c'\n", __func__, ch, **str);
		return CB_ERR;
	}

	++*str;
	return CB_SUCCESS;
}

static enum cb_err parse_ec_version(const char *str, uint32_t *version)
{
	/*
	 * Expected string version format: {year}-{month}-{day}_{git commit hash prefix}
	 *
	 * Year, month and day are expected to be in decimal.  In practice they
	 * take up 4, 2 and 2 characters respectively and each field is padded
	 * with zeroes on the left but parsing doesn't depend on that and
	 * allows weird values.  Commit hash is ignored, merely checking for
	 * `_` separator.
	 *
	 * Returned integer version: (year << 16) | (month << 8) | day
	 */

	int year = 0;
	int month = 0;
	int day = 0;
	if (parse_int(&str, &year) != CB_SUCCESS || parse_char(&str, '-') != CB_SUCCESS ||
	    parse_int(&str, &month) != CB_SUCCESS || parse_char(&str, '-') != CB_SUCCESS ||
	    parse_int(&str, &day) != CB_SUCCESS || parse_char(&str, '_') != CB_SUCCESS)
		return CB_ERR;

	*version = ((year & 0xffff) << 16) | ((month & 0xff) << 8) | (day & 0xff);
	return CB_SUCCESS;
}

static void add_ec_fw_info(struct lb_header *header)
{
	uint8_t guid[16];
	uint32_t ec_version;
	char ec_version_str[256];
	struct lb_efi_fw_info *fw_info;

	const char *guid_str;
	uint32_t lsv;
	uint32_t fw_size;

	/* CONFIG_DRIVERS_EFI_EC_* options may be absent, so some preprocessor is necessary, but
	 * keeping it to a minimum */
#ifdef CONFIG_DRIVERS_EFI_EC_FW_GUID
	guid_str = CONFIG_DRIVERS_EFI_EC_FW_GUID;
	lsv = CONFIG_DRIVERS_EFI_EC_FW_LSV;
	fw_size = CONFIG_DRIVERS_EFI_EC_FW_SIZE;
#else
	/* No EC or its updates aren't done using capsules. */
	return;
#endif

	if (parse_uuid(guid, guid_str)) {
		printk(BIOS_WARNING, "%s(): failed to parse EC firmware's GUID: '%s'\n",
		       __func__, guid_str);
		return;
	}

	if (dasharo_ec_read_version((uint8_t *)ec_version_str) != 0) {
		printk(BIOS_WARNING, "%s(): failed to query EC firmware's version\n", __func__);
		return;
	}

	if (parse_ec_version(ec_version_str, &ec_version) != CB_SUCCESS) {
		printk(BIOS_WARNING, "%s(): failed to parse EC firmware's version: '%s'\n",
		       __func__, ec_version_str);
		return;
	}

	fw_info = (struct lb_efi_fw_info *)lb_new_record(header);
	fw_info->tag = LB_TAG_EFI_EC_FW_INFO;
	fw_info->size = sizeof(*fw_info);

	memcpy(fw_info->guid, guid, sizeof(guid));
	fw_info->version = ec_version;
	fw_info->lowest_supported_version = lsv;
	fw_info->fw_size = fw_size;
}

void lb_efi_fw_info(struct lb_header *header)
{
	add_main_fw_info(header);
	add_ec_fw_info(header);
}
