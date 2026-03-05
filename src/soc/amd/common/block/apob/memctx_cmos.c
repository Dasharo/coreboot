/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <bootstate.h>
#include "memctx_cmos.h"

/*
 * Address of the APOB NV status byte in CMOS. Should be reserved
 * in mainboards' cmos.layout and not covered by checksum.
 */
#define CMOS_OFFSET_APCB_RECOVERY		0x86
#define   CMOS_APCB_RECOVERY_DISABLED		0x5555
#define   CMOS_APCB_RECOVERY_COMPLETED		0xa55a
#define CMOS_OFFSET_MEM_RESTORE			0x8d
#define   CMOS_BITMAP_MEM_RESTORE_BOOT_FAIL	BIT(0)
#define   CMOS_BITMAP_DISCARD_MEM_CONTEXT	BIT(1)
#define   CMOS_BITMAP_APOB_SAVED		BIT(2)
#define   CMOS_BITMAP_APCB_UPDATED		BIT(3)


#if CONFIG(USE_OPTION_TABLE)
#include "option_table.h"
#if CMOS_VSTART_amd_mem_restore != CMOS_OFFSET_MEM_RESTORE * 8
#error "CMOS start for AMD memcontext restore status is not correct, check your cmos.layout"
#endif
#if CMOS_VLEN_amd_mem_restore != 8
#error "CMOS length for AMD memcontext restore status is not correct, check your cmos.layout"
#endif

#if CMOS_VSTART_apcb_recovery != CMOS_OFFSET_APCB_RECOVERY * 8
#error "CMOS start for AMD memcontext restore status is not correct, check your cmos.layout"
#endif
#if CMOS_VLEN_apcb_recovery != 16
#error "CMOS length for AMD memcontext restore status is not correct, check your cmos.layout"
#endif
#endif

void amd_mem_restore_signoff(void)
{
	uint8_t data = cmos_read(CMOS_OFFSET_MEM_RESTORE);
	data &= ~CMOS_BITMAP_MEM_RESTORE_BOOT_FAIL;
	data |= CMOS_BITMAP_APOB_SAVED;
	cmos_write(data, CMOS_OFFSET_MEM_RESTORE);
}

void amd_mem_restore_discard_current_context(void)
{
	uint8_t data = cmos_read(CMOS_OFFSET_MEM_RESTORE);
	data |= CMOS_BITMAP_DISCARD_MEM_CONTEXT;
	cmos_write(data, CMOS_OFFSET_MEM_RESTORE);
}

void amd_mem_restore_keep_current_context(void)
{
	uint8_t data = cmos_read(CMOS_OFFSET_MEM_RESTORE);
	data &= ~CMOS_BITMAP_DISCARD_MEM_CONTEXT;
	cmos_write(data, CMOS_OFFSET_MEM_RESTORE);
}

void amd_mem_restore_apcb_changed(void)
{
	uint8_t data = cmos_read(CMOS_OFFSET_MEM_RESTORE);
	data |= CMOS_BITMAP_APCB_UPDATED;
	cmos_write(data, CMOS_OFFSET_MEM_RESTORE);
}

static void amd_mem_disable_apcb_recovery(void *unused)
{
	/*
	 *  Update the CMOS[6:7] with flag/signature 0x5555 to indicate that
	 *  APCB recovery is disabled in X86 code. On the following boot ABL
	 *  should always read APCB from type 0x60. Without it ABL will be
	 *  setting ApcbRecoveryFlag which will cause APOB to be discarded and
	 *  Memory Context Restore disabled.
	 */
	cmos_write(CMOS_APCB_RECOVERY_DISABLED & 0xff, CMOS_OFFSET_APCB_RECOVERY);
	cmos_write((CMOS_APCB_RECOVERY_DISABLED >> 8) & 0xff, CMOS_OFFSET_APCB_RECOVERY + 1);
}

BOOT_STATE_INIT_ENTRY(BS_PAYLOAD_LOAD, BS_ON_EXIT, amd_mem_disable_apcb_recovery, NULL);
BOOT_STATE_INIT_ENTRY(BS_OS_RESUME, BS_ON_ENTRY, amd_mem_disable_apcb_recovery, NULL);
