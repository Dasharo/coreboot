/* SPDX-License-Identifier: GPL-2.0-only */

#include <amdblocks/psp.h>
#include <boot_device.h>
#include <commonlib/region.h>
#include <console/console.h>
#include <device/mmio.h>
#include <vendorcode/amd/psp/psp_efs.h>
#include <string.h>
#include <types.h>

#define PSP_FW_FILE_HEADER_SIZE		256

bool read_efs_spi_settings(uint8_t *mode, uint8_t *speed)
{
	bool ret = false;
	struct embedded_firmware *efs;

	efs = rdev_mmap(boot_device_ro(), CONFIG_AMD_FWM_POSITION, sizeof(*efs));
	if (!efs)
		return false;

	if (efs->signature == EMBEDDED_FW_SIGNATURE) {
		if (CONFIG(SOC_AMD_STONEYRIDGE)) {
			*mode = efs->spi_readmode_f15_mod_60_6f;
			*speed = efs->fast_speed_new_f15_mod_60_6f;
			ret = true;
		} else if (CONFIG(SOC_AMD_PICASSO)) {
			*mode = efs->spi_readmode_f17_mod_00_2f;
			*speed = efs->spi_fastspeed_f17_mod_00_2f;
			ret = true;
		} else if (CONFIG(SOC_AMD_CEZANNE) || CONFIG(SOC_AMD_MENDOCINO)) {
			*mode = efs->spi_readmode_f17_mod_30_3f;
			*speed = efs->spi_fastspeed_f17_mod_30_3f;
			ret = true;
		} else if (CONFIG(SOC_AMD_GENOA_POC) || CONFIG(SOC_AMD_TURIN_POC)) {
			*mode = efs->spi_readmode_f15_mod_60_6f;
			*speed = efs->fast_speed_new_f15_mod_60_6f;
			ret = true;
		} else {
			printk(BIOS_ERR, "Unknown cpu in psp_efs.h\n");
			printk(BIOS_ERR, "SPI speed/mode not set.\n");
		}
	}
	rdev_munmap(boot_device_ro(), efs);
	return ret;
}

size_t efs_read_promontory_fw(void *buf)
{
	struct embedded_firmware *efs;
	const struct region_device *boot_dev = boot_device_ro();
	uint8_t file_header[PSP_FW_FILE_HEADER_SIZE + 8];
	size_t read_bytes, fw_size;
	uint32_t offset;

	if (!boot_dev || !buf)
		return 0;

	efs = rdev_mmap(boot_dev, CONFIG_AMD_FWM_POSITION, sizeof(*efs));
	if (!efs)
		return 0;

	if (efs->signature != EMBEDDED_FW_SIGNATURE) {
		rdev_munmap(boot_dev, efs);
		return 0;
	}

	offset = efs->promontory_fw_ptr;
	rdev_munmap(boot_dev, efs);

	read_bytes = rdev_readat(boot_dev, file_header, offset, sizeof(file_header));
	if (read_bytes != sizeof(file_header))
		return 0;

	/* Get Promontory FW size */
	if (strncmp((char *)&file_header[0x10], "$PS1", 4)) {
		/* Check Promontory FW signature */
		if (!strncmp((char *)file_header, "_PT_", 4))
			fw_size = *(uint32_t *)&file_header[4];
		else
			return 0;
	} else {
		/* Check Promontory FW signature */
		if (!strncmp((char *)&file_header[PSP_FW_FILE_HEADER_SIZE], "_PT_", 4))
			fw_size = *(uint32_t *)&file_header[PSP_FW_FILE_HEADER_SIZE + 4];
		else
			return 0;

		offset += PSP_FW_FILE_HEADER_SIZE;
	}

	if (fw_size > 256 * KiB) {
		printk(BIOS_DEBUG, "Found Promontory FW too big (size: %lx)\n", fw_size);
		return 0;
	}

	printk(BIOS_DEBUG, "Found Promontory FW @ 0x%08x (size: %lx)\n",
		offset, fw_size);

	read_bytes = rdev_readat(boot_dev, buf, offset, fw_size);
	if (read_bytes != fw_size)
		return 0;

	return fw_size;
}
