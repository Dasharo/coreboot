/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __SOC_IBM_POWER9_XIP_H
#define __SOC_IBM_POWER9_XIP_H

#define XIP_MAGIC_HW      (0x5849502020204857)  // "XIP   HW"
#define XIP_MAGIC_SGPE    (0x5849502053475045)  // "XIP SGPE"
#define XIP_MAGIC_RESTORE (0x5849502052455354)  // "XIP REST"

/* All fields are big-endian */

struct xip_section {
    uint32_t offset;
    uint32_t size;
    uint8_t alignment;
    uint8_t dd_support;
    uint8_t reserved8[2];
};

/* Each XIP header holds 15 XIP sections, some of them are sometimes unused. */
#define XIP_HEADER_COMMON_FIELDS_TOP		\
	uint64_t magic;				\
	uint64_t l1_addr;			\
	uint64_t l2_addr;			\
	uint64_t kernel_addr;			\
	uint64_t link_address;			\
	uint64_t reserved64[3];			\
	struct xip_section sections[5];

#define XIP_HEADER_COMMON_FIELDS_BOTTOM		\
	uint32_t image_size;			\
	/* In yyyymmdd format, e.g. 20110630, when read as decimal, not hex */ \
	uint32_t build_date;			\
	/* In hhmm format, e.g. 0756 */		\
	uint32_t build_time;			\
	char build_tag[20];			\
	uint8_t header_version;			\
	uint8_t normalized;			\
	uint8_t toc_sorted;			\
	uint8_t reserved8[5];			\
	char build_user[16];			\
	char build_host[40];			\
	char reserved_char[8];			\

struct xip_hw_header {
	XIP_HEADER_COMMON_FIELDS_TOP
	struct xip_section sgpe;
	struct xip_section restore;
	struct xip_section cme;
	struct xip_section pgpe;
	struct xip_section ioppe;
	struct xip_section fppe;
	struct xip_section rings;
	struct xip_section overlays;
	struct xip_section unused[2]; /* Pad to 15 sections. */
	XIP_HEADER_COMMON_FIELDS_BOTTOM
};

struct xip_sgpe_header {
	XIP_HEADER_COMMON_FIELDS_TOP
	struct xip_section qpmr;
	struct xip_section l1_bootloader;
	struct xip_section l2_bootloader;
	struct xip_section hcode;
	struct xip_section unused[6]; /* Pad to 15 sections. */
	XIP_HEADER_COMMON_FIELDS_BOTTOM
};

struct xip_restore_header {
	XIP_HEADER_COMMON_FIELDS_TOP
	struct xip_section cpmr;
	struct xip_section self;
	struct xip_section unused[8]; /* Pad to 15 sections. */
	XIP_HEADER_COMMON_FIELDS_BOTTOM
};

#define DD_CONTAINER_MAGIC 0x4444434F // "DDCO"

struct dd_block {
    uint32_t offset;
    uint32_t size;
    uint8_t  dd;
    uint8_t  reserved[3];
};

struct dd_container {
    uint32_t magic;
    uint8_t num;
    uint8_t reserved[3];
    struct dd_block blocks[0];
};


#endif /* __SOC_IBM_POWER9_XIP_H */
