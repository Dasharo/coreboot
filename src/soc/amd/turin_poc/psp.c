/* SPDX-License-Identifier: GPL-2.0-only */

#include <amdblocks/psp.h>
#include <amdblocks/smn.h>
#include <bootstate.h>
#include <console/console.h>
#include <device/device.h>
#include <device/mmio.h>
#include <device/pci.h>
#include <device/pci_ids.h>
#include <soc/amd/common/block/psp/psp_def.h>

#include <Nbio/NbioClass-api.h>

#define MBOX_BIOS_CMD_SMM_LOCK			0x6C
#define MBOX_BIOS_CMD_LOCK_DF_REG		0x1B

#define CMD_CONFIG_ID_NBIF_DDR_CTRL		0x03
#define CMD_CONFIG_ID_L1_SRIOV_SEC_POLICY_CPU	0x0f

#define NBIF_DDR_BASE_ADDRESS_SMN		0x10133284

static void send_generic_psp_command(uint32_t command, const char *msg)
{
	int cmd_status;
	struct mbox_default_buffer buffer = {
		.header = {
			.size = sizeof(buffer)
		},
	};

	printk(BIOS_DEBUG, "PSP: %s... ", msg);

	cmd_status = send_psp_command(command, &buffer);

	/* buffer's status shouldn't change but report it if it does */
	psp_print_cmd_status(cmd_status, &buffer.header);
}

static void psp_command_set_config(uint32_t config, const char *msg)
{
	int cmd_status;
	struct mbox_cmd_set_config_buffer buffer = {
		.header = {
			.size = sizeof(buffer)
		},
		.config = {
			.config_id = config
		}
	};

	printk(BIOS_DEBUG, "PSP: %s... ", msg);

	cmd_status = send_psp_command(MBOX_BIOS_CMD_SET_CONFIG, &buffer);

	/* buffer's status shouldn't change but report it if it does */
	psp_print_cmd_status(cmd_status, &buffer.header);
}

static bool is_nbif_ddr_base_assigned(void)
{
	/* Check NBIF DDR Base address was assigned at ABL phase */
	if (smn_read64(NBIF_DDR_BASE_ADDRESS_SMN) == 0ULL)
		return false;

	return true;
}

/*
 * Send necessary commands after device intiialization so that
 * PSP Exit Boot Services command does not hang the platform.
 */
static void send_psp_commands(void *unused)
{
	NBIOCLASS_DATA_BLOCK *nbio_data = SilFindStructure(SilId_NbioClass, 0);
	NBIO_CONFIG_DATA *input = &nbio_data->NbioConfigData;

	send_generic_psp_command(MBOX_BIOS_CMD_SMM_LOCK, "Locking SMM");
	send_generic_psp_command(MBOX_BIOS_CMD_LOCK_DF_REG, "Locking DF registers");

	if (is_nbif_ddr_base_assigned())
		psp_command_set_config(CMD_CONFIG_ID_NBIF_DDR_CTRL,
				       "Notify NBIF DDR Init sequence");

	if (input->CfgSriovEnDev0F1)
		psp_command_set_config(CMD_CONFIG_ID_L1_SRIOV_SEC_POLICY_CPU,
				       "Notify MPDMA-TF SRIOV enabled");
}

static void update_psp_hsti_state(void *unused)
{
	uint32_t hsti_state = 0;
	uint32_t c2pmsg_63;
	uintptr_t psp_mmio = get_psp_mmio_base();

	if (!psp_mmio) {
		printk(BIOS_ERR, "PSP MMIO not programmed\n");
		return;
	}

	c2pmsg_63 = read32p(psp_mmio + CORE_2_PSP_MSG_63_OFFSET);

	/* If HSTI already reported, skip updating the PSP capability register */
	if (c2pmsg_63 & (1 << 7)){
		printk(BIOS_DEBUG, "PSP HSTI already reported\n");
		return;
	}

	if (psp_get_hsti_state(&hsti_state) == CB_SUCCESS) {
		printk(BIOS_INFO, "PSP: HSTI = %08x\n", hsti_state);
		c2pmsg_63 |= (hsti_state << 8);
		c2pmsg_63 |= (1 << 7); /* Set HSTI state reported bit */
		write32p(psp_mmio + CORE_2_PSP_MSG_63_OFFSET, c2pmsg_63);
	}
}

BOOT_STATE_INIT_ENTRY(BS_POST_DEVICE, BS_ON_EXIT, send_psp_commands, NULL);
BOOT_STATE_INIT_ENTRY(BS_OS_RESUME, BS_ON_ENTRY, update_psp_hsti_state, NULL);
BOOT_STATE_INIT_ENTRY(BS_PAYLOAD_LOAD, BS_ON_EXIT, update_psp_hsti_state, NULL);

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
	.device = 0x156e
};
