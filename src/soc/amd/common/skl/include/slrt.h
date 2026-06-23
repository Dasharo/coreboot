/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef __SLRT_H__
#define __SLRT_H__

#include <types.h>

/* SLR Table header values */
#define SLR_TABLE_MAGIC         0x4452544d
#define SLR_TABLE_REVISION      1

/* Current revisions for the policy and UEFI config */
#define SLR_POLICY_REVISION     1
#define SLR_UEFI_CONFIG_REVISION 1

/* SLR defined architectures */
#define SLR_INTEL_TXT           1
#define SLR_AMD_SKINIT          2

/* SLR defined bootloaders */
#define SLR_BOOTLOADER_INVALID  0
#define SLR_BOOTLOADER_GRUB     1

/* Log formats */
#define SLR_DRTM_TPM12_LOG      1
#define SLR_DRTM_TPM20_LOG      2

/* Array Lengths */
#define TPM_EVENT_INFO_LENGTH   32

/* Tags */
#define SLR_ENTRY_INVALID       0x0000
#define SLR_ENTRY_DL_INFO       0x0001
#define SLR_ENTRY_LOG_INFO      0x0002
#define SLR_ENTRY_ENTRY_POLICY  0x0003
#define SLR_ENTRY_INTEL_INFO    0x0004
#define SLR_ENTRY_AMD_INFO      0x0005
#define SLR_ENTRY_ARM_INFO      0x0006
#define SLR_ENTRY_UEFI_INFO     0x0007
#define SLR_ENTRY_UEFI_CONFIG   0x0008
#define SLR_ENTRY_END           0xffff

/*
 * sl_header as given by AMD
 */
struct sl_header {
    u16 skl_entry_point;
    u16 skl_measured_size;
    u8 reserved[62];
    u16 skl_info_offset;
    u16 bootloader_data_offset;
} __packed;

/*
 * Common SLRT Table Header
 */
struct slr_entry_hdr {
    u32 tag;
    u32 size;
} __packed;

/*
 * Primary Secure Launch Resource Table Header
 */
struct slr_table {
    u32 magic;
    u16 revision;
    u16 architecture;
    u32 size;
    u32 max_size;
    /* Not really a flex array, don't use it that way! */
    struct slr_entry_hdr entries[];
} __packed;

/*
 * Boot loader context
 */
struct slr_bl_context {
    u16 bootloader;
    u16 reserved[3];
    u64 context;
} __packed;

/*
 * DRTM Dynamic Launch Configuration
 */
struct slr_entry_dl_info {
    struct slr_entry_hdr hdr;
    u64 dce_size;
    u64 dce_base;
    u64 dlme_size;
    u64 dlme_base;
    u64 dlme_entry; /* Offset from dlme_base */
    struct slr_bl_context bl_context;
    u64 dl_handler;
} __packed;

/*
 * TPM Log Information
 */
struct slr_entry_log_info {
    struct slr_entry_hdr hdr;
    u16 format;
    u16 reserved;
    u32 size;
    u64 addr;
} __packed;

/*
 * AMD SKINIT Info table
 */
struct slr_entry_amd_info {
    struct slr_entry_hdr hdr;
    u64 next;
    u32 type;
    u32 len;
    u64 slrt_size;
    u64 slrt_base;
    u64 boot_params_base;
    u16 psp_version;
    u16 reserved[3];
} __packed;

static inline void *next_entry(void* t)
{
    void *x = t + ((struct slr_entry_hdr*)t)->size;
    return x;
}

#endif /* __SLRT_H__ */
