/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef CPU_PPC64_VPD_H
#define CPU_PPC64_VPD_H

#define VPD_RECORD_NAME_LEN 4
#define VPD_RECORD_SIZE_LEN 2
#define VPD_KWD_NAME_LEN    2

void vpd_pnor_main(void);

#endif /* CPU_PPC64_VPD_H */
