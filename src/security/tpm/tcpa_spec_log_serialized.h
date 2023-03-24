/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __TCPA_SPEC_LOG_SERIALIZED_H__
#define __TCPA_SPEC_LOG_SERIALIZED_H__

#include <commonlib/tpm_log_serialized.h>
#include <stdint.h>

#define MAX_TCPA_LOG_ENTRIES 50
#define TCPA_PCR_HASH_NAME 50
#define TCPA_PCR_HASH_LEN 10
/* Assumption of 2K TCPA log size reserved for CAR/SRAM */
#define MAX_PRERAM_TCPA_LOG_ENTRIES 15

/*
 * TPM2.0 log entries can't be generally represented as C structures due to
 * varying number of digests and their sizes. However, it works as long as
 * we're only using and supporting SHA1 digests.
 */
#define TCPA_DIGEST_MAX_LENGTH SHA1_DIGEST_SIZE

/* TCG_PCR_EVENT2 */
struct tcpa_entry {
	uint32_t pcr;
	uint32_t event_type;
	uint32_t digest_count;
	uint16_t digest_type;
	uint8_t digest[TCPA_DIGEST_MAX_LENGTH];
	uint32_t name_length;
	char name[TCPA_PCR_HASH_NAME];
} __packed;

struct tcpa_table {
	uint16_t max_entries;
	uint16_t num_entries;
	tcg_efi_spec_id_event header; /* TCG_PCR_EVENT actually */
	struct tcpa_entry entries[0]; /* Variable number of entries */
} __packed;

#endif
