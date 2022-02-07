/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef DEVICE_HYPERTRANSPORT_H
#define DEVICE_HYPERTRANSPORT_H

#include <device/hypertransport_def.h>

/* TODO: Check HT specs for better names for these. */
#define LINK_CONNECTED		(1 << 0)
#define INIT_COMPLETE		(1 << 1)
#define NON_COHERENT			(1 << 2)
#define CONNECTION_PENDING	(1 << 4)
bool ht_is_non_coherent_link(struct bus *link);

unsigned int hypertransport_scan_chain(struct bus *bus);

extern struct device_operations default_ht_ops_bus;

#define HT_IO_HOST_ALIGN 4096
#define HT_MEM_HOST_ALIGN (1024*1024)

#endif /* DEVICE_HYPERTRANSPORT_H */
