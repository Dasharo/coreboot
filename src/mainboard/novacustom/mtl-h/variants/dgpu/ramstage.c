/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <mainboard/variants.h>
#include <ec/dasharo/ec/commands.h>
#include <ec/acpi/ec.h>
#include <ec/dasharo/ec/acpi.h>
#include <gpio.h>
#include <dasharo/options.h>
#include <device/pci_ids.h>
#include <device/pci_ops.h>
#include <device/pci_def.h>
#include <smbios.h>
#include <soc/intel/common/reset.h>

void variant_devtree_update(void)
{
	struct device *i2c_amp_dev = pcidev_on_root(0x19, 1);
	if (i2c_amp_dev) {
		bool have_smartamp = !gpio_get(GPP_E00);
		i2c_amp_dev->enabled = have_smartamp;
	}

	struct device *dgpu_dev = pcidev_on_root(0x01, 0);
	if (dgpu_dev) {
	        dgpu_dev->enabled = dasharo_dgpu_state() != IGPU_ONLY;
	        printk(BIOS_DEBUG, "dgpu_dev->enabled: %d\n", dgpu_dev->enabled);
	}
}

#define REG_DATA 2

static void set_dgpu_only(void)
{
	uint8_t initial_option_state = 99;
	dasharo_read_option(OPT_GPU_MUX_CTRL, &initial_option_state);
	printk(BIOS_DEBUG, "MUX_CTRL_BIOS state in coreboot: %d", initial_option_state);

	/*
	 * If the MUX_CTRL_BIOS option needs to be changed to match the settings, we need a
	 * global reset
	 */

	struct smfi_option_get_cmd {
		uint8_t index;
		uint8_t value;
	} __packed cmd = {
		OPT_GPU_MUX_CTRL,
		1
	};

	if (dasharo_dgpu_state() == DGPU_ONLY) {
		dasharo_ec_smfi_cmd(CMD_OPTION_SET, sizeof(cmd) / sizeof(uint8_t), (uint8_t *)&cmd);
		if (initial_option_state == 0) {
			printk(BIOS_INFO, "dGPU Only mode selected - calling global reset to toggle EC display mux\n");
			do_global_reset();
		}
	} else {
		cmd.value = 0;
		dasharo_ec_smfi_cmd(CMD_OPTION_SET, sizeof(cmd) / sizeof(uint8_t), (uint8_t *)&cmd);
		if (initial_option_state == 1) {
			printk(BIOS_INFO, "dGPU Only mode disabled - calling global reset to toggle EC display mux\n");
			do_global_reset();
		}
	}

	printk(BIOS_DEBUG, "dgpu_state = %d, option_index = %d, option_value = %d; global reset not called \n", dasharo_dgpu_state(), cmd.index, initial_option_state);
}

void variant_final(void)
{
	struct device *dgpu_rp_dev = pcidev_on_root(0x01, 0);

	if (!dgpu_rp_dev)
		return;

	struct device *dgpu_dev = pcidev_path_behind(dgpu_rp_dev->downstream, PCI_DEVFN(0, 0));

	/*
	 * The dGPU may fail to come up after changing from iGPU mode to dGPU
	 * mode. If that happens, we need to kick the platform via global reset.
	 */
	if (!dgpu_dev && dasharo_dgpu_state() != IGPU_ONLY) {
		printk(BIOS_DEBUG, "dGPU did not come up! Kicking the platform to work around it\n");
		do_global_reset();
	} else {
		printk(BIOS_DEBUG, "dGPU is up.\n");
	}

	set_dgpu_only();

}

void smbios_fill_dimm_locator(const struct dimm_info *dimm,
	struct smbios_type17 *t)
{
	switch (dimm->ctrlr_num) {
	case 0:
		t->device_locator = smbios_add_string(t->eos, "RAM1");
		break;
	case 1:
		t->device_locator = smbios_add_string(t->eos, "RAM2");
		break;
	default:
		t->device_locator = smbios_add_string(t->eos, "UNKNOWN");
		break;
	}
}
