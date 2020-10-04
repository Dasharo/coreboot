/* SPDX-License-Identifier: BSD-2-Clause */

#ifndef _SMM_STM_H_
#define _SMM_STM_H_

#include "StmApi.h"
#include <stdbool.h>

/*
 *  Load STM image.
 *
 *  @retval SUCCESS           STM is loaded to MSEG
 *  @retval BUFFER_TOO_SMALL  MSEG is too small
 *  @retval UNSUPPORTED       MSEG is not enabled
 */
int load_stm_image(uintptr_t mseg);

void stm_setup(
	uintptr_t mseg, int cpu, uintptr_t smbase,
	uintptr_t smbase_base, uint32_t offset32);

/*
 *  Add resources in list to database. Allocate new memory areas as needed.
 *
 *  @param resource_list  A pointer to resource list to be added
 *  @param num_entries    Optional number of entries.
 *			  If 0, list must be terminated by END_OF_RESOURCES.
 *
 *  @retval SUCCESS            If resources are added
 *  @retval INVALID_PARAMETER  If nested procedure detected resource failure
 *  @retval OUT_OF_RESOURCES    If nested procedure returned it and we cannot
 *				allocate more areas.
 */
int add_pi_resource(STM_RSC *resource_list, uint32_t num_entries);

/*
 *  Delete resources in list to database.
 *
 *  @param resource_list  A pointer to resource list to be deleted
 *			  NULL means delete all resources.
 *  @param num_entries    Optional number of entries.
 *			  If 0, list must be terminated by END_OF_RESOURCES.
 *
 *  @retval SUCCESS           If resources are deleted
 *  @retval NVALID_PARAMETER  If nested procedure detected resource fail
 */
int delete_pi_resource(STM_RSC *resource_list, uint32_t num_entries);

/*
 *  Get BIOS resources.
 *
 *  @param resource_list  A pointer to resource list to be filled
 *  @param resource_size  On input it means size of resource list input.
 *			 On output it means size of resource list filled,
 *			 or the size of resource list to be filled if
 *			 size is too small.
 *
 *  @retval SUCCESS            If resources are returned.
 *  @retval BUFFER_TOO_SMALL   If resource list buffer is too small to
 *				hold the whole resources.
 */
int get_pi_resource(STM_RSC *resource_list, uint32_t *resource_size);

/*
 *  This function notifies the STM of a resource change.
 *
 *  @param stm_resource BIOS STM resource
 */
void notify_stm_resource_change(void *stm_resource);

/*
 *  This function returns the pointer to the STM BIOS resource list.
 *
 *  @return BIOS STM resource
 */
void *get_stm_resource(void);

void setup_smm_descriptor(void *smbase, void *base_smbase, int32_t apic_id,
				int32_t entry32_off);

/*
 *  Check STM image size.
 *
 *  @param stm_image      STM image
 *  @param stm_image_size  STM image size
 *
 *  @retval true  check pass
 *  @retval false check fail
 */
bool stm_check_stm_image(void *stm_image, uint32_t stm_image_size);

/*
 * Create 4G page table for STM.
 * 4M Non-PAE page table in IA32 version.
 *
 *  @param page_table_base        The page table base in MSEG
 */
void stm_gen_4g_pagetable_ia32(uint32_t pagetable_base);

/*
 *  Create 4G page table for STM.
 *  2M PAE page table in X64 version.
 *
 *  @param pagetable_base        The page table base in MSEG
 */
void stm_gen_4g_pagetable_x64(uint32_t pagetable_base);

#endif
