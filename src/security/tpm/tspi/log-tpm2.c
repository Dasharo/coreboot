/* SPDX-License-Identifier: GPL-2.0-only */

/*
 * Unlike log.c this implements TPM log according to TPM2.0 specification
 * rather then using coreboot-specific log format.
 *
 * First entry is in TPM1.2 format and serves as a header, the rest are in
 * a newer (agile) format which supports SHA256 and multiple hashes, but we
 * store only one hash.
 *
 * This is defined in "TCG EFI Protocol Specification".
 */

#include <endian.h>
#include <commonlib/iobuf.h>
#include <console/console.h>
#include <security/tpm/tspi.h>
#include <security/tpm/tspi/crtm.h>
#include <security/tpm/tspi/logs.h>
#include <region_file.h>
#include <string.h>
#include <symbols.h>
#include <cbmem.h>
#include <vb2_sha.h>

#define TPM_LOG_SIZE  (64 * KiB)

struct log_event {
	uint32_t pcr;
	uint32_t event_type;
	uint32_t digest_count;
	struct tpm_digest digests[1];
	uint32_t name_len;
	const char *name;
};

struct startup_locality_event {
	char signature[16];       /* "StartupLocality" (NUL-terminated) */
	uint8_t startup_locality; /* 0 or 3 */
} __packed;

/* Assumes tclt->header.num_of_algorithms is already set to its final value. */
static struct tpm_2_log_bottom *get_log_bottom(const struct tpm_2_log_table *tclt)
{
	uint8_t *p;

	/* Start at the first variable-sized part of the header. */
	p = (uint8_t *)tclt->header.digest_sizes;
	/* Skip over it. */
	p += le32toh(tclt->header.num_of_algorithms) * sizeof(tclt->header.digest_sizes[0]);
	/* `p` points at `uint8_t vendor_info_size` here. */
	return (struct tpm_2_log_bottom *)p;
}

static uint16_t get_log_footprint(const struct tpm_2_log_table *tclt)
{
	return sizeof(*tclt) +
		le32toh(tclt->header.num_of_algorithms) * sizeof(tclt->header.digest_sizes[0]) +
		sizeof(struct tpm_2_log_bottom) +
		le16toh(get_log_bottom(tclt)->next_offset);
}

void *tpm2_log_cbmem_init(void)
{
	static struct tpm_2_log_table *tclt;
	if (tclt)
		return tclt;

	if (ENV_HAS_CBMEM) {
		struct tcg_efi_spec_id_event *hdr;
		struct tpm_2_log_bottom *bottom;

		tclt = cbmem_find(CBMEM_ID_TPM2_TCG_LOG);
		if (tclt)
			return tclt;

		tclt = cbmem_add(CBMEM_ID_TPM2_TCG_LOG, TPM_LOG_SIZE);
		if (!tclt)
			return NULL;

		memset(tclt, 0, TPM_LOG_SIZE);
		hdr = &tclt->header;

		hdr->event_type = htole32(EV_NO_ACTION);
		hdr->event_size = htole32(28 +
					  1 * sizeof(hdr->digest_sizes[0]) +
					  1 +
					  TPM_20_VENDOR_INFO_SIZE);
		strcpy((char *)hdr->signature, TPM_20_SPEC_ID_EVENT_SIGNATURE);
		hdr->platform_class = htole32(0x00); // client platform
		hdr->spec_version_minor = 0x00;
		hdr->spec_version_major = 0x02;
		hdr->spec_errata = 0x00;
		hdr->uintn_size = 0x02; // 64-bit UINT

		hdr->num_of_algorithms = htole32(1);
		hdr->digest_sizes[0].alg_id = htole16(tpm2_alg_from_vb2_hash(tpm_log_alg()));
		hdr->digest_sizes[0].digest_size = htole16(vb2_digest_size(tpm_log_alg()));

		bottom = get_log_bottom(tclt);
		bottom->vendor_info_size = TPM_20_VENDOR_INFO_SIZE;
		bottom->reserved = 0;
		bottom->version_major = TPM_20_LOG_VI_MAJOR;
		bottom->version_minor = TPM_20_LOG_VI_MINOR;
		bottom->magic = htole32(TPM_20_LOG_VI_MAGIC);
		bottom->num_entries = 0;
		bottom->next_offset = 0;
		bottom->max_offset = htole16(TPM_LOG_SIZE - get_log_footprint(tclt));
	}

	return tclt;
}

/* The function assumes input buffer includes a complete event. */
static void read_log_event(struct ibuf *ib, struct log_event *ev)
{
	ibuf_read_le32(ib, &ev->pcr);
	ibuf_read_le32(ib, &ev->event_type);
	ibuf_read_le32(ib, &ev->digest_count);

	uint32_t i;
	for (i = 0; i < ev->digest_count; ++i) {
		uint16_t alg;
		ibuf_read_le16(ib, &alg);

		ev->digests[i].hash_type = tpm2_alg_to_vb2_hash(alg);
		ev->digests[i].hash = ibuf_oob_drain(ib,
						     vb2_digest_size(ev->digests[i].hash_type));
	}

	ibuf_read_le32(ib, &ev->name_len);
	ev->name = ibuf_oob_drain(ib, ev->name_len);
}

/* Returns true if an event was parsed successfully. */
static bool parse_log_event(const struct tpm_2_log_bottom *bottom,
			    struct log_event *ev,
			    uint16_t *offset)
{
	if (*offset == le16toh(bottom->next_offset))
		return false;

	struct ibuf ib;
	ibuf_init(&ib, &bottom->events[*offset], le16toh(bottom->next_offset) - *offset);

	read_log_event(&ib, ev);

	*offset += ibuf_nr_read(&ib);
	return true;
}

void tpm2_log_dump(void)
{
	uint16_t offset;
	struct log_event ev;
	struct tpm_2_log_table *tclt;
	const struct tpm_2_log_bottom *bottom;

	tclt = tpm_log_init();
	if (!tclt)
		return;

	bottom = get_log_bottom(tclt);

	offset = 0;
	while (parse_log_event(bottom, &ev, &offset)) {
		uint32_t i;

		printk(BIOS_INFO, " PCR-%u [%s]:\n", ev.pcr, ev.name);

		for (i = 0; i < ev.digest_count; ++i) {
			enum vb2_hash_algorithm hash_type;
			int digest_size, j;

			hash_type = ev.digests[i].hash_type;
			digest_size = vb2_digest_size(hash_type);

			printk(BIOS_INFO, "  %6s: ", vb2_get_hash_algorithm_name(hash_type));
			for (j = 0; j < digest_size; ++j)
				printk(BIOS_INFO, "%02x", ev.digests[i].hash[j]);
			printk(BIOS_INFO, "\n");
		}
	}
	printk(BIOS_INFO, "\n");
}

/* The function assumes output buffer has enough space for the new event. */
static void write_log_event(struct obuf *ob,
			    const void *data,
			    size_t data_len,
			    uint32_t pcr,
			    uint32_t type,
			    const struct tpm_digest *digests)
{
	uint32_t digest_count = 0;
	while (digests[digest_count].hash_type != VB2_HASH_INVALID)
		++digest_count;

	obuf_write_le32(ob, pcr);
	obuf_write_le32(ob, type);
	obuf_write_le32(ob, digest_count);

	int i;
	for (i = 0; digests[i].hash_type != VB2_HASH_INVALID; ++i) {
		int hash_size = vb2_digest_size(digests[i].hash_type);

		obuf_write_le16(ob, tpm2_alg_from_vb2_hash(digests[i].hash_type));
		obuf_write(ob, digests[i].hash, hash_size);
	}

	obuf_write_le32(ob, data_len);
	obuf_write(ob, data, data_len);
}

static void add_log_table_entry(struct tpm_2_log_table *tclt,
				const void *data,
				size_t data_len,
				uint32_t pcr,
				uint32_t type,
				const struct tpm_digest *digests)
{
	int i;

	uint16_t needed_size = 4 * sizeof(uint32_t) + data_len;
	for (i = 0; digests[i].hash_type != VB2_HASH_INVALID; ++i)
		needed_size += sizeof(uint16_t) + vb2_digest_size(digests[i].hash_type);

	struct tpm_2_log_bottom *bottom = get_log_bottom(tclt);
	if (le16toh(bottom->next_offset) + needed_size > le16toh(bottom->max_offset)) {
		printk(BIOS_WARNING, "TPM LOG: log is full: %u/%u (need %u)\n",
		       le16toh(bottom->next_offset), le16toh(bottom->max_offset), needed_size);
		return;
	}

	struct obuf ob;
	obuf_init(&ob, &bottom->events[le16toh(bottom->next_offset)],
		  le16toh(bottom->max_offset) - le16toh(bottom->next_offset));

	write_log_event(&ob, data, data_len, pcr, type, digests);

	bottom->next_offset = htole16(le16toh(bottom->next_offset) + needed_size);
	bottom->num_entries = htole16(le16toh(bottom->num_entries) + 1);
}

void tpm2_log_add_table_entry(const char *name, uint32_t pcr, const struct tpm_digest *digests)
{
	struct tpm_2_log_table *tclt = tpm_log_init();
	if (!tclt) {
		printk(BIOS_WARNING, "TPM LOG: non-existent!\n");
		return;
	}

	if (!name) {
		printk(BIOS_WARNING, "TPM LOG: entry name not set\n");
		return;
	}

	add_log_table_entry(tclt, name, strlen(name) + 1, pcr, EV_ACTION, digests);
}

void tpm2_log_startup_locality(int locality)
{
	struct tpm_2_log_table *tclt = tpm_log_init();
	if (!tclt) {
		printk(BIOS_WARNING, "TPM LOG: non-existent!\n");
		return;
	}

	/* EV_NO_ACTION events use zeroes for digest(s). */
	struct vb2_hash zero_hash = {0};
	const struct tpm_digest digests[] = {
		{ .hash_type = tpm_log_alg(), .hash = zero_hash.raw },
		{ .hash_type = VB2_HASH_INVALID }
	};

	struct startup_locality_event event_data;
	strcpy(event_data.signature, "StartupLocality");
	event_data.startup_locality = locality;

	/* EV_NO_ACTION events use PCR-0 by default. */
	add_log_table_entry(tclt, &event_data, sizeof(event_data), 0, EV_NO_ACTION, digests);
}

int tpm2_log_get(int entry_idx, int *pcr, struct tpm_digest *digests, const char **event_name)
{
	uint16_t offset;
	struct log_event ev;
	int idx;
	struct tpm_2_log_table *tclt;
	struct tpm_2_log_bottom *bottom;

	tclt = tpm_log_init();
	if (!tclt)
		return 1;

	bottom = get_log_bottom(tclt);
	if (entry_idx < 0 || entry_idx >= le16toh(bottom->num_entries))
		return 1;

	offset = 0;
	idx = 0;
	while (parse_log_event(bottom, &ev, &offset)) {
		if (idx != entry_idx) {
			++idx;
			continue;
		}

		int i;
		for (i = 0; i < ev.digest_count; ++i)
			digests[i] = ev.digests[i];
		digests[ev.digest_count].hash_type = VB2_HASH_INVALID;

		*pcr = ev.pcr;
		*event_name = ev.name;
		return 0;
	}

	return 1;
}

uint16_t tpm2_log_get_size(const void *log_table)
{
	const struct tpm_2_log_table *tclt = log_table;
	return le16toh(get_log_bottom(tclt)->num_entries);
}

void tpm2_preram_log_clear(void)
{
	printk(BIOS_INFO, "TPM LOG: clearing the log\n");
	/*
	 * Pre-RAM log is only for internal use and isn't exported anywhere, hence it's header
	 * is not fully initialized.
	 */
	struct tpm_2_log_table *tclt = (struct tpm_2_log_table *)_tpm_log;
	tclt->header.num_of_algorithms = htole32(1);

	struct tpm_2_log_bottom *bottom = get_log_bottom(tclt);
	bottom->num_entries = 0;
	bottom->next_offset = 0;
	bottom->max_offset = htole16((_etpm_log - _tpm_log) - get_log_footprint(tclt));
}

void tpm2_log_copy_entries(const void *from, void *to)
{
	const struct tpm_2_log_bottom *from_bottom = get_log_bottom(from);
	struct tpm_2_log_bottom *to_bottom = get_log_bottom(to);

	if (le16toh(to_bottom->max_offset) < le16toh(from_bottom->next_offset)) {
		printk(BIOS_WARNING,
		       "TPM LOG: not enough space at destination to copy event log entries!\n");
		return;
	}

	memcpy(to_bottom->events, from_bottom->events, le16toh(from_bottom->next_offset));
	to_bottom->num_entries = from_bottom->num_entries;
	to_bottom->next_offset = from_bottom->next_offset;
}
