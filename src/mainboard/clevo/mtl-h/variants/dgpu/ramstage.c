/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <mainboard/variants.h>
#include <ec/system76/ec/commands.h>
#include <ec/acpi/ec.h>
#include <ec/system76/ec/acpi.h>
#include <gpio.h>
#include <dasharo/options.h>
#include <device/pci_ids.h>
#include <device/pci_ops.h>
#include <device/pci_def.h>
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
	struct smfi_option_get_cmd {
		uint8_t index;
		uint8_t value;
	} __packed cmd = {
		OPT_GPU_MUX_CTRL,
		1
	};

	/*
	 * If the MUX_CTRL_BIOS option needs to be changed to match the settings, we need a
	 * global reset
	 */

	if (dasharo_dgpu_state() == DGPU_ONLY) {
		printk(BIOS_ERR, "dgpu_state = dgpu_only, index = %d, value = %d; calling global reset \n", cmd.index, cmd.value);
		system76_ec_smfi_cmd(CMD_OPTION_SET, sizeof(cmd) / sizeof(uint8_t), (uint8_t *)&cmd);
		// do_global_reset();
	}

	printk(BIOS_ERR, "dgpu_state = %d, index = %d, value = %d; global reset not called \n", dasharo_dgpu_state(), cmd.index, cmd.value);
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
