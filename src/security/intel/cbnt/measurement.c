/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <arch/fit_table.h>
#include <commonlib/iobuf.h>
#include <console/console.h>
#include <cpu/intel/msr.h>
#include <cpu/x86/msr.h>
#include <device/mmio.h>
#include <security/intel/cbnt/cbnt.h>
#include <security/tpm/tspi/crtm.h>
#include <security/vboot/misc.h>

#define TPM_ALG_RSA     0x0001
#define TPM_ALG_ECC     0x0023

#define TPM_ALG_RSASSA  0x0014
#define TPM_ALG_RSAPSS  0x0016
#define TPM_ALG_ECDSA   0x0018
#define TPM_ALG_SM2     0x001b

/* Bits of `struct bpm_ibbs::flags`. */
#define BPM_IBBS_FLAG_RESERVED1             (1 << 0) /* must be set to 1 */
#define BPM_IBBS_FLAG_RESERVED2             (1 << 1) /* must be set to 1 */
#define BPM_IBBS_FLAG_AUTH_MEASURE          (1 << 2) /* extend PCR-7 */
#define BPM_IBBS_FLAG_CAP_PCRS_ON_TPM_FAIL  (1 << 3)
#define BPM_IBBS_FLAG_TOP_SWAP_SUPPORTED    (1 << 4)
#define BPM_IBBS_FLAG_FORCE_TME             (1 << 5)
#define BPM_IBBS_FLAG_FORCE_IGN             (1 << 6)
#define BPM_IBBS_FLAG_SRTM_AC               (1 << 7) /* attestation control */

/* Mask applied to ACM_POLICY_STATUS when BPM_IBBS_FLAG_SRTM_AC is set. */
#define SRTM_AC_MASK  0x20FFF

/* Bits of `struct km_hash::usage` indicating to which public key the hash corresponds. */
#define KM_HASH_USAGE_BPM       (1 << 0)
#define KM_HASH_USAGE_FIT       (1 << 1)
#define KM_HASH_USAGE_ACM       (1 << 2)
#define KM_HASH_USAGE_SDEV      (1 << 3)
#define KM_HASH_USAGE_PFR_CPLD  (1 << 4)

/*
 * Allow for 4 8192-bit digests in PCR-0 measurements which is an unlikely situation, so this
 * buffer should be enough to not run out of space (data measured into PCR-7 is smaller).
 */
#define MAX_DATA_SIZE  (sizeof(uint64_t) + sizeof(uint16_t) + 4 * KiB)

/*
 * SRTM version is an upper-case hex dump of ACM's "Date" (offset 20) and "TXT SVN" (offset 28)
 * fields as a NUL-terminated UTF16 string.
 */
#define SCRTM_VERSION_LENGTH  13

/*
 * Definitions of the structures come from Intel document #575623 CBnT BWG v1.2.5 ("Intel
 * Converged Boot Guard and Intel Trusted Execution Technology (Intel TXT) BIOS Specification").
 *
 * Versions in structures could be different for other revisions of the document.
 */

/* Header of the Boot Policy Manifest. */
struct bpm_header {
	char structure_id[8];     /* "__ACBP__" */
	uint8_t struct_ver;       /* 0x24 */
	uint8_t hdr_struct_ver;   /* 0x20 */
	uint16_t hdr_size;
	uint16_t key_sig_offset;  /* offset to KeySignature field of Policy Manifest Signature
				     Element (PMSE) */
	uint8_t bpm_revision;
	uint8_t bpm_svn;
	uint8_t acmsvn_auth;
	uint8_t reserved;
	uint16_t nem_data_stack;  /* in 4 KiB pages */
	uint8_t se_element[];
} __packed;

/* Deprecated uses must be initialized as { .alg = TPM_ALG_NULL (0x10), .size = 0 } */
struct hash_struct {
	uint16_t alg;
	uint16_t size;
	uint8_t data[];
} __packed;

struct bpm_hash_list {
	uint16_t size;
	uint16_t count;
	struct hash_struct hashes[];
} __packed;

/* IBB Segment Element (upper part) */
struct bpm_ibbs {
	char structure_id[8];  /* "__IBBS__" */
	uint8_t struct_ver;    /* 0x21 */
	uint8_t reserved1;
	uint16_t element_size;
	uint8_t reserved2;
	uint8_t set_number;    /* 0 */
	uint8_t reserved3;
	uint8_t pbet_value;
	uint32_t flags;        /* see BPM_IBBS_FLAG_* constants for values */
	uint64_t iommu_bar0;
	uint64_t iommu_bar1;
	uint32_t dma_prot_base0;
	uint32_t dma_prot_limit0;
	uint64_t dma_prot_base1;
	uint64_t dma_prot_limit1;
	struct hash_struct post_ibb_hash; /* deprecated since v1.2.0 of CBnT BWG */
	uint32_t ibb_entry_point;
	struct bpm_hash_list digest_list; /* each entry is of variable size */
	/* struct bpm_ibbs_bottom bottom; */
} __packed;

/* IBB Segment Element (lower part) */
struct bpm_ibbs_bottom {
	struct hash_struct obb_hash; /* deprecated since v1.2.0 of CBnT BWG */
	uint8_t reserved4[3];
	uint8_t segment_count;
	/* ibb_segments[segment_count]; */
} __packed;

/* KMHASH_STRUCT */
struct km_hash {
	uint64_t usage;           /* see KM_HASH_USAGE_* constants for values */
	struct hash_struct hash;
} __packed;

/* Key Manifest */
struct km_header {
	char structure_id[8];       /* "__KEYM__" */
	uint8_t struct_ver;         /* 0x21 */
	uint8_t reserved1[3];
	uint16_t key_sig_offset;
	uint8_t reserved2[3];
	uint8_t km_revision;
	uint8_t km_svn;
	uint8_t km_id;
	uint16_t km_pub_key_hash_alg;
	uint16_t key_count;
	struct km_hash key_hash[];  /* of variable size */
	/* key_sig_offset points here where Key Manifest Signature data is one
	   of 2 Key Signature Structure formats: RSA or ECC / SM2. */
} __packed;

static const void *find_in_fit(uint8_t type, size_t *size)
{
	struct fit_entry *entry = fit_table_search(type);
	if (entry == NULL) {
		printk(BIOS_ERR, "CBnT: failed to find FIT entry: %#02x\n", type);
		return NULL;
	}

	void *addr = (void *)(uintptr_t)entry->address.u64;

	const struct acm_header_v0 *acm;
	switch (type) {
	case FIT_ENTRY_TYPE_SACM:
		if (fit_entry_type(&entry[1]) == FIT_ENTRY_TYPE_SACM) {
			/* A BIOS image can include more than Startup ACM, but this function
			   always returns the first one which might be wrong. */
			printk(BIOS_WARNING, "CBnT: picking SACM from FIT at random!\n");
		}

		/* All known versions of the header (up to v5.4) have compatible version and
		   size fields. */
		acm = (const struct acm_header_v0 *)addr;
		if (acm->header_version[1] > 5 || acm->header_version[0] > 4) {
			printk(BIOS_ERR, "CBnT: unsupported version of SACM: %d.%d\n",
			       acm->header_version[1], acm->header_version[0]);
			return NULL;
		}

		*size = acm->size * 4;
		break;
	case FIT_ENTRY_TYPE_KM:
		if (fit_entry_type(&entry[1]) == FIT_ENTRY_TYPE_KM) {
			/* A BIOS image can include more than one manifest, but this function
			   always returns the first one which might be wrong. */
			printk(BIOS_WARNING,
			       "CBnT: picking Key Manifest from FIT at random!\n");
		}
		*size = fit_entry_size(entry);
		break;
	case FIT_ENTRY_TYPE_BPM:
		*size = fit_entry_size(entry);
		break;

	default:
		printk(BIOS_ERR, "CBnT: unexpected FIT entry type: %#02x\n", type);
		return NULL;
	}

	return addr;
}

static enum cb_err get_flags(bool *conventional_measurements, bool *auth_measure,
			     uint64_t *biosacm_sts_mask)
{
	size_t bpm_len;
	const struct bpm_header *bpm = find_in_fit(FIT_ENTRY_TYPE_BPM, &bpm_len);
	if (bpm == NULL) {
		printk(BIOS_ERR, "CBnT: failed to find BPM\n");
		return CB_ERR;
	}

	/* Meteor Lake seems to be the first generation with new (more TCG-like) style of
	   measurements. */
	*conventional_measurements = !CONFIG(SOC_INTEL_METEORLAKE)
				  && !CONFIG(SOC_INTEL_PANTHERLAKE_BASE);

	const struct bpm_ibbs *ibbs = (const void *)bpm->se_element;
	*auth_measure = (ibbs->flags & BPM_IBBS_FLAG_AUTH_MEASURE) != 0;
	*biosacm_sts_mask =
		(ibbs->flags & BPM_IBBS_FLAG_SRTM_AC) != 0 ? SRTM_AC_MASK : ~(uint64_t)0;

	/* Auth data is measured only for TPM2. */
	if (*auth_measure && tlcl_get_family() != TPM_2)
		*auth_measure = false;

	return CB_SUCCESS;
}

static enum cb_err copy_acm_signature(struct obuf *ob)
{
	/* Not running any checks on the ACM like TXT code does, it must be correct if we got
	   here. */
	size_t acm_len;
	const struct acm_header_v3 *acm = find_in_fit(FIT_ENTRY_TYPE_SACM, &acm_len);
	if (acm == NULL) {
		printk(BIOS_ERR, "CBnT: failed to find SACM\n");
		return CB_ERR;
	}

	/* Versions v3.0 through v5.4 support CBnT and have this field in the same place. */
	int err = obuf_write(ob, acm->rsa3072_sig, sizeof(acm->rsa3072_sig));
	if (err)
		printk(BIOS_ERR, "CBnT: failed to write ACM signature\n");

	return err ? CB_ERR : CB_SUCCESS;
}

static enum cb_err fill_pcr0_acm_fields(struct obuf *ob)
{
	/* Not running any checks on the ACM like TXT code does, it must be correct if we got
	   here. */
	size_t acm_len;
	const struct acm_header_v3 *acm = find_in_fit(FIT_ENTRY_TYPE_SACM, &acm_len);
	if (acm == NULL) {
		printk(BIOS_ERR, "CBnT: failed to find SACM\n");
		return CB_ERR;
	}

	/*
	 * ACM.Header.SVN (won't run out of space on this one).
	 * All known versions of the header (up to v5.4) have compatible txt_svn field.
	 */
	(void)obuf_write_le16(ob, acm->txt_svn);

	return copy_acm_signature(ob);
}

static enum cb_err skip_signature_key(struct ibuf *ib, uint16_t *key_alg)
{
	if (ibuf_read_le16(ib, key_alg)) {
		printk(BIOS_ERR, "CBnT: failed to read key algorithm\n");
		return CB_ERR;
	}
	if (*key_alg != TPM_ALG_RSA && *key_alg != TPM_ALG_ECC) {
		printk(BIOS_ERR, "CBnT: unexpected public key algorithm: %#04x\n", *key_alg);
		return CB_ERR;
	}

	uint8_t key_version;
	if (ibuf_read_le8(ib, &key_version)) {
		printk(BIOS_ERR, "CBnT: failed to read public key version\n");
		return CB_ERR;
	}
	if (key_version != 0x10) {
		printk(BIOS_ERR, "CBnT: unexpected version of public key: %#02x\n",
		       key_version);
		return CB_ERR;
	}

	uint16_t key_size;
	if (ibuf_read_le16(ib, &key_size)) {
		printk(BIOS_ERR, "CBnT: failed to read public key size\n");
		return CB_ERR;
	}

	if (*key_alg == TPM_ALG_RSA) {
		(void)ibuf_oob_drain(ib, 4);            /* public exponent */
		(void)ibuf_oob_drain(ib, key_size / 8); /* the public key */
	} else if (*key_alg == TPM_ALG_ECC) {
		(void)ibuf_oob_drain(ib, key_size / 8); /* Qx */
		(void)ibuf_oob_drain(ib, key_size / 8); /* Qy */
	}

	return CB_SUCCESS;
}

/* Copies a signature after parsing "RSA Key Signature Structure Format" or
   "ECC / SM2 Key Signature Structure Format". */
static enum cb_err copy_signature(struct ibuf ib, struct obuf *ob)
{
	uint8_t key_version;
	if (ibuf_read_le8(&ib, &key_version)) {
		printk(BIOS_ERR, "CBnT: failed to read key signature version\n");
		return CB_ERR;
	}
	if (key_version != 0x10) {
		printk(BIOS_ERR, "CBnT: unexpected key signature version: %#02x\n",
		       key_version);
		return CB_ERR;
	}

	uint16_t key_alg;
	if (skip_signature_key(&ib, &key_alg) != CB_SUCCESS) {
		printk(BIOS_ERR, "CBnT: failed to skip signature key\n");
		return CB_ERR;
	}

	uint16_t sig_scheme;
	if (ibuf_read_le16(&ib, &sig_scheme)) {
		printk(BIOS_ERR, "CBnT: failed to read signature scheme\n");
		return CB_ERR;
	}
	if (key_alg == TPM_ALG_RSA) {
		if (sig_scheme != TPM_ALG_RSASSA && sig_scheme != TPM_ALG_RSAPSS) {
			printk(BIOS_ERR, "CBnT: unexpected public signature scheme: %#04x\n",
			       sig_scheme);
			return CB_ERR;
		}
	} else if (key_alg == TPM_ALG_ECC) {
		if (sig_scheme != TPM_ALG_ECDSA && sig_scheme != TPM_ALG_SM2) {
			printk(BIOS_ERR, "CBnT: unexpected public signature scheme: %#04x\n",
			       sig_scheme);
			return CB_ERR;
		}
	}

	uint8_t version;
	if (ibuf_read_le8(&ib, &version)) {
		printk(BIOS_ERR, "CBnT: failed to read signature version\n");
		return CB_ERR;
	}
	if (version != 0x10) {
		printk(BIOS_ERR, "CBnT: unexpected signature version: %#02x\n", version);
		return CB_ERR;
	}

	uint16_t size;
	if (ibuf_read_le16(&ib, &size)) {
		printk(BIOS_ERR, "CBnT: failed to read signature size\n");
		return CB_ERR;
	}

	uint16_t hash_alg;
	if (ibuf_read_le16(&ib, &hash_alg)) {
		printk(BIOS_ERR, "CBnT: failed to read hash algorithm of signature\n");
		return CB_ERR;
	}

	size_t sig_size = 0;
	if (key_alg == TPM_ALG_RSA) {
		sig_size += size / 8; /* the signature */
	} else if (key_alg == TPM_ALG_ECC) {
		sig_size += size / 8; /* R */
		sig_size += size / 8; /* S */
	}

	const void *sig_data = ibuf_oob_drain(&ib, sig_size);
	if (sig_data == NULL) {
		printk(BIOS_ERR, "CBnT: failed to read signature\n");
		return CB_ERR;
	}

	if (obuf_write(ob, sig_data, sig_size)) {
		printk(BIOS_ERR, "CBnT: failed to write signature\n");
		return CB_ERR;
	}

	return CB_SUCCESS;
}

static enum cb_err copy_km_signature(struct obuf *ob)
{
	size_t km_len;
	const struct km_header *km = find_in_fit(FIT_ENTRY_TYPE_KM, &km_len);
	if (km == NULL) {
		printk(BIOS_ERR, "CBnT: failed to find KM\n");
		return CB_ERR;
	}

	struct ibuf km_sig_ib;
	ibuf_init(&km_sig_ib, (const uint8_t *)km + km->key_sig_offset,
		  km_len - km->key_sig_offset);

	/* KM.Signature[KM signature size] */
	enum cb_err err = copy_signature(km_sig_ib, ob);
	if (err != CB_SUCCESS)
		printk(BIOS_ERR, "CBnT: failed to copy KM signature\n");

	return err;
}

/* Finds public key used to sign Key Manifest and hashes it with the algorithm specified in the
   header. */
static enum cb_err hash_signature_key(const struct km_header *km, size_t km_len,
				      struct vb2_hash *hash)
{
	/* All safety checks are in skip_signature_key() which is always run for PCR-0, so not
	   bothering here for optional PCR-7 which is also computed after PCR-0. */
	struct ibuf sig_ib;
	ibuf_init(&sig_ib, (uint8_t *)km + km->key_sig_offset, km_len - km->key_sig_offset);

	uint16_t key_alg;
	(void)ibuf_read_le16(&sig_ib, &key_alg);

	(void)ibuf_oob_drain(&sig_ib, 1); /* key signature version */
	(void)ibuf_oob_drain(&sig_ib, 2); /* key algorithm */
	(void)ibuf_oob_drain(&sig_ib, 1); /* key version */

	uint16_t key_size;
	(void)ibuf_read_le16(&sig_ib, &key_size);

	size_t key_data_size = 0;
	if (key_alg == TPM_ALG_RSA)
		key_data_size = 4 + key_size / 8;   /* public exponent and public key */
	else if (key_alg == TPM_ALG_ECC)
		key_data_size = (key_size / 8) * 2; /* Qx + Qy */

	if (vb2_hash_calculate(vboot_hwcrypto_allowed(), ibuf_oob_drain(&sig_ib, key_data_size),
			       key_data_size, tpm2_alg_to_vb2_hash(km->km_pub_key_hash_alg),
			       hash)) {
		printk(BIOS_ERR, "CBnT: failed to hash PCR-0 measurement data\n");
		return CB_ERR;
	}

	return CB_SUCCESS;
}

static enum cb_err fill_pcr7_km_fields(struct obuf *ob)
{
	size_t km_len;
	const struct km_header *km = find_in_fit(FIT_ENTRY_TYPE_KM, &km_len);

	struct vb2_hash km_pubkey_hash;
	if (hash_signature_key(km, km_len, &km_pubkey_hash) != CB_SUCCESS) {
		printk(BIOS_ERR, "CBnT: failed to hash Key Manifest's signature key\n");
		return CB_ERR;
	}

	/* BP.KEY (hash of public part of the key used to sign Key Manifest) */
	if (obuf_write(ob, km_pubkey_hash.raw, vb2_digest_size(km_pubkey_hash.algo))) {
		printk(BIOS_ERR,
		       "CBnT: failed to write hash of Key Manifest's signature key\n");
		return CB_ERR;
	}

	/* Find hash that corresponds to BPM. */
	const struct km_hash *key_hash = km->key_hash;
	unsigned int i;
	for (i = 0; i < km->key_count; ++i) {
		if (key_hash->usage & KM_HASH_USAGE_BPM)
			break;
		key_hash = (const struct km_hash *)&key_hash->hash.data[key_hash->hash.size];
	}
	if (i == km->key_count) {
		printk(BIOS_ERR, "CBnT: failed to find hash of BPM pubkey in KM\n");
		return CB_ERR;
	}

	/* KM.BPKey.hashBuffer[BPM pubkey digest size] */
	if (obuf_write(ob, key_hash->hash.data, key_hash->hash.size)) {
		printk(BIOS_ERR, "CBnT: failed to write ACM signature\n");
		return CB_ERR;
	}

	return CB_SUCCESS;
}

static const struct hash_struct *find_ibb_digest(const struct bpm_ibbs *ibbs,
						 enum vb2_hash_algorithm alg)
{
	unsigned int i;
	const struct hash_struct *ibb_hash = ibbs->digest_list.hashes;
	uint16_t tpm2_alg = tpm2_alg_from_vb2_hash(alg);
	for (i = 0; i < ibbs->digest_list.count; ++i) {
		if (ibb_hash->alg == tpm2_alg)
			return ibb_hash;

		ibb_hash = (const void *)&ibb_hash->data[ibb_hash->size];
	}

	return NULL;
}

static enum cb_err copy_ibb_hash(struct obuf *ob, enum vb2_hash_algorithm alg)
{
	size_t bpm_len;
	const struct bpm_header *bpm = find_in_fit(FIT_ENTRY_TYPE_BPM, &bpm_len);
	if (bpm == NULL) {
		printk(BIOS_ERR, "CBnT: failed to find BPM\n");
		return CB_ERR;
	}

	const struct bpm_ibbs *ibbs = (const void *)bpm->se_element;
	const struct hash_struct *ibb_hash = find_ibb_digest(ibbs, alg);
	if (ibb_hash == NULL) {
		printk(BIOS_ERR, "CBnT: failed to find matching IBB digest\n");
		return CB_ERR;
	}

	/* IBB.Digest[IBB digest size] */
	if (obuf_write(ob, ibb_hash->data, ibb_hash->size)) {
		printk(BIOS_ERR, "CBnT: failed to write IBB digest\n");
		return CB_ERR;
	}

	return CB_SUCCESS;
}

static enum cb_err copy_bpm_signature(struct obuf *ob, enum vb2_hash_algorithm alg)
{
	size_t bpm_len;
	const struct bpm_header *bpm = find_in_fit(FIT_ENTRY_TYPE_BPM, &bpm_len);
	if (bpm == NULL) {
		printk(BIOS_ERR, "CBnT: failed to find BPM\n");
		return CB_ERR;
	}

	struct ibuf bpm_sig_ib;
	ibuf_init(&bpm_sig_ib, (const uint8_t *)bpm + bpm->key_sig_offset,
		  bpm_len - bpm->key_sig_offset);

	/* BPM.Signature[BPM signature size] */
	if (copy_signature(bpm_sig_ib, ob) != CB_SUCCESS) {
		printk(BIOS_ERR, "CBnT: failed to copy BPM signature\n");
		return CB_ERR;
	}

	return copy_ibb_hash(ob, alg);
}

/*
 * Pseudo-code of the data to be measured into PCR-7 for TigerLake and newer (older hardware
 * isn't supported):
 *
 *     struct {
 *         uint64_t ACM_POLICY_STATUS;
 *         uint16_t ACM.Header.SVN;
 *         uint8_t  ACM.KeyHash[32];
 *         uint8_t  BP.KEY[32 or 48];  // Hash of the public key from Key Manifest's signature.
 *         uint8_t  KM.BPKey.hashBuffer[BPM pubkey digest size];
 *     } PCR7_DATA
 *
 * Applies only for TPM2.0.
 *
 * Returns true on success.
 */
static bool make_pcr7_data(struct obuf *ob)
{
	/*
	 * ACM.KeyHash[32]
	 *
	 * BTG BWG says to use TXT.PUBLIC.KEY, but TXT SDG (#315168-017.4) and CBnT BWG say
	 * that it doesn't contain anything useful on modern platforms utilizing SGX and
	 * suggests to use MSRs 0x20::0x23.
	 */
	(void)obuf_write_le64(ob, rdmsr(MSR_ACM_CPU_KEY_HASH_0).raw);
	(void)obuf_write_le64(ob, rdmsr(MSR_ACM_CPU_KEY_HASH_1).raw);
	(void)obuf_write_le64(ob, rdmsr(MSR_ACM_CPU_KEY_HASH_2).raw);
	(void)obuf_write_le64(ob, rdmsr(MSR_ACM_CPU_KEY_HASH_3).raw);

	if (fill_pcr7_km_fields(ob) != CB_SUCCESS) {
		printk(BIOS_ERR, "CBnT: failed to fill KM fields of PCR-7 measurement data\n");
		return false;
	}

	return true;
}

int intel_cbnt_get_locality(void)
{
	/* Startup locality is always 0 for TPM 1.2. */
	if (tlcl_get_family() == TPM_1)
		return 0;

	union cbnt_biosacm_policy biosacm_sts = {
		.raw = read64p(CBNT_BIOSACM_POLICY_STS),
	};

	/* It's 0 without active CBnT. */
	if (biosacm_sts.status.scrtm_status == 0)
		return 0;

	return biosacm_sts.status.tpm_startup_locality == 0 ? 3 : 0;
}

static enum cb_err cap_pcrs_for_alg(enum vb2_hash_algorithm alg)
{
	size_t bpm_len;
	const struct bpm_header *bpm = find_in_fit(FIT_ENTRY_TYPE_BPM, &bpm_len);
	if (bpm == NULL) {
		printk(BIOS_ERR, "CBnT: failed to find BPM\n");
		return CB_ERR;
	}

	const struct bpm_ibbs *ibbs = (const void *)bpm->se_element;

	/* Alternative to capping is disabling the TPM which requires no event log entries. */
	if (!(ibbs->flags & BPM_IBBS_FLAG_CAP_PCRS_ON_TPM_FAIL))
		return CB_SUCCESS;

	const uint32_t separator = 0x00000001;
	struct vb2_hash hash;
	if (vb2_hash_calculate(vboot_hwcrypto_allowed(), &separator, sizeof(separator), alg,
			       &hash)) {
		printk(BIOS_ERR, "CBnT: failed to hash PCR separator\n");
		return CB_ERR;
	}

	const struct tpm_digest cap_digests[] = {
		{ .hash_type = hash.algo, .hash = hash.raw },
		{ .hash_type = VB2_HASH_INVALID }
	};

	for (int pcr = 0; pcr < 8; pcr++) {
		tpm_log_add_table_entry("CBnT hit TPM failure during boot", pcr, cap_digests);
		printk(BIOS_INFO, "CBnT: capped PCR-%d\n", pcr);
	}
	return CB_SUCCESS;
}

static enum cb_err cap_pcrs(union cbnt_biosacm_policy biosacm_sts)
{
	size_t bpm_len;
	const struct bpm_header *bpm = find_in_fit(FIT_ENTRY_TYPE_BPM, &bpm_len);
	if (bpm == NULL) {
		printk(BIOS_ERR, "CBnT: failed to find BPM\n");
		return CB_ERR;
	}

	const struct bpm_ibbs *ibbs = (const void *)bpm->se_element;

	int i;
	for (i = 0; i < ENABLED_TPM_ALGS_NUM; ++i) {
		enum vb2_hash_algorithm alg = enabled_tpm_algs[i];
		if (!tpm_log_alg_active(alg))
			continue;

		/*
		 * If there was a TPM failure, cap all active banks.  Otherwise, cap those
		 * active banks for which there is no corresponding IBB digests in BPM.
		 */
		if (biosacm_sts.status.tpm_success && find_ibb_digest(ibbs, alg) != NULL)
			continue;

		enum cb_err err = cap_pcrs_for_alg(alg);
		if (err != CB_SUCCESS) {
			printk(BIOS_WARNING, "CBnT: failed to cap %s bank\n",
			       vb2_get_hash_algorithm_name(alg));
			return err;
		}
	}

	return CB_SUCCESS;
}

static char hex_digit(int nibble)
{
	if (nibble < 10)
		return '0' + nibble;
	return 'A' + nibble - 10;
}

static void measure_intel_tgl_style(uint64_t biosacm_policy, bool auth_measure)
{
	uint8_t data[MAX_DATA_SIZE];
	struct obuf data_ob;
	obuf_init(&data_ob, data, sizeof(data));

	/*
	 * Making and hashing PCR-0 data.
	 *
	 * Pseudo-code of the data to be measured into PCR-0 for TigerLake and newer
	 * (older hardware isn't supported yet):
	 *
	 *     struct {
	 *          uint64_t ACM_POLICY_STATUS;
	 *          uint16_t ACM.Header.SVN;
	 *          uint8_t  ACM.Signature[ACM signature size];
	 *          uint8_t  KM.Signature[KM signature size];
	 *          uint8_t  BPM.Signature[BPM signature size];
	 *          uint8_t  IBB.Digest[IBB digest size];
	 *     } PCR0_DATA;
	 */

	/* ACM_POLICY_STATUS (won't run out of space on this one) */
	(void)obuf_write_le64(&data_ob, biosacm_policy);

	if (fill_pcr0_acm_fields(&data_ob) != CB_SUCCESS) {
		printk(BIOS_ERR, "CBnT: failed to fill ACM fields of PCR-0 measurement data\n");
		return;
	}

	if (copy_km_signature(&data_ob) != CB_SUCCESS) {
		printk(BIOS_ERR, "CBnT: failed to copy KM signature for PCR-0 measurement\n");
		return;
	}

	int i, j;
	struct tpm_digests pcr0_digests;
	for (i = 0, j = 0; i < ENABLED_TPM_ALGS_NUM; ++i) {
		enum vb2_hash_algorithm alg = enabled_tpm_algs[i];
		if (!tpm_log_alg_active(alg))
			continue;

		struct obuf local_ob = data_ob;
		if (copy_bpm_signature(&local_ob, alg) != CB_SUCCESS) {
			printk(BIOS_ERR,
			       "CBnT: failed to copy BPM signature for PCR-0 measurement\n");
			return;
		}

		if (vb2_hash_calculate(vboot_hwcrypto_allowed(), data,
				       obuf_nr_written(&data_ob), alg,
				       &pcr0_digests.hashes[i])) {
			printk(BIOS_ERR, "CBnT: failed to hash PCR-0 measurement data\n");
			return;
		}

		pcr0_digests.values[j].hash_type = alg;
		pcr0_digests.values[j].hash = pcr0_digests.hashes[i].raw;
		++j;
	}

	pcr0_digests.values[j].hash_type = VB2_HASH_INVALID;

	/* Per BWG this should be logged with EV_S_CRTM_CONTENTS type. */
	tpm_log_add_table_entry(CBNT_EVENT_LOG_MESSAGE, 0, pcr0_digests.values);

	/* Optionally making and hashing PCR-7 data (not supported since MTL). */
	if (!auth_measure)
		return;

	struct tpm_digests pcr7_digests;

	/* Reuse the first 2 fields of PCR-0 data which are identical in both cases. */
	obuf_init(&data_ob, data, sizeof(data));
	(void)obuf_oob_fill(&data_ob, sizeof(uint64_t) + sizeof(uint16_t));

	if (!make_pcr7_data(&data_ob)) {
		printk(BIOS_ERR, "CBnT: failed to build PCR-7 measurement data\n");
		return;
	}

	if (!tpm_make_digests(data, obuf_nr_written(&data_ob), NULL, &pcr7_digests)) {
		printk(BIOS_ERR, "CBnT: failed to hash PCR-7 measurement data\n");
		return;
	}

	/* Per BWG this should be logged with EV_EFI_VARIABLE_DRIVER_CONFIG type
	   and event name should be a Unicode string. */
	tpm_log_add_table_entry(CBNT_EVENT_LOG_MESSAGE, 7, pcr7_digests.values);
	printk(BIOS_INFO, "CBnT: reconstructed PCR-7 measurement\n");
}

static void measure_tcg_style(uint64_t biosacm_policy)
{
	size_t acm_len;
	const struct acm_header_v3 *acm = find_in_fit(FIT_ENTRY_TYPE_SACM, &acm_len);
	if (acm == NULL) {
		printk(BIOS_ERR, "CBnT: failed to find SACM\n");
		return;
	}

	uint8_t crtm_version[6];
	memcpy(&crtm_version[0], &acm->date, 4);
	memcpy(&crtm_version[4], &acm->txt_svn, 2);

	char crtm_version_str[SCRTM_VERSION_LENGTH] = {0};
	uint16_t crtm_version_utf16[SCRTM_VERSION_LENGTH] = {0};
	for (int i = 0; i < sizeof(crtm_version); ++i) {
		crtm_version_str[i*2 + 0] = hex_digit(crtm_version[i] >> 4);
		crtm_version_str[i*2 + 1] = hex_digit(crtm_version[i] & 0xf);

		crtm_version_utf16[i*2 + 0] = crtm_version_str[i*2 + 0];
		crtm_version_utf16[i*2 + 1] = crtm_version_str[i*2 + 1];
	}

	struct tpm_digests crtm_digests;
	if (!tpm_make_digests(crtm_version_utf16, sizeof(crtm_version_utf16), NULL,
			      &crtm_digests)) {
		printk(BIOS_ERR, "CBnT: failed to hash CRTM version\n");
		return;
	}
	/* Per BWG this should be logged with EV_S_CRTM_VERSION type. */
	tpm_log_add_table_entry(crtm_version_str, 0, crtm_digests.values);

	int i, j;

	struct tpm_digests post_code_digests;
	for (i = 0, j = 0; i < ENABLED_TPM_ALGS_NUM; ++i) {
		enum vb2_hash_algorithm alg = enabled_tpm_algs[i];
		if (!tpm_log_alg_active(alg))
			continue;

		struct obuf data_ob;
		obuf_init(&data_ob, post_code_digests.hashes[j].sha512,
			  sizeof(post_code_digests.hashes[j].sha512));

		if (copy_ibb_hash(&data_ob, alg) != CB_SUCCESS) {
			printk(BIOS_ERR, "CBnT: failed to obtain IBB digest\n");
			return;
		}

		++j;
	}

	post_code_digests.values[j].hash_type = VB2_HASH_INVALID;

	/* Per BWG this should be logged with EV_POST_CODE type. */
	tpm_log_add_table_entry("Boot Guard Measured IBB", 0, post_code_digests.values);

	/*
	 * struct {
	 *      uint64_t ACM_POLICY_STATUS;
	 *      uint8_t  KM.Signature[KM signature size];
	 *      uint8_t  BPM.Signature[BPM signature size];
	 * } POLICY_DATA;
	 */
	uint8_t data[MAX_DATA_SIZE];
	struct obuf data_ob;
	obuf_init(&data_ob, data, sizeof(data));

	/* ACM_POLICY_STATUS (won't run out of space on this one) */
	(void)obuf_write_le64(&data_ob, biosacm_policy);
	if (copy_acm_signature(&data_ob) != CB_SUCCESS) {
		printk(BIOS_ERR, "CBnT: failed to copy ACM signature\n");
		return;
	}
	if (copy_km_signature(&data_ob) != CB_SUCCESS) {
		printk(BIOS_ERR, "CBnT: failed to copy KM signature\n");
		return;
	}

	struct tpm_digests policy_digests;
	for (i = 0, j = 0; i < ENABLED_TPM_ALGS_NUM; ++i) {
		enum vb2_hash_algorithm alg = enabled_tpm_algs[i];
		if (!tpm_log_alg_active(alg))
			continue;

		struct obuf local_ob = data_ob;
		if (copy_bpm_signature(&local_ob, alg) != CB_SUCCESS) {
			printk(BIOS_ERR,
			       "CBnT: failed to copy BPM signature for PCR-0 measurement\n");
			return;
		}

		if (vb2_hash_calculate(vboot_hwcrypto_allowed(), data,
				       obuf_nr_written(&data_ob), alg,
				       &policy_digests.hashes[i])) {
			printk(BIOS_ERR, "CBnT: failed to hash PCR-0 measurement data\n");
			return;
		}

		policy_digests.values[j].hash_type = alg;
		policy_digests.values[j].hash = policy_digests.hashes[i].raw;
		++j;
	}

	policy_digests.values[j].hash_type = VB2_HASH_INVALID;

	/* Per BWG this should be logged with EV_POST_CODE type. */
	if (tpm_extend_pcr(0, policy_digests.values,
			   "BIOS Measured Boot Guard Policy") != TPM_SUCCESS)
		printk(BIOS_ERR, "CBnT: failed to extend POLICY_DATA\n");
}

void intel_cbnt_inject_ibg_measurements(void)
{
	const union cbnt_biosacm_policy biosacm_sts = {
		.raw = read64p(CBNT_BIOSACM_POLICY_STS),
	};

	/* Do nothing if BootGuard wasn't involved in the boot process. */
	if (biosacm_sts.status.scrtm_status == 0)
		return;

	/* Do nothing if BootGuard doesn't measure anything. */
	if (!(biosacm_sts.status.bp & CBNT_BP_TYPE_M))
		return;

	if (cap_pcrs(biosacm_sts) != CB_SUCCESS) {
		printk(BIOS_ERR, "CBnT: failed to cap PCRs\n");
		return;
	}

	/* cap_pcrs() must have logged that PCRs were capped, nothing else to do on TPM
	   error. */
	if (!biosacm_sts.status.tpm_success) {
		printk(BIOS_ERR, "CBnT: TPM failure detected\n");
		return;
	}

	bool conventional_measurements;
	bool auth_measure;
	uint64_t biosacm_sts_mask;
	if (get_flags(&conventional_measurements, &auth_measure, &biosacm_sts_mask) !=
	    CB_SUCCESS) {
		printk(BIOS_ERR, "CBnT: failed to obtain IBBS flags from BPM\n");
		return;
	}

	uint64_t biosacm_policy = biosacm_sts.raw & biosacm_sts_mask;
	if (conventional_measurements)
		measure_intel_tgl_style(biosacm_policy, auth_measure);
	else
		measure_tcg_style(biosacm_policy);

	/*
	 * BWG suggests to Perform reconstruction at this point, but we are likely to continue
	 * the boot anyway as Measured Boot failures generally aren't fatal and incorrect hash
	 * will be caught later during the boot process.  So not bothering.
	 */
}
