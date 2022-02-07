/* SPDX-License-Identifier: GPL-2.0-only */

#include <stdint.h>
#include <arch/cpu.h>
#include <console/console.h>
#include <cpu/amd/microcode.h>
#include <cpu/amd/msr.h>
#include <cbfs.h>

#define UCODE_DEBUG(fmt, args...)	\
	do { printk(BIOS_DEBUG, "[microcode] "fmt, ##args); } while (0)

#define UCODE_MAGIC			0x00414d44
#define UCODE_SECTION_START_ID		0x00000001

#define F15H_MPB_MAX_SIZE	4096
#define CONT_HDR		12

struct id_mapping {
	u32 orig_id;
	u32 new_id;
};

/*
 * STRUCTURE OF A MICROCODE (UCODE) FILE
 *	Container Header
 *	Section Header
 *	Microcode Header
 *	Microcode "Blob"
 *	Section Header
 *	Microcode Header
 *	Microcode "Blob"
 *	...
 *	...
 *	(end of file)
 *
 *
 * CONTAINER HEADER (offset 0 bytes from start of file)
 * Total size = fixed size (12 bytes) + variable size
 *	[0:3]  32-bit unique ID
 *	[4:7]  don't-care
 *	[8-11] Size (n) in bytes of variable portion of container header
 *	[12-n] don't-care
 *
 * SECTION HEADER (offset += 12+n)
 * Total size = 8 bytes
 *	[0:3] Unique identifier signaling start of section (0x00000001)
 *	[4:7] Total size (m) of following microcode section, including microcode header
 *
 * MICROCODE HEADER (offset += 8)
 * Total size = 64 bytes
 *	[0:3]	Data code		(32 bits)
 *	[4:7]	Patch ID		(32 bits)
 *	[8:9]	Microcode patch data ID (16 bits)
 *	[10]	c patch data length	(8  bits)
 *	[11]	init flag		(8 bits)
 *	[12:15]	ucode patch data cksum	(32 bits)
 *	[16:19]	nb dev ID		(32 bits)
 *	[20:23]	sb dev ID		(32 bits)
 *	[24:25]	Processor rev ID	(16 bits)
 *	[26]	nb revision ID		(8 bits)
 *	[27]	sb revision ID		(8 bits)
 *	[28]	BIOS API revision	(8 bits)
 *	[29-31]	Reserved 1 (array of three 8-bit values)
 *	[32-63]	Match reg (array of eight 32-bit values)
 *
 * MICROCODE BLOB (offset += 64)
 * Total size = m bytes
 *
 */

struct container_header {
	u32 unique_id;
	u32 reserved1;
	u32 header_size;
} __packed;

struct section_header {
	u32 start_id;
	u32 ucode_size;
} __packed;

struct microcode {
	u32 data_code;
	u32 patch_id;

	u16 mc_patch_data_id;
	u8 mc_patch_data_len;
	u8 init_flag;

	u32 mc_patch_data_checksum;

	u32 nb_dev_id;
	u32 sb_dev_id;

	u16 processor_rev_id;
	u8 nb_rev_id;
	u8 sb_rev_id;

	u8 bios_api_rev;
	u8 reserved1[3];

	u32 match_reg[8];

	u8 m_patch_data[896];
	u8 resv2[896];

	u8 x86_code_present;
	u8 x86_code_entry[191];
} __packed;

static u32 get_equivalent_processor_rev_id(u32 orig_id) {
	static const struct id_mapping id_mapping_table[] = {
		/* Family 10h */
		{ 0x100f00, 0x1000 },
		{ 0x100f01, 0x1000 },
		{ 0x100f02, 0x1000 },
		{ 0x100f20, 0x1020 },
		{ 0x100f21, 0x1020 }, /* DR-B1 */
		{ 0x100f2A, 0x1020 }, /* DR-BA */
		{ 0x100f22, 0x1022 }, /* DR-B2 */
		{ 0x100f23, 0x1022 }, /* DR-B3 */
		{ 0x100f42, 0x1041 }, /* RB-C2 */
		{ 0x100f43, 0x1043 }, /* RB-C3 */
		{ 0x100f52, 0x1041 }, /* BL-C2 */
		{ 0x100f62, 0x1062 }, /* DA-C2 */
		{ 0x100f63, 0x1043 }, /* DA-C3 */
		{ 0x100f81, 0x1081 }, /* HY-D1 */
		{ 0x100f91, 0x1081 }, /* HY-D1 */
		{ 0x100fa0, 0x10A0 }, /* PH-E0 */

		/* Family 15h */
		{ 0x600f12, 0x6012 }, /* OR-B2 */
		{ 0x600f20, 0x6020 }, /* OR-C0 */

		/* Array terminator */
		{ 0xffffff, 0x0000 },
	};

	u32 new_id;
	int i;

	new_id = 0;

	for (i = 0; id_mapping_table[i].orig_id != 0xffffff; i++) {
		if (id_mapping_table[i].orig_id == orig_id) {
			new_id = id_mapping_table[i].new_id;
			break;
		}
	}

	return new_id;
}

static void apply_microcode_patch(const struct microcode *m)
{
	u32 new_patch_id;
	msr_t msr;

	msr.hi = (u64)(uintptr_t)m >> 32;
	msr.lo = (uintptr_t)m & 0xffffffff;

	wrmsr(MSR_PATCH_LOADER, msr);

	printk(BIOS_DEBUG, "microcode: patch id to apply = 0x%08x\n", m->patch_id);

	msr = rdmsr(IA32_BIOS_SIGN_ID);
	new_patch_id = msr.lo;

	if (new_patch_id == m->patch_id)
		printk(BIOS_INFO, "microcode: being updated to patch id = 0x%08x succeeded\n",
			new_patch_id);
	else
		printk(BIOS_ERR, "microcode: being updated to patch id = 0x%08x failed\n",
			new_patch_id);
}

static void amd_update_microcode(const void *ucode,  size_t ucode_len,
				 u32 equivalent_processor_rev_id)
{
	const struct microcode *m;
	const struct container_header *c_hdr;
	const struct section_header *s_hdr;
	const u8 *ucode_end = (u8*)ucode + ucode_len;

	c_hdr = (const struct container_header *)ucode;

	/* Container Header */
	if (c_hdr->unique_id != UCODE_MAGIC) {
		UCODE_DEBUG("Invalid container header ID\n");
		return;
	}

	s_hdr = (const struct section_header *)
			((uintptr_t)ucode + CONT_HDR + c_hdr->header_size);

	/* Loop through sections */
	while (s_hdr->start_id == UCODE_SECTION_START_ID &&
		(uintptr_t)&s_hdr->ucode_size  <= (uintptr_t)(ucode_end - F15H_MPB_MAX_SIZE)) {

		m = (const struct microcode *)((uintptr_t)s_hdr + sizeof(*s_hdr));

		if (m->processor_rev_id == equivalent_processor_rev_id) {
			apply_microcode_patch(m);
			break;
		}

		s_hdr = (const struct section_header *)
				((uintptr_t)s_hdr + sizeof(*s_hdr) + s_hdr->ucode_size);
	}
}

static const char *microcode_cbfs_file[] = {
	"microcode_amd.bin",
	"microcode_amd_fam15h.bin",
};

void amd_update_microcode_from_cbfs(void)
{
	const void *ucode;
	size_t ucode_len;
	u32 i;
	u32 equivalent_processor_rev_id = get_equivalent_processor_rev_id(cpuid_eax(1));

	if (equivalent_processor_rev_id == 0) {
		UCODE_DEBUG("rev id not found. Skipping microcode patch!\n");
		return;
	}

	for (i = 0; i < ARRAY_SIZE(microcode_cbfs_file); i++) {
		ucode = cbfs_map(microcode_cbfs_file[i], &ucode_len);
		if (!ucode)
			continue;

		amd_update_microcode(ucode, ucode_len, equivalent_processor_rev_id);

	}
}
