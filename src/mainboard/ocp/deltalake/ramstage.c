/* SPDX-License-Identifier: GPL-2.0-only */

#include <assert.h>
#include <console/console.h>
#include <bootstate.h>
#include <drivers/ipmi/ipmi_ops.h>
#include <drivers/ocp/dmi/ocp_dmi.h>
#include <gpio.h>
#include <soc/lewisburg_pch_gpio_defs.h>
#include <soc/ramstage.h>
#include <soc/soc_util.h>
#include <stdio.h>
#include <string.h>
#include <smbios.h>
#include <device/pci_def.h>
#include <device/pci_ops.h>
#include <soc/util.h>
#include <hob_iiouds.h>
#include <hob_memmap.h>
#include <cpxsp_dl_gpio.h>

#include "ipmi.h"

#define SLOT_ID_LEN 2

extern struct fru_info_str fru_strings;
static char slot_id_str[SLOT_ID_LEN];

/*
 * Update SMBIOS type 0 ec version.
 * In deltalake, BMC version is used to represent ec version.
 * In current version of OpenBMC, it follows IPMI v2.0 to define minor revision as BCD
 * encoded, so the format of it must be transferred before send to SMBIOS.
 */
void smbios_ec_revision(uint8_t *ec_major_revision, uint8_t *ec_minor_revision)
{
	uint8_t bmc_major_revision, bmc_minor_revision;

	ipmi_bmc_version(&bmc_major_revision, &bmc_minor_revision);
	*ec_major_revision = bmc_major_revision & 0x7f; /* bit[6:0] Major Firmware Revision */
	*ec_minor_revision = ((bmc_minor_revision / 16) * 10) + (bmc_minor_revision % 16);
}

/* Override SMBIOS 2 Location In Chassis from BMC */
const char *smbios_mainboard_location_in_chassis(void)
{
	uint8_t slot_id = 0;

	if (ipmi_get_slot_id(&slot_id) != CB_SUCCESS) {
		printk(BIOS_ERR, "IPMI get slot_id failed\n");
		return "";
	}
	/* Sanity check, slot_id can only be 1~4 since there are 4 slots in YV3 */
	if (slot_id < PCIE_CONFIG_A || slot_id > PCIE_CONFIG_D) {
		printk(BIOS_ERR, "slot_id %d is not between 1~4\n", slot_id);
		return "";
	}
	snprintf(slot_id_str, SLOT_ID_LEN, "%d", slot_id);
	return slot_id_str;
}

/* System Slot Socket, Stack, Type and Data bus width Information */
typedef struct {
	u8 stack;
	u8 slot_type;
	u8 slot_data_bus_width;
	u8 dev_func;
	const char *slot_designator;
} slot_info;

slot_info slotinfo[] = {
	{CSTACK,  SlotTypePciExpressGen3X4, SlotDataBusWidth4X, 0xE8, "SSD1_M2_Data_Drive"},
	{PSTACK1, SlotTypePciExpressGen3X4, SlotDataBusWidth4X, 0x10, "SSD0_M2_Boot_Drive"},
	{PSTACK1, SlotTypePciExpressGen3X4, SlotDataBusWidth4X, 0x18, "BB_OCP_NIC"},
	{PSTACK2, SlotTypePciExpressGen3X4, SlotDataBusWidth4X, 0x00, "1OU_JD2_M2_3"},
	{PSTACK2, SlotTypePciExpressGen3X4, SlotDataBusWidth4X, 0x08, "1OU_JD2_M2_2"},
	{PSTACK2, SlotTypePciExpressGen3X4, SlotDataBusWidth4X, 0x10, "1OU_JD1_M2_1"},
	{PSTACK2, SlotTypePciExpressGen3X4, SlotDataBusWidth4X, 0x18, "1OU_JD1_M2_0"},
	{PSTACK0, SlotTypePciExpressGen3X4, SlotDataBusWidth4X, 0x00, "2OU_JD1_M2_0"},
	{PSTACK0, SlotTypePciExpressGen3X4, SlotDataBusWidth4X, 0x08, "2OU_JD1_M2_1"},
	{PSTACK0, SlotTypePciExpressGen3X4, SlotDataBusWidth4X, 0x10, "2OU_JD3_M2_4"},
	{PSTACK0, SlotTypePciExpressGen3X4, SlotDataBusWidth4X, 0x18, "2OU_JD3_M2_5"},
	{PSTACK1, SlotTypePciExpressGen3X4, SlotDataBusWidth4X, 0x00, "2OU_JD2_M2_3"},
	{PSTACK1, SlotTypePciExpressGen3X4, SlotDataBusWidth4X, 0x08, "2OU_JD2_M2_2"},
	{PSTACK2, SlotTypePciExpressGen3X16, SlotDataBusWidth16X, 0x00, "1OU_OCP_NIC"},
};

#define SPD_REGVID_LEN 6
/* A 4-digit long number plus a space */
static void write_oem_word(uint16_t val, char *str)
{
	snprintf(str, SPD_REGVID_LEN, "%04x ", val);
}

static void dl_oem_smbios_strings(struct device *dev, struct smbios_type11 *t)
{
	uint8_t pcie_config = 0;
	const struct SystemMemoryMapHob *hob;
	char spd_reg_vid[SPD_REGVID_LEN];
	char empty[1] = "";
	char *oem_str7 = empty;

	/* OEM string 1 to 6 */
	ocp_oem_smbios_strings(dev, t);

	/* OEM string 7 is the register vendor ID in SPD for each DIMM strung together */
	hob = get_system_memory_map();
	assert(hob != NULL);
	/* There are at most 6 channels and 2 DIMMs per channel, but Delta Lake has 6 DIMMs,
	   e.g. b300 0000 b300 0000 b300 0000 b300 0000 b300 0000 b300 0000 */
	for (int ch = 0; ch < MAX_CH; ch++) {
		for (int dimm = 0; dimm < MAX_IMC; dimm++) {
			write_oem_word(hob->Socket[0].ChannelInfo[ch].DimmInfo[dimm].SPDRegVen,
				spd_reg_vid);
			oem_str7 = strconcat(oem_str7, spd_reg_vid);
		}
	}
	t->count = smbios_add_oem_string(t->eos, oem_str7);

	/* Add OEM string 8 */
	if (ipmi_get_pcie_config(&pcie_config) == CB_SUCCESS) {
		switch (pcie_config) {
		case PCIE_CONFIG_UNKNOWN:
			t->count = smbios_add_oem_string(t->eos, "0x0: Unknown");
			break;
		case PCIE_CONFIG_A:
			t->count = smbios_add_oem_string(t->eos, "0x1: YV3 Config-A");
			break;
		case PCIE_CONFIG_B:
			t->count = smbios_add_oem_string(t->eos, "0x2: YV3 Config-B");
			break;
		case PCIE_CONFIG_C:
			t->count = smbios_add_oem_string(t->eos, "0x3: YV3 Config-C");
			break;
		case PCIE_CONFIG_D:
			t->count = smbios_add_oem_string(t->eos, "0x4: YV3 Config-D");
			break;
		default:
			t->count = smbios_add_oem_string(t->eos, "Check BMC return data");
		}
	} else {
		printk(BIOS_ERR, "Failed to get IPMI PCIe config\n");
	}
}

static int create_smbios_type9(int *handle, unsigned long *current)
{
	int index;
	int length = 0;
	uint8_t slot_length;
	uint8_t sec_bus;
	uint8_t slot_usage;
	uint8_t pcie_config = 0;
	uint8_t characteristics_1 = 0;
	uint8_t characteristics_2 = 0;
	uint32_t vendor_device_id;
	uint32_t stack_busnos[6];
	pci_devfn_t pci_dev;
	unsigned int cap;
	uint16_t sltcap;

	if (ipmi_get_pcie_config(&pcie_config) != CB_SUCCESS)
		printk(BIOS_ERR, "Failed to get IPMI PCIe config\n");

	get_stack_busnos(stack_busnos);

	for (index = 0; index < ARRAY_SIZE(slotinfo); index++) {
		if (pcie_config == PCIE_CONFIG_A) {
			if (index == 0 || index == 1 || index == 2)
				printk(BIOS_INFO, "Find Config-A slot: %s\n",
					slotinfo[index].slot_designator);
			else
				continue;
		}
		if (pcie_config == PCIE_CONFIG_B) {
			if (index == 0 || index == 1 || index == 2 || index == 3 || index == 4
				|| index == 5 || index == 6)
				printk(BIOS_INFO, "Find Config-B slot: %s\n",
					slotinfo[index].slot_designator);
			else
				continue;
		}
		if (pcie_config == PCIE_CONFIG_C) {
			if (index == 0 || index == 1 || index == 7 || index == 8 || index == 9
				|| index == 10 || index == 11 || index == 12 || index == 13)
				printk(BIOS_INFO, "Find Config-C slot: %s\n",
					slotinfo[index].slot_designator);
			else
				continue;
		}
		if (pcie_config == PCIE_CONFIG_D) {
			if (index != 13)
				printk(BIOS_INFO, "Find Config-D slot: %s\n",
					slotinfo[index].slot_designator);
			else
				continue;
		}

		if (slotinfo[index].slot_data_bus_width == SlotDataBusWidth16X)
			slot_length = SlotLengthLong;
		else
			slot_length = SlotLengthShort;

		pci_dev = PCI_DEV(stack_busnos[slotinfo[index].stack],
			slotinfo[index].dev_func >> 3, slotinfo[index].dev_func & 0x7);
		sec_bus = pci_s_read_config8(pci_dev, PCI_SECONDARY_BUS);

		if (sec_bus == 0xFF) {
			slot_usage = SlotUsageUnknown;
		} else {
			/* Checking for Slot device availability */
			pci_dev = PCI_DEV(sec_bus, 0, 0);
			vendor_device_id = pci_s_read_config32(pci_dev, 0);
			if (vendor_device_id == 0xFFFFFFFF)
				slot_usage  = SlotUsageAvailable;
			else
				slot_usage = SlotUsageInUse;
		}

		characteristics_1 |= SMBIOS_SLOT_3P3V; // Provides33Volts
		characteristics_2 |= SMBIOS_SLOT_PME; // PmeSiganalSupported

		cap = pci_s_find_capability(pci_dev, PCI_CAP_ID_PCIE);
		sltcap = pci_s_read_config16(pci_dev, cap + PCI_EXP_SLTCAP);
		if (sltcap & PCI_EXP_SLTCAP_HPC)
			characteristics_2 |= SMBIOS_SLOT_HOTPLUG;

		length += smbios_write_type9(current, handle,
					  slotinfo[index].slot_designator,
					  slotinfo[index].slot_type,
					  slotinfo[index].slot_data_bus_width,
					  slot_usage,
					  slot_length,
					  characteristics_1,
					  characteristics_2,
					  stack_busnos[slotinfo[index].stack],
					  slotinfo[index].dev_func);
	}

	return length;
}

static int mainboard_smbios_data(struct device *dev, int *handle, unsigned long *current)
{
	int len = 0;

	len += create_smbios_type9(handle, current);

	return len;
}

void smbios_fill_dimm_locator(const struct dimm_info *dimm, struct smbios_type17 *t)
{
	char buf[40];

	snprintf(buf, sizeof(buf), "DIMM %c0", 'A' + dimm->channel_num);
	t->device_locator = smbios_add_string(t->eos, buf);

	snprintf(buf, sizeof(buf), "_Node0_Channel%d_Dimm0", dimm->channel_num);
	t->bank_locator = smbios_add_string(t->eos, buf);
}

unsigned int smbios_processor_family(struct cpuid_result res)
{
	return 0xb3; /* Xeon */
}

unsigned int smbios_processor_characteristics(void)
{
	/* 64-bit Capable, Multi-Core, Power/Performance Control */
	return 0x8c; /* BIT2 + BIT3 + BIT7 */
}

static void mainboard_enable(struct device *dev)
{
	dev->ops->get_smbios_strings = dl_oem_smbios_strings,
	read_fru_areas(CONFIG_BMC_KCS_BASE, CONFIG_FRU_DEVICE_ID, 0, &fru_strings);
	dev->ops->get_smbios_data = mainboard_smbios_data;
}

void mainboard_silicon_init_params(FSPS_UPD *params)
{
	/* configure Lewisburg PCH GPIO controller after FSP-M */
	gpio_configure_pads(gpio_table, ARRAY_SIZE(gpio_table));
}

static void mainboard_final(void *chip_info)
{
	struct ppin_req req = {0};

	req.cpu0_lo = xeon_sp_ppin[0].lo;
	req.cpu0_hi = xeon_sp_ppin[0].hi;
	/* Set PPIN to BMC */
	if (ipmi_set_ppin(&req) != CB_SUCCESS)
		printk(BIOS_ERR, "ipmi_set_ppin failed\n");
}

struct chip_operations mainboard_ops = {
	.enable_dev = mainboard_enable,
	.final = mainboard_final,
};

static void pull_post_complete_pin(void *unused)
{
	/* Pull Low post complete pin */
	gpio_output(GPP_B20, 0);
}

BOOT_STATE_INIT_ENTRY(BS_PAYLOAD_BOOT, BS_ON_ENTRY, pull_post_complete_pin, NULL);
