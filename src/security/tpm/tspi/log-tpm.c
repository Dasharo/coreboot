/* SPDX-License-Identifier: GPL-2.0-only */

/*
 * Unlike log.c this implements TCPA log according to TPM2 specification
 * rather then using coreboot-specific log format.
 *
 * First entry is in TPM1.2 format and serves as a header, the rest are in
 * a newer format which supports SHA256 and multiple hashes, but we always
 * store one hash in SHA1 format.
 *
 * This is defined in "TCG EFI Protocol Specification" and is what skiboot
 * uses.
 */

#include <endian.h>
#include <console/console.h>
#include <security/tpm/tspi.h>
#include <region_file.h>
#include <string.h>
#include <symbols.h>
#include <cbmem.h>
#include <bootstate.h>
#include <vb2_sha.h>

static struct tcpa_table *tcpa_cbmem_init(void)
{
	static struct tcpa_table *tclt;
	if (tclt)
		return tclt;

	if (cbmem_possibly_online()) {
		tclt = cbmem_find(CBMEM_ID_TCPA_SPEC_LOG);
		if (!tclt) {
			size_t tcpa_log_len = sizeof(struct tcpa_table) +
				MAX_TCPA_LOG_ENTRIES * sizeof(struct tcpa_entry);
			tclt = cbmem_add(CBMEM_ID_TCPA_SPEC_LOG, tcpa_log_len);
			if (tclt) {
				struct tcpa_log_ref *tcpa_ref;

				tclt->max_entries = MAX_TCPA_LOG_ENTRIES;
				tclt->num_entries = 0;

				tcg_efi_spec_id_event *hdr = &tclt->header;
				memset(hdr, 0, sizeof(*hdr));
				hdr->event_type = htole32(EV_NO_ACTION);
				hdr->event_size = htole32(37);
				memcpy(hdr->signature, TCG_EFI_SPEC_ID_EVENT_SIGNATURE,
				       sizeof(hdr->signature));
				hdr->spec_version_minor = 0;
				hdr->spec_version_major = 2;
				hdr->uintn_size = 0x02; // 64-bit UINT
				hdr->num_of_algorithms = 2;
				hdr->digest_sizes[0].alg_id = TPM2_ALG_SHA1;
				hdr->digest_sizes[0].digest_size = SHA1_DIGEST_SIZE;
				hdr->digest_sizes[1].alg_id = TPM2_ALG_SHA256;
				hdr->digest_sizes[1].digest_size = SHA256_DIGEST_SIZE;

				tcpa_ref = cbmem_add(CBMEM_ID_TCPA_LOG_REF, sizeof(*tcpa_ref));
				tcpa_ref->start = (uintptr_t)&tclt->header;
				tcpa_ref->size = tcpa_log_len
					       - offsetof(struct tcpa_table, header);
			}
		}
	}
	return tclt;
}

struct tcpa_table *tcpa_log_init(void)
{
	static struct tcpa_table *tclt;

	/* We are dealing here with pre CBMEM environment.
	 * If cbmem isn't available use CAR or SRAM */
	if (!cbmem_possibly_online() &&
		!CONFIG(VBOOT_RETURN_FROM_VERSTAGE))
		return (struct tcpa_table *)_tpm_tcpa_log;
	else if (ENV_ROMSTAGE &&
		!CONFIG(VBOOT_RETURN_FROM_VERSTAGE)) {
		tclt = tcpa_cbmem_init();
		if (!tclt)
			return (struct tcpa_table *)_tpm_tcpa_log;
	} else {
		tclt = tcpa_cbmem_init();
	}

	return tclt;
}

void tcpa_log_dump(void *unused)
{
	int i, j;
	struct tcpa_table *tclt;

	tclt = tcpa_log_init();
	if (!tclt)
		return;

	printk(BIOS_INFO, "coreboot TCPA measurements:\n\n");
	for (i = 0; i < tclt->num_entries; i++) {
		struct tcpa_entry *tce = &tclt->entries[i];

		printk(BIOS_INFO, " PCR-%u ", le32toh(tce->pcr));

		for (j = 0; j < SHA1_DIGEST_SIZE; j++)
			printk(BIOS_INFO, "%02x", tce->digest[j]);

		printk(BIOS_INFO, " %s [%s]\n", "SHA1", tce->name);
	}
	printk(BIOS_INFO, "\n");
}

void tcpa_log_add_table_entry(const char *name, const uint32_t pcr,
			enum vb2_hash_algorithm digest_algo,
			const uint8_t *digest,
			const size_t digest_len)
{
	struct tcpa_table *tclt = tcpa_log_init();
	if (!tclt) {
		printk(BIOS_WARNING, "TCPA: Log non-existent!\n");
		return;
	}

	if (digest_algo != VB2_HASH_SHA1) {
		printk(BIOS_WARNING, "TCPA: digest is of unsupported type: %s\n",
		       vb2_get_hash_algorithm_name(digest_algo));
		return;
	}

	if (digest_len != SHA1_DIGEST_SIZE) {
		printk(BIOS_WARNING, "TCPA: digest is of unsupported length: %d\n",
		       (int)digest_len);
		return;
	}

	if (tclt->num_entries >= tclt->max_entries) {
		printk(BIOS_WARNING, "TCPA: TCPA log table is full\n");
		return;
	}

	if (!name) {
		printk(BIOS_WARNING, "TCPA: TCPA entry name not set\n");
		return;
	}

	struct tcpa_entry *tce = &tclt->entries[tclt->num_entries++];

	tce->pcr = htole32(pcr);
	tce->event_type = htole32(EV_ACTION);

	tce->digest_count = htole32(1);
	tce->digest_type = htole16(TPM2_ALG_SHA1);
	memcpy(tce->digest, digest, SHA1_DIGEST_SIZE);

	tce->name_length = htole32(TCPA_PCR_HASH_NAME);
	strncpy(tce->name, name, TCPA_PCR_HASH_NAME - 1);
}

void tcpa_preram_log_clear(void)
{
	printk(BIOS_INFO, "TCPA: Clearing coreboot TCPA log\n");
	struct tcpa_table *tclt = (struct tcpa_table *)_tpm_tcpa_log;
	tclt->max_entries = MAX_TCPA_LOG_ENTRIES;
	tclt->num_entries = 0;
}

#if !CONFIG(VBOOT_RETURN_FROM_VERSTAGE)
static void recover_tcpa_log(int is_recovery)
{
	struct tcpa_table *preram_log = (struct tcpa_table *)_tpm_tcpa_log;
	struct tcpa_table *ram_log = NULL;
	int i;

	if (preram_log->num_entries > MAX_PRERAM_TCPA_LOG_ENTRIES) {
		printk(BIOS_WARNING, "TCPA: Pre-RAM TCPA log is too full, possible corruption\n");
		return;
	}

	ram_log = tcpa_cbmem_init();
	if (!ram_log) {
		printk(BIOS_WARNING, "TCPA: CBMEM not available something went wrong\n");
		return;
	}

	for (i = 0; i < preram_log->num_entries; i++) {
		struct tcpa_entry *tce = &ram_log->entries[ram_log->num_entries++];

		tce->pcr = preram_log->entries[i].pcr;
		tce->event_type = preram_log->entries[i].event_type;

		tce->digest_count = preram_log->entries[i].digest_count;
		tce->digest_type = preram_log->entries[i].digest_type;
		memcpy(tce->digest, preram_log->entries[i].digest, SHA1_DIGEST_SIZE);

		tce->name_length = preram_log->entries[i].name_length;
		strncpy(tce->name, preram_log->entries[i].name, TCPA_PCR_HASH_NAME - 1);
	}
}
ROMSTAGE_CBMEM_INIT_HOOK(recover_tcpa_log);
#endif

BOOT_STATE_INIT_ENTRY(BS_PAYLOAD_BOOT, BS_ON_ENTRY, tcpa_log_dump, NULL);
