/* SPDX-License-Identifier: GPL-2.0-only */

#include <stdint.h>
#include <string.h>
#include <rmodule.h>
#include <cpu/x86/smm.h>
#include <commonlib/helpers.h>
#include <console/console.h>
#include <security/intel/stm/SmmStm.h>

#define FXSAVE_SIZE 512
#define SMM_CODE_SEGMENT_SIZE 0x10000
/* FXSAVE area during relocation. While it may not be strictly needed the
   SMM stub code relies on the FXSAVE area being non-zero to enable SSE
   instructions within SMM mode. */
static uint8_t fxsave_area_relocation[CONFIG_MAX_CPUS][FXSAVE_SIZE]
__attribute__((aligned(16)));

/*
 * Components that make up the SMRAM:
 * 1. Save state - the total save state memory used
 * 2. Stack - stacks for the CPUs in the SMM handler
 * 3. Stub - SMM stub code for calling into handler
 * 4. Handler - C-based SMM handler.
 *
 * The components are assumed to consist of one consecutive region.
 */

/* These parameters are used by the SMM stub code. A pointer to the params
 * is also passed to the C-base handler. */
struct smm_stub_params {
	u32 stack_size;
	u32 stack_top;
	u32 c_handler;
	u32 c_handler_arg;
	u32 fxsave_area;
	u32 fxsave_area_size;
	struct smm_runtime runtime;
} __packed;

/*
 * The stub is the entry point that sets up protected mode and stacks for each
 * CPU. It then calls into the SMM handler module. It is encoded as an rmodule.
 */
extern unsigned char _binary_smmstub_start[];

/* Per CPU minimum stack size. */
#define SMM_MINIMUM_STACK_SIZE 32

struct cpu_smm_info {
	uint8_t active;
	uintptr_t smbase;
	uintptr_t entry;
	uintptr_t ss_start;
	uintptr_t code_start;
	uintptr_t code_end;
};
struct cpu_smm_info cpus[CONFIG_MAX_CPUS] = { 0 };

/*
 * This method creates a map of all the CPU entry points, save state locations
 * and the beginning and end of code segments for each CPU. This map is used
 * during relocation to properly align as many CPUs that can fit into the SMRAM
 * region. For more information on how SMRAM works, refer to the latest Intel
 * developer's manuals (volume 3, chapter 34). SMRAM is divided up into the
 * following regions:
 * +-----------------+ Top of SMRAM
 * |                 | <- MSEG, FXSAVE
 * +-----------------+
 * |    common       |
 * |  smi handler    | 64K
 * |                 |
 * +-----------------+
 * | CPU 0 code  seg |
 * +-----------------+
 * | CPU 1 code seg  |
 * +-----------------+
 * | CPU x code seg  |
 * +-----------------+
 * |                 |
 * |                 |
 * +-----------------+
 * |    stacks       |
 * +-----------------+ <- START of SMRAM
 *
 * The code below checks when a code segment is full and begins placing the remainder
 * CPUs in the lower segments. The entry point for each CPU is smbase + 0x8000
 * and save state is smbase + 0x8000 + (0x8000 - state save size). Save state
 * area grows downward into the CPUs entry point.  Therefore staggering too many
 * CPUs in one 32K block will corrupt CPU0's entry code as the save states move
 * downward.
 * input : smbase of first CPU (all other CPUs
 *         will go below this address)
 * input : num_cpus in the system. The map will
 *         be created from 0 to num_cpus.
 */
static int smm_create_map(uintptr_t smbase, unsigned int num_cpus,
			const struct smm_loader_params *params)
{
	unsigned int i;
	struct rmodule smm_stub;
	unsigned int ss_size = params->per_cpu_save_state_size, stub_size;
	unsigned int smm_entry_offset = params->smm_main_entry_offset;
	unsigned int seg_count = 0, segments = 0, available;
	unsigned int cpus_in_segment = 0;
	unsigned int base = smbase;

	if (rmodule_parse(&_binary_smmstub_start, &smm_stub)) {
		printk(BIOS_ERR, "%s: unable to get SMM module size\n", __func__);
		return 0;
	}

	stub_size = rmodule_memory_size(&smm_stub);
	/* How many CPUs can fit into one 64K segment? */
	available = 0xFFFF - smm_entry_offset - ss_size - stub_size;
	if (available > 0) {
		cpus_in_segment = available / ss_size;
		/* minimum segments needed will always be 1 */
		segments = num_cpus / cpus_in_segment + 1;
		printk(BIOS_DEBUG,
			"%s: cpus allowed in one segment %d\n", __func__, cpus_in_segment);
		printk(BIOS_DEBUG,
			"%s: min # of segments needed %d\n", __func__, segments);
	} else {
		printk(BIOS_ERR, "%s: not enough space in SMM to setup all CPUs\n", __func__);
		printk(BIOS_ERR, "    save state & stub size need to be reduced\n");
		printk(BIOS_ERR, "    or increase SMRAM size\n");
		return 0;
	}

	if (sizeof(cpus) / sizeof(struct cpu_smm_info) < num_cpus) {
		printk(BIOS_ERR,
			"%s: increase MAX_CPUS in Kconfig\n", __func__);
		return 0;
	}

	for (i = 0; i < num_cpus; i++) {
		cpus[i].smbase = base;
		cpus[i].entry = base + smm_entry_offset;
		cpus[i].ss_start = cpus[i].entry + (smm_entry_offset - ss_size);
		cpus[i].code_start = cpus[i].entry;
		cpus[i].code_end = cpus[i].entry + stub_size;
		cpus[i].active = 1;
		base -= ss_size;
		seg_count++;
		if (seg_count >= cpus_in_segment) {
			base -= smm_entry_offset;
			seg_count = 0;
		}
	}

	if (CONFIG_DEFAULT_CONSOLE_LOGLEVEL >= BIOS_DEBUG) {
		seg_count = 0;
		for (i = 0; i < num_cpus; i++) {
			printk(BIOS_DEBUG, "CPU 0x%x\n", i);
			printk(BIOS_DEBUG,
				"    smbase %zx  entry %zx\n",
				cpus[i].smbase, cpus[i].entry);
			printk(BIOS_DEBUG,
				"           ss_start %zx  code_end %zx\n",
				cpus[i].ss_start, cpus[i].code_end);
			seg_count++;
			if (seg_count >= cpus_in_segment) {
				printk(BIOS_DEBUG,
					"-------------NEW CODE SEGMENT --------------\n");
				seg_count = 0;
			}
		}
	}
	return 1;
}

/*
 * This method expects the smm relocation map to be complete.
 * This method does not read any HW registers, it simply uses a
 * map that was created during SMM setup.
 * input: cpu_num - cpu number which is used as an index into the
 *       map to return the smbase
 */
u32 smm_get_cpu_smbase(unsigned int cpu_num)
{
	if (cpu_num < CONFIG_MAX_CPUS) {
		if (cpus[cpu_num].active)
			return cpus[cpu_num].smbase;
	}
	return 0;
}

/*
 * This method assumes that at least 1 CPU has been set up from
 * which it will place other CPUs below its smbase ensuring that
 * save state does not clobber the first CPUs init code segment. The init
 * code which is the smm stub code is the same for all CPUs. They enter
 * smm, setup stacks (based on their apic id), enter protected mode
 * and then jump to the common smi handler.  The stack is allocated
 * at the beginning of smram (aka tseg base, not smbase). The stack
 * pointer for each CPU is calculated by using its apic id
 * (code is in smm_stub.s)
 * Each entry point will now have the same stub code which, sets up the CPU
 * stack, enters protected mode and then jumps to the smi handler. It is
 * important to enter protected mode before the jump because the "jump to
 * address" might be larger than the 20bit address supported by real mode.
 * SMI entry right now is in real mode.
 * input: smbase - this is the smbase of the first cpu not the smbase
 *        where tseg starts (aka smram_start). All CPUs code segment
 *        and stack will be below this point except for the common
 *        SMI handler which is one segment above
 * input: num_cpus - number of cpus that need relocation including
 *        the first CPU (though its code is already loaded)
 * input: top of stack (stacks work downward by default in Intel HW)
 * output: return -1, if runtime smi code could not be installed. In
 *         this case SMM will not work and any SMI's generated will
 *         cause a CPU shutdown or general protection fault because
 *         the appropriate smi handling code was not installed
 */

static int smm_place_entry_code(uintptr_t smbase, unsigned int num_cpus,
				unsigned int stack_top, const struct smm_loader_params *params)
{
	unsigned int i;
	unsigned int size;
	if (smm_create_map(smbase, num_cpus, params)) {
		/*
		 * Ensure there was enough space and the last CPUs smbase
		 * did not encroach upon the stack. Stack top is smram start
		 * + size of stack.
		 */
		if (cpus[num_cpus].active) {
			if (cpus[num_cpus - 1].smbase +
				params->smm_main_entry_offset < stack_top) {
				printk(BIOS_ERR, "%s: stack encroachment\n", __func__);
				printk(BIOS_ERR, "%s: smbase %zx, stack_top %x\n",
					__func__, cpus[num_cpus].smbase, stack_top);
				return 0;
			}
		}
	} else {
		printk(BIOS_ERR, "%s: unable to place smm entry code\n", __func__);
		return 0;
	}

	printk(BIOS_INFO, "%s: smbase %zx, stack_top %x\n",
		__func__, cpus[num_cpus-1].smbase, stack_top);

	/* start at 1, the first CPU stub code is already there */
	size = cpus[0].code_end - cpus[0].code_start;
	for (i = 1; i < num_cpus; i++) {
		memcpy((int *)cpus[i].code_start, (int *)cpus[0].code_start, size);
		printk(BIOS_DEBUG,
			"SMM Module: placing smm entry code at %zx,  cpu # 0x%x\n",
			cpus[i].code_start, i);
		printk(BIOS_DEBUG, "%s: copying from %zx to %zx 0x%x bytes\n",
			__func__, cpus[0].code_start, cpus[i].code_start, size);
	}
	return 1;
}

/*
 * Place stacks in base -> base + size region, but ensure the stacks don't
 * overlap the staggered entry points.
 */
static void *smm_stub_place_stacks(char *base, size_t size,
				   struct smm_loader_params *params)
{
	size_t total_stack_size;
	char *stacks_top;

	/* If stack space is requested assume the space lives in the lower
	 * half of SMRAM. */
	total_stack_size = params->per_cpu_stack_size *
			   params->num_concurrent_stacks;
	printk(BIOS_DEBUG, "%s: cpus: %zx : stack space: needed -> %zx\n",
		__func__, params->num_concurrent_stacks,
		total_stack_size);
	printk(BIOS_DEBUG, "  available -> %zx : per_cpu_stack_size : %zx\n",
		size, params->per_cpu_stack_size);

	/* There has to be at least one stack user. */
	if (params->num_concurrent_stacks < 1)
		return NULL;

	/* Total stack size cannot fit. */
	if (total_stack_size > size)
		return NULL;

	/* Stacks extend down to SMBASE */
	stacks_top = &base[total_stack_size];
	printk(BIOS_DEBUG, "%s: exit, stack_top %p\n", __func__, stacks_top);

	return stacks_top;
}

/*
 * Place the staggered entry points for each CPU. The entry points are
 * staggered by the per CPU SMM save state size extending down from
 * SMM_ENTRY_OFFSET.
 */
static int smm_stub_place_staggered_entry_points(char *base,
	const struct smm_loader_params *params, const struct rmodule *smm_stub)
{
	size_t stub_entry_offset;
	int rc = 1;
	stub_entry_offset = rmodule_entry_offset(smm_stub);
	/* Each CPU now has its own stub code, which enters protected mode,
	 * sets up the stack, and then jumps to common SMI handler
	 */
	if (params->num_concurrent_save_states > 1 || stub_entry_offset != 0) {
		rc = smm_place_entry_code((unsigned int)base,
			params->num_concurrent_save_states,
			(unsigned int)params->stack_top, params);
	}
	return rc;
}

/*
 * The stub setup code assumes it is completely contained within the
 * default SMRAM size (0x10000) for the default SMI handler (entry at
 * 0x30000), but no assumption should be made for the permanent SMI handler.
 * The placement of CPU entry points for permanent handler are determined
 * by the number of CPUs in the system and the amount of SMRAM.
 * There are potentially 3 regions to place
 * within the default SMRAM size:
 * 1. Save state areas
 * 2. Stub code
 * 3. Stack areas
 *
 * The save state and smm stack are treated as contiguous for the number of
 * concurrent areas requested. The save state always lives at the top of the
 * the CPUS smbase (and the entry point is at offset 0x8000). This allows only a certain
 * number of CPUs with staggered entry points until the save state area comes
 * down far enough to overwrite/corrupt the entry code (stub code). Therefore,
 * an SMM map is created to avoid this corruption, see smm_create_map() above.
 * This module setup code works for the default (0x30000) SMM handler setup and the
 * permanent SMM handler.
 */
static int smm_module_setup_stub(void *smbase, size_t smm_size,
				 struct smm_loader_params *params,
				 void *fxsave_area)
{
	size_t total_save_state_size;
	size_t smm_stub_size;
	size_t stub_entry_offset;
	char *smm_stub_loc;
	void *stacks_top;
	size_t size;
	char *base;
	size_t i;
	struct smm_stub_params *stub_params;
	struct rmodule smm_stub;
	unsigned int total_size_all;
	base = smbase;
	size = smm_size;

	/* The number of concurrent stacks cannot exceed CONFIG_MAX_CPUS. */
	if (params->num_concurrent_stacks > CONFIG_MAX_CPUS) {
		printk(BIOS_ERR, "%s: not enough stacks\n", __func__);
		return -1;
	}

	/* Fail if can't parse the smm stub rmodule. */
	if (rmodule_parse(&_binary_smmstub_start, &smm_stub)) {
		printk(BIOS_ERR, "%s: unable to parse smm stub\n", __func__);
		return -1;
	}

	/* Adjust remaining size to account for save state. */
	total_save_state_size = params->per_cpu_save_state_size *
				params->num_concurrent_save_states;
	if (total_save_state_size > size) {
		printk(BIOS_ERR,
			"%s: more state save space needed:need -> %zx:available->%zx\n",
			__func__, total_save_state_size, size);
		return -1;
	}

	size -= total_save_state_size;

	/* The save state size encroached over the first SMM entry point. */
	if (size <= params->smm_main_entry_offset) {
		printk(BIOS_ERR, "%s: encroachment over SMM entry point\n", __func__);
		printk(BIOS_ERR, "%s: state save size: %zx : smm_entry_offset -> %x\n",
			__func__, size, params->smm_main_entry_offset);
		return -1;
	}

	/* Need a minimum stack size and alignment. */
	if (params->per_cpu_stack_size <= SMM_MINIMUM_STACK_SIZE ||
	    (params->per_cpu_stack_size & 3) != 0) {
		printk(BIOS_ERR, "%s: need minimum stack size\n", __func__);
		return -1;
	}

	smm_stub_loc = NULL;
	smm_stub_size = rmodule_memory_size(&smm_stub);
	stub_entry_offset = rmodule_entry_offset(&smm_stub);

	/* Put the stub at the main entry point */
	smm_stub_loc = &base[params->smm_main_entry_offset];

	/* Stub is too big to fit. */
	if (smm_stub_size > (size - params->smm_main_entry_offset)) {
		printk(BIOS_ERR, "%s: stub is too big to fit\n", __func__);
		return -1;
	}

	/* The stacks, if requested, live in the lower half of SMRAM space
	 * for default handler, but for relocated handler it lives at the beginning
	 * of SMRAM which is TSEG base
	 */
	size = params->num_concurrent_stacks * params->per_cpu_stack_size;
	stacks_top = smm_stub_place_stacks((char *)params->smram_start, size, params);
	if (stacks_top == NULL) {
		printk(BIOS_ERR, "%s: not enough space for stacks\n", __func__);
		printk(BIOS_ERR, "%s: ....need -> %p : available -> %zx\n", __func__,
			base, size);
		return -1;
	}
	params->stack_top = stacks_top;
	/* Load the stub. */
	if (rmodule_load(smm_stub_loc, &smm_stub)) {
		printk(BIOS_ERR, "%s: load module failed\n", __func__);
		return -1;
	}

	if (!smm_stub_place_staggered_entry_points(base, params, &smm_stub)) {
		printk(BIOS_ERR, "%s: staggered entry points failed\n", __func__);
		return -1;
	}

	/* Setup the parameters for the stub code. */
	stub_params = rmodule_parameters(&smm_stub);
	stub_params->stack_top = (uintptr_t)stacks_top;
	stub_params->stack_size = params->per_cpu_stack_size;
	stub_params->c_handler = (uintptr_t)params->handler;
	stub_params->c_handler_arg = (uintptr_t)params->handler_arg;
	stub_params->fxsave_area = (uintptr_t)fxsave_area;
	stub_params->fxsave_area_size = FXSAVE_SIZE;
	stub_params->runtime.smbase = (uintptr_t)smbase;
	stub_params->runtime.smm_size = smm_size;
	stub_params->runtime.save_state_size = params->per_cpu_save_state_size;
	stub_params->runtime.num_cpus = params->num_concurrent_stacks;

	printk(BIOS_DEBUG, "%s: stack_end = 0x%x\n",
		__func__, stub_params->runtime.smbase);
	printk(BIOS_DEBUG,
		"%s: stack_top = 0x%x\n", __func__, stub_params->stack_top);
	printk(BIOS_DEBUG, "%s: stack_size = 0x%x\n",
		__func__, stub_params->stack_size);
	printk(BIOS_DEBUG, "%s: runtime.smbase = 0x%x\n",
		__func__, stub_params->runtime.smbase);
	printk(BIOS_DEBUG, "%s: runtime.start32_offset = 0x%x\n", __func__,
		stub_params->runtime.start32_offset);
	printk(BIOS_DEBUG, "%s: runtime.smm_size = 0x%zx\n",
		__func__, smm_size);
	printk(BIOS_DEBUG, "%s: per_cpu_save_state_size = 0x%x\n",
		__func__, stub_params->runtime.save_state_size);
	printk(BIOS_DEBUG, "%s: num_cpus = 0x%x\n", __func__,
		stub_params->runtime.num_cpus);
	printk(BIOS_DEBUG, "%s: total_save_state_size = 0x%x\n",
		__func__, (stub_params->runtime.save_state_size *
		stub_params->runtime.num_cpus));
	total_size_all = stub_params->stack_size +
		(stub_params->runtime.save_state_size *
		stub_params->runtime.num_cpus);
	printk(BIOS_DEBUG, "%s: total_size_all = 0x%x\n", __func__,
		total_size_all);

	/* Initialize the APIC id to CPU number table to be 1:1 */
	for (i = 0; i < params->num_concurrent_stacks; i++)
		stub_params->runtime.apic_id_to_cpu[i] = i;

	/* Allow the initiator to manipulate SMM stub parameters. */
	params->runtime = &stub_params->runtime;

	printk(BIOS_DEBUG, "SMM Module: stub loaded at %p. Will call %p(%p)\n",
	       smm_stub_loc, params->handler, params->handler_arg);
	return 0;
}

/*
 * smm_setup_relocation_handler assumes the callback is already loaded in
 * memory. i.e. Another SMM module isn't chained to the stub. The other
 * assumption is that the stub will be entered from the default SMRAM
 * location: 0x30000 -> 0x40000.
 */
int smm_setup_relocation_handler(struct smm_loader_params *params)
{
	void *smram = (void *)(SMM_DEFAULT_BASE);
	printk(BIOS_SPEW, "%s: enter\n", __func__);
	/* There can't be more than 1 concurrent save state for the relocation
	 * handler because all CPUs default to 0x30000 as SMBASE. */
	if (params->num_concurrent_save_states > 1)
		return -1;

	/* A handler has to be defined to call for relocation. */
	if (params->handler == NULL)
		return -1;

	/* Since the relocation handler always uses stack, adjust the number
	 * of concurrent stack users to be CONFIG_MAX_CPUS. */
	if (params->num_concurrent_stacks == 0)
		params->num_concurrent_stacks = CONFIG_MAX_CPUS;

	params->smm_main_entry_offset = SMM_ENTRY_OFFSET;
	params->smram_start = SMM_DEFAULT_BASE;
	params->smram_end = SMM_DEFAULT_BASE + SMM_DEFAULT_SIZE;
	return smm_module_setup_stub(smram, SMM_DEFAULT_SIZE,
				params, fxsave_area_relocation);
	printk(BIOS_SPEW, "%s: exit\n", __func__);
}

/*
 *The SMM module is placed within the provided region in the following
 * manner:
 * +-----------------+ <- smram + size
 * | BIOS resource   |
 * | list (STM)      |
 * +-----------------+
 * |  fxsave area    |
 * +-----------------+
 * |  smi handler    |
 * |      ...        |
 * +-----------------+ <- cpu0
 * |    stub code    | <- cpu1
 * |    stub code    | <- cpu2
 * |    stub code    | <- cpu3, etc
 * |                 |
 * |                 |
 * |                 |
 * |    stacks       |
 * +-----------------+ <- smram start

 * It should be noted that this algorithm will not work for
 * SMM_DEFAULT_SIZE SMRAM regions such as the A segment. This algorithm
 * expects a region large enough to encompass the handler and stacks
 * as well as the SMM_DEFAULT_SIZE.
 */
int smm_load_module(void *smram, size_t size, struct smm_loader_params *params)
{
	struct rmodule smm_mod;
	size_t total_stack_size;
	size_t handler_size;
	size_t module_alignment;
	size_t alignment_size;
	size_t fxsave_size;
	void *fxsave_area;
	size_t total_size = 0;
	char *base;

	if (size <= SMM_DEFAULT_SIZE)
		return -1;

	/* Load main SMI handler at the top of SMRAM
	 * everything else will go below
	 */
	base = smram;
	base += size;
	params->smram_start = (uintptr_t)smram;
	params->smram_end = params->smram_start + size;
	params->smm_main_entry_offset = SMM_ENTRY_OFFSET;

	/* Fail if can't parse the smm rmodule. */
	if (rmodule_parse(&_binary_smm_start, &smm_mod))
		return -1;

	/* Clear SMM region */
	if (CONFIG(DEBUG_SMI))
		memset(smram, 0xcd, size);

	total_stack_size = params->per_cpu_stack_size *
			   params->num_concurrent_stacks;
	total_size += total_stack_size;
	/* Stacks are the base of SMRAM */
	params->stack_top = smram + total_stack_size;

	/* MSEG starts at the top of SMRAM and works down */
	if (CONFIG(STM)) {
		base -= CONFIG_MSEG_SIZE + CONFIG_BIOS_RESOURCE_LIST_SIZE;
		total_size += CONFIG_MSEG_SIZE + CONFIG_BIOS_RESOURCE_LIST_SIZE;
	}

	/* FXSAVE goes below MSEG */
	if (CONFIG(SSE)) {
		fxsave_size = FXSAVE_SIZE * params->num_concurrent_stacks;
		fxsave_area = base - fxsave_size;
		base -= fxsave_size;
		total_size += fxsave_size;
	} else {
		fxsave_size = 0;
		fxsave_area = NULL;
	}

	handler_size = rmodule_memory_size(&smm_mod);
	base -= handler_size;
	total_size += handler_size;
	module_alignment = rmodule_load_alignment(&smm_mod);
	alignment_size = module_alignment -
				((uintptr_t)base % module_alignment);
	if (alignment_size != module_alignment) {
		handler_size += alignment_size;
		base += alignment_size;
	}

	printk(BIOS_DEBUG,
		"%s: total_smm_space_needed %zx, available -> %zx\n",
		 __func__, total_size, size);

	/* Does the required amount of memory exceed the SMRAM region size? */
	if (total_size > size) {
		printk(BIOS_ERR, "%s: need more SMRAM\n", __func__);
		return -1;
	}
	if (handler_size > SMM_CODE_SEGMENT_SIZE) {
		printk(BIOS_ERR, "%s: increase SMM_CODE_SEGMENT_SIZE: handler_size = %zx\n",
			__func__, handler_size);
		return -1;
	}

	if (rmodule_load(base, &smm_mod))
		return -1;

	params->handler = rmodule_entry(&smm_mod);
	params->handler_arg = rmodule_parameters(&smm_mod);

	printk(BIOS_DEBUG, "%s: smram_start: 0x%p\n",
		 __func__, smram);
	printk(BIOS_DEBUG, "%s: smram_end: %p\n",
		 __func__, smram + size);
	printk(BIOS_DEBUG, "%s: stack_top: %p\n",
		 __func__, params->stack_top);
	printk(BIOS_DEBUG, "%s: handler start %p\n",
		 __func__, params->handler);
	printk(BIOS_DEBUG, "%s: handler_size %zx\n",
		 __func__, handler_size);
	printk(BIOS_DEBUG, "%s: handler_arg %p\n",
		 __func__, params->handler_arg);
	printk(BIOS_DEBUG, "%s: fxsave_area %p\n",
		 __func__, fxsave_area);
	printk(BIOS_DEBUG, "%s: fxsave_size %zx\n",
		 __func__, fxsave_size);
	printk(BIOS_DEBUG, "%s: CONFIG_MSEG_SIZE 0x%x\n",
		 __func__, CONFIG_MSEG_SIZE);
	printk(BIOS_DEBUG, "%s: CONFIG_BIOS_RESOURCE_LIST_SIZE 0x%x\n",
		 __func__, CONFIG_BIOS_RESOURCE_LIST_SIZE);

	/* CPU 0 smbase goes first, all other CPUs
	 * will be staggered below
	 */
	base -= SMM_CODE_SEGMENT_SIZE;
	printk(BIOS_DEBUG, "%s: cpu0 entry: %p\n",
		 __func__, base);
	params->smm_entry = (uintptr_t)base + params->smm_main_entry_offset;
	return smm_module_setup_stub(base, size, params, fxsave_area);
}
