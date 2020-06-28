/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __TPM_LOG_SERIALIZED_H__
#define __TPM_LOG_SERIALIZED_H__

#include <stdint.h>

#define MAX_TCPA_LOG_ENTRIES 50
#define TCPA_DIGEST_MAX_LENGTH 64
#define TCPA_PCR_HASH_NAME 50
#define TCPA_PCR_HASH_LEN 10
/* Assumption of 2K TCPA log size reserved for CAR/SRAM */
#define MAX_PRERAM_TCPA_LOG_ENTRIES 15

struct tcpa_entry {
	uint32_t pcr;
	char digest_type[TCPA_PCR_HASH_LEN];
	uint8_t digest[TCPA_DIGEST_MAX_LENGTH];
	uint32_t digest_length;
	char name[TCPA_PCR_HASH_NAME];
} __packed;

struct tcpa_table {
	uint16_t max_entries;
	uint16_t num_entries;
	struct tcpa_entry entries[0]; /* Variable number of entries */
} __packed;

#define TCPA_SPEC_ID_EVENT_SIGNATURE "Spec ID Event00"

struct tcpa_log_entry {
	uint32_t pcr;
	uint32_t event_type;
	uint8_t digest[20];
	uint32_t event_data_size;
	uint8_t event[0];
} __packed;

struct tcpa_spec_entry {
	struct tcpa_log_entry entry;
	uint8_t signature[16];
	uint32_t platform_class;
	uint8_t spec_version_minor;
	uint8_t spec_version_major;
	uint8_t spec_errata;
	uint8_t reserved;
	uint8_t vendor_info_size;
	uint8_t vendor_info[0];
} __packed;

/* Basic TPM2 types. */
typedef uint16_t TPM_SU;
typedef uint16_t TPM_ALG_ID;
typedef uint32_t TPM_HANDLE;
typedef uint32_t TPM_RC;
typedef uint8_t TPMI_YES_NO;
typedef TPM_ALG_ID TPMI_ALG_HASH;
typedef TPM_HANDLE TPMI_DH_PCR;
typedef TPM_HANDLE TPMI_RH_NV_INDEX;
typedef TPM_HANDLE TPMI_RH_ENABLES;
typedef TPM_HANDLE TPMI_SH_AUTH_SESSION;
typedef TPM_HANDLE TPM_RH;

/* Some hardcoded algorithm values. */
/* Table 7 - TPM_ALG_ID Constants */
#define TPM_ALG_ERROR   ((TPM_ALG_ID)0x0000)
#define TPM_ALG_HMAC    ((TPM_ALG_ID)0x0005)
#define TPM_ALG_NULL    ((TPM_ALG_ID)0x0010)
#define TPM_ALG_SHA1    ((TPM_ALG_ID)0x0004)
#define TPM_ALG_SHA256  ((TPM_ALG_ID)0x000b)
#define TPM_ALG_SHA384  ((TPM_ALG_ID)0x000C)
#define TPM_ALG_SHA512  ((TPM_ALG_ID)0x000D)
#define TPM_ALG_SM3_256 ((TPM_ALG_ID)0x0012)

/* Annex A Algorithm Constants */

/* Table 205 - Defines for SHA1 Hash Values */
#define SHA1_DIGEST_SIZE    20
/* Table 206 - Defines for SHA256 Hash Values */
#define SHA256_DIGEST_SIZE  32
/* Table 207 - Defines for SHA384 Hash Values */
#define SHA384_DIGEST_SIZE  48
/* Table 208 - Defines for SHA512 Hash Values */
#define SHA512_DIGEST_SIZE  64
/* Table 209 - Defines for SM3_256 Hash Values */
#define SM3_256_DIGEST_SIZE 32

#define HASH_COUNT 2

/* Table 66 - TPMU_HA Union */
typedef union {
	uint8_t sha1[SHA1_DIGEST_SIZE];
	uint8_t sha256[SHA256_DIGEST_SIZE];
	uint8_t sm3_256[SM3_256_DIGEST_SIZE];
	uint8_t sha384[SHA384_DIGEST_SIZE];
	uint8_t sha512[SHA512_DIGEST_SIZE];
} TPMU_HA;

typedef struct {
	TPMI_ALG_HASH  hashAlg;
	TPMU_HA        digest;
} TPMT_HA;

/* Table 96 -- TPML_DIGEST_VALUES Structure <I/O> */
typedef struct {
	uint32_t   count;
	TPMT_HA digests[HASH_COUNT];
} TPML_DIGEST_VALUES;

typedef struct {
	uint16_t alg_id;
	uint16_t digest_size;
} __packed TPM_DIGEST_SIZES;

typedef struct {
	uint32_t pcr_index;
	uint32_t event_type;
	TPML_DIGEST_VALUES digest;
	uint32_t event_size;
	uint8_t event[0];
} __packed TCG_PCR_EVENT2_HDR;

typedef struct {
	uint32_t pcr_index;
	uint32_t event_type;
	uint8_t digest[20];
	uint32_t event_size;
	uint8_t signature[16];
	uint32_t platform_class;
	uint8_t spec_version_minor;
	uint8_t spec_version_major;
	uint8_t spec_errata;
	uint8_t uintn_size;
	uint32_t num_of_algorithms;
	TPM_DIGEST_SIZES digest_sizes[HASH_COUNT];
	uint8_t vendor_info_size;
	uint8_t vendor_info[0];
} __packed TCG_EFI_SPEC_ID_EVENT;

#define TCG_EFI_SPEC_ID_EVENT_SIGNATURE "Spec ID Event03"

#define EV_PREBOOT_CERT			0x00000000
#define EV_POST_CODE			0x00000001
#define EV_UNUSED			0x00000002
#define EV_NO_ACTION			0x00000003
#define EV_SEPARATOR			0x00000004
#define EV_ACTION			0x00000005
#define EV_EVENT_TAG			0x00000006
#define EV_S_CRTM_CONTENTS		0x00000007
#define EV_S_CRTM_VERSION		0x00000008
#define EV_CPU_MICROCODE		0x00000009
#define EV_PLATFORM_CONFIG_FLAGS	0x0000000A
#define EV_TABLE_OF_DEVICES		0x0000000B
#define EV_COMPACT_HASH			0x0000000C
#define EV_IPL				0x0000000D
#define EV_IPL_PARTITION_DATA		0x0000000E
#define EV_NONHOST_CODE			0x0000000F
#define EV_NONHOST_CONFIG		0x00000010
#define EV_NONHOST_INFO			0x00000011
#define EV_OMIT_BOOT_DEVICE_EVENTS	0x00000012

static const char *tpm_event_types[] = {
	[EV_PREBOOT_CERT]		= "Reserved",
	[EV_POST_CODE]			= "POST code",
	[EV_UNUSED]			= "Unused",
	[EV_NO_ACTION]			= "No action",
	[EV_SEPARATOR]			= "Separator",
	[EV_ACTION]			= "Action",
	[EV_EVENT_TAG]			= "Event tag",
	[EV_S_CRTM_CONTENTS]		= "S-CRTM contents",
	[EV_S_CRTM_VERSION]		= "S-CRTM version",
	[EV_CPU_MICROCODE]		= "CPU microcode",
	[EV_PLATFORM_CONFIG_FLAGS]	= "Platform configuration flags",
	[EV_TABLE_OF_DEVICES]		= "Table of devices",
	[EV_COMPACT_HASH]		= "Compact hash",
	[EV_IPL]			= "IPL",
	[EV_IPL_PARTITION_DATA]		= "IPL partition data",
	[EV_NONHOST_CODE]		= "Non-host code",
	[EV_NONHOST_CONFIG]		= "Non-host configuration",
	[EV_NONHOST_INFO]		= "Non-host information",
	[EV_OMIT_BOOT_DEVICE_EVENTS]	= "Omit boot device events",
};

#endif
