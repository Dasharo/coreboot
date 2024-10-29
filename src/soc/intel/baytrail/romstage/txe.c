/* SPDX-License-Identifier: GPL-2.0-only */

#include <cf9_reset.h>
#include <commonlib/bsd/helpers.h>
#include <console/console.h>
#include <delay.h>
#include <device/device.h>
#include <device/pci.h>
#include <device/pci_def.h>
#include <device/pci_ops.h>
#include <soc/iomap.h>
#include <soc/iosf.h>
#include <soc/pci_devs.h>
#include <soc/pm.h>
#include <soc/txe.h>

#define FTPM_SIZE	4*KiB

static bool is_ftpm_enabled(void)
{
	uint32_t reg = pci_read_config32(PCI_DEV(0, TXE_DEV, TXE_FUNC),
					 TXE_SATT4_TPM_CTRL);

	if((reg & SATT4_SEC_TPM_DIS) == SATT4_SEC_TPM_DIS) {
		printk(BIOS_DEBUG, "TXE fTPM disabled\n");
		return false;
	}

	printk(BIOS_DEBUG, "TXE fTPM enabled\n");
	return true;
}

int txe_get_uma_size(void)
{
	uint32_t uma_size, reg;
	pci_devfn_t txe = PCI_DEV(0, TXE_DEV, TXE_FUNC);

	if (txe_is_excluded_from_bios_flows())
		return 0;

	if (!is_txe_devfn_visible())
		return 0;

	printk(BIOS_DEBUG, "Obtaining TXE UMA requirements\n");

	do {
		reg = pci_read_config32(txe, TXE_MEM_REQ);
	} while (((reg & MEM_REQ_VALID) != MEM_REQ_VALID) &&
		 ((reg & MEM_REQ_INVALID) != MEM_REQ_INVALID));

	uma_size = 0;

	if ((reg & MEM_REQ_VALID) == MEM_REQ_VALID) {
		reg = pci_read_config32(txe, TXE_MEM_REQ);
		uma_size = (reg & MEM_REQ_MEM_SIZE_MASK) * KiB / MiB;
		printk(BIOS_INFO, "SeC UMA Size Requested: %d MB\n", uma_size);
	} else if ((reg & MEM_REQ_INVALID) == MEM_REQ_INVALID) {
		printk(BIOS_ERR, "TXE UMA size request invalid\n");
	} else {
		printk(BIOS_ERR, "TXE not working properly\n");
	}

	/*
	 * Add an extra 1*MiB memory for fTPM communication buffer.
	 * MRC allocates memory in MiB granualrity.
	 */
	if (CONFIG(BAYTRAIL_FTPM) && uma_size != 0) {
		if (is_ftpm_enabled())
			uma_size++;
	}

	return (int)uma_size;
}

void txe_early_init(void)
{
	pci_devfn_t txe = PCI_DEV(0, TXE_DEV, TXE_FUNC);
	uint32_t reg;
	bool txe_disable = false;

	if (!is_txe_devfn_visible())
		return;

	if (txe_is_excluded_from_bios_flows()) {
		printk(BIOS_INFO, "TXE excluded from platform flows\n");
		printk(BIOS_INFO, "Taking TXE Error BIOS Path\n");
		return;
	}

	/* Init PCI Power Management capability ID */
	pci_write_config32 (txe, TXE_POWER_CAPID, 0x4803A001);

	if (txe_in_recovery()) {
		printk(BIOS_INFO, "Taking TXE Recovery BIOS Path\n");
		return;
	}

	if (txe_has_error()) {
		printk(BIOS_ERR, "Taking TXE Error BIOS Path\n");
		return;
	}

	if (txe_fw_update_in_progress()) {
		printk(BIOS_INFO, "Taking TXE FW Update BIOS Path\n");
		return;
	}

	/* Check if we don't need to hide the TXE device */
	reg = pci_read_config32(txe, TXE_FWSTS0);

	switch ((reg & TXE_CURR_OPMODE_MASK) >> TXE_CURR_OPMODE_SHIFT) {
	case TXE_FWSTS0_COM_NORMAL:
		printk(BIOS_INFO, "TXE Current Operational Mode: NORMAL\n");
		printk(BIOS_INFO, "Taking TXE Normal BIOS Path\n");
		break;
	case TXE_FWSTS0_COM_DEBUG:
		printk(BIOS_INFO, "TXE Current Operational Mode: DEBUG\n");
		txe_disable = true;
		break;
	case TXE_FWSTS0_COM_SOFT_TEMP_DISABLE:
		printk(BIOS_INFO, "TXE Current Operational Mode: SOFT_TEMP_DISABLE\n");
		txe_disable = true;
		break;
	case TXE_FWSTS0_COM_SECOVER_JMPR:
		printk(BIOS_INFO, "TXE Current Operational Mode: SECOVER_JMPR\n");
		txe_disable = true;
		break;
	case TXE_FWSTS0_COM_SECOVER_TXEI_MSG:
		printk(BIOS_INFO, "TXE Current Operational Mode: SECOVER_TXEI_MSG\n");
		txe_disable = true;
		break;
	}

	if (txe_disable) {
		printk(BIOS_INFO, "Taking TXE Disable BIOS Path\n");
		txe_hide_device();
	}
}

static void txei_assign_resource(pci_devfn_t dev)
{
	u16 pcireg;

	/* Assign Resources */
	/* Clear BIT 1-2 of Command Register */
	pcireg = pci_read_config16(dev, PCI_COMMAND);
	pcireg &= ~(PCI_COMMAND_MASTER | PCI_COMMAND_MEMORY);
	pci_write_config16(dev, PCI_COMMAND, pcireg);

	/* Program Temporary BAR for TXEI device */
	pci_write_config32(dev, PCI_BASE_ADDRESS_0, TXEI_BASE_ADDRESS);
	pci_write_config32(dev, PCI_BASE_ADDRESS_1, TXEI2_BASE_ADDRESS);

	/* Enable Bus Master and MMIO Space */
	pci_or_config16(dev, PCI_COMMAND, PCI_COMMAND_MASTER | PCI_COMMAND_MEMORY);
}

static void txe_write_did_message(pci_devfn_t txe, uint32_t status)
{
	uint32_t reg;

	if (pci_read_config32(txe, TXE_DID_MSG) & DID_MSG_DID_DONE) {
		printk(BIOS_WARNING, "TXE DRAM Init Done already set! Skipping DID message\n");
		return;
	}

	printk(BIOS_INFO, "Sending TXE DID message with status: %u\n", status >> 24);
	pci_write_config32(txe, TXE_DID_MSG, status | DID_MSG_DID_DONE);

	reg = pci_read_config32(txe, TXE_FWSTS0);
	printk(BIOS_DEBUG, "TXE FWSTS0 before DID %08x\n", reg);

	/* Poll for DID ACK for 5 seconds */
	for (int i =0; i < 5000; i++) {
		reg = pci_read_config32(txe, TXE_FWSTS0);
		if ((reg & DID_MSG_ACK_MASK) == DID_MSG_GOT_ACK) {
			printk(BIOS_DEBUG, "TXE DRAM Init Done ACK received\n");
			break;
		}
		mdelay(1);
	}

	if ((reg & DID_MSG_ACK_MASK) != DID_MSG_GOT_ACK) {
		printk(BIOS_ERR, "Timeout waiting for TXE DRAM Init Done ACK!\n");
		return;
	}

	reg &= DID_ACK_DATA_MASK;
	printk(BIOS_DEBUG, "TXE BIOS action: %u\n", reg >> DID_ACK_DATA_SHIFT);
	/* Ensure resources are programmed before requesting global reset */
	txei_assign_resource(txe);

	txe_handle_bios_action(reg);
}

static bool is_uma_allocated_for_ftpm(uintptr_t uma_base)
{
	pci_devfn_t txe = PCI_DEV(0, TXE_DEV, TXE_FUNC);
	uint32_t txe_uma_size, tolm;

	txe_uma_size = pci_read_config32(txe, TXE_MEM_REQ);
	txe_uma_size = (txe_uma_size & MEM_REQ_MEM_SIZE_MASK) * KiB;

	tolm = iosf_bunit_read(BUNIT_BMBOUND) & ~((1 << 27) - 1);

	if (tolm - uma_base >= txe_uma_size + FTPM_SIZE)
		return true;

	printk(BIOS_ERR, "Not enough TXE UMA memory allocated for fTPM\n");

	return false;
}

void txe_send_did(uintptr_t uma_base, int uma_size, int s3resume)
{
	uintptr_t ftpm_base;
	pci_devfn_t txe = PCI_DEV(0, TXE_DEV, TXE_FUNC);
	uint32_t txe_uma_size;

	if (!is_txe_devfn_visible()) {
		printk(BIOS_ERR, "TXE device hidden! Skipping DID message\n");
		return;
	}

	txe_uma_size = pci_read_config32(txe, TXE_MEM_REQ);
	txe_uma_size = (txe_uma_size & MEM_REQ_MEM_SIZE_MASK) * KiB;

	if (uma_base == 0 || uma_size == 0) {
		printk(BIOS_ERR, "TXE UMA invalid! Disabling UMA\n");
		pci_and_config32(txe, TXE_SATT4_TPM_CTRL, ~SATT_VALID);
		pci_and_config32(txe, TXE_SATT1_BIOS_CTRL, ~SATT_VALID);
		txe_write_did_message(txe, DID_STATUS_NO_MEM);
		return;
	}

	/* MRC return the UMA base as bits [32:20]*/
	uma_base = uma_base << 20;
	printk(BIOS_DEBUG, "TXE: UMA base: %08lx\n", uma_base);

	if (CONFIG(BAYTRAIL_FTPM) && is_ftpm_enabled() && is_uma_allocated_for_ftpm(uma_base)) {
		ftpm_base = uma_base + txe_uma_size;
		if (!s3resume)
			memset((void *)ftpm_base, 0, FTPM_SIZE);

		printk(BIOS_DEBUG, "TXE: fTPM base: %08lx\n", ftpm_base);

		pci_write_config32(txe, TXE_SATT4_TPM_BASE, ftpm_base);
		pci_write_config32(txe, TXE_SATT4_TPM_SIZE, FTPM_SIZE);
		pci_update_config32(txe, TXE_SATT4_TPM_CTRL, ~SATT_TARGET_MASK,
				    SATT_TARGET_MEM);
		pci_or_config32(txe, TXE_SATT4_TPM_CTRL, SATT_VALID);

		printk(BIOS_DEBUG, "TXE: SATT TPM BASE: %08x\n",
		       pci_read_config32(txe, TXE_SATT4_TPM_BASE));
		printk(BIOS_DEBUG, "TXE: SATT TPM SIZE: %08x\n",
		       pci_read_config32(txe, TXE_SATT4_TPM_SIZE));
		printk(BIOS_DEBUG, "TXE: SATT TPM CTRL: %08x\n",
		       pci_read_config32(txe, TXE_SATT4_TPM_CTRL));
	} else {
		pci_and_config32(txe, TXE_SATT4_TPM_CTRL, ~SATT_VALID);
	}

	pci_write_config32(txe, TXE_SATT1_BIOS_BASE, uma_base);
	pci_write_config32(txe, TXE_SATT1_BIOS_SIZE, uma_size * MiB);
	pci_or_config32(txe, TXE_SATT1_BIOS_CTRL, SATT_VALID);

	printk(BIOS_DEBUG, "TXE: SATT UMA BASE: %08x\n",
		pci_read_config32(txe, TXE_SATT1_BIOS_BASE));
	printk(BIOS_DEBUG, "TXE: SATT UMA SIZE: %08x\n",
		pci_read_config32(txe, TXE_SATT1_BIOS_SIZE));
	printk(BIOS_DEBUG, "TXE: SATT UMA CTRL: %08x\n",
		pci_read_config32(txe, TXE_SATT1_BIOS_CTRL));

	txe_write_did_message(txe, DID_STATUS_SUCCESS);
}
