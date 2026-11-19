/* SPDX-License-Identifier: GPL-2.0-only */

#include <amdblocks/acpimmio.h>
#include <amdblocks/psp.h>
#include <soc/smi.h>
#include <stdint.h>

void soc_fill_smm_trig_info(struct smm_trigger_info *trig)
{
	if (!trig)
		return;

	trig->address = (uintptr_t)acpimmio_smi + SMI_REG_SMITRIG0;
	trig->address_type = SMM_TRIGGER_MEM;
	trig->value_width = SMM_TRIGGER_DWORD;
	trig->value_and_mask = (uint32_t)~SMITRIG0_PSP;
	trig->value_or_mask = (uint32_t)SMITRIG0_PSP;
}
