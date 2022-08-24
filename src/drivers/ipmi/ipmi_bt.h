/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __IPMI_BT_H
#define __IPMI_BT_H

#include <stdint.h>

/* Drops events from BMC and resets state of the BT interface, returns zero on success. */
int ipmi_bt_clear(uint16_t port);

#endif /* __IPMI_BT_H */
