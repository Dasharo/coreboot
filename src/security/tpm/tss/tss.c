/* SPDX-License-Identifier: BSD-3-Clause */

#include <console/console.h>
#include <security/tpm/tis.h>
#include <security/tpm/tss.h>

/*
 * This unit dispatches to either TPM1.2 or TPM2.0 implementation based on
 * TPM family determined on probing during initialization.
 */

enum tpm_family tlcl_tpm_family;

tis_sendrecv_fn tlcl_tis_sendrecv;

static int init_done = 0;

/* Probe for TPM device and choose implementation based on the returned TPM family. */
tpm_result_t tlcl_lib_init(void)
{
	tis_probe_fn *tis_probe;

	if (init_done)
		return tlcl_tpm_family == TPM_UNKNOWN ? TPM_CB_NO_DEVICE : TPM_SUCCESS;

	if (tlcl_tpm_family != TPM_UNKNOWN)
		return TPM_SUCCESS;

	tlcl_tis_sendrecv = NULL;
	for (tis_probe = _tis_drivers; tis_probe != _etis_drivers; tis_probe++) {
		tlcl_tis_sendrecv = (*tis_probe)(&tlcl_tpm_family);
		if (tlcl_tis_sendrecv != NULL)
			break;
	}

	init_done = 1;

	if (tlcl_tis_sendrecv == NULL) {
		printk(BIOS_ERR, "%s: tis_probe failed\n", __func__);
		tlcl_tpm_family = TPM_UNKNOWN;
		return TPM_CB_NO_DEVICE;
	}

	if (tlcl_tpm_family != TPM_1 && tlcl_tpm_family != TPM_2) {
		printk(BIOS_ERR, "%s: tis_probe returned incorrect TPM family: %d\n", __func__,
		       tlcl_tpm_family);
		tlcl_tpm_family = TPM_UNKNOWN;
	}

	return tlcl_tpm_family == TPM_UNKNOWN ? TPM_CB_NO_DEVICE : TPM_SUCCESS;
}
