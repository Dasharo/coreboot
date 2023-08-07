/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef SECURITY_INTEL_CBNT_H_
#define SECURITY_INTEL_CBNT_H_

#include <security/intel/txt/txt_register.h>

#define CBNT_BASE_ADDRESS TXT_PUBLIC_SPACE

#define CBNT_ERRORCODE (CBNT_BASE_ADDRESS + 0x30)
#define CBNT_BOOTSTATUS (CBNT_BASE_ADDRESS + 0xa0)
#define CBNT_BIOSACM_ERRORCODE (CBNT_BASE_ADDRESS + 0x328)
#define CBNT_BIOSACM_POLICY_STS (CBNT_BASE_ADDRESS + 0x378)

void intel_cbnt_log_registers(void);

#endif
