/* SPDX-License-Identifier: GPL-2.0-only */

#include <arch/cpuid.h>
#include <bootstate.h>
#include <console/console.h>
#include <delay.h>
#include <smbios.h>
#include <stddef.h>
#include <types.h>

#include <drivers/aspeed/common/ast_ahb.h>

#include "gigabyte_smbios_oem.h"
#include "ipmi_if.h"

static enum cb_err gigabyte_get_shm_addr(int port, uint32_t *shm_address)
{
	const struct gigabyte_oem_req req = {
		.iana = GIGABYTE_IANA,
		.subcmd = GIGABYTE_SUBCMD_GET_SHM_ADDR,
	};
	struct gigabyte_get_shm_addr_rsp rsp;
	int ret;

	ret = ipmi_message(port, IPMI_NETFN_GIGABYTE_OEM, 0, IPMI_CMD_GIGABYTE_OEM_GET,
			   (const unsigned char *)&req, sizeof(req),
			   (unsigned char *)&rsp, sizeof(rsp));
	if (ret < (int)sizeof(struct ipmi_rsp) || rsp.resp.completion_code) {
		printk(BIOS_ERR, "IPMI: Gigabyte get shared memory address failed (ret=%d resp=0x%x)\n",
		       ret, rsp.resp.completion_code);
		return CB_ERR;
	}

	if (ret < (int)(offsetof(struct gigabyte_get_shm_addr_rsp, shm_address) +
			sizeof(rsp.shm_address))) {
		printk(BIOS_ERR, "IPMI: Gigabyte get shared memory address response too short (ret=%d)\n",
		       ret);
		return CB_ERR;
	}

	*shm_address = rsp.shm_address;

	printk(BIOS_DEBUG, "IPMI: Gigabyte shared memory address: 0x%08x\n", *shm_address);

	return CB_SUCCESS;
}

/*
 * The BMC answers with the number of bytes of SMBIOS data it stored: the entry
 * point and the structures up to and including the type 127 one. The blocks
 * after the table are not counted.
 */
static enum cb_err gigabyte_set_smbios_ready(int port, uint16_t total_size, size_t smbios_size)
{
	const struct gigabyte_set_smbios_ready_req req = {
		.hdr = {
			.iana = GIGABYTE_IANA,
			.subcmd = GIGABYTE_SUBCMD_SET_SMBIOS_READY,
		},
		.total_size = total_size,
		.data_size = total_size - sizeof(struct gigabyte_sysinfo_header),
	};
	struct gigabyte_set_smbios_ready_rsp rsp;
	int ret;

	for (int retry_count = 0; retry_count < 5; retry_count++) {
		ret = ipmi_message(port, IPMI_NETFN_GIGABYTE_OEM, 0, IPMI_CMD_GIGABYTE_OEM_SET,
				(const unsigned char *)&req, sizeof(req),
				(unsigned char *)&rsp, sizeof(rsp));
		if (ret > 0)
			break;

		mdelay(1000);
	}

	if (ret < (int)sizeof(struct ipmi_rsp) || rsp.resp.completion_code) {
		printk(BIOS_ERR, "IPMI: Gigabyte SMBIOS ready failed (ret=%d resp=0x%x)\n",
		       ret, rsp.resp.completion_code);
		return CB_ERR;
	}

	if (ret >= (int)sizeof(rsp))
		printk(rsp.smbios_size == smbios_size ? BIOS_DEBUG : BIOS_WARNING,
		       "IPMI: BMC accepted %u of %zu bytes of SMBIOS data (entry point and table)\n",
		       rsp.smbios_size, smbios_size);

	return CB_SUCCESS;
}

static uint16_t cpu_feature(bool enabled)
{
	return enabled ? GIGABYTE_SYSINFO_CPU_FEATURE_ENABLED : 0;
}

/* The vendor BIOS fills both socket entries with the BSP's CPUID data */
static void fill_cpu_entry(struct gigabyte_sysinfo_cpu_entry *entry)
{
	entry->smt = cpu_feature((cpuid_ext(0xb, 0).ebx & 0xff) > 1);
	entry->vmx = cpu_feature(cpuid_ecx(1) & (1 << 5));
	entry->nx = cpu_feature(cpuid_edx(0x80000001) & (1 << 20));
	entry->turbo = cpu_feature(cpuid_eax(6) & (1 << 1));
}

enum cb_err ipmi_gigabyte_send_smbios(const int port)
{
	struct smbios_entry30 *entry_point;
	void *table;
	size_t table_len;
	size_t total_len;
	uint32_t shm_address;
	struct gigabyte_sysinfo_trailer trailer = {
		.hd = {
			.signature = GIGABYTE_SYSINFO_HD_SIGNATURE,
			.revision = GIGABYTE_SYSINFO_HD_REVISION,
		},
		.cpu = { .signature = GIGABYTE_SYSINFO_CPU_SIGNATURE },
		.mem = {
			.signature = GIGABYTE_SYSINFO_MEM_SIGNATURE,
			.max_size = GIGABYTE_SYSINFO_MEM_MAX_SIZE,
			.max_count = GIGABYTE_SYSINFO_MEM_MAX_COUNT,
		},
		.lan = {
			.signature = GIGABYTE_SYSINFO_LAN_SIGNATURE,
			.max_size = GIGABYTE_SYSINFO_LAN_MAX_SIZE,
		},
		.mdata = { .signature = GIGABYTE_SYSINFO_MDATA_SIGNATURE },
		.header = {
			.signature = GIGABYTE_SYSINFO_HEADER_SIGNATURE,
			.num_blocks = GIGABYTE_SYSINFO_HEADER_NUM_BLOCKS,
		},
	};

	entry_point = smbios_get_entry_point();
	table = smbios_get_table(&table_len);
	if (!entry_point || !table || !table_len) {
		printk(BIOS_ERR, "IPMI: Gigabyte sysinfo: no SMBIOS table to forward to BMC\n");
		return CB_ERR;
	}

	/* The BMC takes the table size from the entry point */
	if (entry_point->struct_table_length != table_len) {
		printk(BIOS_ERR, "IPMI: Gigabyte sysinfo: SMBIOS table size mismatch\n");
		return CB_ERR;
	}

	total_len = sizeof(*entry_point) + table_len + sizeof(trailer);
	if (total_len > UINT16_MAX) {
		printk(BIOS_ERR, "IPMI: Gigabyte sysinfo: SMBIOS table too large (%zu bytes)\n",
		       table_len);
		return CB_ERR;
	}

	for (size_t i = 0; i < ARRAY_SIZE(trailer.cpu.cpu); i++)
		fill_cpu_entry(&trailer.cpu.cpu[i]);

	trailer.header.block_size[GIGABYTE_SYSINFO_BLOCK_SMBIOS_TABLE] = table_len;
	trailer.header.block_size[GIGABYTE_SYSINFO_BLOCK_HD] = sizeof(trailer.hd);
	trailer.header.block_size[GIGABYTE_SYSINFO_BLOCK_CPU] = sizeof(trailer.cpu);
	trailer.header.block_size[GIGABYTE_SYSINFO_BLOCK_MEM] = sizeof(trailer.mem);
	trailer.header.block_size[GIGABYTE_SYSINFO_BLOCK_LAN] = sizeof(trailer.lan);
	trailer.header.block_size[GIGABYTE_SYSINFO_BLOCK_MDATA] = sizeof(trailer.mdata);
	trailer.header.block_size[GIGABYTE_SYSINFO_BLOCK_UNNAMED] = sizeof(trailer.unnamed);

	if (gigabyte_get_shm_addr(port, &shm_address) != CB_SUCCESS)
		return CB_ERR;

	if (ast_ahb_write_buffer(shm_address, entry_point,
				 sizeof(*entry_point)) != CB_SUCCESS) {
		printk(BIOS_ERR, "IPMI: Gigabyte sysinfo: failed to write SMBIOS entry point\n");
		return CB_ERR;
	}

	if (ast_ahb_write_buffer(shm_address + sizeof(*entry_point), table,
				 table_len) != CB_SUCCESS) {
		printk(BIOS_ERR, "IPMI: Gigabyte sysinfo: failed to write SMBIOS table\n");
		return CB_ERR;
	}

	if (ast_ahb_write_buffer(shm_address + sizeof(*entry_point) + table_len, &trailer,
				 sizeof(trailer)) != CB_SUCCESS) {
		printk(BIOS_ERR, "IPMI: Gigabyte sysinfo: failed to write trailing blocks\n");
		return CB_ERR;
	}

	if (gigabyte_set_smbios_ready(port, total_len,
				      sizeof(*entry_point) + table_len) != CB_SUCCESS)
		return CB_ERR;

	printk(BIOS_INFO, "IPMI: Gigabyte sysinfo: forwarded %zu bytes of SMBIOS data to BMC\n",
	       sizeof(*entry_point) + table_len);

	return CB_SUCCESS;
}

static void ipmi_gigabyte_smbios_boot_callback(void *unused)
{
#if CONFIG(IPMI_KCS)
	ipmi_gigabyte_send_smbios(CONFIG_BMC_KCS_BASE);
#elif CONFIG(IPMI_BT)
	ipmi_gigabyte_send_smbios(CONFIG_BMC_BT_BASE);
#endif
}

BOOT_STATE_INIT_ENTRY(BS_PAYLOAD_LOAD, BS_ON_ENTRY, ipmi_gigabyte_smbios_boot_callback, NULL);
