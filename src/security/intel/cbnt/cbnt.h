/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef SECURITY_INTEL_CBNT_H_
#define SECURITY_INTEL_CBNT_H_

#include <security/intel/txt/txt_register.h>

#define CBNT_BASE_ADDRESS TXT_PUBLIC_SPACE

#define CBNT_ERRORCODE (CBNT_BASE_ADDRESS + 0x30)
#define CBNT_BOOTSTATUS (CBNT_BASE_ADDRESS + 0xa0)
#define CBNT_BIOSACM_ERRORCODE (CBNT_BASE_ADDRESS + 0x328)
#define CBNT_BIOSACM_POLICY_STS (CBNT_BASE_ADDRESS + 0x378)

union cbnt_biosacm_policy {
	struct {
		uint64_t km_id : 4;
		uint64_t bp : 9; /* See CBNT_BP_* constants. */
		uint64_t tpm_type : 2;
		uint64_t tpm_success : 1;
		uint64_t: 1;
		uint64_t pfr : 1;
		uint64_t bckup_act : 2;
		uint64_t txt_profile : 5;
		uint64_t scrub_policy : 2;
		uint64_t: 2;
		uint64_t dma_protection : 1;
		uint64_t: 2;
		uint64_t scrtm_status : 3;
		uint64_t cpu_co_signing : 1;
		uint64_t tpm_startup_locality : 1;
		uint64_t: 27;
	} status;
	uint64_t raw;
};
_Static_assert(sizeof(union cbnt_biosacm_policy) == sizeof(uint64_t),
	       "Wrong size of cbnt_biosacm_policy");

void intel_cbnt_log_registers(void);

#endif
