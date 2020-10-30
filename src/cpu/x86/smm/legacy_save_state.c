/* SPDX-License-Identifier: GPL-2.0-only */

#include <string.h>
#include <cpu/x86/smm.h>
#include <cpu/x86/save_state.h>
#include <cpu/x86/legacy_save_state.h>

static void *legacy_get_reg_base(const enum cpu_reg reg, const int node)
{
	const legacy_smm_state_save_area_t *save_state =
		(const legacy_smm_state_save_area_t *)smm_get_save_state(node);

	switch (reg) {
	case RAX:
		return (void *)&save_state->eax;
	case RBX:
		return (void *)&save_state->ebx;
	case RCX:
		return (void *)&save_state->ecx;
	case RDX:
		return (void *)&save_state->edx;
	}

	return NULL;
}

enum get_set {
	GET,
	SET
};

static int legacy_get_set(const enum get_set op_type, const enum cpu_reg reg,
			    const int node, void *in_out, const uint8_t length)
{

	void *reg_base = legacy_get_reg_base(reg, node);

	if (!reg_base)
		return -1;

	switch (length) {
	case 1:
	case 2:
	case 4:
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

static int legacy_get_reg(const enum cpu_reg reg, const int node, void *out,
			  const uint8_t length)
{
	return legacy_get_set(GET, reg, node, out, length);
}

static int legacy_set_reg(const enum cpu_reg reg, const int node, void *in,
			  const uint8_t length)
{
	return legacy_get_set(SET, reg, node, in, length);
}

static int legacy_apmc_node(u8 cmd)
{
	legacy_smm_state_save_area_t *state;
	int node;

	for (node = 0; node < smm_max_cpus(); node++) {
		state = smm_get_save_state(node);

		/* Check AL against the requested command */
		if ((state->eax & 0xff) != cmd)
			continue;

		return node;
	}

	return -1;
}

static const uint32_t revisions[] = {
	0x00030002,
	0x00030007,
	SMM_REV_INVALID,
};

static const struct smm_save_state_ops ops = {
	.revision_table = revisions,
	.get_reg = legacy_get_reg,
	.set_reg = legacy_set_reg,
	.apmc_node = legacy_apmc_node,
};

const struct smm_save_state_ops *legacy_ops = &ops;
