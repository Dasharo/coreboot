/* SPDX-License-Identifier: GPL-2.0-only */

#include <amdblocks/psp_efs.h>
#include <boot_device.h>
#include <commonlib/region.h>
#include <console/console.h>
#include <device/mmio.h>
#include <string.h>
#include <types.h>

#define PSP_FW_FILE_HEADER_SIZE		256

bool read_efs_spi_settings(uint8_t *mode, uint8_t *speed)
{
	bool ret = false;
	struct embedded_firmware *efs;

	efs = rdev_mmap(boot_device_ro(), EFS_OFFSET, sizeof(*efs));
	if (!efs)
		return false;

	if (efs->signature == EMBEDDED_FW_SIGNATURE) {
#ifndef SPI_MODE_FIELD
		printk(BIOS_ERR, "Unknown cpu in psp_efs.h\n");
		printk(BIOS_ERR, "SPI speed/mode not set.\n");
#else
		*mode = efs->SPI_MODE_FIELD;
		*speed = efs->SPI_SPEED_FIELD;
		ret = true;
#endif
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

	efs = rdev_mmap(boot_dev, EFS_OFFSET, sizeof(*efs));
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
