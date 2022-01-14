/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef S3UTILS_H
#define S3UTILS_H

#include <drivers/amd/amdmct/wrappers/mcti.h>

#ifdef __RAMSTAGE__
int8_t save_mct_information_to_nvram(void);
void copy_mct_data_to_save_variable(struct amd_s3_persistent_data* persistent_data);
#endif

void calculate_and_store_spd_hashes(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat);
void compare_nvram_spd_hashes(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat);

#endif
