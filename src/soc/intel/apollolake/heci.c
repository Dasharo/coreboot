/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <device/device.h>
#include <device/pci_def.h>
#include <device/pci_ops.h>
#include <soc/heci.h>
#include <soc/pci_devs.h>
#include <intelblocks/cse.h>

uint32_t heci_fw_sts(void)
{
	return pci_read_config32(PCH_DEV_CSE, REG_SEC_FW_STS0);
}

bool heci_cse_normal(void)
{
	return ((heci_fw_sts() & MASK_SEC_STATUS) == SEC_STATE_NORMAL);
}

bool heci_cse_done(void)
{
	return (!!(heci_fw_sts() & MASK_SEC_FIRMWARE_COMPLETE));
}

unsigned int soc_get_heci_dev(unsigned int heci_idx)
{
	if (heci_idx > 2)
		return 0;

	static const unsigned int heci_devs[] = {
		PCH_DEVFN_CSE,
		PCH_DEVFN_CSE_2,
		PCH_DEVFN_CSE_3,
	};

	return heci_devs[heci_idx];
}
