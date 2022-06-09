/* SPDX-License-Identifier: GPL-2.0-only */

#include <assert.h>
#include <cbmem.h>
#include <commonlib/cbmem_id.h>
#include <security/tpm/tspi.h>
#include <drivers/ipmi/ipmi_bt.h>
#include <program_loading.h>
#include <cbfs.h>
#include <device_tree.h>
#include <cpu/power/istep_13.h>
#include <cpu/power/istep_14.h>
#include <cpu/power/istep_18.h>
#include <cpu/power/proc.h>
#include <cpu/power/spr.h>
#include <commonlib/stdlib.h>		// xzalloc
#include <security/tpm/tis.h>

#include "istep_13_scom.h"
#include "chip.h"
#include "fsi.h"
#include "pci.h"

/*
 * Chip ID, not sure why it's shifted (group id + chip id?).
 * Might be specific to group pump mode.
 */
#define CHIP_ID(chip) ((chip) << 3)

/* Copy of data put together by the romstage */
mcbist_data_t mem_data[MAX_CHIPS];

#define SIZE_MASK	PPC_BITMASK(13,23)
#define SIZE_SHIFT	(63 - 23)
#define BASE_MASK	PPC_BITMASK(24,47)
#define BASE_SHIFT	(63 - 47)

/* Values in registers are in 4GB units, ram_resource() expects kilobytes. */
#define CONVERT_4GB_TO_KB(x)	((x) << 22)

static inline unsigned long base_k(uint64_t reg)
{
	return CONVERT_4GB_TO_KB((reg & BASE_MASK) >> BASE_SHIFT);
}

static inline unsigned long size_k(uint64_t reg)
{
	return CONVERT_4GB_TO_KB(((reg & SIZE_MASK) >> SIZE_SHIFT) + 1);
}

static uint64_t nominal_freq[MAX_CHIPS];

/*
 * These are various definitions of the page sizes and segment sizes supported
 * by the MMU. Values are the same as dumped from original firmware, comments
 * are copied from Hostboot for POWER8. Compared to POWER8, POWER9 doesn't have
 * 1M entries in segment page sizes.
 */
static uint32_t page_sizes[4] = { 0xc, 0x10, 0x18, 0x22 };
static uint32_t segment_sizes[4] = { 0x1c, 0x28, 0xffffffff, 0xffffffff };
static uint32_t segment_page_sizes[] =
{
	12, 0x0, 3,   /* 4k SLB page size, L,LP = 0,x1, 3 page size encodings */
	12, 0x0,      /* 4K PTE page size, L,LP = 0,x0 */
	16, 0x7,      /* 64K PTE page size, L,LP = 1,x7 */
	24, 0x38,     /* 16M PTE page size, L,LP = 1,x38 */
	16, 0x110, 2, /* 64K SLB page size, L,LP = 1,x1, 2 page size encodings*/
	16, 0x1,      /* 64K PTE page size, L,LP = 1,x1 */
	24, 0x8,      /* 16M PTE page size, L,LP = 1,x8 */
	24, 0x100, 1, /* 16M SLB page size, L,LP = 1,x0, 1 page size encoding */
	24, 0x0,      /* 16M PTE page size, L,LP = 1,x0 */
	34, 0x120, 1, /* 16G SLB page size, L,LP = 1,x2, 1 page size encoding */
	34, 0x3       /* 16G PTE page size, L,LP = 1,x3 */
};
static uint32_t radix_AP_enc[4] = { 0x0c, 0xa0000010, 0x20000015, 0x4000001e };

/*
 * Dumped from Hostboot, might need reviewing. Comment in
 * skiboot/external/mambo/skiboot.tcl says that PAPR defines up to byte 63 (plus
 * 2 bytes for header), but the newest version I found describes only up to byte
 * number 23 (Revision 2.9_pre7 from June 11, 2020).
 */
static uint8_t pa_features[] =
{
	64, 0, /* Header: size and format, respectively */
	0xf6, 0x3f, 0xc7, 0xc0, 0x80, 0xd0, 0x80, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x00, 0x00,
	0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x00, 0x00,
	0x80, 0x00, 0x80, 0x00, 0x00, 0x00, 0x80, 0x00,
	0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00,
	0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00,
	0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00
};

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

static void fill_l3_node(struct device_tree *tree,
                         struct device_tree_node *node, uint32_t pir)
{
	dt_assign_new_phandle(tree, node);
	dt_add_u32_prop(node, "reg", pir);
	dt_add_string_prop(node, "device_type", "cache");
	dt_add_bin_prop(node, "cache-unified", NULL, 0);
	dt_add_string_prop(node, "status", "okay");

	/* POWER9 Processor User's Manual, 7.3 */
	dt_add_u32_prop(node, "d-cache-size", 10 * MiB);
	dt_add_u32_prop(node, "d-cache-sets", 8);  /* Per Hostboot. Why not 20? */
	dt_add_u32_prop(node, "i-cache-size", 10 * MiB);
	dt_add_u32_prop(node, "i-cache-sets", 8);  /* Per Hostboot. Why not 20? */
}

static void fill_l2_node(struct device_tree *tree,
                         struct device_tree_node *node, uint32_t pir,
                         uint32_t next_lvl_phandle)
{
	dt_assign_new_phandle(tree, node);
	/* This is not a typo, "l2-cache" points to the node of L3 cache */
	dt_add_u32_prop(node, "l2-cache", next_lvl_phandle);
	dt_add_u32_prop(node, "reg", pir);
	dt_add_string_prop(node, "device_type", "cache");
	dt_add_bin_prop(node, "cache-unified", NULL, 0);
	dt_add_string_prop(node, "status", "okay");

	/* POWER9 Processor User's Manual, 6.1 */
	dt_add_u32_prop(node, "d-cache-size", 512 * KiB);
	dt_add_u32_prop(node, "d-cache-sets", 8);
	dt_add_u32_prop(node, "i-cache-size", 512 * KiB);
	dt_add_u32_prop(node, "i-cache-sets", 8);
}

static void fill_cpu_node(struct device_tree *tree,
                          struct device_tree_node *node, uint8_t chip, uint32_t pir,
                          uint32_t next_lvl_phandle)
{
	/* Mandatory/standard properties */
	dt_assign_new_phandle(tree, node);
	dt_add_string_prop(node, "device_type", "cpu");
	dt_add_bin_prop(node, "64-bit", NULL, 0);
	dt_add_bin_prop(node, "32-64-bridge", NULL, 0);
	dt_add_bin_prop(node, "graphics", NULL, 0);
	dt_add_bin_prop(node, "general-purpose", NULL, 0);
	dt_add_u32_prop(node, "l2-cache", next_lvl_phandle);

	/*
	 * The "status" property indicate whether the core is functional. It's
	 * a string containing "okay" for a good core or "bad" for a non-functional
	 * one. You can also just omit the non-functional ones from the DT
	 */
	dt_add_string_prop(node, "status", "okay");

	/*
	 * This is the same value as the PIR of thread 0 of that core
	 * (ie same as the @xx part of the node name)
	 */
	dt_add_u32_prop(node, "reg", pir);
	dt_add_u32_prop(node, "ibm,pir", pir);

	/* Chip ID of this core */
	dt_add_u32_prop(node, "ibm,chip-id", CHIP_ID(chip));

	/*
	 * Interrupt server numbers (aka HW processor numbers) of all threads
	 * on that core. This should have 4 numbers and the first one should
	 * have the same value as the above ibm,pir and reg properties
	 */
	uint32_t int_srvrs[4] = {pir, pir+1, pir+2, pir+3};
	/*
	 * This will be added to actual FDT later, so local array on stack can't
	 * be used.
	 */
	void *int_srvrs_ptr = xmalloc(sizeof(int_srvrs));
	memcpy(int_srvrs_ptr, int_srvrs, sizeof(int_srvrs));
	dt_add_bin_prop(node, "ibm,ppc-interrupt-server#s", int_srvrs_ptr,
	                sizeof(int_srvrs));

	/*
	 * This is the "architected processor version" as defined in PAPR.
	 */
	dt_add_u32_prop(node, "cpu-version", read_spr(SPR_PVR));

	/*
	 * Page sizes and segment sizes supported by the MMU.
	 */
	dt_add_bin_prop(node, "ibm,processor-page-sizes", &page_sizes,
	                sizeof(page_sizes));
	dt_add_bin_prop(node, "ibm,processor-segment-sizes", &segment_sizes,
	                sizeof(segment_sizes));
	dt_add_bin_prop(node, "ibm,segment-page-sizes", &segment_page_sizes,
	                sizeof(segment_page_sizes));
	dt_add_bin_prop(node, "ibm,processor-radix-AP-encodings", &radix_AP_enc,
	                sizeof(radix_AP_enc));

	dt_add_bin_prop(node, "ibm,pa-features", &pa_features,
	                sizeof(pa_features));

	/* SLB size, use as-is */
	dt_add_u32_prop(node, "ibm,slb-size", 0x20);

	/* VSX support, use as-is */
	dt_add_u32_prop(node, "ibm,vmx", 0x2);

	/* DFP support, use as-is */
	dt_add_u32_prop(node, "ibm,dfp", 0x2);

	/* PURR/SPURR support, use as-is */
	dt_add_u32_prop(node, "ibm,purr", 0x1);
	dt_add_u32_prop(node, "ibm,spurr", 0x1);

	/*
	 * Old-style core clock frequency. Only create this property if the
	 * frequency fits in a 32-bit number. Do not create it if it doesn't.
	 */
	if ((nominal_freq[chip] >> 32) == 0)
		dt_add_u32_prop(node, "clock-frequency", nominal_freq[chip]);

	/*
	 * Mandatory: 64-bit version of the core clock frequency, always create
	 * this property.
	 */
	dt_add_u64_prop(node, "ibm,extended-clock-frequency", nominal_freq[chip]);

	/* Timebase freq has a fixed value, always use that */
	dt_add_u32_prop(node, "timebase-frequency", 512 * MHz);
	/* extended-timebase-frequency will be deprecated at some point */
	dt_add_u64_prop(node, "ibm,extended-timebase-frequency", 512 * MHz);

	/* Use as-is, values dumped from booted system */
	dt_add_u32_prop(node, "reservation-granule-size", 0x80);
	dt_add_u64_prop(node, "performance-monitor", 1);
	/* POWER9 Processor User's Manual, 2.3.1 */
	dt_add_u32_prop(node, "i-cache-size", 32 * KiB);
	dt_add_u32_prop(node, "i-cache-sets", 8);
	dt_add_u32_prop(node, "i-cache-block-size", 128);
	dt_add_u32_prop(node, "i-cache-line-size", 128);	// Makes Linux happier
	dt_add_u32_prop(node, "i-tlb-size", 0);
	dt_add_u32_prop(node, "i-tlb-sets", 0);
	/* POWER9 Processor User's Manual, 2.3.5 */
	dt_add_u32_prop(node, "d-cache-size", 32 * KiB);
	dt_add_u32_prop(node, "d-cache-sets", 8);
	dt_add_u32_prop(node, "d-cache-block-size", 128);
	dt_add_u32_prop(node, "d-cache-line-size", 128);	// Makes Linux happier
	/* POWER9 Processor User's Manual, 2.3.7 */
	dt_add_u32_prop(node, "d-tlb-size", 1024);
	dt_add_u32_prop(node, "d-tlb-sets", 4);
	dt_add_u32_prop(node, "tlb-size", 1024);
	dt_add_u32_prop(node, "tlb-sets", 4);
}

static void add_memory_node(struct device_tree *tree, uint8_t chip, uint64_t reg)
{
	struct device_tree_node *node;
	/* /memory@0123456789abcdef - 24 characters + null byte */
	char path[26] = {};

	union {uint32_t u32[2]; uint64_t u64;} addr = { .u64 = base_k(reg) * KiB };
	union {uint32_t u32[2]; uint64_t u64;} size = { .u64 = size_k(reg) * KiB };

	snprintf(path, sizeof(path), "/memory@%llx", addr.u64);
	node = dt_find_node_by_path(tree, path, NULL, NULL, 1);

	dt_add_string_prop(node, "device_type", (char *)"memory");
	/* Use 2 cells each for address and size. This assumes BE. */
	dt_add_reg_prop(node, &addr.u64, &size.u64, 1, 2, 2);

	/* Don't know why the value needs to be shifted (group id + chip id?) */
	dt_add_u32_prop(node, "ibm,chip-id", chip << 3);
}

static bool add_mem_reserve_node(const struct range_entry *r, void *arg)
{
	struct device_tree *tree = arg;

	if (range_entry_tag(r) != BM_MEM_RAM) {
		struct device_tree_reserve_map_entry *entry = xzalloc(sizeof(*entry));
		entry->start = range_entry_base(r);
		entry->size = range_entry_size(r);

		list_insert_after(&entry->list_node, &tree->reserve_map);
	}

	return true;
}

static void add_memory_nodes(struct device_tree *tree)
{
	uint8_t chip;
	uint8_t chips = fsi_get_present_chips();

	/*
	 * Not using bootmem_walk_os_mem() to be consistent with Hostboot,
	 * whose "memory" nodes include reserved regions.
	 */
	for (chip = 0; chip < MAX_CHIPS; chip++) {
		int mcs_i;

		if (!(chips & (1 << chip)))
			continue;

		for (mcs_i = 0; mcs_i < MCS_PER_PROC; mcs_i++) {
			uint64_t reg;
			chiplet_id_t nest = mcs_to_nest[mcs_ids[mcs_i]];

			/* These registers are undocumented, see istep 14.5. */
			/* MCS_MCFGP */
			reg = read_scom_for_chiplet(chip, nest, 0x0501080A);
			if (reg & PPC_BIT(0))
				add_memory_node(tree, chip, reg);

			/* MCS_MCFGPM */
			reg = read_scom_for_chiplet(chip, nest, 0x0501080C);
			if (reg & PPC_BIT(0))
				add_memory_node(tree, chip, reg);
		}
	}

	bootmem_walk_os_mem(add_mem_reserve_node, tree);
}

/* Finds first root complex for a given chip that's present in DT else returns NULL */
static bool dt_contains_pcie(struct device_tree *tree, uint8_t chip_id)
{
	int phb;

	/* See comment before pec0_lane_cfg global variable in istep_10_10.c */
	for (phb = 0; phb < MAX_PHB_PER_PROC; phb++) {
		struct device_tree_node *node;

		char path[40];
		snprintf(path, sizeof(path), "/ibm,pcie-slots/root-complex@%d,%d", chip_id,
			 phb);

		node = dt_find_node_by_path(tree, path, NULL, NULL, 0);
		if (node != NULL)
			return true;
	}

	return false;
}

/* Checks input device tree for sanity and dies on failure */
static void validate_dt(struct device_tree *tree, uint8_t chips)
{
	struct device_tree_node *xscom;
	bool found_pcie;

	/* TODO: is the address always the same? */
	xscom = dt_find_node_by_path(tree, "/xscom@603fc00000000", NULL, NULL, 0);
	if (xscom == NULL)
		die("No 'xscom' node for chip#0 in device tree!\n");

	/* Check for xscom node of the second CPU (assuming group pump mode) */
	xscom = dt_find_node_by_path(tree, "/xscom@623fc00000000", NULL, NULL, 0);
	if (chips & 0x2) {
		if (xscom == NULL)
			die("No 'xscom' node for chip#1 in device tree!\n");
	} else {
		if (xscom != NULL)
			die("Found 'xscom' node for missing chip#1 in device tree!\n");
	}

	if (!dt_contains_pcie(tree, /*chip_id=*/0))
		die("No 'root-complex' nodes for chip#0 in device tree!\n");

	/* Check for xscom node of the second CPU (assuming group pump mode) */
	found_pcie = dt_contains_pcie(tree, /*chip_id=*/CHIP_ID(1));
	if (chips & 0x2) {
		if (!found_pcie)
			die("No 'root-complex' node for chip#1 in device tree!\n");
	} else {
		if (found_pcie)
			die("Found 'root-complex' node for missing chip#1 in device tree!\n");
	}
}

/* Mind that this function creates nodes without chip ID, but some types of sensors need it */
static void add_sensor_node(struct device_tree *tree, uint8_t number, uint8_t ipmi_type)
{
	char path[32];
	struct device_tree_node *node;

	snprintf(path, sizeof(path), "/bmc/sensors/sensor@%x", number);
	node = dt_find_node_by_path(tree, path, NULL, NULL, 1);

	dt_add_string_prop(node, "compatible", "ibm,ipmi-sensor");
	dt_add_u32_prop(node, "reg", number);
	dt_add_u32_prop(node, "ipmi-sensor-type", ipmi_type);
}

static void add_dimm_sensor_nodes(struct device_tree *tree, uint8_t chips)
{
	enum {
		/* Base numbers for sensor ids */
		DIMM_STATE_BASE = 0x0B,
		DIMM_TEMP_BASE = 0x1B,

		/* IPMI sensor types */
		STATE_IPMI_SENSOR = 0x0C,
		TEMP_IPMI_SENSOR = 0x01,
	};

	int dimm_i;
	for (dimm_i = 0; dimm_i < MAX_CHIPS * DIMMS_PER_PROC; dimm_i++) {
		int chip = dimm_i / DIMMS_PER_PROC;
		int mcs = (dimm_i % DIMMS_PER_PROC) / DIMMS_PER_MCS;
		int mca = (dimm_i % DIMMS_PER_MCS) / DIMMS_PER_MCA;
		int dimm = dimm_i % DIMMS_PER_MCA;

		if (!(chips & (1 << chip)))
			continue;
		if (!mem_data[chip].mcs[mcs].mca[mca].dimm[dimm].present)
			continue;

		add_sensor_node(tree, DIMM_STATE_BASE + dimm_i, STATE_IPMI_SENSOR);
		add_sensor_node(tree, DIMM_TEMP_BASE + dimm_i, TEMP_IPMI_SENSOR);
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

static void add_tpm_node(struct device_tree *tree)
{
#if CONFIG(TALOS_2_INFINEON_TPM_1)
	uint32_t xscom_base = 0xA0000 | (CONFIG_DRIVER_TPM_I2C_BUS << 12);
	uint8_t port = (CONFIG_DRIVER_TPM_I2C_ADDR & 0x80 ? 1 : 0);
	uint8_t addr = (CONFIG_DRIVER_TPM_I2C_ADDR & 0x7F);

	struct device_tree_node *tpm;
	char path[64];
	char compatible[24];

	struct tcpa_table *tcpa_table;

	/* TODO: is the XSCOM address always the same? */
	snprintf(path, sizeof(path), "/xscom@603fc00000000/i2cm@%x/i2c-bus@%x/tpm@%x",
		 xscom_base, port, addr);

	tpm = dt_find_node_by_path(tree, path, NULL, NULL, 1);

	snprintf(compatible, sizeof(compatible), "infineon,%s", tis_name());

	dt_add_string_prop(tpm, "compatible", strdup(compatible));
	dt_add_string_prop(tpm, "status", "okay");
	dt_add_u32_prop(tpm, "reg", addr);

	tcpa_table = cbmem_find(CBMEM_ID_TCPA_SPEC_LOG);
	if (tcpa_table == NULL)
		die("TPM events (TCPA) log is missing from CBMEM!");

	dt_add_u64_prop(tpm, "linux,sml-base", (uintptr_t)&tcpa_table->header);
	dt_add_u32_prop(tpm, "linux,sml-size",
			sizeof(tcg_efi_spec_id_event) +
			tcpa_table->max_entries * sizeof(struct tcpa_entry));
#endif
}

/*
 * Device tree passed to Skiboot has to have phandles set either for all nodes
 * or none at all. Because relative phandles are set for cpu->l2_cache->l3_cache
 * chain, only first option is possible.
 */
static int dt_platform_update(struct device_tree *tree, uint8_t chips)
{
	struct device_tree_node *cpus;

	validate_dt(tree, chips);

	add_memory_nodes(tree);
	add_dimm_sensor_nodes(tree, chips);
	add_cb_fdt_data(tree);
	add_tpm_node(tree);

	/* Find "cpus" node */
	cpus = dt_find_node_by_path(tree, "/cpus", NULL, NULL, 0);
	if (cpus == NULL)
		die("No 'cpus' node in device tree!\n");

	for (uint8_t chip = 0; chip < MAX_CHIPS; chip++) {
		uint64_t cores;

		if (!(chips & (1 << chip)))
			continue;

		cores = read_scom(chip, 0x0006C090);
		assert(cores != 0);

		for (int core_id = 0; core_id < MAX_CORES_PER_CHIP; core_id++) {
			if (!IS_EC_FUNCTIONAL(core_id, cores))
				continue;

			/*
			 * Not sure who is the original author of this comment, it is
			 * duplicated in Hostboot and Skiboot, and now also here. It
			 * lacks one important piece of information: PIR is PIR value
			 * of thread 0 of _first_ core in pair, both for L2 and L3.
			 */
			/*
			 * Cache nodes. Those are siblings of the processor nodes under /cpus
			 * and represent the various level of caches.
			 *
			 * The unit address (and reg property) is mostly free-for-all as long as
			 * there is no collisions. On HDAT machines we use the following
			 * encoding which I encourage you to also follow to limit surprises:
			 *
			 * L2   :  (0x20 << 24) | PIR (PIR is PIR value of thread 0 of core)
			 * L3   :  (0x30 << 24) | PIR
			 * L3.5 :  (0x35 << 24) | PIR
			 *
			 * In addition, each cache points to the next level cache via its
			 * own "l2-cache" (or "next-level-cache") property, so the core node
			 * points to the L2, the L2 points to the L3 etc...
			 */
			/*
			 * This is for ATTR_PROC_FABRIC_PUMP_MODE == PUMP_MODE_CHIP_IS_GROUP,
			 * when chip ID is actually a group ID and "chip ID" field is zero.
			 */
			uint32_t pir = PPC_PLACE(chip, 49, 4) | PPC_PLACE(core_id, 57, 5);
			uint32_t l2_pir = (0x20 << 24) | (pir & ~7);
			uint32_t l3_pir = (0x30 << 24) | (pir & ~7);
			/* "/cpus/l?-cache@12345678" -> 23 characters + terminator */
			char l2_path[24];
			char l3_path[24];
			snprintf(l2_path, sizeof(l2_path), "/cpus/l%d-cache@%x", 2, l2_pir);
			snprintf(l3_path, sizeof(l3_path), "/cpus/l%d-cache@%x", 3, l3_pir);

			/*
			 * 21 for "/cpus/PowerPC,POWER9@", 4 for PIR just in case (2nd CPU),
			 * 1 for terminator
			 */
			char cpu_path[26];
			snprintf(cpu_path, sizeof(cpu_path), "/cpus/PowerPC,POWER9@%x", pir);

			struct device_tree_node *l2_node =
			       dt_find_node_by_path(tree, l2_path, NULL, NULL, 1);
			struct device_tree_node *l3_node =
			       dt_find_node_by_path(tree, l3_path, NULL, NULL, 1);
			struct device_tree_node *cpu_node =
			       dt_find_node_by_path(tree, cpu_path, NULL, NULL, 1);

			/*
			 * Cache nodes may already be created if this is the second active
			 * core in a pair. If L3 node doesn't exist, L2 also doesn't - they
			 * are created at the same time, no need to test both.
			 */
			if (!l3_node->phandle) {
				fill_l3_node(tree, l3_node, l3_pir);
				fill_l2_node(tree, l2_node, l2_pir, l3_node->phandle);
			}

			fill_cpu_node(tree, cpu_node, chip, pir, l2_node->phandle);
		}
	}

	dt_fill_all_phandles(tree, tree->root);

	return 0;
}

static void rng_init(uint8_t chips)
{
	uint8_t chip;

	for (chip = 0; chip < MAX_CHIPS; chip++) {
		if (!(chips & (1 << chip)))
			continue;

		/*
		 * RNG is allowed to run for M cycles (M = enough time to complete init;
		 * recommend 1 second of time).
		 *
		 * The only thing that ensures this is delay between istep 10.13 and now.
		 * 14.1 is the most time-consuming istep, its duration depends on the amount
		 * of installed RAM under the bigger of MCBISTs (i.e. sides of CPU on the
		 * board). This is more than enough in Hostboot.
		 *
		 * TODO: test if this is enough for coreboot with initial ECC scrubbing
		 * skipped, low amount of RAM and no debug output.
		 */
		/* NX.PBI.PBI_RNG.NX_RNG_CFG
		 *   [0-9]   FAIL_REG - abort if any of these bits is set
		 *   [17]    BIST_COMPLETE - should be 1 at this point
		 */
		uint64_t rng_status = read_scom(chip, 0x020110E0);
		assert(rng_status & PPC_BIT(17));
		while (!((rng_status = read_scom(chip, 0x020110E0)) & PPC_BIT(17)));

		if (rng_status & PPC_BITMASK(0, 9))
			die("RNG initialization failed, NX_RNG_CFG = %#16.16llx\n", rng_status);

		/*
		 * Hostboot sets 'enable' bit again even though it was already set.
		 * Following that behavior just in case.
		 */
		write_scom(chip, 0x020110E0, rng_status | PPC_BIT(63));

		/*
		 * This would be the place to set BARs, but it is done as part of quad SCOM
		 * restore.
		 */

		/* Lock NX RNG configuration */
		scom_or(chip, 0x00010005, PPC_BIT(9));
	}
}

static void enable_soc_dev(struct device *dev)
{
	int chip, idx = 0;
	unsigned long reserved_size, homers_size, occ_area, top = 0;
	uint8_t chips = fsi_get_present_chips();
	uint8_t tod_mdmt;

	for (chip = 0; chip < MAX_CHIPS; chip++) {
		int mcs_i;

		if (!(chips & (1 << chip)))
			continue;

		for (mcs_i = 0; mcs_i < MCS_PER_PROC; mcs_i++) {
			uint64_t reg;
			chiplet_id_t nest = mcs_to_nest[mcs_ids[mcs_i]];

			/* These registers are undocumented, see istep 14.5. */
			/* MCS_MCFGP */
			reg = read_scom_for_chiplet(chip, nest, 0x0501080A);
			if (reg & PPC_BIT(0)) {
				uint64_t end = base_k(reg) + size_k(reg);
				ram_resource(dev, idx++, base_k(reg), size_k(reg));
				if (end > top)
					top = end;
			}

			/* MCS_MCFGPM */
			reg = read_scom_for_chiplet(chip, nest, 0x0501080C);
			if (reg & PPC_BIT(0)) {
				uint64_t end = base_k(reg) + size_k(reg);
				ram_resource(dev, idx++, base_k(reg), size_k(reg));
				if (end > top)
					top = end;
			}
		}
	}

	/*
	 * Reserve top 8M (OCC common area) + 4M (HOMER).
	 *
	 * 8M + (4M per CPU), hostboot always reserves 8M + 8 * 4M.
	 */
	homers_size = 4*1024 * __builtin_popcount(chips);
	reserved_size = 8*1024 + homers_size;
	top -= reserved_size;
	reserved_ram_resource(dev, idx++, top, reserved_size);

	/*
	 * Assumption: OCC boots successfully or coreboot die()s, booting in safe
	 * mode without runtime power management is not supported.
	 */
	occ_area = top + homers_size;
	build_homer_image((void *)(top * 1024), (void *)(occ_area * 1024), nominal_freq);

	rng_init(chips);
	istep_18_11(chips, &tod_mdmt);
	istep_18_12(chips, tod_mdmt);
}

static void activate_slave_cores(uint8_t chip)
{
	enum { DOORBELL_MSG_TYPE = 0x0000000028000000 };

	uint8_t i;

	/* Read OCC CCSR written by the code earlier */
	const uint64_t functional_cores = read_scom(chip, 0x0006C090);

	/*
	 * This is for ATTR_PROC_FABRIC_PUMP_MODE == PUMP_MODE_CHIP_IS_GROUP,
	 * when chip ID is actually a group ID and "chip ID" field is zero.
	 */
	const uint64_t chip_msg = DOORBELL_MSG_TYPE | PPC_PLACE(chip, 49, 4);

	/* Find and process the first core in a separate loop to slightly
	 * simplify processing of all the other cores by removing a conditional */
	for (i = 0; i < MAX_CORES_PER_CHIP; ++i) {
		uint8_t thread;
		uint64_t core_msg;

		if (!IS_EC_FUNCTIONAL(i, functional_cores))
			continue;

		/* Message value for thread 0 of the current core */
		core_msg = chip_msg | (i << 2);

		/* Skip sending doorbell to the current thread of the current core */
		for (thread = (chip == 0 ? 1 : 0); thread < 4; ++thread) {
			register uint64_t msg = core_msg | thread;
			asm volatile("msgsnd %0" :: "r" (msg));
		}

		break;
	}

	for (++i; i < MAX_CORES_PER_CHIP; ++i) {
		uint8_t thread;
		uint64_t core_msg;

		if (!IS_EC_FUNCTIONAL(i, functional_cores))
			continue;

		/* Message value for thread 0 of the i-th core */
		core_msg = chip_msg | (i << 2);

		for (thread = 0; thread < 4; ++thread) {
			register uint64_t msg = core_msg | thread;
			asm volatile("msgsnd %0" :: "r" (msg));
		}
	}
}

static void * load_fdt(const char *dtb_file, uint8_t chips)
{
	void *fdt;
	void *fdt_rom;
	struct device_tree *tree;

	fdt_rom = cbfs_map(dtb_file, NULL);
	if (fdt_rom == NULL)
		die("Unable to load %s from CBFS\n", dtb_file);

	tree = fdt_unflatten(fdt_rom);

	dt_platform_update(tree, chips);

	fdt = malloc(dt_flat_size(tree));
	if (fdt == NULL)
		die("Unable to allocate memory for flat device tree\n");

	dt_flatten(tree, fdt);
	return fdt;
}

void platform_prog_run(struct prog *prog)
{
	uint8_t chips = fsi_get_present_chips();

	void *fdt;
	const char *dtb_file;

	assert(chips == 0x01 || chips == 0x03);

	dtb_file = (chips == 0x01 ? "1-cpu.dtb" : "2-cpus.dtb");
	fdt = load_fdt(dtb_file, chips);

	/* See asm/head.S in skiboot where fdt_entry starts at offset 0x10 */
	prog_set_entry(prog, prog_start(prog) + 0x10, fdt);

	/*
	 * Clear SMS_ATN aka EVT_ATN in BT_CTRL - Block Transfer IPMI protocol
	 *
	 * BMC sends event telling us that HIOMAP (access to flash, either real or
	 * emulated, through LPC) daemon has been started. This sets the mentioned bit.
	 * Skiboot enables interrupts, but because those are triggered on 0->1
	 * transition and bit is already set, they do not arrive.
	 *
	 * While we're at it, clear read and write pointers, in case circular buffer
	 * rolls over.
	 */
	if (ipmi_bt_clear(CONFIG_BMC_BT_BASE))
		die("ipmi_bt_clear() has failed.\n");

	/*
	 * Now that the payload and its interrupt vectors are already loaded
	 * perform 16.2.
	 *
	 * This MUST be done as late as possible so that none of the newly
	 * activated threads start execution before current thread jumps into
	 * the payload.
	 */
	for (uint8_t chip = 0; chip < MAX_CHIPS; chip++) {
		if (chips & (1 << chip))
			activate_slave_cores(chip);
	}
}

struct chip_operations soc_ibm_power9_ops = {
  CHIP_NAME("POWER9")
  .enable_dev = enable_soc_dev,
};

/* Restores global mem_data variable from cbmem */
static void restore_mem_data(int is_recovery)
{
	const struct cbmem_entry *entry;
	uint8_t *data;
	int dimm_i;

	(void)is_recovery; /* unused */

	entry = cbmem_entry_find(CBMEM_ID_MEMINFO);
	if (entry == NULL)
		die("Failed to find mem_data entry in CBMEM in ramstage!");

	/* Layout: mem_data itself then SPD data of each dimm which has it */
	data = cbmem_entry_start(entry);

	memcpy(&mem_data, data, sizeof(mem_data));
	data += sizeof(mem_data);

	for (dimm_i = 0; dimm_i < MAX_CHIPS * DIMMS_PER_PROC; dimm_i++) {
		int chip = dimm_i / DIMMS_PER_PROC;
		int mcs = (dimm_i % DIMMS_PER_PROC) / DIMMS_PER_MCS;
		int mca = (dimm_i % DIMMS_PER_MCS) / DIMMS_PER_MCA;
		int dimm = dimm_i % DIMMS_PER_MCA;

		rdimm_data_t *dimm_data = &mem_data[chip].mcs[mcs].mca[mca].dimm[dimm];
		if (dimm_data->spd == NULL)
			continue;

		/* We're not deleting the entry so this is valid */
		dimm_data->spd = data;
		data += CONFIG_DIMM_SPD_SIZE;
	}
}
RAMSTAGE_CBMEM_INIT_HOOK(restore_mem_data);
