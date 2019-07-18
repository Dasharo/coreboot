/*
 * Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <libpayload.h>
#include <coreboot_tables.h>

#include "lib_vpd.h"
#include "vpd_tables.h"

/* Currently we only support Google VPD 2.0, which has a fixed offset. */
enum {
	GOOGLE_VPD_2_0_OFFSET = 0x600,
	CROSVPD_CBMEM_MAGIC = 0x43524f53,
	CROSVPD_CBMEM_VERSION = 0x0001,
};

struct vpd_gets_arg {
	const uint8_t *key;
	const uint8_t *value;
	int32_t key_len, value_len;
	int matched;
};

struct vpd_cbmem {
	uint32_t magic;
	uint32_t version;
	uint32_t ro_size;
	uint32_t rw_size;
	uint8_t blob[0];
	/* The blob contains both RO and RW data. It starts with RO (0 ..
	 * ro_size) and then RW (ro_size .. ro_size+rw_size).
	 */
};

static int vpd_gets_callback(const uint8_t *key, int32_t key_len,
			       const uint8_t *value, int32_t value_len,
			       void *arg)
{
	struct vpd_gets_arg *result = (struct vpd_gets_arg *)arg;
	if (key_len != result->key_len ||
	    memcmp(key, result->key, key_len) != 0)
		/* Returns VPD_OK to continue parsing. */
		return VPD_OK;

	result->matched = 1;
	result->value = value;
	result->value_len = value_len;
	/* Returns VPD_FAIL to stop parsing. */
	return VPD_FAIL;
}

const void *vpd_find(const char *key, int *size, enum vpd_region region)
{
	struct vpd_gets_arg arg = {0};
	int consumed = 0;
	struct vpd_cbmem *vpd;
	
	if (!lib_sysinfo.chromeos_vpd)
		lib_get_sysinfo();

	vpd = (struct vpd_cbmem *)(lib_sysinfo.chromeos_vpd);

	if (!vpd || !vpd->ro_size)
		return NULL;

	arg.key = (const uint8_t *)key;
	arg.key_len = strlen(key);

	if (region == VPD_ANY || region == VPD_RO)
		while (VPD_OK == decodeVpdString(vpd->ro_size, vpd->blob,
		       &consumed, vpd_gets_callback, &arg)) {
		/* Iterate until found or no more entries. */
		}

	if (!arg.matched && region != VPD_RO)
		while (VPD_OK == decodeVpdString(vpd->rw_size,
		       vpd->blob + vpd->ro_size, &consumed,
		       vpd_gets_callback, &arg)) {
		/* Iterate until found or no more entries. */
		}

	if (!arg.matched)
		return NULL;

	*size = arg.value_len;
	return arg.value;
}

char *vpd_gets(const char *key, char *buffer, int size, enum vpd_region region)
{
	const void *string_address;
	int string_size;

	string_address = vpd_find(key, &string_size, region);

	if (!string_address)
		return NULL;

	if (size > (string_size + 1)) {
		memcpy(buffer, string_address, string_size);
		buffer[string_size] = '\0';
	} else {
		memcpy(buffer, string_address, size - 1);
		buffer[size - 1] = '\0';
	}
	return buffer;
}
