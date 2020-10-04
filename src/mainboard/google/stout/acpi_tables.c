/* SPDX-License-Identifier: GPL-2.0-only */

#include <acpi/acpi.h>
#include <acpi/acpi_gnvs.h>
#include <device/device.h>
#include <vendorcode/google/chromeos/gnvs.h>
#include <bootmode.h>
#include <ec/quanta/it8518/ec.h>
#include "ec.h"
#include "onboard.h"

#include <southbridge/intel/bd82x6x/pch.h>
#include <southbridge/intel/bd82x6x/nvs.h>
#include "thermal.h"

void acpi_create_gnvs(struct global_nvs *gnvs)
{
	/* Disable USB ports in S3 by default */
	gnvs->s3u0 = 0;
	gnvs->s3u1 = 0;

	/* Disable USB ports in S5 by default */
	gnvs->s5u0 = 0;
	gnvs->s5u1 = 0;

#if CONFIG(CHROMEOS)
	gnvs->chromeos.vbt2 = get_recovery_mode_switch() ?
			ACTIVE_ECFW_RO : ACTIVE_ECFW_RW;
#endif

	/* EC handles all thermal and fan control on Stout. */
	gnvs->tcrt = CRITICAL_TEMPERATURE;
	gnvs->tpsv = PASSIVE_TEMPERATURE;
	gnvs->tmax = MAX_TEMPERATURE;

	// the lid is open by default.
	gnvs->lids = 1;

	/* XHCI Mode */
	gnvs->xhci = XHCI_MODE;
}
