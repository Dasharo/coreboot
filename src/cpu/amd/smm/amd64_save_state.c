/* SPDX-License-Identifier: GPL-2.0-only */

#include <string.h>
#include <cpu/x86/save_state.h>
#include <cpu/amd/amd64_save_state.h>

static void *amd64_get_reg_base(const enum cpu_reg reg, const int node)
{
	const amd64_smm_state_save_area_t *save_state =
		(const amd64_smm_state_save_area_t *)smm_get_save_state(node);

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

static int amd64_get_set(const enum get_set op_type, const enum cpu_reg reg,
			    const int node, void *in_out, const uint8_t length)
{

	void *reg_base = amd64_get_reg_base(reg, node);

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

static int amd64_get_reg(const enum cpu_reg reg, const int node, void *out,
			 const uint8_t length)
{
	return amd64_get_set(GET, reg, node, out, length);
}

static int amd64_set_reg(const enum cpu_reg reg, const int node, void *in, const uint8_t length)
{
	return amd64_get_set(SET, reg, node, in, length);
}

/* bits in smm_io_trap   */
#define SMM_IO_TRAP_PORT_OFFSET		16
#define SMM_IO_TRAP_PORT_ADDRESS_MASK	0xffff
#define SMM_IO_TRAP_RW			(1 << 0)
#define SMM_IO_TRAP_VALID		(1 << 1)

static inline u16 get_io_address(u32 info)
{
	return ((info >> SMM_IO_TRAP_PORT_OFFSET) &
		SMM_IO_TRAP_PORT_ADDRESS_MASK);
}

static int amd64_apmc_node(u8 cmd)
{
	amd64_smm_state_save_area_t *state;
	u32 smm_io_trap;
	int node;
	u8 reg_al;

	for (node = 0; node < smm_max_cpus(); node++) {
		state = smm_get_save_state(node);
		smm_io_trap = state->smm_io_trap_offset;

		/* Check for Valid IO Trap Word (bit1==1) */
		if (!(smm_io_trap & SMM_IO_TRAP_VALID))
			continue;
		/* Make sure it was a write (bit0==0) */
		if (smm_io_trap & SMM_IO_TRAP_RW)
			continue;
		/* Check for APMC IO port. This tends to be configurable on AMD CPUs */
		if (pm_acpi_smi_cmd_port() != get_io_address(smm_io_trap))
			continue;
		/* Check AL against the requested command */
		reg_al = state->rax;
		if (reg_al == cmd)
			return node;
	}

	return -1;
}

static const uint32_t revisions[] = {
	0x00020064,
	0x00030064,
	SMM_REV_INVALID,
};

static const struct smm_save_state_ops ops = {
	.revision_table = revisions,
	.get_reg = amd64_get_reg,
	.set_reg = amd64_set_reg,
	.apmc_node = amd64_apmc_node,
};

const struct smm_save_state_ops *amd64_ops = &ops;
