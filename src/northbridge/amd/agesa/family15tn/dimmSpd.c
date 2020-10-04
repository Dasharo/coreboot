/* SPDX-License-Identifier: GPL-2.0-only */

#include <device/pci_def.h>
#include <device/device.h>

/* warning: Porting.h includes an open #pragma pack(1) */
#include <Porting.h>
#include <AGESA.h>
#include "chip.h"

#include <northbridge/amd/agesa/dimmSpd.h>

/**
 * Gets the SMBus address for an SPD from the array in devicetree.cb
 * then read the SPD into the supplied buffer.
 */
AGESA_STATUS AmdMemoryReadSPD (UINT32 unused1, UINTN unused2, AGESA_READ_SPD_PARAMS *info)
{
	UINT8 spdAddress;

	DEVTREE_CONST struct device *dev = pcidev_on_root(0x18, 2);
	if (dev == NULL)
		return AGESA_ERROR;

	DEVTREE_CONST struct northbridge_amd_agesa_family15tn_config *config = dev->chip_info;
	if (config == NULL)
		return AGESA_ERROR;

	if (info->SocketId >= ARRAY_SIZE(config->spdAddrLookup))
		return AGESA_ERROR;
	if (info->MemChannelId >= ARRAY_SIZE(config->spdAddrLookup[0]))
		return AGESA_ERROR;
	if (info->DimmId >= ARRAY_SIZE(config->spdAddrLookup[0][0]))
		return AGESA_ERROR;

	spdAddress = config->spdAddrLookup
		[info->SocketId][info->MemChannelId][info->DimmId];

	if (spdAddress == 0)
		return AGESA_ERROR;

	int err = hudson_readSpd(spdAddress, (void *) info->Buffer, DDR3_SPD_SIZE);
	if (err)
		return AGESA_ERROR;
	return AGESA_SUCCESS;
}
