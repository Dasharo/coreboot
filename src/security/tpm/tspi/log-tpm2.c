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
	struct tpm_digest digests[ENABLED_TPM_ALGS_NUM];
	uint32_t name_len;
	const char *name;
};

struct startup_locality_event {
	char signature[16];       /* "StartupLocality" (NUL-terminated) */
	uint8_t startup_locality; /* 0 or 3 */
} __packed;

struct pcr_banks_info {
	int active_count;                     /* Number is_active entries set to true. */
	bool is_active[ENABLED_TPM_ALGS_NUM]; /* Set of active banks. */
};

static int find_alg_index(enum vb2_hash_algorithm alg)
{
	unsigned int i;
	for (i = 0; i < ENABLED_TPM_ALGS_NUM; i++) {
		if (enabled_tpm_algs[i] == alg)
			return i;
	}

	return -1;
}

static bool is_all_zeroes(const void *buffer, size_t size)
{
	const uint8_t *p = buffer;
	while (size-- != 0) {
		if (*p++ != 0)
			return false;
	}
	return true;
}

static bool supports_digest(const struct tpm_2_log_table *tclt, uint16_t alg_id)
{
	const struct tcg_efi_spec_id_event *hdr = &tclt->header;

	int i;
	for (i = 0; i < le32toh(hdr->num_of_algorithms); ++i) {
		if (hdr->digest_sizes[i].alg_id == htole16(alg_id))
			return true;
	}

	return false;
}

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

/* See comment on tpm2_log_align_with_tpm(). */
static const struct pcr_banks_info *get_pcr_banks_info(void)
{
	static bool initialized;
	static struct pcr_banks_info info;

	if (initialized)
		return &info;

	/* Start by pretending that all supported banks are active in case there will be an
	   error. */
	unsigned int i;
	for (i = 0; i < ENABLED_TPM_ALGS_NUM; ++i)
		info.is_active[i] = true;
	info.active_count = ENABLED_TPM_ALGS_NUM;

	if (CONFIG(TPM_INIT_RAMSTAGE) && !ENV_RAMSTAGE && !ENV_POSTCAR && !ENV_PAYLOAD_LOADER) {
		/* TPM initialization is postponed until ramstage, no use poking it. */
		initialized = true;
		return &info;
	}

	tpm_result_t rc = tlcl_lib_init();
	if (rc != TPM_SUCCESS) {
		printk(BIOS_WARNING, "TPM LOG: failed to initialize TLCL: %d\n", rc);
		return &info;
	}

	TPML_PCR_SELECTION pcrs;
	rc = tlcl2_get_capability_pcrs(&pcrs);
	if (rc != TPM_SUCCESS) {
		printk(BIOS_WARNING, "TPM LOG: failed to query PCR capabilities: %d\n", rc);
		return &info;
	}

	if (pcrs.count == 0) {
		/* TPM likely just hasn't been set up yet. */
		return &info;
	}

	info.active_count = 0;
	memset(info.is_active, 0, sizeof(info.is_active));

	for (i = 0; i < pcrs.count; i++) {
		TPMS_PCR_SELECTION *selection = &pcrs.pcrSelections[i];

		enum vb2_hash_algorithm alg = tpm2_alg_to_vb2_hash(selection->hash);
		if (alg == VB2_HASH_INVALID) {
			printk(BIOS_INFO, "TPM LOG: unsupported PCR bank: %#x\n",
			       selection->hash);
			continue;
		}

		int alg_index = find_alg_index(alg);
		if (alg_index < 0) {
			printk(BIOS_INFO,
			       "TPM LOG: skipping PCR bank disabled at build-time: %s\n",
			       vb2_get_hash_algorithm_name(alg));
			continue;
		}

		bool is_active = !is_all_zeroes(selection->pcrSelect, selection->sizeofSelect);
		if (is_active) {
			info.is_active[alg_index] = true;
			++info.active_count;
		}
	}

	initialized = true;
	return &info;
}

static uint16_t get_log_footprint(const struct tpm_2_log_table *tclt)
{
	return sizeof(*tclt) +
		le32toh(tclt->header.num_of_algorithms) * sizeof(tclt->header.digest_sizes[0]) +
		sizeof(struct tpm_2_log_bottom) +
		le16toh(get_log_bottom(tclt)->next_offset);
}

/* Populates `num_of_algorithms` and `digest_sizes[]` in the header from the active banks. */
static void fill_digest_sizes(struct tcg_efi_spec_id_event *hdr,
			      const struct pcr_banks_info *info)
{
	int i, j;

	hdr->num_of_algorithms = htole32(info->active_count);
	for (i = 0, j = -1; i < info->active_count; ++i) {
		/* Find the next active bank. */
		while (!info->is_active[++j])
			continue;

		hdr->digest_sizes[i].alg_id =
			htole16(tpm2_alg_from_vb2_hash(enabled_tpm_algs[j]));
		hdr->digest_sizes[i].digest_size =
			htole16(vb2_digest_size(enabled_tpm_algs[j]));
	}
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

		strcpy((char *)hdr->signature, TPM_20_SPEC_ID_EVENT_SIGNATURE);
		hdr->event_type = htole32(EV_NO_ACTION);
		hdr->platform_class = htole32(0x00); // client platform
		hdr->spec_version_minor = 0x00;
		hdr->spec_version_major = 0x02;
		hdr->spec_errata = 0x00;
		hdr->uintn_size = 0x02; // 64-bit UINT

		/*
		 * The CBMEM log is only created in the stage that creates CBMEM (where
		 * the pre-RAM log buffer `_tpm_log` is also linked).  In that stage,
		 * mirror the pre-RAM header's algorithm list so events copied in by
		 * `recover_tpm_log()` parse consistently against this header.  Other
		 * stages have already returned via `cbmem_find()` above.
		 */
		if (ENV_CREATES_CBMEM) {
			const struct tpm_2_log_table *preram =
				(const struct tpm_2_log_table *)_tpm_log;
			uint32_t preram_nalg =
				le32toh(preram->header.num_of_algorithms);

			if (preram_nalg != 0) {
				uint32_t i;
				hdr->num_of_algorithms =
					preram->header.num_of_algorithms;
				for (i = 0; i < preram_nalg; ++i)
					hdr->digest_sizes[i] =
						preram->header.digest_sizes[i];
			} else {
				fill_digest_sizes(hdr, get_pcr_banks_info());
			}
		} else {
			fill_digest_sizes(hdr, get_pcr_banks_info());
		}

		hdr->event_size = htole32(28 +
					  le32toh(hdr->num_of_algorithms) *
						sizeof(hdr->digest_sizes[0]) +
					  1 +
					  TPM_20_VENDOR_INFO_SIZE);

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

bool tpm2_log_alg_active(enum vb2_hash_algorithm alg)
{
	int alg_index = find_alg_index(alg);
	if (alg_index < 0)
		return false;

	return get_pcr_banks_info()->is_active[alg_index];
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

	const struct pcr_banks_info *pcr_banks_info = get_pcr_banks_info();

	/* EV_NO_ACTION events use zeroes for digest(s). */
	struct vb2_hash zero_hash = {0};
	struct tpm_digest digests[ENABLED_TPM_ALGS_NUM + 1];

	int i, j;
	for (i = 0, j = 0; i < ARRAY_SIZE(pcr_banks_info->is_active); ++i) {
		if (!pcr_banks_info->is_active[i])
			continue;

		digests[j].hash_type = enabled_tpm_algs[i];
		digests[j].hash = zero_hash.raw;
		++j;
	}
	digests[j].hash_type = VB2_HASH_INVALID;

	struct startup_locality_event event_data;
	strcpy(event_data.signature, "StartupLocality");
	event_data.startup_locality = locality;

	/* EV_NO_ACTION events use PCR-0 by default. */
	add_log_table_entry(tclt, &event_data, sizeof(event_data), 0, EV_NO_ACTION, digests);
}

int tpm2_log_get(int entry_idx, int *pcr, struct tpm_digest *digests, const char **event_name,
		 uint32_t *event_type)
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
		*event_type = ev.event_type;
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
	fill_digest_sizes(&tclt->header, get_pcr_banks_info());

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

/*
 * Events can be logged before TPM is initialized but the set of active PCR banks can be queried
 * only after its initialization.  For this reason get_pcr_banks_info() reports all supported
 * banks as active if TPM is unavailable, collecting all possible digests in the log itself.
 * Once TPM is initialized, tspi_measure_cache_to_pcr() is called which invokes this function.
 * The purpose of the function is to trim down the log by removing unnecessary digests.
 * Normally, this function gets called in the ramstage, but if TPM gets initialized in the
 * bootblock, no trimming (and no unnecessary hashing) will occur.
 */
void tpm2_log_align_with_tpm(void)
{
	/* There is no point doing anything if exactly one digest algorithm is enabled. */
	if (ENABLED_TPM_ALGS_NUM == 1)
		return;

	struct tpm_2_log_table *tclt = tpm_log_init();
	if (tclt == NULL) {
		printk(BIOS_WARNING, "TPM LOG: can't adjust a non-existent log!\n");
		return;
	}

	struct tcg_efi_spec_id_event *hdr = &tclt->header;
	const int old_alg_count = le32toh(hdr->num_of_algorithms);

	/* Filter-out unsupported digest algorithms in place. */
	int i;
	int new_alg_count = 0;
	for (i = 0; i < old_alg_count; ++i) {
		uint16_t alg_id = le16toh(hdr->digest_sizes[i].alg_id);
		if (tpm2_log_alg_active(tpm2_alg_to_vb2_hash(alg_id)))
			hdr->digest_sizes[new_alg_count++] = hdr->digest_sizes[i];
	}

	if (new_alg_count == old_alg_count) {
		printk(BIOS_INFO, "TPM LOG: none of %d digests were filtered\n", old_alg_count);
		return;
	}

	if (new_alg_count == 0) {
		printk(BIOS_WARNING, "TPM LOG: can't filter-out all digests, leaving as is\n");
		return;
	}

	printk(BIOS_INFO, "TPM LOG: filtering-out digests of inactive banks: %d -> %d\n",
	       old_alg_count, new_alg_count);

	/*
	 * The following code excludes inactive hashes in place.  This allows using it in a
	 * bootblock which lacks dynamic memory allocation and avoid reserving memory for a
	 * temporary copy.  Updating in place is safe to do and success is guaranteed because
	 * the data is merely shifted.  The only possible concern is that obuf_write() uses
	 * memcpy() which per standard doesn't deal with overlapping ranges but that should be
	 * fine in this environment which has its own memcpy().
	 *
	 * How the update is performed:
	 *  1. Remember current range of event data.
	 *  2. Unsupported digest algorithms inside log's header are already gone, merely need
	 *     to update their count.
	 *  3. Increase size of log's header (it got shorter).
	 *  4. Rediscover bottom part of the log (where vendor data is) whose location depends
	 *     on the number of digest algorithms.
	 *  5. Reinsert all log entries while skipping inactive banks.
	 *  6. Fill bottom part with correct values and update offsets there.
	 *  7. Clear the remainder of the log.
	 */

	/* Save original bottom part on the stack. */
	struct tpm_2_log_bottom *bottom = get_log_bottom(tclt);
	const struct tpm_2_log_bottom old_bottom = *bottom;

	/* Set up input buffer before bottom gets moved (updates to the header above don't
	   hurt anything). */
	struct ibuf ib;
	ibuf_init(&ib, bottom->events, le16toh(bottom->next_offset));

	size_t size_diff =
		(old_alg_count - new_alg_count) * sizeof(tclt->header.digest_sizes[0]);

	hdr->num_of_algorithms = htole32(new_alg_count);
	hdr->event_size = htole32(le32toh(hdr->event_size) - size_diff);

	/* Updating `hdr->num_of_algorithms` above shifted location of the bottom part. */
	bottom = get_log_bottom(tclt);

	/* Set up output buffer at the new location. */
	struct obuf ob;
	obuf_init(&ob, bottom->events, le16toh(old_bottom.max_offset) + size_diff);

	for (i = 0; i < le16toh(old_bottom.num_entries); ++i) {
		struct log_event ev;
		read_log_event(&ib, &ev);

		int j, k = 0;
		struct tpm_digest digests[ENABLED_TPM_ALGS_NUM + 1];
		for (j = 0; j < ev.digest_count; ++j) {
			uint16_t alg = tpm2_alg_from_vb2_hash(ev.digests[j].hash_type);
			if (supports_digest(tclt, alg))
				digests[k++] = ev.digests[j];
		}
		digests[k].hash_type = VB2_HASH_INVALID;

		write_log_event(&ob, ev.name, ev.name_len, ev.pcr, ev.event_type, digests);
	}

	*bottom = old_bottom;
	bottom->max_offset = htole16(le16toh(old_bottom.max_offset) + size_diff);
	bottom->next_offset = htole16(obuf_nr_written(&ob));

	memset(&bottom->events[obuf_nr_written(&ob)], 0,
	       le16toh(bottom->max_offset) - obuf_nr_written(&ob));
}
