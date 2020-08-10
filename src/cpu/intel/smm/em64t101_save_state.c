/* SPDX-License-Identifier: GPL-2.0-only */

#include <string.h>
#include <cpu/x86/smm.h>
#include <cpu/x86/save_state.h>
#include <cpu/intel/em64t101_save_state.h>

static void *em64t101_get_reg_base(const enum cpu_reg reg, const int node)
{
	const em64t101_smm_state_save_area_t *save_state =
		(const em64t101_smm_state_save_area_t *)smm_get_save_state(node);

	switch (reg) {
	case RAX:
		return (void *)&save_state->rax;
	case RBX:
		return (void *)&save_state->rbx;
	case RCX:
		return (void *)&save_state->rcx;
	case RDX:
		return (void *)&save_state->rdx;
	}

	return NULL;
}

enum get_set {
	GET,
	SET
};

static int em64t101_get_set(const enum get_set op_type, const enum cpu_reg reg,
			    const int node, void *in_out, const uint8_t length)
{

	void *reg_base = em64t101_get_reg_base(reg, node);

	if (!reg_base)
		return -1;

	switch (length) {
	case 1:
	case 2:
	case 4:
	case 8:
		switch (op_type) {
		case GET:
			memcpy(in_out, reg_base, length);
			return 0;
		case SET:
			memcpy(reg_base, in_out, length);
		}
	}

	return -1;
}

static int em64t101_get_reg(const enum cpu_reg reg, const int node, void *out,
			    const uint8_t length)
{
	return em64t101_get_set(GET, reg, node, out, length);
}

static int em64t101_set_reg(const enum cpu_reg reg, const int node, void *in,
			    const uint8_t length)
{
	return em64t101_get_set(SET, reg, node, in, length);
}

static int em64t101_apmc_node(u8 cmd)
{
	em64t101_smm_state_save_area_t *state;
	int node;

	for (node = 0; node < CONFIG_MAX_CPUS; node++) {
		state = smm_get_save_state(node);

		/* Check for Synchronous IO (bit0 == 1) */
		if (!(state->io_misc_info & (1 << 0)))
			continue;

		/* Make sure it was a write (bit4 == 0) */
		if (state->io_misc_info & (1 << 4))
			continue;

		/* Check for APMC IO port */
		if (((state->io_misc_info >> 16) & 0xff) != APM_CNT)
			continue;

		/* Check AL against the requested command */
		if ((state->rax & 0xff) != cmd)
			continue;

		return node;
	}

	return -1;
}

static const uint32_t revisions[] = {
	0x00030101,
	SMM_REV_INVALID,
};

static const struct smm_save_state_ops ops = {
	.revision_table = revisions,
	.get_reg = em64t101_get_reg,
	.set_reg = em64t101_set_reg,
	.apmc_node = em64t101_apmc_node,
};

const struct smm_save_state_ops *em64t101_ops = &ops;
