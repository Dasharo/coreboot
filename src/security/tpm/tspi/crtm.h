/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __SECURITY_TSPI_CRTM_H__
#define __SECURITY_TSPI_CRTM_H__

#include <program_loading.h>
#include <security/tpm/tspi.h>
#include <security/tpm/tss_errors.h>
#include <types.h>
#include <vb2_sha.h>

/**
 * Measure digests cached in TPM log entries into PCRs
 */
tpm_result_t tspi_measure_cache_to_pcr(void);

/**
 * Extend a measurement hash taken for a CBFS file into the appropriate PCR.
 */
tpm_result_t tspi_cbfs_measurement(const char *name, uint32_t type, const struct vb2_hash *hash);

/*
 * Provide a function on SoC level to measure the bootblock for cases where bootblock is
 * neither in FMAP nor in CBFS (e.g. in IFWI).
 */
int tspi_soc_measure_bootblock(int pcr_index);

#endif /* __SECURITY_TSPI_CRTM_H__ */
