/* SPDX-License-Identifier: GPL-2.0-only */

#include <cbmem.h>
#include <commonlib/stdlib.h>		// xzalloc
#include <cpu/power/spr.h>
#include <device_tree.h>
#include <program_loading.h>

static bool add_mem_reserve_node(const struct range_entry *r, void *arg)
{
	struct device_tree *tree = arg;

	if (range_entry_tag(r) != BM_MEM_RAM) {
		struct device_tree_reserve_map_entry *entry = xzalloc(sizeof(*entry));
		entry->start = range_entry_base(r);
		entry->size = range_entry_size(r);

		printk(BIOS_WARNING, "Adding reserved memory: %llx - %llx\n", entry->start, entry->start + entry->size -1);

		list_insert_after(&entry->list_node, &tree->reserve_map);
	}

	return true;
}

static void add_mem_reserve_props(struct device_tree *tree)
{
	struct device_tree_reserve_map_entry *entry;
	uint64_t *range;
	char str[20];
	range = xzalloc(2 * sizeof(uint64_t));

	/* FIXME: make it add all entries properly, not just the last one */
	list_for_each(entry, tree->reserve_map, list_node) {
		range[0] = entry->start;
		range[1] = entry->size;
		snprintf(str, sizeof(str) - 1, "coreboot@%llx", entry->start);

		printk(BIOS_WARNING, "Adding %s: %llx ; %llx\n", str, range[0], range[1]);

		dt_add_string_prop(tree->root, "reserved-names", strdup(str));
		dt_add_bin_prop(tree->root, "reserved-ranges", range, 2 * sizeof(uint64_t));
	}
}

/* Add coreboot tables and CBMEM information to the device tree */
static void add_cb_fdt_data(struct device_tree *tree)
{
	u32 addr_cells = 1, size_cells = 1;
	u64 reg_addrs[2], reg_sizes[2];
	void *baseptr = NULL;
	size_t size = 0;

	static const char *firmware_path[] = {"firmware", NULL};
	struct device_tree_node *firmware_node = dt_find_node(tree->root,
		firmware_path, &addr_cells, &size_cells, 1);

	/* Need to add 'ranges' to the intermediate node to make 'reg' work. */
	dt_add_bin_prop(firmware_node, "ranges", NULL, 0);

	static const char *coreboot_path[] = {"coreboot", NULL};
	struct device_tree_node *coreboot_node = dt_find_node(firmware_node,
		coreboot_path, &addr_cells, &size_cells, 1);

	dt_add_string_prop(coreboot_node, "compatible", "coreboot");

	/* Fetch CB tables from cbmem */
	void *cbtable = cbmem_find(CBMEM_ID_CBTABLE);
	if (!cbtable) {
		printk(BIOS_WARNING, "FIT: No coreboot table found!\n");
		return;
	}

	/* First 'reg' address range is the coreboot table. */
	const struct lb_header *header = cbtable;
	reg_addrs[0] = (uintptr_t)header;
	reg_sizes[0] = header->header_bytes + header->table_bytes;

	/* Second is the CBMEM area (which usually includes the coreboot table). */
	cbmem_get_region(&baseptr, &size);
	if (!baseptr || size == 0) {
		printk(BIOS_WARNING, "FIT: CBMEM pointer/size not found!\n");
		return;
	}

	reg_addrs[1] = (uintptr_t)baseptr;
	reg_sizes[1] = size;

	dt_add_reg_prop(coreboot_node, reg_addrs, reg_sizes, 2, addr_cells, size_cells);
}

static void dt_assign_new_phandle(struct device_tree *tree,
                                  struct device_tree_node *node)
{
	struct device_tree_property *prop;
	uint32_t phandle;

	list_for_each(prop, node->properties, list_node) {
		if (!strcmp("phandle", prop->prop.name)) {
			/* Node already has phandle set, keep it */
			return;
		}
	}

	phandle = ++tree->max_phandle;
	node->phandle = phandle;
	dt_add_u32_prop(node, "phandle", phandle);
}

static void dt_fill_all_phandles(struct device_tree *tree,
                                 struct device_tree_node *node)
{
	struct device_tree_node *child;

	dt_assign_new_phandle(tree, node);

	list_for_each(child, node->children, list_node)
		dt_fill_all_phandles(tree, child);
}

/*
 * Payload's entry point is an offset to the real entry point, not to OPD
 * (Official Procedure Descriptor) for entry point.
 *
 * Also pass FDT address to payload stored in SPR_HSPRG0 by bootblock.
 */
void platform_prog_run(struct prog *prog)
{
	void *fdt;
	void *fdt_rom;
	struct device_tree *tree;

	asm volatile(
	    "mfspr %0, %1\n" /* pass pointer to FDT */
	    : "=r"(fdt_rom)
	    : "i"(SPR_HSPRG0)
	);

	tree = fdt_unflatten(fdt_rom);

	bootmem_walk_os_mem(add_mem_reserve_node, tree);
	add_mem_reserve_props(tree);
	add_cb_fdt_data(tree);
	dt_fill_all_phandles(tree, tree->root);

	fdt = malloc(dt_flat_size(tree));
	if (fdt == NULL)
		die("Unable to allocate memory for flat device tree\n");

	dt_flatten(tree, fdt);

	dt_print_node(tree->root);

	asm volatile(
	    "mr %%r27, %0\n" /* pass pointer to FDT */
	    "mtctr %2\n"
	    "mr 3, %1\n"
	    "bctr\n"
	    :: "r"(fdt), "r"(prog_entry_arg(prog)), "r"(prog_entry(prog))
	    : "memory");
}
