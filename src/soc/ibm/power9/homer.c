/* SPDX-License-Identifier: GPL-2.0-only */

#include <assert.h>
#include <commonlib/region.h>
#include <console/console.h>
#include <cpu/power/scom.h>
#include <string.h>		// memset, memcpy

#include "chip.h"
#include "homer.h"
#include "xip.h"

#include <lib.h>

extern void mount_part_from_pnor(const char *part_name,
				 struct mmap_helper_region_device *mdev);

static size_t copy_section(void *dst, struct xip_section *section, void *base,
                           uint8_t dd)
{
	if (!section->dd_support) {
		memcpy(dst, base + section->offset, section->size);
		return section->size;
	}

	struct dd_container *cont = base + section->offset;
	int i;

	assert(cont->magic == DD_CONTAINER_MAGIC);
	for (i = 0; i < cont->num; i++) {
		if (cont->blocks[i].dd == dd) {
			memcpy(dst, (void *)cont + cont->blocks[i].offset,
			       cont->blocks[i].size);
			return cont->blocks[i].size;
		}
	}

	die("XIP: Can't find container for DD=%x\n", dd);
}

static void build_spge(struct homer_st *homer, struct xip_sgpe_header *sgpe,
                       uint8_t dd)
{
	struct sgpe_img_header *hdr;
	size_t size;

	assert(sgpe->magic == XIP_MAGIC_SGPE);

	/* SPGE header */
	size = copy_section(&homer->qpmr.spge.header, &sgpe->qpmr, sgpe, dd);
	assert(size <= sizeof(struct qpmr_header));

	/*
	 * 0xFFF00000 (SRAM base) + 4k (IPC) + 60k (GPE0) + 64k (GPE1) + 50k (PGPE)
	 * + 2k (aux) + 2k (shared) = 0xFFF2D800
	 *
	 * WARNING: I have no idea if this is constant or depends on SPGE version.
	 */
	assert(homer->qpmr.spge.header.sram_region_start == 0xFFF2D800);
	assert(homer->qpmr.spge.header.sram_region_size == SPGE_SRAM_IMG_SIZE);
	/*
	 * Apart from these the only filled fields (same values for all DDs) are:
	 * - magic ("XIP SPGE")
	 * - build_date
	 * - build_ver
	 * - img_offset (0x0a00, overwritten with the same value later by code)
	 * - img_len (0xbf64, ~49kB, overwritten with the same value later by code)
	 */

	/* SPGE L1 bootloader */
	size = copy_section(&homer->qpmr.spge.l1_bootloader, &sgpe->l1_bootloader,
	                    sgpe, dd);
	homer->qpmr.spge.header.l1_offset = offsetof(struct qpmr_st,
	                                             spge.l1_bootloader);
	assert(size <= PGE_BOOTLOADER_SIZE);

	/* SPGE L2 bootloader */
	size = copy_section(&homer->qpmr.spge.l2_bootloader, &sgpe->l2_bootloader,
	                    sgpe, dd);
	homer->qpmr.spge.header.l2_offset = offsetof(struct qpmr_st,
	                                             spge.l2_bootloader);
	homer->qpmr.spge.header.l2_len = size;
	assert(size <= PGE_BOOTLOADER_SIZE);

	/* SPGE HCODE */
	size = copy_section(&homer->qpmr.spge.sram_image, &sgpe->hcode, sgpe, dd);
	homer->qpmr.spge.header.img_offset = offsetof(struct qpmr_st,
	                                              spge.sram_image);
	homer->qpmr.spge.header.img_len = size;
	assert(size <= SPGE_SRAM_IMG_SIZE);

	/* Cache SCOM region */
	homer->qpmr.spge.header.scom_offset =
	         offsetof(struct qpmr_st, cache_scom_region);
	homer->qpmr.spge.header.scom_len = CACHE_SCOM_REGION_SIZE;

	/* Update SRAM image header */
	hdr = (struct sgpe_img_header *)
	      &homer->qpmr.spge.sram_image[SPGE_IMG_HEADER_OFFSET];
	hdr->ivpr_addr = homer->qpmr.spge.header.sram_region_start;
	hdr->cmn_ring_occ_offset = homer->qpmr.spge.header.img_len;
	hdr->cmn_ring_ovrd_occ_offset = 0;
	hdr->spec_ring_occ_offset = 0;
	hdr->scom_offset = 0;
	/* Nest frequency divided by 64. */
	hdr->timebase_hz = (1866 * MHz) / 64;

	/* SPGE auxiliary functions */
	/*
	 * TODO: check if it is really enabled. This comes from hostboot attributes,
	 * but I don't know if/where those are set.
	 */
	hdr->aux_control = 1 << 24;
	/*
	 * 0x80000000 (HOMER in OCI PBA memory space) + 1M (QPMR offset)
	 * + 512k (offset to aux)
	 */
	hdr->aux_offset = 0x80000000 + offsetof(struct homer_st, qpmr.aux);
	hdr->aux_len = CACHE_SCOM_AUX_SIZE;
	homer->qpmr.spge.header.enable_24x7_ima = 1;
	homer->qpmr.spge.header.aux_offset = offsetof(struct homer_st, qpmr.aux);
	homer->qpmr.spge.header.aux_len = CACHE_SCOM_AUX_SIZE;
}

/*
 * This logic is for SMF disabled only!
 */
void build_homer_image(void *homer_bar)
{
	struct mmap_helper_region_device mdev = {0};
	struct homer_st *homer = homer_bar;
	struct xip_hw_header *hw = homer_bar;
	uint8_t dd = get_dd();

	printk(BIOS_ERR, "DD%2.2x\n", dd);

	memset(homer_bar, 0, 4 * MiB);

	/*
	 * This will work as long as we don't call mmap(). mmap() calls
	 * mem_poll_alloc() which doesn't check if mdev->pool is valid or at least
	 * not NULL.
	 */
	mount_part_from_pnor("HCODE", &mdev);
	/* First MB of HOMER is unused, we can write OCC image from PNOR there. */
	rdev_readat(&mdev.rdev, homer_bar, 0, 1 * MiB);

	build_spge(homer, (struct xip_sgpe_header *)(homer_bar + hw->sgpe.offset),
	           dd);

	//~ hexdump(&homer->qpmr, sgpe->qpmr.size);
}
