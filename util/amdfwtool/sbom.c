/* SPDX-License-Identifier: GPL-2.0-only */

/*
 * Generate CoSWID (Concise Software Identification) SBOM JSON files for
 * AMD PSP firmware blobs processed by amdfwtool.
 *
 * Type classifications are derived from PSPTool:
 *   https://github.com/PSPReverse/PSPTool
 */

#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "amdfwtool.h"

/*
 * Public key entry types (PUBKEY_ENTRY_TYPES from PSPTool).
 * These entries contain public keys; they have AMD firmware headers but are
 * presented with a dedicated CoSWID template.
 */
static const uint8_t pubkey_entry_types[] = {
	AMD_FW_PSP_PUBKEY,
	AMD_FW_PSP_RTM_PUBKEY,
	AMD_FW_PSP_SECURED_DEBUG,
	AMD_FW_ABL_PUBKEY,
	AMD_FW_PSP_TRUSTLETKEY,
	0x43,
	0x4e,
	AMD_FW_VERSTAGE_SIG,
	0x81,
	0x97,
	0xad,
};

/*
 * No-header entry types (NO_HDR_ENTRY_TYPES from PSPTool).
 * These entries lack the standard AMD firmware header and are skipped from
 * SBOM generation unless noted as exceptions below.
 *
 * Exceptions handled separately:
 *   0x0b (AMD_PSP_FUSE_CHAIN)  - soft fuse; must never appear in SBOM
 *   0x1a (AMD_SEV_DRIVER)      - add as regular PSP FW if $PS1 magic present
 *   0x21, 0x82, 0x84, 0x8d    - wrapped KEK; use wrapped-KEK template
 */
static const uint8_t no_hdr_entry_types[] = {
	AMD_FW_PSP_NVRAM,
	AMD_PSP_FUSE_CHAIN,
	AMD_WRAPPED_IKEK,
	AMD_FW_L2_PTR,
	AMD_FW_RECOVERYAB_A,
	AMD_FW_BIOS_TABLE,
	AMD_FW_RECOVERYAB_B,
	AMD_BIOS_L2_PTR,
	0x06,
	AMD_BIOS_APOB,
	AMD_BIOS_APCB,
	AMD_BIOS_APCB_BK,
	AMD_FW_TPMLITE,
	AMD_SEV_DRIVER,
	AMD_TOKEN_UNLOCK,
	AMD_BIOS_APOB_NV,
	AMD_FW_FHP_DRIVER,
	AMD_BIOS_UCODE,
	AMD_BIOS_NV_ST,
	AMD_BIOS_BIN,
	AMD_BIOS_SIG,
	AMD_SEV_DATA,
	0x46,
	AMD_RPMC_NVRAM,
	0x82,
	0x84,
	AMD_TA_IKEK,
	AMD_FW_DPE_DRIVER,
	0x7c,
	AMD_FW_FCFG_TABLE,
};

/*
 * Key store types (KEY_STORE_TYPES from PSPTool).
 * Despite being related to cryptographic keys, these entries DO have AMD
 * firmware headers and should be treated as regular PSP FW in the SBOM.
 */
static const uint8_t key_store_types[] = {
	AMD_FW_KEYDB_BL,
	AMD_FW_KEYDB_TOS,
};

/*
 * Wrapped Key Encryption Key types.
 * These are a subset of NO_HDR types that carry no standard header.
 * The raw file content is included as the version field in the SBOM.
 */
static const uint8_t wrapped_kek_types[] = {
	AMD_WRAPPED_IKEK,
	0x82,
	0x84,
	AMD_TA_IKEK,
};

/*
 * BIOS entry types that should be processed for SBOM.
 */
static const uint8_t bios_entry_types[] = {
	AMD_BIOS_PMUI,
	AMD_BIOS_PMUD
};

/*
 * CoSWID tag-ids for each PSP FW template type.
 * Generated with: uuidgen --name <name>
 *                         --namespace 6ba7b810-9dad-11d1-80b4-00c04fd430c8
 *                         --sha1
 *
 *   amd-pspfw:             064a09b4-429a-5070-9f41-889708b12456
 *   amd-pspfw-pubkey:      3f904d76-8f5f-5235-a2bb-f54a2f290256
 *   amd-pspfw-wrapped-kek: 23ba5178-17a4-5402-90bd-75ba51441491
 */
#define TAG_ID_PSP_FW        "064a09b4-429a-5070-9f41-889708b12456"
#define TAG_ID_PSP_PUBKEY    "3f904d76-8f5f-5235-a2bb-f54a2f290256"
#define TAG_ID_WRAPPED_KEK   "23ba5178-17a4-5402-90bd-75ba51441491"

typedef enum {
	FW_SBOM_SKIP,        /* Entry has no SBOM representation */
	FW_SBOM_REGULAR,     /* Regular PSP FW with AMD firmware header */
	FW_SBOM_PUBKEY,      /* Public key with AMD firmware header */
	FW_SBOM_WRAPPED_KEK, /* Wrapped KEK without standard header */
} sbom_fw_class_t;

static bool type_in_array(const uint8_t *arr, size_t count, uint8_t type)
{
	size_t i;

	for (i = 0; i < count; i++)
		if (arr[i] == type)
			return true;
	return false;
}

#define IN_PUBKEY_TYPES(t) \
	type_in_array(pubkey_entry_types, ARRAY_SIZE(pubkey_entry_types), (t))
#define IN_NO_HDR_TYPES(t) \
	type_in_array(no_hdr_entry_types, ARRAY_SIZE(no_hdr_entry_types), (t))
#define IN_KEY_STORE_TYPES(t) \
	type_in_array(key_store_types, ARRAY_SIZE(key_store_types), (t))
#define IN_WRAPPED_KEK_TYPES(t) \
	type_in_array(wrapped_kek_types, ARRAY_SIZE(wrapped_kek_types), (t))
#define IN_BIOS_ENTRY_TYPES(t) \
	type_in_array(bios_entry_types, ARRAY_SIZE(bios_entry_types), (t))

/* Return true if the first four bytes of filename match the $PS1 magic. */
static bool has_ps1_magic(const char *filename)
{
	char buf[4];
	int fd;
	ssize_t n;

	fd = open(filename, O_RDONLY);
	if (fd < 0)
		return false;
	n = read(fd, buf, sizeof(buf));
	close(fd);
	return (n == 4 && !strncmp(buf, "$PS1", n));
}

/*
 * Classify a PSP firmware type ID into an SBOM template category.
 *
 * Rules (in order of precedence):
 *  1. Soft fuse (0x0b) - always skip; it is a value, not a firmware blob.
 *  2. Key store types (0x50, 0x51) - have headers; treat as regular PSP FW.
 *  3. Wrapped KEK types (0x21, 0x82, 0x84, 0x8d) - wrapped KEK template.
 *  4. SEV driver (0x1a) - add as regular PSP FW only if $PS1 magic present.
 *  5. Remaining NO_HDR types - skip.
 *  6. Public key types - pubkey template.
 *  7. All others - regular PSP FW template.
 */
static sbom_fw_class_t classify_fw_type(amd_fw_type type, const char *filename)
{
	/* Rule 1: soft fuse must never appear in SBOM */
	if (type == AMD_PSP_FUSE_CHAIN)
		return FW_SBOM_SKIP;

	/* Rule 2: key store types have headers */
	if (IN_KEY_STORE_TYPES(type))
		return FW_SBOM_REGULAR;

	/* Rule 3: wrapped KEK types use a dedicated template */
	if (IN_WRAPPED_KEK_TYPES(type))
		return FW_SBOM_WRAPPED_KEK;

	/* Rule 4: SEV driver - check for $PS1 magic */
	if (type == AMD_SEV_DRIVER) {
		if (filename && has_ps1_magic(filename))
			return FW_SBOM_REGULAR;
		return FW_SBOM_SKIP;
	}

	/* Rule 5: all other no-header types are skipped */
	if (IN_NO_HDR_TYPES(type))
		return FW_SBOM_SKIP;

	/* Rule 6: public key types */
	if (IN_PUBKEY_TYPES(type))
		return FW_SBOM_PUBKEY;

	/* Rule 7: regular PSP FW */
	return FW_SBOM_REGULAR;
}


static sbom_fw_class_t classify_bios_fw_type(amd_bios_type type,
					     __maybe_unused const char *filename)
{
	/* Rule 1: key store types have headers */
	if (IN_BIOS_ENTRY_TYPES(type))
		return FW_SBOM_REGULAR;

	return FW_SBOM_SKIP;
}

/* Return a human-readable software name for the given PSP firmware type. */
static const char *psp_fw_type_name(amd_fw_entry *entry)
{
	static char buffer[30];

	switch (entry->type) {
	case AMD_FW_PSP_PUBKEY:        return "AMD PSP Public Key";
	case AMD_FW_PSP_BOOTLOADER:    return "AMD PSP Bootloader";
	case AMD_FW_PSP_SECURED_OS:    return "AMD PSP Trusted OS";
	case AMD_FW_PSP_RECOVERY:      return "AMD PSP Recovery Bootloader";
	case AMD_FW_PSP_RTM_PUBKEY:    return "AMD BIOS RTM Public Key";
	case AMD_FW_PSP_SMU_FIRMWARE:  return "AMD SMU Firmware";
	case AMD_FW_PSP_SECURED_DEBUG: return "AMD PSP Secure Debug Key";
	case AMD_FW_ABL_PUBKEY:        return "AMD ABL Public Key";
	case AMD_FW_PSP_TRUSTLETS:     return "AMD PSP Trustlets";
	case AMD_FW_PSP_TRUSTLETKEY:   return "AMD PSP Trustlet Key";
	case AMD_FW_PSP_SMU_FIRMWARE2: return "AMD SMU Firmware 2";
	case AMD_DEBUG_UNLOCK:         return "AMD Debug Unlock";
	case AMD_FW_PSP_TEEIPKEY:      return "AMD PSP TEE IP Key";
	case AMD_SEV_DRIVER:           return "AMD SEV Driver";
	case AMD_BOOT_DRIVER:          return "AMD Boot Driver";
	case AMD_SOC_DRIVER:           return "AMD SoC Driver";
	case AMD_DEBUG_DRIVER:         return "AMD Debug Driver";
	case AMD_INTERFACE_DRIVER:     return "AMD Interface Driver";
	case AMD_HW_IPCFG:             return "AMD HW IP Config";
	case AMD_WRAPPED_IKEK:         return "AMD Wrapped iKEK";
	case AMD_SEC_GASKET:           return "AMD Security Gasket";
	case AMD_MP2_FW:               return "AMD MP2 Firmware";
	case AMD_DRIVER_ENTRIES:       return "AMD Driver Entries";
	case AMD_FW_KVM_IMAGE:         return "AMD KVM Image";
	case AMD_FW_MP5:               return "AMD MP5 Firmware";
	case AMD_S0I3_DRIVER:          return "AMD S0i3 Driver";
	case AMD_ABL0:                 return "AMD Agesa Boot Loader 0";
	case AMD_ABL1:                 return "AMD Agesa Boot Loader 1";
	case AMD_ABL2:                 return "AMD Agesa Boot Loader 2";
	case AMD_ABL3:                 return "AMD Agesa Boot Loader 3";
	case AMD_ABL4:                 return "AMD Agesa Boot Loader 4";
	case AMD_ABL5:                 return "AMD Agesa Boot Loader 5";
	case AMD_ABL6:                 return "AMD Agesa Boot Loader 6";
	case AMD_ABL7:                 return "AMD Agesa Boot Loader 7";
	case AMD_FW_PSP_WHITELIST:     return "AMD PSP Whitelist";
	case AMD_VBIOS_BTLOADER:       return "AMD VBIOS Bootloader";
	case AMD_FW_DXIO:              return "AMD DXIO Firmware";
	case AMD_FW_USB_PHY:           return "AMD USB PHY Firmware";
	case AMD_FW_TOS_SEC_POLICY:    return "AMD TOS Security Policy";
	case AMD_FW_DRTM_TA:           return "AMD DRTM TA";
	case AMD_FW_KEYDB_BL:          return "AMD Key Database Bootloader";
	case AMD_FW_KEYDB_TOS:         return "AMD Key Database TOS";
	case AMD_FW_PSP_VERSTAGE:      return "AMD PSP Verstage";
	case AMD_FW_VERSTAGE_SIG:      return "AMD Verstage Signature";
	case AMD_FW_SPL:               return "AMD SPL Table";
	case AMD_FW_DMCU_ERAM:         return "AMD DMCU ERAM";
	case AMD_FW_DMCU_ISR:          return "AMD DMCU ISR";
	case AMD_FW_MSMU:              return "AMD MSMU Firmware";
	case AMD_FW_SPIROM_CFG:        return "AMD SPI ROM Config";
	case AMD_FW_MPIO:              return "AMD MPIO Firmware";
	case AMD_FW_RAS_DRIVER:        return "AMD RAS Driver";
	case AMD_FW_RAS_TA:            return "AMD RAS TA";
	case AMD_FW_FHP_DRIVER:        return "AMD FHP Driver";
	case AMD_FW_SPDM_DRIVER:       return "AMD SPDM Driver";
	case AMD_FW_DPE_DRIVER:        return "AMD DPE Driver";
	case AMD_FW_DMCUB:             return "AMD DMCUB Firmware";
	case AMD_FW_PSP_BOOTLOADER_AB: return "AMD PSP Bootloader AB";
	case AMD_RIB:                  return "AMD Register Init Binary";
	case AMD_FW_AMF_SRAM:          return "AMD AMF SRAM Firmware";
	case AMD_FW_AMF_DRAM:          return "AMD AMF DRAM Firmware";
	case AMD_FW_MFD_MPM:           return "AMD MFD MPM Firmware";
	case AMD_FW_AMF_WLAN:          return "AMD AMF WLAN Firmware";
	case AMD_FW_AMF_MFD:           return "AMD AMF MFD Firmware";
	case AMD_FW_MPDMA_TF:          return "AMD MPDMA TF Firmware";
	case AMD_TA_IKEK:              return "AMD TA iKEK";
	case AMD_FW_MPCCX:             return "AMD MPCCX Firmware";
	case AMD_FW_GMI3_PHY:          return "AMD GMI3 PHY Firmware";
	case AMD_FW_MPDMA_PM:          return "AMD MPDMA PM Firmware";
	case AMD_FW_PROM21:            return "AMD Promontory 21 Firmware";
	case AMD_FW_PROM19:            return "AMD Promontory 19 Firmware";
	case AMD_FW_LSDMA:             return "AMD LSDMA Firmware";
	case AMD_FW_C20_MP:            return "AMD C20 MP Firmware";
	case AMD_FW_MINIMSMU:          return "AMD Mini-SMU Firmware";
	case AMD_FW_GFXIMU_0:          return "AMD GFX IMU 0 Firmware";
	case AMD_FW_GFXIMU_1:          return "AMD GFX IMU 1 Firmware";
	case AMD_FW_SRAM_FW_EXT:       return "AMD SRAM FW Extension";
	case AMD_FW_TOS_WHITELIST:     return "AMD TOS Whitelist";
	case AMD_FW_UMSMU:             return "AMD UMSMU Firmware";
	case AMD_FW_USBDP:             return "AMD USBDP Firmware";
	case AMD_FW_USBSS:             return "AMD USBSS Firmware";
	case AMD_FW_USB4:              return "AMD USB4 Firmware";
	case AMD_FW_S3IMG:
		snprintf(buffer, 49, "AMD S3 Image %u-%u",
			 entry->subprog, entry->inst);
		return buffer;
	default:
		snprintf(buffer, 29, "AMD PSP Firmware type 0x%02x", entry->type);
		return buffer;
	}
}


/* Return a human-readable software name for the given PSP BIOS firmware type. */
static const char *bios_fw_type_name(amd_bios_entry *entry)
{
	static char buffer[50];

	/*
	 * These are the only types which have a version and header.
	 * Microcode is processed separately.
	 */
	switch (entry->type) {
	case AMD_BIOS_PMUI:
		snprintf(buffer, 49, "AMD PSP PMU Code %u-%u",
			 entry->subpr, entry->inst);
		return buffer;
	case AMD_BIOS_PMUD:
		snprintf(buffer, 49, "AMD PSP PMU Data %u-%u",
			 entry->subpr, entry->inst);
		return buffer;
	default:
		snprintf(buffer, 49, "AMD PSP BIOS Firmware type 0x%02x", entry->type);
		return buffer;
	}
}

/*
 * Read the 4-byte version from the AMD firmware header at offset 0x60 and
 * format it as "MM.mm.pp.bb" (major.minor.patch.build in hex).
 */
static bool read_fw_version(const char *filename, sbom_fw_class_t class, char *buf,
			    size_t bufsize)
{
	struct amd_fw_header hdr;
	struct amd_fw_key *key_hdr;
	int fd;
	ssize_t n, offset;

	fd = open(filename, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "SBOM: cannot open %s: %s\n",
			filename, strerror(errno));
		return false;
	}
	n = read(fd, &hdr, sizeof(hdr));
	close(fd);

	if (n != (ssize_t)sizeof(hdr)) {
		fprintf(stderr, "SBOM: short read from %s\n", filename);
		return false;
	}

	if (class == FW_SBOM_PUBKEY) {
		key_hdr = (struct amd_fw_key *)&hdr;
		offset = snprintf(buf, bufsize, "Version: %x KeyID: ",
				  key_hdr->version);
		for (size_t i = 0; i < sizeof(key_hdr->key_id); i++) {
			offset += snprintf(&buf[offset], bufsize - offset,
					   "%02x", key_hdr->key_id[i]);
		}
		return true;
	}

	snprintf(buf, bufsize, "%02x.%02x.%02x.%02x",
		 hdr.version[3], hdr.version[2],
		 hdr.version[1], hdr.version[0]);
	return true;
}

/*
 * Read the entire file and return a newly allocated NUL-terminated hex string.
 * Used to represent wrapped-KEK content as the SBOM "software-version" field.
 * The caller must free() the returned pointer.
 * Returns NULL on error.
 */
static char *read_file_as_hex(const char *filename)
{
	FILE *f;
	long size;
	uint8_t *data;
	char *hex;
	size_t i, n;

	f = fopen(filename, "rb");
	if (!f) {
		fprintf(stderr, "SBOM: cannot open %s: %s\n",
			filename, strerror(errno));
		return NULL;
	}

	fseek(f, 0, SEEK_END);
	size = ftell(f);
	fseek(f, 0, SEEK_SET);

	/* Wrapped KEK blobs are small (typically 16-64 bytes); cap at 4 KiB */
	if (size <= 0 || size > 4096) {
		fprintf(stderr, "SBOM: unexpected size %ld for %s\n",
			size, filename);
		fclose(f);
		return NULL;
	}

	data = malloc((size_t)size);
	if (!data) {
		fclose(f);
		return NULL;
	}

	hex = malloc((size_t)size * 2 + 1);
	if (!hex) {
		free(data);
		fclose(f);
		return NULL;
	}

	n = fread(data, 1, (size_t)size, f);
	fclose(f);

	for (i = 0; i < n; i++)
		snprintf(hex + i * 2, 3, "%02x", data[i]);
	hex[n * 2] = '\0';

	free(data);
	return hex;
}

/*
 * Write a CoSWID SBOM JSON file.
 *
 * The emitted format matches the templates in src/sbom/ and is compatible
 * with the goswid tool used to assemble the final sbom.uswid.
 */
static bool write_sbom_json(const char *path, const char *tag_id,
			    const char *sw_name, const char *sw_version,
			    const char *persistent_id, const char *summary)
{
	FILE *f = fopen(path, "w");

	if (!f) {
		fprintf(stderr, "SBOM: cannot create %s: %s\n",
			path, strerror(errno));
		return false;
	}

	fprintf(f,
		"/* SPDX-License-Identifier: GPL-2.0-only */\n"
		"{\n"
		"  \"lang\": \"en-US\",\n"
		"  \"tag-id\": \"%s\",\n"
		"  \"tag-version\": 0,\n"
		"  \"software-name\": \"%s\",\n"
		"  \"software-version\": \"%s\",\n"
		"  \"version-scheme\": \"alphanumeric\",\n"
		"  \"software-meta\": [\n"
		"    {\n"
		"      \"persistent-id\": \"%s\",\n"
		"      \"summary\": \"%s\"\n"
		"    }\n"
		"  ],\n"
		"  \"entity\": [\n"
		"    {\n"
		"      \"entity-name\": \"coreboot\",\n"
		"      \"reg-id\": \"coreboot.org\",\n"
		"      \"role\": [\n"
		"        \"tagCreator\"\n"
		"      ]\n"
		"    }\n"
		"  ]\n"
		"}\n",
		tag_id, sw_name, sw_version, persistent_id, summary);

	fclose(f);
	return true;
}

/*
 * Generate a CoSWID SBOM JSON file for a single PSP firmware table entry.
 * Returns false on error; returns true (and does nothing) for entries that
 * should not appear in the SBOM.
 */
static bool generate_entry_sbom(const char *sbom_dir, amd_fw_entry *entry)
{
	char path[PATH_MAX];
	char version[80];
	char persistent_id[64];
	char *tmp_fn;
	const char *bname;
	const char *sw_name;
	const char *tag_id;
	const char *summary;
	sbom_fw_class_t fw_class;
	bool ok;

	if (!entry->filename)
		return true;

	fw_class = classify_fw_type(entry->type, entry->filename);
	if (fw_class == FW_SBOM_SKIP)
		return true;

	/* Derive the output filename from the input firmware basename */
	tmp_fn = strdup(entry->filename);
	if (!tmp_fn)
		return false;
	bname = basename(tmp_fn);

	if (snprintf(path, sizeof(path), "%s/amd-pspfw-%s.json",
		     sbom_dir, bname) >= (int)sizeof(path)) {
		fprintf(stderr, "SBOM: path too long for %s\n", entry->filename);
		free(tmp_fn);
		return false;
	}

	sw_name = psp_fw_type_name(entry);
	snprintf(persistent_id, sizeof(persistent_id),
		 "com.amd.pspfw.0x%02x", (uint8_t)entry->type);

	if (fw_class == FW_SBOM_WRAPPED_KEK) {
		char *hex = read_file_as_hex(entry->filename);

		if (!hex) {
			free(tmp_fn);
			return false;
		}
		tag_id = TAG_ID_WRAPPED_KEK;
		summary = "AMD Wrapped Key Encryption Key";
		ok = write_sbom_json(path, tag_id, sw_name, hex,
				     persistent_id, summary);
		free(hex);
		free(tmp_fn);
		return ok;
	}

	/* Regular or pubkey entry: extract version from AMD firmware header */
	if (!read_fw_version(entry->filename, fw_class, version, sizeof(version))) {
		free(tmp_fn);
		return false;
	}

	if (fw_class == FW_SBOM_PUBKEY) {
		tag_id  = TAG_ID_PSP_PUBKEY;
		summary = "AMD Platform Security Processor Public Key";
	} else {
		tag_id  = TAG_ID_PSP_FW;
		summary = "AMD Platform Security Processor Firmware";
	}

	ok = write_sbom_json(path, tag_id, sw_name, version,
			     persistent_id, summary);
	free(tmp_fn);
	return ok;
}

static bool generate_bios_entry_sbom(const char *sbom_dir, amd_bios_entry *entry)
{
	char path[PATH_MAX];
	char version[80];
	char persistent_id[64];
	char *tmp_fn;
	const char *bname;
	const char *sw_name;
	const char *tag_id;
	const char *summary;
	sbom_fw_class_t fw_class;
	bool ok;

	if (!entry->filename)
		return true;

	fw_class = classify_bios_fw_type(entry->type, entry->filename);
	if (fw_class == FW_SBOM_SKIP)
		return true;

	/* Derive the output filename from the input firmware basename */
	tmp_fn = strdup(entry->filename);
	if (!tmp_fn)
		return false;
	bname = basename(tmp_fn);

	if (snprintf(path, sizeof(path), "%s/amd-pspfw-%s.json",
		     sbom_dir, bname) >= (int)sizeof(path)) {
		fprintf(stderr, "SBOM: path too long for %s\n", entry->filename);
		free(tmp_fn);
		return false;
	}

	sw_name = bios_fw_type_name(entry);
	snprintf(persistent_id, sizeof(persistent_id),
		 "com.amd.pspbiosfw.0x%02x", (uint8_t)entry->type);

	/* Regular or pubkey entry: extract version from AMD firmware header */
	if (!read_fw_version(entry->filename, fw_class, version, sizeof(version))) {
		free(tmp_fn);
		return false;
	}

	if (fw_class == FW_SBOM_PUBKEY) {
		tag_id  = TAG_ID_PSP_PUBKEY;
		summary = "AMD Platform Security Processor Public Key";
	} else {
		tag_id  = TAG_ID_PSP_FW;
		summary = "AMD Platform Security Processor Firmware";
	}

	ok = write_sbom_json(path, tag_id, sw_name, version,
			     persistent_id, summary);
	free(tmp_fn);
	return ok;
}

/*
 * Generate CoSWID SBOM JSON files for all PSP firmware entries that have a
 * filename set (i.e. are included in the current build).
 *
 * Output files are written to sbom_dir as amd-pspfw-<filename>.json.
 */
void generate_sbom_psp(const char *sbom_dir, amd_fw_entry *fw_table)
{
	amd_fw_entry *entry;

	if (mkdir(sbom_dir, 0755) < 0 && errno != EEXIST) {
		fprintf(stderr, "SBOM: cannot create directory %s: %s\n",
			sbom_dir, strerror(errno));
		return;
	}

	for (entry = fw_table; entry->type != AMD_FW_INVALID; entry++) {
		if (!entry->filename)
			continue;

		if (classify_fw_type(entry->type, entry->filename)
				== FW_SBOM_SKIP)
			continue;

		generate_entry_sbom(sbom_dir, entry);
	}
}

/*
 * Generate CoSWID SBOM JSON files for all PSP firmware entries that have a
 * filename set (i.e. are included in the current build).
 *
 * Output files are written to sbom_dir as amd-pspfw-<filename>.json.
 */
void generate_sbom_bios(const char *sbom_dir, amd_bios_entry *fw_table)
{
	amd_bios_entry *entry;

	if (mkdir(sbom_dir, 0755) < 0 && errno != EEXIST) {
		fprintf(stderr, "SBOM: cannot create directory %s: %s\n",
			sbom_dir, strerror(errno));
		return;
	}

	for (entry = fw_table; entry->type != AMD_BIOS_INVALID; entry++) {
		if (!entry->filename)
			continue;

		if (classify_bios_fw_type(entry->type, entry->filename)
				== FW_SBOM_SKIP)
			continue;

		generate_bios_entry_sbom(sbom_dir, entry);
	}
}
