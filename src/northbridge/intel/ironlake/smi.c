/* SPDX-License-Identifier: GPL-2.0-only */

#define __SIMPLE_DEVICE__

#include <types.h>
#include <device/device.h>
#include <device/pci_ops.h>
#include "ironlake.h"

#include <cpu/intel/smm_reloc.h>

void northbridge_write_smram(u8 smram)
{
	pci_write_config8(QPI_SAD, QPD0F1_SMRAM, smram);
}
