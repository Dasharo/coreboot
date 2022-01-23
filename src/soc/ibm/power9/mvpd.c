/* SPDX-License-Identifier: GPL-2.0-only */

#include <cpu/power/mvpd.h>

#include <assert.h>
#include <commonlib/region.h>
#include <console/console.h>
#include <cpu/power/proc.h>
#include <cpu/power/scom.h>
#include <cpu/power/vpd.h>
#include <device/i2c_simple.h>
#include <endian.h>
#include <stdint.h>
#include <string.h>
#include <symbols.h>

#include "tor.h"
#include "rs4.h"

#define MVPD_TOC_ENTRIES 32
#define MVPD_TOC_SIZE    (MVPD_TOC_ENTRIES*sizeof(struct mvpd_toc_entry))

#define EEPROM_CHIP_SIZE (64 * KiB)

/* Each entry points to a VPD record */
struct mvpd_toc_entry {
	char name[4];		// Name without trailing NUL byte
	uint16_t offset;	// Offset from the beginning of partition in LE
	uint8_t reserved[2];	// Unused
} __attribute__((packed));

struct pt_record {
	char record_name[4];
	/* All of these fields are in little endian */
	uint16_t record_type;
	uint16_t record_offset;
	uint16_t record_length;
	uint16_t ecc_offset;
	uint16_t ecc_length;
} __attribute__((packed));

/*
 * Configuration of EEPROM with VPD data in talos.xml:
 *
 * <attribute>
 *  <id>EEPROM_VPD_PRIMARY_INFO</id>
 *  <default>
 *   <field>
 *    <id>i2cMasterPath</id>
 *    <value>
 *     /sys-0/node-0/motherboard-0/proc_socket-0/sforza-0/p9_proc_s/i2c-master-prom0-mvpd-primary/
 *    </value>
 *   </field>
 *   <field><id>port</id><value>0</value></field>
 *   <field><id>devAddr</id><value>0xA0</value></field>
 *   <field><id>engine</id><value>1</value></field>
 *   <field><id>byteAddrOffset</id><value>0x02</value></field>
 *   <field><id>maxMemorySizeKB</id><value>0x80</value></field>
 *   <field><id>chipCount</id><value>0x02</value></field>
 *   <field><id>writePageSize</id><value>0x80</value></field>
 *   <field><id>writeCycleTime</id><value>0x0A</value></field>
 *  </default>
 * </attribute>
 */

/* Reads from a single EEPROM chip, which is deduced from offset. Returns number
 * of bytes read. */
static int read_eeprom_chip(uint8_t cpu, uint32_t offset, void *data, uint16_t len)
{
	const unsigned int bus = (cpu == 0 ? 1 : FSI_I2C_BUS);
	uint16_t addr = 0xA0;
	uint16_t slave = 0;
	uint16_t actual_offset = 0;

	struct i2c_msg seg[2];

	/* Two chips at two different addresses */
	if (offset >= EEPROM_CHIP_SIZE) {
		offset -= EEPROM_CHIP_SIZE;
		addr += 0x02;
	}

	assert(offset < EEPROM_CHIP_SIZE);
	actual_offset = offset;

	/* Most-significant bit is port number */
	slave = addr >> 1;

	seg[0].flags = 0;
	seg[0].slave = slave;
	seg[0].buf   = (uint8_t *)&actual_offset;
	seg[0].len   = sizeof(actual_offset);
	seg[1].flags = I2C_M_RD;
	seg[1].slave = slave;
	seg[1].buf   = data;
	seg[1].len   = len;

	return i2c_transfer(bus, seg, ARRAY_SIZE(seg)) - sizeof(actual_offset);
}

/* Reads from EEPROM handling accesses across chip boundaries (64 KiB).  Returns
 * number of bytes read. */
static int read_eeprom(uint8_t cpu, uint32_t offset, void *data, uint32_t len)
{
	int ret_value1 = 0;
	int ret_value2 = 0;
	uint16_t len1 = 0;
	uint16_t len2 = 0;

	assert(len != 0);
	if (offset / EEPROM_CHIP_SIZE == (offset + len - 1) / EEPROM_CHIP_SIZE)
		return read_eeprom_chip(cpu, offset, data, len);

	len1 = EEPROM_CHIP_SIZE - offset;
	len2 = len - len1;

	ret_value1 = read_eeprom_chip(cpu, offset, data, len1);
	if (ret_value1 < 0)
		return ret_value1;

	ret_value2 = read_eeprom_chip(cpu, EEPROM_CHIP_SIZE, (uint8_t *)data + len1, len2);
	if (ret_value2 < 0)
		return ret_value2;

	return ret_value1 + ret_value2;
}

/* Finds and extracts i-th keyword (`index` specifies which one) from a record
 * in EEPROM that starts at specified offset */
static bool eeprom_extract_kwd(uint8_t cpu, uint64_t offset, uint8_t index,
			       const char *record_name, const char *kwd_name,
			       uint8_t *buf, size_t *size)
{
	uint16_t record_size = 0;
	uint8_t name[VPD_RECORD_NAME_LEN];

	if (strlen(record_name) != VPD_RECORD_NAME_LEN)
		die("Record name has wrong length: %s!\n", record_name);
	if (strlen(kwd_name) != VPD_KWD_NAME_LEN)
		die("Keyword name has wrong length: %s!\n", kwd_name);

	if (read_eeprom(cpu, offset, &record_size, sizeof(record_size)) != VPD_RECORD_SIZE_LEN)
		die("Failed to read record size from EEPROM\n");

	offset += VPD_RECORD_SIZE_LEN;
	record_size = le16toh(record_size);

	/* Skip mandatory "RT" and one byte of keyword size (always 4) */
	offset += VPD_KWD_NAME_LEN + 1;

	if (read_eeprom(cpu, offset, name, sizeof(name)) != sizeof(name))
		die("Failed to read record name from EEPROM\n");

	if (memcmp(name, record_name, VPD_RECORD_NAME_LEN))
		die("Expected to be working with %s record, got %.4s!\n",
		    record_name, name);

	offset += VPD_RECORD_NAME_LEN;

	while (offset < record_size) {
		uint8_t name_buf[VPD_KWD_NAME_LEN];
		uint16_t kwd_size = 0;

		if (read_eeprom(cpu, offset, name_buf, sizeof(name_buf)) != sizeof(name_buf))
			die("Failed to read keyword name from EEPROM\n");

		/* This is always the last keyword */
		if (!memcmp(name_buf, "PF", VPD_KWD_NAME_LEN))
			break;

		offset += VPD_KWD_NAME_LEN;

		if (name_buf[0] == '#') {
			/* This is a large (two-byte size) keyword */
			if (read_eeprom(cpu, offset, &kwd_size,
					sizeof(kwd_size)) != sizeof(kwd_size))
				die("Failed to read large keyword size from EEPROM\n");
			kwd_size = le16toh(kwd_size);
			offset += 2;
		} else {
			uint8_t small_size;
			if (read_eeprom(cpu, offset, &small_size,
					sizeof(small_size)) != sizeof(small_size))
				die("Failed to read small keyword size from EEPROM\n");
			kwd_size = small_size;
			offset += 1;
		}

		if (!memcmp(name_buf, kwd_name, VPD_KWD_NAME_LEN) && index-- == 0) {
			if (*size < kwd_size)
				die("Keyword buffer is too small: %llu instead of %llu\n",
				    (unsigned long long)*size, (unsigned long long)kwd_size);

			if (read_eeprom(cpu, offset, buf, kwd_size) != kwd_size)
				die("Failed to read keyword body from EEPROM\n");

			*size = kwd_size;
			return true;
		}

		offset += kwd_size;
	}

	return false;
}

/* Builds MVPD partition for a single processor (up to 64 KiB) or returns an
 * already built one */
static const uint8_t *mvpd_get(uint8_t cpu)
{
	enum { MAX_MVPD_SIZE = 64 * KiB };

	const char *mvpd_records[] = {
		"CRP0", "CP00", "VINI",
		"LRP0", "LRP1", "LRP2", "LRP3", "LRP4", "LRP5",
		"LWP0", "LWP1", "LWP2", "LWP3", "LWP4", "LWP5",
		"VRML", "VWML", "VER0", "MER0", "VMSC",
	};

	uint8_t *mvpd_buf = &_mvpd_cache[cpu * MAX_MVPD_SIZE];

	struct mvpd_toc_entry *toc = (void *)mvpd_buf;
	uint16_t mvpd_offset = MVPD_TOC_SIZE;

	uint8_t pt_buf[256];
	struct pt_record *pt_record = (void *)pt_buf;
	size_t pt_size = sizeof(struct pt_record);

	uint8_t i = 0;

	/* Skip the ECC data + "large resource" byte (0x84) in the VHDR */
	uint64_t offset = 12;

	if (cpu >= 2)
		die("Unsupported CPU number for MVPD query: %d.\n", cpu);

	/* Partition is already constructed (filled one can't be empty) */
	if (mvpd_buf[0] != '\0')
		return mvpd_buf;

	if (!eeprom_extract_kwd(cpu, offset, 0, "VHDR", "PT", pt_buf, &pt_size))
		die("Failed to find PT keyword of VHDR record in EEPROM.\n");

	if (memcmp(pt_record->record_name, "VTOC", VPD_RECORD_NAME_LEN))
		die("VHDR in EEPROM is invalid (got %.4s instead of VTOC.\n",
		    pt_record->record_name);

	/* Move to the TOC record, skip "large resource" byte (0x84) */
	offset = le16toh(pt_record->record_offset) + 1;

	/* Fill whole TOC with 0xFF */
	memset(toc, 0xFF, MVPD_TOC_SIZE);

	/* Up to three PT keywords in VTOC record */
	for (i = 0; i < 3; ++i) {
		uint8_t j;
		uint8_t entry_count;

		pt_size = sizeof(pt_buf);
		if (!eeprom_extract_kwd(cpu, offset, i, "VTOC", "PT", pt_buf, &pt_size)) {
			if (i == 0)
				die("Failed to find any PT keyword of VTOC record in EEPROM\n");
			break;
		}

		entry_count = pt_size / sizeof(struct pt_record);

		for (j = 0; j < entry_count; ++j) {
			const char *record_name = pt_record[j].record_name;
			/* Skip "large resource" byte (0x84) */
			const uint16_t record_offset = le16toh(pt_record[j].record_offset) + 1;
			const uint16_t record_size = le16toh(pt_record[j].record_length);

			uint8_t k;
			for (k = 0; k < ARRAY_SIZE(mvpd_records); ++k) {
				if (!memcmp(record_name, mvpd_records[k], 4))
					break;
			}

			if (k == ARRAY_SIZE(mvpd_records))
				continue;

			if (mvpd_offset + record_size > MAX_MVPD_SIZE)
				die("MVPD section doesn't have space for %.4s record of size %d\n",
				    record_name, record_size);

			/* Store this record to MVPD */

			memcpy(toc->name, record_name, VPD_RECORD_NAME_LEN);
			toc->offset = htole16(mvpd_offset);
			toc->reserved[0] = 0x5A;
			toc->reserved[1] = 0x5A;

			if (read_eeprom(cpu, record_offset, mvpd_buf + mvpd_offset,
					record_size) != record_size)
				die("Failed to read %.4s record from EEPROM\n", record_name);

			++toc;
			mvpd_offset += record_size;
		}
	}

	return mvpd_buf;
}

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
static struct ring_hdr *find_ring_step(uint8_t chiplet_id, uint8_t even_odd,
				       uint16_t ring_id,
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

	switch (ring_id) {
		case EX_L3_REPR: even_odd_mask = 0x00001000; break;
		case EX_L2_REPR: even_odd_mask = 0x00000400; break;
		case EX_L3_REFR_TIME:
		case EX_L3_REFR_REPR: even_odd_mask = 0x00000040; break;

		default: even_odd_mask = 0; break;
	}

	even_odd_mask >>= even_odd;

	if (be16toh(hdr->ring_id) != ring_id)
		return NULL;
	if (((be32toh(hdr->scan_addr) >> 24) & 0xFF) != chiplet_id)
		return NULL;
	if (even_odd_mask != 0 && !(be32toh(hdr->scan_addr) & even_odd_mask))
		return NULL;

	return hdr;
}

/* Searches for a specific ring in a keyword */
static struct ring_hdr *find_ring(uint8_t chiplet_id, uint8_t even_odd,
				  uint16_t ring_id, const uint8_t *buf,
				  uint32_t buf_len)
{
	/* Skip version number */
	--buf_len;
	++buf;

	while (!is_end_of_rings(buf, buf_len)) {
		struct ring_hdr *ring = find_ring_step(chiplet_id, even_odd,
						       ring_id, &buf, &buf_len);
		if (ring != NULL)
			return ring;
	}

	return NULL;
}

static const uint8_t *mvpd_get_keyword(uint8_t cpu, const char *record_name,
				       const char *kwd_name, size_t *kwd_size)
{
	const uint8_t *mvpd = mvpd_get(cpu);
	struct mvpd_toc_entry *mvpd_toc = (void *)mvpd;

	struct mvpd_toc_entry *toc_entry = NULL;
	const uint8_t *record_data = NULL;

	const uint8_t *kwd = NULL;

	toc_entry = find_record(mvpd_toc, record_name);
	if (toc_entry == NULL)
		die("Failed to find %s MVPD record!\n", record_name);

	record_data = mvpd + le16toh(toc_entry->offset);

	kwd = vpd_find_kwd(record_data, record_name, kwd_name, kwd_size);
	if (kwd == NULL)
		die("Failed to find %s keyword in %s!\n", kwd_name,
		    record_name);

	return kwd;
}

void mvpd_get_mcs_pg(uint8_t chip, uint16_t *pg)
{
	enum {
		VPD_CP00_PG_HDR_LENGTH   = 1,
		VPD_CP00_PG_DATA_LENGTH  = 128,
		VPD_CP00_PG_DATA_ENTRIES = VPD_CP00_PG_DATA_LENGTH / 2,
	};

	uint8_t raw_pg_data[VPD_CP00_PG_HDR_LENGTH + VPD_CP00_PG_DATA_LENGTH];
	uint16_t pg_data[VPD_CP00_PG_DATA_ENTRIES];
	uint32_t size = sizeof(raw_pg_data);

	if (!mvpd_extract_keyword(chip, "CP00", "PG", raw_pg_data, &size))
		die("Failed to read CPU%d/MVPD/CP00/PG", chip);

	memcpy(pg_data, raw_pg_data + VPD_CP00_PG_HDR_LENGTH, sizeof(pg_data));

	pg[0] = pg_data[MC01_CHIPLET_ID];
	pg[1] = pg_data[MC23_CHIPLET_ID];
}

bool mvpd_extract_keyword(uint8_t chip, const char *record_name,
			  const char *kwd_name, uint8_t *buf, uint32_t *size)
{
	const uint8_t *kwd = NULL;
	size_t kwd_size = 0;
	bool copied_data = false;

	kwd = mvpd_get_keyword(chip, record_name, kwd_name, &kwd_size);
	if (kwd == NULL)
		die("Failed to find %s keyword in %s!\n", kwd_name,
		    record_name);

	if (*size >= kwd_size) {
		memcpy(buf, kwd, kwd_size);
		copied_data = true;
	}

	*size = kwd_size;

	return copied_data;
}

const struct voltage_kwd *mvpd_get_voltage_data(uint8_t chip, int lrp)
{
	static int inited_chip = -1;
	static int inited_lrp = -1;
	static uint8_t buf[sizeof(struct voltage_kwd)];

	char record_name[] = { 'L', 'R', 'P', '0' + lrp, '\0' };
	uint32_t buf_size = sizeof(buf);
	struct voltage_kwd *voltage = (void *)buf;

	assert(lrp >= 0 && lrp < 6);
	if (inited_chip == chip && inited_lrp == lrp)
		return voltage;

	inited_chip = -1;
	inited_lrp = -1;

	if (!mvpd_extract_keyword(chip, record_name, "#V", buf, &buf_size)) {
		printk(BIOS_ERR, "Failed to read LRP0 record from MVPD\n");
		return NULL;
	}

	if (voltage->version != VOLTAGE_DATA_VERSION) {
		printk(BIOS_ERR, "Only version %d of voltage data is supported, got: %d\n",
		       VOLTAGE_DATA_VERSION, voltage->version);
		return NULL;
	}

	inited_chip = chip;
	inited_lrp = lrp;
	return voltage;
}

/* Finds a specific ring in MVPD partition and extracts it */
bool mvpd_extract_ring(uint8_t chip, const char *record_name,
		       const char *kwd_name, uint8_t chiplet_id,
		       uint8_t even_odd, uint16_t ring_id,
		       uint8_t *buf, uint32_t buf_size)
{
	const uint8_t *rings = NULL;
	size_t rings_size = 0;

	struct ring_hdr *ring = NULL;
	uint32_t ring_size = 0;

	rings = mvpd_get_keyword(chip, record_name, kwd_name, &rings_size);
	if (rings == NULL)
		die("Failed to find %s keyword in %s!\n", kwd_name,
		    record_name);

	ring = find_ring(chiplet_id, even_odd, ring_id, rings, rings_size);
	if (ring == NULL)
		return false;

	ring_size = be32toh(ring->size);
	if (buf_size >= ring_size)
		memcpy(buf, ring, ring_size);

	return (buf_size >= ring_size);
}
