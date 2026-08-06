/* SPDX-License-Identifier: GPL-2.0-only */

#include <security/intel/stm/SmmStm.h>
#include <security/intel/stm/StmPlatformResource.h>

// Nuvoton SIO resources
static STM_RSC_IO_DESC rsc_sio_io = {{IO_RANGE, sizeof(STM_RSC_IO_DESC)},
				     0x4e, 2};

static STM_RSC_IO_DESC rsc_hwm_io = {{IO_RANGE, sizeof(STM_RSC_IO_DESC)},
				     0xa20, 8};

int mainboard_stm_add_resources(void)
{
	int Status = 0;

	Status |= add_pi_resource((void *)&rsc_sio_io, 1);
	Status |= add_pi_resource((void *)&rsc_hwm_io, 1);

	return Status;
}
