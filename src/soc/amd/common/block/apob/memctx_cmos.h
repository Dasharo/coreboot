/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <amdblocks/apob_cache.h>
#include <console/console.h>
#include <pc80/mc146818rtc.h>
#include <types.h>

/* Called to update CMOS when S3 is completed with memory context restore */
void amd_mem_restore_signoff(void);
void amd_mem_restore_discard_current_context(void);
void amd_mem_restore_keep_current_context(void);
void amd_mem_restore_apcb_changed(void);
