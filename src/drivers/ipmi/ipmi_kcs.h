/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __IPMI_KCS_H
#define __IPMI_KCS_H

#include <stdint.h>

void ipmi_bmc_version(uint8_t *ipmi_bmc_major_revision, uint8_t *ipmi_bmc_minor_revision);

#endif
