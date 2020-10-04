/* SPDX-License-Identifier: GPL-2.0-only */

#include <string.h>
#include <boot_device.h>
#include <bootstate.h>
#include <bootmode.h>
#include <console/console.h>
#include <cbmem.h>
#include <elog.h>
#include <fmap.h>
#include <ip_checksum.h>
#include <region_file.h>
#include <security/vboot/vboot_common.h>
#include <spi_flash.h>

#include "mrc_cache.h"

#define DEFAULT_MRC_CACHE	"RW_MRC_CACHE"
#define VARIABLE_MRC_CACHE	"RW_VAR_MRC_CACHE"
#define RECOVERY_MRC_CACHE	"RECOVERY_MRC_CACHE"
#define UNIFIED_MRC_CACHE	"UNIFIED_MRC_CACHE"

#define MRC_DATA_SIGNATURE       (('M'<<0)|('R'<<8)|('C'<<16)|('D'<<24))

struct mrc_metadata {
	uint32_t signature;
	uint32_t data_size;
	uint16_t data_checksum;
	uint16_t header_checksum;
	uint32_t version;
} __packed;

enum result {
	UPDATE_FAILURE		= -1,
	UPDATE_SUCCESS		= 0,
	ALREADY_UPTODATE	= 1
};

#define NORMAL_FLAG (1 << 0)
#define RECOVERY_FLAG (1 << 1)

struct cache_region {
	const char *name;
	uint32_t cbmem_id;
	int type;
	int elog_slot;
	int flags;
};

static const struct cache_region recovery_training = {
	.name = RECOVERY_MRC_CACHE,
	.cbmem_id = CBMEM_ID_MRCDATA,
	.type = MRC_TRAINING_DATA,
	.elog_slot = ELOG_MEM_CACHE_UPDATE_SLOT_RECOVERY,
#if CONFIG(HAS_RECOVERY_MRC_CACHE)
	.flags = RECOVERY_FLAG,
#else
	.flags = 0,
#endif
};

static const struct cache_region normal_training = {
	.name = DEFAULT_MRC_CACHE,
	.cbmem_id = CBMEM_ID_MRCDATA,
	.type = MRC_TRAINING_DATA,
	.elog_slot = ELOG_MEM_CACHE_UPDATE_SLOT_NORMAL,
	.flags = NORMAL_FLAG | RECOVERY_FLAG,
};

static const struct cache_region variable_data = {
	.name = VARIABLE_MRC_CACHE,
	.cbmem_id = CBMEM_ID_VAR_MRCDATA,
	.type = MRC_VARIABLE_DATA,
	.elog_slot = ELOG_MEM_CACHE_UPDATE_SLOT_VARIABLE,
	.flags = NORMAL_FLAG | RECOVERY_FLAG,
};

/* Order matters here for priority in matching. */
static const struct cache_region *cache_regions[] = {
	&recovery_training,
	&normal_training,
	&variable_data,
};

static int lookup_region_by_name(const char *name, struct region *r)
{
	if (fmap_locate_area(name, r) == 0)
		return 0;
	return -1;
}

static const struct cache_region *lookup_region_type(int type)
{
	int i;
	int flags;

	if (CONFIG(VBOOT_STARTS_IN_BOOTBLOCK) && vboot_recovery_mode_enabled())
		flags = RECOVERY_FLAG;
	else
		flags = NORMAL_FLAG;

	for (i = 0; i < ARRAY_SIZE(cache_regions); i++) {
		if (cache_regions[i]->type != type)
			continue;
		if ((cache_regions[i]->flags & flags) == flags)
			return cache_regions[i];
	}

	return NULL;
}

static const struct cache_region *lookup_region(struct region *r, int type)
{
	const struct cache_region *cr;

	cr = lookup_region_type(type);

	if (cr == NULL) {
		printk(BIOS_ERR, "MRC: failed to locate region type %d.\n",
		       type);
		return NULL;
	}

	if (lookup_region_by_name(cr->name, r) < 0)
		return NULL;

	return cr;
}

static int mrc_header_valid(struct region_device *rdev, struct mrc_metadata *md)
{
	uint16_t checksum;
	uint16_t checksum_result;
	size_t size;

	if (rdev_readat(rdev, md, 0, sizeof(*md)) < 0) {
		printk(BIOS_ERR, "MRC: couldn't read metadata\n");
		return -1;
	}

	if (md->signature != MRC_DATA_SIGNATURE) {
		printk(BIOS_ERR, "MRC: invalid header signature\n");
		return -1;
	}

	/* Compute checksum over header with 0 as the value. */
	checksum = md->header_checksum;
	md->header_checksum = 0;
	checksum_result = compute_ip_checksum(md, sizeof(*md));

	if (checksum != checksum_result) {
		printk(BIOS_ERR, "MRC: header checksum mismatch: %x vs %x\n",
			checksum, checksum_result);
		return -1;
	}

	/* Put back original. */
	md->header_checksum = checksum;

	/* Re-size the region device according to the metadata as a region_file
	 * does block allocation. */
	size = sizeof(*md) + md->data_size;
	if (rdev_chain(rdev, rdev, 0, size) < 0) {
		printk(BIOS_ERR, "MRC: size exceeds rdev size: %zx vs %zx\n",
			size, region_device_sz(rdev));
		return -1;
	}

	return 0;
}

static int mrc_data_valid(const struct mrc_metadata *md,
			  void *data, size_t data_size)
{
	uint16_t checksum;

	if (md->data_size != data_size)
		return -1;

	checksum = compute_ip_checksum(data, data_size);

	if (md->data_checksum != checksum) {
		printk(BIOS_ERR, "MRC: data checksum mismatch: %x vs %x\n",
			md->data_checksum, checksum);
		return -1;
	}

	return 0;
}

static int mrc_cache_get_latest_slot_info(const char *name,
				const struct region_device *backing_rdev,
				struct mrc_metadata *md,
				struct region_file *cache_file,
				struct region_device *rdev,
				bool fail_bad_data)
{
	/* Init and obtain a handle to the file data. */
	if (region_file_init(cache_file, backing_rdev) < 0) {
		printk(BIOS_ERR, "MRC: region file invalid in '%s'\n", name);
		return -1;
	}

	/* Provide a 0 sized region_device from here on out so the caller
	 * has a valid yet unusable region_device. */
	rdev_chain(rdev, backing_rdev, 0, 0);

	/* No data to return. */
	if (region_file_data(cache_file, rdev) < 0) {
		printk(BIOS_ERR, "MRC: no data in '%s'\n", name);
		return fail_bad_data ? -1 : 0;
	}

	/* Validate header and resize region to reflect actual usage on the
	 * saved medium (including metadata and data). */
	if (mrc_header_valid(rdev, md) < 0) {
		printk(BIOS_ERR, "MRC: invalid header in '%s'\n", name);
		return fail_bad_data ? -1 : 0;
	}

	return 0;
}

static int mrc_cache_find_current(int type, uint32_t version,
				  struct region_device *rdev,
				  struct mrc_metadata *md)
{
	const struct cache_region *cr;
	struct region region;
	struct region_device read_rdev;
	struct region_file cache_file;
	size_t data_size;
	const size_t md_size = sizeof(*md);
	const bool fail_bad_data = true;

	cr = lookup_region(&region, type);

	if (cr == NULL)
		return -1;

	if (boot_device_ro_subregion(&region, &read_rdev) < 0)
		return -1;

	if (mrc_cache_get_latest_slot_info(cr->name,
					   &read_rdev,
					   md,
					   &cache_file,
					   rdev,
					   fail_bad_data) < 0)
		return -1;

	if (version != md->version) {
		printk(BIOS_INFO, "MRC: version mismatch: %x vs %x\n",
			md->version, version);
		return -1;
	}

	/* Re-size rdev to only contain the data. i.e. remove metadata. */
	data_size = md->data_size;
	return rdev_chain(rdev, rdev, md_size, data_size);
}

int mrc_cache_load_current(int type, uint32_t version, void *buffer,
			   size_t buffer_size)
{
	struct region_device rdev;
	struct mrc_metadata md;
	size_t data_size;

	if (mrc_cache_find_current(type, version, &rdev, &md) < 0)
		return -1;

	data_size = region_device_sz(&rdev);
	if (buffer_size < data_size)
		return -1;

	if (rdev_readat(&rdev, buffer, 0, data_size) != data_size)
		return -1;

	if (mrc_data_valid(&md, buffer, data_size) < 0)
		return -1;

	return 0;
}

void *mrc_cache_current_mmap_leak(int type, uint32_t version,
				  size_t *data_size)
{
	struct region_device rdev;
	void *data;
	size_t region_device_size;
	struct mrc_metadata md;

	if (mrc_cache_find_current(type, version, &rdev, &md) < 0)
		return NULL;

	region_device_size = region_device_sz(&rdev);
	if (data_size)
		*data_size = region_device_size;
	data = rdev_mmap_full(&rdev);

	if (data == NULL) {
		printk(BIOS_INFO, "MRC: mmap failure.\n");
		return NULL;
	}

	if (mrc_data_valid(&md, data, region_device_size) < 0)
		return NULL;

	return data;
}

static bool mrc_cache_needs_update(const struct region_device *rdev,
				   const struct mrc_metadata *new_md,
				   const void *new_data, size_t new_data_size)
{
	void *mapping, *data_mapping;
	size_t size = region_device_sz(rdev);
	bool need_update = false;

	if (new_data_size != size)
		return true;

	mapping = rdev_mmap_full(rdev);
	if (mapping == NULL) {
		printk(BIOS_ERR, "MRC: cannot mmap existing cache.\n");
		return true;
	}
	data_mapping = mapping + sizeof(struct mrc_metadata);

	/* we need to compare the md and the data separately */
	/* check the mrc_metadata */
	if (memcmp(new_md, mapping, sizeof(struct mrc_metadata)))
		need_update = true;

	/* check the data */
	if (!need_update && memcmp(new_data, data_mapping, new_data_size))
		need_update = true;

	rdev_munmap(rdev, mapping);

	return need_update;
}

static void log_event_cache_update(uint8_t slot, enum result res)
{
	const int type = ELOG_TYPE_MEM_CACHE_UPDATE;
	struct elog_event_mem_cache_update event = {
		.slot = slot
	};

	/* Filter through interesting events only */
	switch (res) {
	case UPDATE_FAILURE:
		event.status = ELOG_MEM_CACHE_UPDATE_STATUS_FAIL;
		break;
	case UPDATE_SUCCESS:
		event.status = ELOG_MEM_CACHE_UPDATE_STATUS_SUCCESS;
		break;
	default:
		return;
	}

	if (elog_add_event_raw(type, &event, sizeof(event)) < 0)
		printk(BIOS_ERR, "Failed to log mem cache update event.\n");
}

/* During ramstage this code purposefully uses incoherent transactions between
 * read and write. The read assumes a memory-mapped boot device that can be used
 * to quickly locate and compare the up-to-date data. However, when an update
 * is required it uses the writeable region access to perform the update. */
static void update_mrc_cache_by_type(int type,
				     struct mrc_metadata *new_md,
				     const void *new_data,
				     size_t new_data_size)
{
	const struct cache_region *cr;
	struct region region;
	struct region_device read_rdev;
	struct region_device write_rdev;
	struct region_file cache_file;
	struct mrc_metadata md;
	struct incoherent_rdev backing_irdev;
	const struct region_device *backing_rdev;
	struct region_device latest_rdev;
	const bool fail_bad_data = false;

	cr = lookup_region(&region, type);

	if (cr == NULL)
		return;

	printk(BIOS_DEBUG, "MRC: Checking cached data update for '%s'.\n",
		cr->name);

	if (boot_device_ro_subregion(&region, &read_rdev) < 0)
		return;

	if (boot_device_rw_subregion(&region, &write_rdev) < 0)
		return;

	backing_rdev = incoherent_rdev_init(&backing_irdev, &region, &read_rdev,
						&write_rdev);

	if (backing_rdev == NULL)
		return;

	/* Note that mrc_cache_get_latest_slot_info doesn't check the
	 * validity of the current slot.  If the slot is invalid,
	 * we'll overwrite it anyway when we update the mrc_cache.
	 */
	if (mrc_cache_get_latest_slot_info(cr->name,
					   backing_rdev,
					   &md,
					   &cache_file,
					   &latest_rdev,
					   fail_bad_data) < 0)

		return;

	if (!mrc_cache_needs_update(&latest_rdev,
				    new_md, new_data, new_data_size)) {
		printk(BIOS_DEBUG, "MRC: '%s' does not need update.\n", cr->name);
		log_event_cache_update(cr->elog_slot, ALREADY_UPTODATE);
		return;
	}

	printk(BIOS_DEBUG, "MRC: cache data '%s' needs update.\n", cr->name);

	struct update_region_file_entry entries[] = {
		[0] = {
			.size = sizeof(struct mrc_metadata),
			.data = new_md,
		},
		[1] = {
			.size = new_data_size,
			.data = new_data,
		},
	};
	if (region_file_update_data_arr(&cache_file, entries, ARRAY_SIZE(entries)) < 0) {
		printk(BIOS_ERR, "MRC: failed to update '%s'.\n", cr->name);
		log_event_cache_update(cr->elog_slot, UPDATE_FAILURE);
	} else {
		printk(BIOS_DEBUG, "MRC: updated '%s'.\n", cr->name);
		log_event_cache_update(cr->elog_slot, UPDATE_SUCCESS);
	}
}

/* Read flash status register to determine if write protect is active */
static int nvm_is_write_protected(void)
{
	u8 sr1;
	u8 wp_gpio;
	u8 wp_spi;

	if (!CONFIG(CHROMEOS))
		return 0;

	if (!CONFIG(BOOT_DEVICE_SPI_FLASH))
		return 0;

	/* Read Write Protect GPIO if available */
	wp_gpio = get_write_protect_state();

	/* Read Status Register 1 */
	if (spi_flash_status(boot_device_spi_flash(), &sr1) < 0) {
		printk(BIOS_ERR, "Failed to read SPI status register 1\n");
		return -1;
	}
	wp_spi = !!(sr1 & 0x80);

	printk(BIOS_DEBUG, "SPI flash protection: WPSW=%d SRP0=%d\n",
		wp_gpio, wp_spi);

	return wp_gpio && wp_spi;
}

/* Apply protection to a range of flash */
static int nvm_protect(const struct region *r)
{
	if (!CONFIG(MRC_SETTINGS_PROTECT))
		return 0;

	if (!CONFIG(BOOT_DEVICE_SPI_FLASH))
		return 0;

	return spi_flash_ctrlr_protect_region(boot_device_spi_flash(), r, WRITE_PROTECT);
}

/* Protect mrc region with a Protected Range Register */
static int protect_mrc_cache(const char *name)
{
	struct region region;

	if (!CONFIG(MRC_SETTINGS_PROTECT))
		return 0;

	if (lookup_region_by_name(name, &region) < 0) {
		printk(BIOS_INFO, "MRC: Could not find region '%s'\n", name);
		return -1;
	}

	if (nvm_is_write_protected() <= 0) {
		printk(BIOS_INFO, "MRC: NOT enabling PRR for '%s'.\n", name);
		return 0;
	}

	if (nvm_protect(&region) < 0) {
		printk(BIOS_ERR, "MRC: ERROR setting PRR for '%s'.\n", name);
		return -1;
	}

	printk(BIOS_INFO, "MRC: Enabled Protected Range on '%s'.\n", name);
	return 0;
}

static void protect_mrc_region(void)
{
	/*
	 * Check if there is a single unified region that encompasses both
	 * RECOVERY_MRC_CACHE and DEFAULT_MRC_CACHE. In that case protect the
	 * entire region using a single PRR.
	 *
	 * If we are not able to protect the entire region, try protecting
	 * individual regions next.
	 */
	if (protect_mrc_cache(UNIFIED_MRC_CACHE) == 0)
		return;

	if (CONFIG(HAS_RECOVERY_MRC_CACHE))
		protect_mrc_cache(RECOVERY_MRC_CACHE);

	protect_mrc_cache(DEFAULT_MRC_CACHE);
}

static void invalidate_normal_cache(void)
{
	struct region_file cache_file;
	struct region_device rdev;
	const char *name = DEFAULT_MRC_CACHE;
	const uint32_t invalid = ~MRC_DATA_SIGNATURE;

	/* Invalidate only on recovery mode with retraining enabled. */
	if (!vboot_recovery_mode_enabled())
		return;
	if (!get_recovery_mode_retrain_switch())
		return;

	if (fmap_locate_area_as_rdev_rw(name, &rdev) < 0) {
		printk(BIOS_ERR, "MRC: Couldn't find '%s' region. Invalidation failed\n",
			name);
		return;
	}

	if (region_file_init(&cache_file, &rdev) < 0) {
		printk(BIOS_ERR, "MRC: region file invalid for '%s'. Invalidation failed\n",
			name);
		return;
	}

	/* Push an update that consists of 4 bytes that is smaller than the
	 * MRC metadata as well as an invalid signature. */
	if (region_file_update_data(&cache_file, &invalid, sizeof(invalid)) < 0)
		printk(BIOS_ERR, "MRC: invalidation failed for '%s'.\n", name);
}

static void update_mrc_cache_from_cbmem(int type)
{
	const struct cache_region *cr;
	struct region region;
	const struct cbmem_entry *to_be_updated;

	cr = lookup_region(&region, type);

	if (cr == NULL) {
		printk(BIOS_ERR, "MRC: could not find cache_region type %d\n", type);
		return;
	}

	to_be_updated = cbmem_entry_find(cr->cbmem_id);

	if (to_be_updated == NULL) {
		printk(BIOS_INFO, "MRC: No data in cbmem for '%s'.\n",
		       cr->name);
		return;
	}

	update_mrc_cache_by_type(type,
				 /* pointer to mrc_cache entry metadata header */
				 cbmem_entry_start(to_be_updated),
				 /* pointer to start of mrc_cache entry data */
				 cbmem_entry_start(to_be_updated) +
					sizeof(struct mrc_metadata),
				 /* size of just data portion of the entry */
				 cbmem_entry_size(to_be_updated) -
					sizeof(struct mrc_metadata));
}

static void finalize_mrc_cache(void *unused)
{
	if (CONFIG(MRC_STASH_TO_CBMEM)) {
		update_mrc_cache_from_cbmem(MRC_TRAINING_DATA);

		if (CONFIG(MRC_SETTINGS_VARIABLE_DATA))
			update_mrc_cache_from_cbmem(MRC_VARIABLE_DATA);
	}

	if (CONFIG(MRC_CLEAR_NORMAL_CACHE_ON_RECOVERY_RETRAIN))
		invalidate_normal_cache();

	protect_mrc_region();
}

int mrc_cache_stash_data(int type, uint32_t version, const void *data,
			 size_t size)
{
	const struct cache_region *cr;

	cr = lookup_region_type(type);
	if (cr == NULL) {
		printk(BIOS_ERR, "MRC: failed to add to cbmem for type %d.\n",
			type);
		return -1;
	}

	struct mrc_metadata md = {
		.signature = MRC_DATA_SIGNATURE,
		.data_size = size,
		.version = version,
		.data_checksum = compute_ip_checksum(data, size),
	};
	md.header_checksum =
		compute_ip_checksum(&md, sizeof(struct mrc_metadata));

	if (CONFIG(MRC_STASH_TO_CBMEM)) {
		/* Store data in cbmem for use in ramstage */
		struct mrc_metadata *cbmem_md;
		size_t cbmem_size;
		cbmem_size = sizeof(*cbmem_md) + size;

		cbmem_md = cbmem_add(cr->cbmem_id, cbmem_size);

		if (cbmem_md == NULL) {
			printk(BIOS_ERR, "MRC: failed to add '%s' to cbmem.\n",
			       cr->name);
			return -1;
		}

		memcpy(cbmem_md, &md, sizeof(*cbmem_md));
		/* cbmem_md + 1 is the pointer to the mrc_cache data */
		memcpy(cbmem_md + 1, data, size);
	} else {
		/* Otherwise store to mrc_cache right away */
		update_mrc_cache_by_type(type, &md, data, size);
	}
	return 0;
}

/*
 * Ensures MRC training data is stored into SPI after PCI enumeration is done.
 * Some implementations may require this to be later than others.
 */
#if CONFIG(MRC_WRITE_NV_LATE)
BOOT_STATE_INIT_ENTRY(BS_OS_RESUME_CHECK, BS_ON_ENTRY, finalize_mrc_cache, NULL);
#else
BOOT_STATE_INIT_ENTRY(BS_DEV_ENUMERATE, BS_ON_EXIT, finalize_mrc_cache, NULL);
#endif
