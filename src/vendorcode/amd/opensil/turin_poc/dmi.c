/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <cbmem.h>
#include <console/console.h>
#include <device/dram/ddr4.h>
#include <device/dram/ddr5.h>
#include <device/dram/lpddr4.h>
#include <dimm_info_util.h>
#include <drivers/amd/opensil/opensil.h>
#include <lib.h>
#include <memory_info.h>
#include <smbios.h>
#include <string.h>

/*
 * There might be some ifdefs in xPRF-api.h, like CONFIG_SOC_F19M10.
 * Include opensil_config file to resolve them properly.
 */
#include <opensil_config.h>
#include <xPRF-api.h>

/**
 * Convert DDR clock speed (based on memory type) in MHz to the standard reported speed in MT/s
 */
static uint16_t ddr_speed_mhz_to_reported_mts(uint16_t ddr_type, uint16_t speed)
{
	if (CONFIG(DRAM_SUPPORT_DDR4) && ddr_type == MEMORY_TYPE_DDR4)
		return ddr4_speed_mhz_to_reported_mts(speed);
	else if (CONFIG(DRAM_SUPPORT_LPDDR4) && ddr_type == MEMORY_TYPE_LPDDR4)
		return lpddr4_speed_mhz_to_reported_mts(speed);
	else if (CONFIG(DRAM_SUPPORT_DDR5) && (ddr_type == MEMORY_TYPE_DDR5 ||
			ddr_type == MEMORY_TYPE_LPDDR5))
		return ddr5_speed_mhz_to_reported_mts(speed);

	printk(BIOS_ERR, "Unknown memory type %x\n", ddr_type);
	return 0;
}

/**
 * Return DDR voltage (in mV) based on memory type
 */
static uint16_t ddr_get_voltage(uint16_t ddr_type)
{
	switch (ddr_type) {
	case MEMORY_TYPE_DDR4:
		return 1200;
	case MEMORY_TYPE_LPDDR4:
	case MEMORY_TYPE_DDR5:
		return 1100;
	case MEMORY_TYPE_LPDDR5:
		return 1050;
	default:
		printk(BIOS_ERR, "Unknown memory type %x\n", ddr_type);
		return 0;
	}
}

/**
 * Populate dimm_info using OpenSIL SIL_TYPE17_DMI_INFO.
 */
static void transfer_memory_info(const SIL_TYPE17_DMI_INFO *dmi17, struct dimm_info *dimm)
{
	hexstrtobin(dmi17->SerialNumber, dimm->serial, sizeof(dimm->serial));

	dimm->dimm_size = smbios_memory_size_to_mib(dmi17->MemorySize, dmi17->ExtSize);

	dimm->ddr_type = dmi17->MemoryType;
	memcpy(&dimm->type_detail, &dmi17->TypeDetail, sizeof(dimm->type_detail));

	if (dmi17->ConfigSpeed == 0xffff)
		dimm->configured_speed_mts = dmi17->ExtendedConfiguredMemorySpeed;
	else
		dimm->configured_speed_mts = ddr_speed_mhz_to_reported_mts(dmi17->MemoryType,
									   dmi17->ConfigSpeed);

	if (dmi17->Speed == 0xffff)
		dimm->max_speed_mts = dmi17->ExtendedSpeed;
	else
		dimm->max_speed_mts = ddr_speed_mhz_to_reported_mts(dmi17->MemoryType,
								    dmi17->Speed);

	dimm->rank_per_dimm = dmi17->Attributes;

	dimm->mod_type = smbios_form_factor_to_spd_mod_type(
		(smbios_memory_type)dmi17->MemoryType,
		(smbios_memory_form_factor)dmi17->FormFactor);

	dimm->bus_width = smbios_bus_width_to_spd_width(dmi17->MemoryType, dmi17->TotalWidth,
						dmi17->DataWidth);

	dimm->mod_id = dmi17->ManufacturerIdCode;

	dimm->bank_locator = 0;

	dimm->vdd_voltage = dmi17->ConfiguredVoltage ? dmi17->ConfiguredVoltage
						     : ddr_get_voltage(dmi17->MemoryType);

	dimm->vdd_min_voltage = dmi17->MinimumVoltage ? dmi17->MinimumVoltage : dimm->vdd_voltage;
	dimm->vdd_max_voltage = dmi17->MaximumVoltage ? dmi17->MaximumVoltage : dimm->vdd_voltage;

	/* Added in SMBIOS v3.2 */
	dimm->memory_technology = dmi17->MemoryTechnology;
	dimm->memory_operating_mode_capability = dmi17->MemoryOperatingModeCapability.AsUint16;

	memset(dimm->fw_version, 0, sizeof(dimm->fw_version));
	strncpy((char *)dimm->fw_version, dmi17->FirmwareVersion,
	        MIN(sizeof(dimm->fw_version), sizeof(dmi17->FirmwareVersion)));

	dimm->module_product_id = dmi17->ModuleProductId;
	dimm->memory_subsys_cntrlr_manuf_id = dmi17->MemorySubsystemControllerManufacturerId;
	dimm->memory_subsys_cntrlr_product_id = dmi17->MemorySubsystemControllerProductId;

	dimm->non_volatile_size = dmi17->NonvolatileSize;
	dimm->volatile_size = dmi17->VolatileSize;
	dimm->cache_size = dmi17->CacheSize;
	dimm->logical_size = dmi17->LogicalSize;
}

static void print_dimm_info(const struct dimm_info *dimm)
{
	printk(BIOS_SPEW,
	       "MEMINFO DIMM:\n"
	       "  dimm_size: %u\n"
	       "  ddr_type: 0x%hx\n"
	       "  max_speed_mts: %hu\n"
	       "  config_speed_mts: %hu\n"
	       "  vdd_voltage: %hu\n"
	       "  rank_per_dimm: %hhu\n"
	       "  channel_num: %hhu\n"
	       "  dimm_num: %hhu\n"
	       "  bank_locator: %hhu\n"
	       "  mod_id: %hx\n"
	       "  mod_type: 0x%hhx\n"
	       "  bus_width: %hhu\n"
	       "  serial: %02hhx%02hhx%02hhx%02hhx\n"
	       "  module_part_number(%zu): %s\n",
	       dimm->dimm_size,
	       dimm->ddr_type,
	       dimm->max_speed_mts,
	       dimm->configured_speed_mts,
	       dimm->vdd_voltage,
	       dimm->rank_per_dimm,
	       dimm->channel_num,
	       dimm->dimm_num,
	       dimm->bank_locator,
	       dimm->mod_id,
	       dimm->mod_type,
	       dimm->bus_width,
	       dimm->serial[0],
	       dimm->serial[1],
	       dimm->serial[2],
	       dimm->serial[3],
	       strlen((const char *)dimm->module_part_number),
	       (char *)dimm->module_part_number);
}

static void print_type17_info(const SIL_TYPE17_DMI_INFO *dmi17)
{
	uint16_t type_detail;

	memcpy(&type_detail, &dmi17->TypeDetail, sizeof(type_detail));

	printk(BIOS_SPEW,
	       "OpenSIL TYPE 17 DMI INFO:\n"
	       "  Handle: %x\n"
	       "  TotalWidth: %hu\n"
	       "  DataWidth: %hu\n"
	       "  MemorySize: %x\n"
	       "  FormFactor: %hu\n"
	       "  DeviceSet: %hhu\n"
	       "  Speed: %hu\n"
	       "  ManufacturerIdCode: %llx\n"
	       "  Attributes: %hhu\n"
	       "  ExtSize: %u\n"
	       "  ConfigSpeed: %hu\n"
	       "  MinimumVoltage: %hu\n"
	       "  MaximumVoltage: %hu\n"
	       "  ConfiguredVoltage: %hu\n"
	       "  MemoryType: 0x%x\n"
	       "  TypeDetail 0x%x\n"
	       "  FormFactor: 0x%x\n"
	       "  DeviceLocator: %8s\n"
	       "  BankLocator: %13s\n"
	       "  SerialNumber(%zu): %9s\n"
	       "  PartNumber(%zu): %19s\n"
	       "  ExtendedSpeed %u\n"
	       "  ExtendedConfiguredMemorySpeed: %u\n",
	       dmi17->Handle,
	       dmi17->TotalWidth,
	       dmi17->DataWidth,
	       dmi17->MemorySize,
	       dmi17->FormFactor,
	       dmi17->DeviceSet,
	       dmi17->Speed,
	       dmi17->ManufacturerIdCode,
	       dmi17->Attributes,
	       dmi17->ExtSize,
	       dmi17->ConfigSpeed,
	       dmi17->MinimumVoltage,
	       dmi17->MaximumVoltage,
	       dmi17->ConfiguredVoltage,
	       dmi17->MemoryType,
	       type_detail,
	       dmi17->FormFactor,
	       dmi17->DeviceLocator,
	       dmi17->BankLocator,
	       strlen((const char *)dmi17->SerialNumber),
	       dmi17->SerialNumber,
	       strlen((const char *)dmi17->PartNumber),
	       dmi17->PartNumber,
	       dmi17->ExtendedSpeed,
	       dmi17->ExtendedConfiguredMemorySpeed);
}

static void print_type20_info(const SIL_TYPE20_DMI_INFO *dmi20)
{
	for (unsigned int region = 0; region < SIL_MAX_T20_REGION_SUPPORTED; region++) {
		printk(BIOS_SPEW,
		       "OpenSIL TYPE 20 DMI INFO region %u:\n"
		       "  StartingAddr: %08x\n"
		       "  EndingAddr: %08x\n"
		       "  MemoryDeviceHandle: %04x\n"
		       "  MemoryArrayMappedAddressHandle: %04x\n"
		       "  PartitionRowPosition: %hhu\n"
		       "  InterleavePosition: %hhu\n"
		       "  InterleavedDataDepth: %hhu\n"
		       "  ExtStartingAddr: %016llx\n"
		       "  ExtEndingAddr: %016llx\n",
		       region,
		       dmi20[region].StartingAddr,
		       dmi20[region].EndingAddr,
		       dmi20[region].MemoryDeviceHandle,
		       dmi20[region].MemoryArrayMappedAddressHandle,
		       dmi20[region].PartitionRowPosition,
		       dmi20[region].InterleavePosition,
		       dmi20[region].InterleavedDataDepth,
		       dmi20[region].ExtStartingAddr,
		       dmi20[region].ExtEndingAddr);
	}
}

static void print_type19_info(const SIL_TYPE19_DMI_INFO *dmi19)
{
	for (unsigned int region = 0; region < SIL_MAX_T19_REGION_SUPPORTED; region++) {
		printk(BIOS_SPEW,
		       "OpenSIL TYPE 19 DMI INFO region %u:\n"
		       "  StartingAddr: %08x\n"
		       "  EndingAddr: %08x\n"
		       "  MemoryArrayHandle: %04x\n"
		       "  PartitionWidth: %hhu\n"
		       "  ExtStartingAddr: %016llx\n"
		       "  ExtEndingAddr: %016llx\n",
		       region,
		       dmi19[region].StartingAddr,
		       dmi19[region].EndingAddr,
		       dmi19[region].MemoryArrayHandle,
		       dmi19[region].PartitionWidth,
		       dmi19[region].ExtStartingAddr,
		       dmi19[region].ExtEndingAddr);
	}
}

static void print_type16_info(const SIL_TYPE16_DMI_INFO *dmi16)
{
	printk(BIOS_SPEW,
	       "OpenSIL TYPE 16 DMI INFO:\n"
	       "  MemoryErrorCorrection: %hu\n"
	       "  NumberOfMemoryDevices: %hu\n"
	       "  Use: %hu\n"
	       "  Location: %hu\n",
	       dmi16->MemoryErrorCorrection,
	       dmi16->NumberOfMemoryDevices,
	       dmi16->Use,
	       dmi16->Location);
}

void opensil_smbios_fill_cbmem_meminfo(void)
{
	SIL_STATUS status;
	SIL_DMI_INFO *dmi_info;
	struct memory_info *mem_info;
	struct dimm_info *dimm_info;
	size_t dimm_cnt = 0;

	/* SIL_DMI_INFO is a big structure and we may run out of stack */
	dmi_info = (SIL_DMI_INFO *)malloc(sizeof(SIL_DMI_INFO));
	if (!dmi_info) {
		printk(BIOS_ERR, "Could not allocate memory for OpenSIL SMBIOS Mem Info\n");
		return;
	}

	memset(dmi_info, 0, sizeof(SIL_DMI_INFO));

	status = xPrfGetSmbiosMemInfo(dmi_info);
	if (status != SilPass) {
		free(dmi_info);
		printk(BIOS_ERR,
		       "Failed to obtain OpenSIL SMBIOS Mem Info\n");
		return;
	}

	/* Allocate meminfo in cbmem. */
	mem_info = cbmem_add(CBMEM_ID_MEMINFO, sizeof(struct memory_info));
	if (!mem_info) {
		printk(BIOS_ERR,
		       "Failed to add memory info to CBMEM, DMI tables will be incomplete\n");
		free(dmi_info);
		return;
	}
	memset(mem_info, 0, sizeof(struct memory_info));

	mem_info->ecc_type = dmi_info->T16.MemoryErrorCorrection;
	mem_info->number_of_devices = dmi_info->T16.NumberOfMemoryDevices;

	print_type16_info(&dmi_info->T16);

	/* Loop up to CONFIG_MAX_SOCKETS only. The array me not be filled for more sockets than 1 currently */
	for (unsigned int socket = 0;
	     socket < MIN(CONFIG_MAX_SOCKET, SIL_MAX_SOCKETS_SUPPORTED);
	     socket++) {
		for (unsigned int ch = 0;
		     ch < SIL_MAX_CHANNELS_PER_SOCKET;
		     ch++) {
			for (unsigned int dimm = 0; dimm < SIL_MAX_DIMMS_PER_CHANNEL; dimm++) {
				if (dimm_cnt >= CONFIG_DIMM_MAX) {
					printk(BIOS_WARNING,
					       "%s: DIMMs info exceeds CONFIG_DIMM_MAX\n",
					       __func__);
					goto out;
				}

				if (dmi_info->T17[socket][ch][dimm].MemorySize == 0) {
					if (mainboard_dimm_slot_exists(socket, ch, dimm)) {
						printk(BIOS_DEBUG,
						      "Found empty DIMM slot on Socket %u Channel %u DIMM %u\n",
						       socket, ch, dimm);
						dimm_info = &mem_info->dimm[dimm_cnt];
						dimm_info->dimm_size = 0;
						dimm_info->soc_num = socket;
						dimm_info->channel_num = ch;
						dimm_info->dimm_num = dimm;
						dimm_cnt++;
					}
					continue;
				}
				printk(BIOS_DEBUG,
				       "Found DIMM on Socket %u Channel %u DIMM %u\n",
				       socket, ch, dimm);

				print_type17_info(&dmi_info->T17[socket][ch][dimm]);
				print_type20_info(&dmi_info->T20[socket][ch][dimm][0]);
				dimm_info = &mem_info->dimm[dimm_cnt];
				dimm_info->soc_num = socket;
				dimm_info->channel_num = ch;
				dimm_info->dimm_num = dimm;
				transfer_memory_info(&dmi_info->T17[socket][ch][dimm],
						     dimm_info);
				print_dimm_info(dimm_info);
				dimm_cnt++;
			}
		}
	}

out:
	print_type19_info(&dmi_info->T19[0]);

	mem_info->dimm_cnt = dimm_cnt;

	free(dmi_info);
}

int opensil_smbios_fill_dimm_locator(const struct dimm_info *dimm, struct smbios_type17 *t)
{
	char locator[40];
	SIL_STATUS status;
	SIL_DMI_INFO *dmi_info;
	SIL_TYPE17_DMI_INFO *type17;

	/* SIL_DMI_INFO is a big structure and we may run out of stack */
	dmi_info = (SIL_DMI_INFO *)malloc(sizeof(SIL_DMI_INFO));
	if (!dmi_info) {
		printk(BIOS_ERR, "Could not allocate memory for OpenSIL SMBIOS Mem Info\n");
		return -1;
	}

	status = xPrfGetSmbiosMemInfo(dmi_info);
	if (status != SilPass) {
		free(dmi_info);
		printk(BIOS_ERR,
		       "Failed to obtain OpenSIL SMBIOS Mem Info\n");
		return -1;
	}

	type17 = &dmi_info->T17[dimm->soc_num][dimm->channel_num][dimm->dimm_num];

	if (sizeof(type17->DeviceLocator) > (sizeof(locator) - 1)) {
		printk(BIOS_ERR, "DIMM device locator buffer too small, increase locator size\n");
		return -1;
	}

	if (sizeof(type17->BankLocator) > (sizeof(locator) - 1)) {
		printk(BIOS_ERR, "DIMM bank locator buffer too small, increase locator size\n");
		return -1;
	}

	memset(locator, 0, sizeof(locator));
	memcpy(locator, type17->DeviceLocator, sizeof(type17->DeviceLocator));
	t->device_locator = smbios_add_string(t->eos, locator);

	memset(locator, 0, sizeof(locator));
	memcpy(locator, type17->BankLocator, sizeof(type17->BankLocator));
	t->bank_locator = smbios_add_string(t->eos, locator);

	return 0;
}
