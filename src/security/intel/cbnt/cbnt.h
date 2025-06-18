/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef SECURITY_INTEL_CBNT_H_
#define SECURITY_INTEL_CBNT_H_

#include <security/intel/txt/txt_register.h>

#define CBNT_BASE_ADDRESS TXT_PUBLIC_SPACE

#define CBNT_ERRORCODE (CBNT_BASE_ADDRESS + 0x30)
#define CBNT_BOOTSTATUS (CBNT_BASE_ADDRESS + 0xa0)
#define CBNT_BIOSACM_ERRORCODE (CBNT_BASE_ADDRESS + 0x328)
#define CBNT_BIOSACM_POLICY_STS (CBNT_BASE_ADDRESS + 0x378)

/* Measured Boot, if set Boot Guard ACM mesures IBB to TPM. */
#define CBNT_BP_TYPE_M     (1 << 0)
/* Verified Boot, Boot Guard ACM verifies IBB. */
#define CBNT_BP_TYPE_V     (1 << 1)
/* High Assurance Platform, the bit that is used to instruct ME to halt right after paltform
   initialization. */
#define CBNT_BP_TYPE_HAP   (1 << 2)
/* TXT Supported. */
#define CBNT_BP_TYPE_T     (1 << 3)
#define CBNT_BP_RESERVED1  (1 << 4)
/* Disable CPU debugging, if set DCI JTAG that would tamper with ACM execution is disabled. */
#define CBNT_BP_RSTR_DCD   (1 << 5)
/* Disable BSP #INIT, means to block INIT signals to mitigate code refetch. */
#define CBNT_BP_RSTR_DBI   (1 << 6)
/* Protect BIOS Environment, means Boot Guard ACM copies IBB to cache. */
#define CBNT_BP_RSTR_PBE   (1 << 6)
#define CBNT_BP_RESERVED2  (1 << 8)

#define CBNT_EVENT_LOG_MESSAGE  "Boot Guard Measured S-CRTM"

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

int intel_cbnt_get_locality(void);

void intel_cbnt_inject_ibg_measurements(void);

#endif
