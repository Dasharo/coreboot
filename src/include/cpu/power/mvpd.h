/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef CPU_PPC64_MVPD_H
#define CPU_PPC64_MVPD_H

#include <stdbool.h>
#include <stdint.h>

/* Single bucket within #V keyword of version 3 */
struct voltage_data
{
	uint16_t freq;		// MHz
	uint16_t vdd_voltage;
	uint16_t idd_current;
	uint16_t vcs_voltage;
	uint16_t ics_current;
} __attribute__((__packed__));

/* Single bucket within #V keyword of version 3 */
struct voltage_bucket_data
{
	uint8_t id;

	struct voltage_data nominal;
	struct voltage_data powersave;
	struct voltage_data turbo;
	struct voltage_data ultra_turbo;
	struct voltage_data powerbus;

	uint16_t sort_power_normal;
	uint16_t sort_power_turbo;

	uint8_t reserved[6];
} __attribute__((__packed__));

#define VOLTAGE_DATA_VERSION 3
#define VOLTAGE_BUCKET_COUNT 6

/* #V of LRP[0-5] in MVPD */
struct voltage_kwd
{
	uint8_t version;
	uint8_t pnp[3];
	struct voltage_bucket_data buckets[VOLTAGE_BUCKET_COUNT];
} __attribute__((__packed__));

struct region_device;

void mvpd_pnor_main(void);

void mvpd_device_init(void);

void mvpd_device_unmount(void);

const struct region_device *mvpd_device_ro(void);

/* Reads #V of one of LRP records (mind that there is only one buffer) */
const struct voltage_kwd *mvpd_get_voltage_data(uint8_t chip, int lrp);

/* Sets pg[0] and pg[1] to partial good values for MC01_CHIPLET_ID and
 * MC23_CHIPLET_ID respectively */
void mvpd_get_mcs_pg(uint8_t chip, uint16_t *pg);

/* Builds bitmask of functional cores based on Partial Good vector stored in PG
 * keyword of CP00 record */
uint64_t mvpd_get_available_cores(uint8_t chip);

/* Finds a specific keyword in MVPD partition and extracts it. *size is updated
 * to reflect needed or used space in the buffer. */
bool mvpd_extract_keyword(uint8_t chip, const char *record_name,
			  const char *kwd_name, uint8_t *buf, uint32_t *size);

/* Finds a specific ring in MVPD partition and extracts it */
bool mvpd_extract_ring(uint8_t chip, const char *record_name,
		       const char *kwd_name, uint8_t chiplet_id,
		       uint8_t even_odd, uint16_t ring_id,
		       uint8_t *buf, uint32_t buf_size);

#endif /* CPU_PPC64_MVPD_H */
