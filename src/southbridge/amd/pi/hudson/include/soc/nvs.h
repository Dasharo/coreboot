/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef SOC_NVS_H
#define SOC_NVS_H

struct __packed global_nvs {
	// TODO
	uint8_t		rsv0;
	uint8_t		rsv1;
	uint8_t		rsv2;
	uint32_t	rsv3;
	uint64_t	pm1i; /* 0x07 - 0x0e - System Wake Source - PM1 Index */
	uint64_t	gpei; /* 0x0f - 0x16 - GPE Wake Source */
	uint8_t		rsv4;
	uint8_t		rsv5;
	uint8_t		rsv6;
};

#endif /* SOC_NVS_H */
