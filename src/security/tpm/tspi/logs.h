/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef LOGS_H_
#define LOGS_H_

#include <commonlib/bsd/tpm_log_defs.h>
#include <stdint.h>
#include <vb2_api.h>

struct tpm_digest;

/* coreboot-specific TPM log format */

void *tpm_cb_log_init(void);
void *tpm_cb_log_cbmem_init(void);
void tpm_cb_preram_log_clear(void);
uint16_t tpm_cb_log_get_size(const void *log_table);
void tpm_cb_log_copy_entries(const void *from, void *to);
int tpm_cb_log_get(int entry_idx, int *pcr, struct tpm_digest *digests, const char **event_name);
void tpm_cb_log_add_table_entry(const char *name, const uint32_t pcr,
				const struct tpm_digest *digests);
void tpm_cb_log_dump(void);

/* TPM 1.2 log format */

void *tpm1_log_init(void);
void *tpm1_log_cbmem_init(void);
void tpm1_preram_log_clear(void);
uint16_t tpm1_log_get_size(const void *log_table);
void tpm1_log_copy_entries(const void *from, void *to);
int tpm1_log_get(int entry_idx, int *pcr, struct tpm_digest *digests, const char **event_name);
void tpm1_log_add_table_entry(const char *name, const uint32_t pcr,
			      const struct tpm_digest *digests);
void tpm1_log_dump(void);

/* TPM 2.0 log format */

void *tpm2_log_init(void);
void *tpm2_log_cbmem_init(void);
void tpm2_preram_log_clear(void);
uint16_t tpm2_log_get_size(const void *log_table);
void tpm2_log_copy_entries(const void *from, void *to);
int tpm2_log_get(int entry_idx, int *pcr, struct tpm_digest *digests, const char **event_name);
void tpm2_log_add_table_entry(const char *name, const uint32_t pcr,
			      const struct tpm_digest *digests);
void tpm2_log_startup_locality(int locality);
void tpm2_log_align_with_tpm(void);
void tpm2_log_dump(void);
bool tpm2_log_alg_active(enum vb2_hash_algorithm alg);

static inline uint16_t tpm2_alg_from_vb2_hash(enum vb2_hash_algorithm hash_type)
{
	switch (hash_type) {
	case VB2_HASH_SHA1:
		return TPM2_ALG_SHA1;
	case VB2_HASH_SHA256:
		return TPM2_ALG_SHA256;
	case VB2_HASH_SHA384:
		return TPM2_ALG_SHA384;
	case VB2_HASH_SHA512:
		return TPM2_ALG_SHA512;

	default:
		return 0xFF;
	}
}

static inline enum vb2_hash_algorithm tpm2_alg_to_vb2_hash(uint16_t hash_type)
{
	switch (hash_type) {
	case TPM2_ALG_SHA1:
		return VB2_HASH_SHA1;
	case TPM2_ALG_SHA256:
		return VB2_HASH_SHA256;
	case TPM2_ALG_SHA384:
		return VB2_HASH_SHA384;
	case TPM2_ALG_SHA512:
		return VB2_HASH_SHA512;

	default:
		return VB2_HASH_INVALID;
	}
}

#endif /* LOGS_H_ */
