/* SPDX-License-Identifier: GPL-2.0-only */

#include <bootstate.h>
#include <console/console.h>
#include <device/device.h>
#include <device/mmio.h>
#include <device/pci.h>
#include <device/pci_ids.h>
#include <soc/amd/common/block/psp/psp_def.h>

#define MBOX_BIOS_CMD_SMM_LOCK			0x6C
#define MBOX_BIOS_CMD_LOCK_DF_REG		0x1B

#define CMD_CONFIG_ID_Z_STATE_ENABLEMENT_STATUS	0x02

/*
 * Send necessary commands after device intiialization so that
 * PSP Exit Boot Services command does not hang the platform.
 */
static void send_psp_commands(void *unused)
{
	uint32_t args[4] = { 0 };

	psp_command_set_config(CMD_CONFIG_ID_Z_STATE_ENABLEMENT_STATUS, args, "Z-State Enablement Status");
	psp_send_generic_command(MBOX_BIOS_CMD_SMM_LOCK, "Locking SMM");
	psp_send_generic_command(MBOX_BIOS_CMD_LOCK_DF_REG, "Locking DF registers");
}

static void psp_finalize(void *unused)
{
	uint32_t hsti_state = 0;
	uint32_t c2pmsg_63;
	uintptr_t psp_mmio = get_psp_mmio_base();

	if (!psp_mmio) {
		printk(BIOS_ERR, "PSP MMIO not programmed\n");
		return;
	}

	c2pmsg_63 = read32p(psp_mmio + CORE_2_PSP_MSG_63_OFFSET);

	if (psp_get_hsti_state(&hsti_state) == CB_SUCCESS) {
		printk(BIOS_INFO, "PSP: HSTI = %08x\n", hsti_state);
		c2pmsg_63 |= (hsti_state << 8);
		c2pmsg_63 |= (1 << 7); /* Set HSTI state reported bit */
		write32p(psp_mmio + CORE_2_PSP_MSG_63_OFFSET, c2pmsg_63);
	}
}

BOOT_STATE_INIT_ENTRY(BS_POST_DEVICE, BS_ON_EXIT, send_psp_commands, NULL);
BOOT_STATE_INIT_ENTRY(BS_OS_RESUME, BS_ON_ENTRY, psp_finalize, NULL);
BOOT_STATE_INIT_ENTRY(BS_PAYLOAD_LOAD, BS_ON_EXIT, psp_finalize, NULL);

static void enable_bus_master(struct device *dev)
{
	if (CONFIG(PCI_ALLOW_BUS_MASTER_ANY_DEVICE)) {
		pci_or_config16(dev, PCI_COMMAND, PCI_COMMAND_MASTER);
		printk(BIOS_DEBUG, "PSP PCI CMD: %04x\n", pci_read_config16(dev, PCI_COMMAND));
	}
}

static struct device_operations turin_psp_ops  = {
	.read_resources   = pci_dev_read_resources,
	.set_resources    = pci_dev_set_resources,
	.enable_resources = pci_dev_enable_resources,
	.enable           = enable_bus_master,
};

static const struct pci_driver turin_psp_driver __pci_driver = {
	.ops    = &turin_psp_ops,
	.vendor = PCI_VID_AMD,
	.device = 0x15c7
};
