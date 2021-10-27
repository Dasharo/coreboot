/*/* SPDX-License-Identifier: GPL-2.0-only */

#include <stdint.h>
#include <arch/cpu.h>
#include <console/console.h>
#include <cpu/amd/microcode.h>
#include <cpu/amd/msr.h>

#define UCODE_DEBUG(fmt, args...)	\
	do { printk(BIOS_DEBUG, "[microcode] "fmt, ##args); } while (0)

#define UCODE_MAGIC			0x00414d44
#define UCODE_SECTION_START_ID		0x00000001
#define UCODE_MAGIC			0x00414d44

#define F15H_MPB_MAX_SIZE	4096
#define CONT_HDR		12

struct id_mapping {
	uint32_t orig_id;
	uint16_t new_id;
};

static u16 get_equivalent_processor_rev_id(u32 orig_id) {
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
	uint32_t new_patch_id;
	msr_t msr;

	msr.hi = (uint64_t)(uintptr_t)m >> 32;
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

static uint16_t get_equivalent_processor_rev_id(void)
{
	uint32_t cpuid_family = cpuid_eax(1);

	return (uint16_t)((cpuid_family & 0xff0000) >> 8 | (cpuid_family & 0xff));
}

static void amd_update_microcode(const void *ucode,  size_t ucode_len,
				 uint32_t equivalent_processor_rev_id)
{
	const struct microcode *m;
	const uint8_t *c = ucode;
	const uint8_t *ucode_end = (uint8_t*)ucode + ucode_len;
	const uint8_t *cur_section_hdr;

	uint32_t container_hdr_id;
	uint32_t container_hdr_size;
	uint32_t blob_size;
	uint32_t sec_hdr_id;

	/* Container Header */
	container_hdr_id = read32(c);
	if (container_hdr_id != UCODE_MAGIC) {
		UCODE_DEBUG("Invalid container header ID\n");
		return;
	}

	container_hdr_size = read32(c + 8);
	cur_section_hdr = c + CONT_HDR + container_hdr_size;

	/* Read in first section header ID */
	sec_hdr_id = read32(cur_section_hdr);
	c = cur_section_hdr + 4;

	/* Loop through sections */
	while (sec_hdr_id == UCODE_SECTION_START_ID &&
		c <= (ucode_end - F15H_MPB_MAX_SIZE)) {

		blob_size = read32(c);

		m = (struct microcode *)(c + 4);

		if (m->processor_rev_id == equivalent_processor_rev_id) {
			apply_microcode_patch(m);
			break;
		}

		cur_section_hdr = c + 4 + blob_size;
		sec_hdr_id = read32(cur_section_hdr);
		c = cur_section_hdr + 4;
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
	uint16_t equivalent_processor_rev_id = get_equivalent_processor_rev_id();

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

void update_microcode(u32 cpu_deviceid)
{
	u32 equivalent_processor_rev_id = get_equivalent_processor_rev_id(cpu_deviceid);
	amd_update_microcode_from_cbfs(equivalent_processor_rev_id);
}
