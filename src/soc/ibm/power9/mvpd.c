/* SPDX-License-Identifier: GPL-2.0-only */

#include <cpu/power/mvpd.h>

#include <commonlib/region.h>
#include <console/console.h>
#include <cpu/power/vpd.h>
#include <endian.h>
#include <stdint.h>
#include <string.h>

#include "tor.h"
#include "rs4.h"

#define MVPD_TOC_ENTRIES 32
#define MVPD_TOC_SIZE    (MVPD_TOC_ENTRIES*sizeof(struct mvpd_toc_entry))

/* Each entry points to a VPD record */
struct mvpd_toc_entry {
	char name[4];		// Name without trailing NUL byte
	uint16_t offset;	// Offset from the beginning of partition in LE
	uint8_t reserved[2];	// Unused
} __attribute__((packed));

static struct mvpd_toc_entry *find_record(struct mvpd_toc_entry *toc,
					  const char *name)
{
	int i = 0;
	for (i = 0; i < MVPD_TOC_ENTRIES; ++i) {
		if (memcmp(toc[i].name, name, VPD_RECORD_NAME_LEN) == 0)
			return &toc[i];
	}
	return NULL;
}

/* Checks if rings ends here.  End is marked by an "END" string. */
static bool is_end_of_rings(const uint8_t *buf_left, uint32_t len_left)
{
	return (len_left < 3 || memcmp(buf_left, "END", 3) == 0);
}

/* Finds specific ring by combination of chiplet and ring ids */
static struct ring_hdr *find_ring_step(uint8_t chiplet_id, uint16_t ring_id,
				       const uint8_t **buf_left,
				       uint32_t *len_left)
{
	uint32_t even_odd_mask = 0;
	struct ring_hdr *hdr = (struct ring_hdr *)*buf_left;

	if (*len_left < sizeof(struct ring_hdr) ||
	    be16toh(hdr->magic) != RS4_MAGIC)
		return NULL;

	*buf_left += be16toh(hdr->size);
	*len_left -= be16toh(hdr->size);

	switch (even_odd_mask) {
		case EX_L3_REPR: even_odd_mask = 0x00001000; break;
		case EX_L2_REPR: even_odd_mask = 0x00000400; break;
		case EX_L3_REFR_TIME:
		case EX_L3_REFR_REPR: even_odd_mask = 0x00000040; break;

		default: even_odd_mask = 0; break;
	}

	if (be16toh(hdr->ring_id) != ring_id)
		return NULL;
	if (((be32toh(hdr->scan_addr) >> 24) & 0xFF) != chiplet_id)
		return NULL;
	if (even_odd_mask != 0 && !(be32toh(hdr->scan_addr) & even_odd_mask))
		return NULL;

	return hdr;
}

/* Searches for a specific ring in a keyword */
static struct ring_hdr *find_ring(uint8_t chiplet_id, uint16_t ring_id,
				  const uint8_t *buf, uint32_t buf_len)
{
	/* Skip version number */
	--buf_len;
	++buf;

	while (!is_end_of_rings(buf, buf_len)) {
		struct ring_hdr *ring = find_ring_step(chiplet_id, ring_id,
						       &buf, &buf_len);
		if (ring != NULL)
			return ring;
	}

	return NULL;
}

/* Finds a specific ring in MVPD partition and extracts it */
bool mvpd_extract_ring(const char *record_name, const char *kwd_name,
		       uint8_t chiplet_id, uint16_t ring_id, uint8_t *buf,
		       uint32_t buf_size)
{
	const struct region_device *mvpd_device;

	uint8_t mvpd_buf[MVPD_TOC_SIZE];
	struct mvpd_toc_entry *mvpd_toc = (struct mvpd_toc_entry *)mvpd_buf;

	struct mvpd_toc_entry *cp00 = NULL;
	uint16_t cp00_offset = 0;
	const uint8_t *cp00_data = NULL;
	uint16_t cp00_size = 0;

	const uint8_t *rings = NULL;
	size_t rings_size = 0;

	struct ring_hdr *ring = NULL;
	uint32_t ring_size = 0;

	mvpd_device_init();
	mvpd_device = mvpd_device_ro();

	/* Copy all TOC at once */
	if (rdev_readat(mvpd_device, mvpd_buf, 0,
			sizeof(mvpd_buf)) != sizeof(mvpd_buf))
		die("Failed to read MVPD TOC!\n");

	cp00 = find_record(mvpd_toc, record_name);
	if (cp00 == NULL)
		die("Failed to find %s MVPD record!\n", record_name);
	cp00_offset = le16toh(cp00->offset);

	/* Read size of the record */
	if (rdev_readat(mvpd_device, &cp00_size, cp00_offset,
			sizeof(cp00_size)) != sizeof(cp00_size))
		die("Failed to read size of %s!\n", record_name);

	cp00_data = rdev_mmap(mvpd_device, cp00_offset, cp00_size);
	if (!cp00_data)
		die("Failed to map %s record!\n", record_name);

	rings = vpd_find_kwd(cp00_data, record_name, kwd_name, &rings_size);
	if (rings == NULL)
		die("Failed to find %s keyword in %s!\n", kwd_name,
		    record_name);

	ring = find_ring(chiplet_id, ring_id, rings, rings_size);
	if (ring == NULL) {
		if (rdev_munmap(mvpd_device, (void *)cp00_data))
			die("Failed to unmap %s record!\n", record_name);

		return false;
	}

	ring_size = be32toh(ring->size);
	if (buf_size >= ring_size)
		memcpy(buf, ring, ring_size);

	if (rdev_munmap(mvpd_device, (void *)cp00_data))
		die("Failed to unmap %s record!\n", record_name);

	return (buf_size >= ring_size);
}
