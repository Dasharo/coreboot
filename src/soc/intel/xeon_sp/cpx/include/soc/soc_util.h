/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef _SOC_UTIL_H_
#define _SOC_UTIL_H_

#include <hob_iiouds.h>
#include <hob_memmap.h>

struct iiostack_resource {
	uint8_t     no_of_stacks;
	STACK_RES   res[MAX_SOCKET * MAX_LOGIC_IIO_STACK];
};

uint8_t get_iiostack_info(struct iiostack_resource *info);

void xeonsp_init_cpu_config(void);
const IIO_UDS *get_iio_uds(void);
void get_core_thread_bits(uint32_t *core_bits, uint32_t *thread_bits);
void get_cpu_info_from_apicid(uint32_t apicid, uint32_t core_bits, uint32_t thread_bits,
	uint8_t *package, uint8_t *core, uint8_t *thread);
/* Return socket count, as obtained from FSP HOB */
unsigned int soc_get_num_cpus(void);

int get_platform_thread_count(void);
int get_threads_per_package(void);
const struct SystemMemoryMapHob *get_system_memory_map(void);

void set_bios_init_completion(void);

#endif /* _SOC_UTIL_H_ */
