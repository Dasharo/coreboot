/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __SECURITY_TSPI_CRTM_H__
#define __SECURITY_TSPI_CRTM_H__

#include <program_loading.h>
#include <security/tpm/tspi.h>
#include <security/tpm/tss_errors.h>
#include <types.h>
#include <vb2_sha.h>

/**
 * Checks whether TPM log was already initialized.
 */
bool tspi_tpm_log_available(void);

/**
 * Initializes the Core Root of Trust for Measurements in coreboot.  The initial code in a
 * chain of trust must measure itself.
 *
 * Summary:
 *  - measures the FMAP FMAP partition
 *  - measures bootblock in CBFS or BOOTBLOCK FMAP partition
 *  - if vboot starts in romstage, it measures the romstage in CBFS
 *  - measures the verstage if it is compiled as a separate stage
 *
 * On success returns TPM_SUCCESS, otherwise returns a TPM error.
 */
tpm_result_t tspi_init_crtm(void);

/**
 * Measure digests cached in TPM log entries into PCRs
 */
tpm_result_t tspi_measure_cache_to_pcr(void);

/**
 * Extend a measurement hash of a CBFS file into the appropriate PCR.  hash_hint can be passed
 * in to avoid recomputing the digest if it's already known.
 */
tpm_result_t tspi_cbfs_measurement(const char *name, const void *buffer, size_t size,
				   uint32_t type, const struct vb2_hash *hash_hint);

/*
 * Provide a function on SoC level to measure the bootblock for cases where bootblock is
 * neither in FMAP nor in CBFS (e.g. in IFWI).
 */
int tspi_soc_measure_bootblock(int pcr_index);

#endif /* __SECURITY_TSPI_CRTM_H__ */
