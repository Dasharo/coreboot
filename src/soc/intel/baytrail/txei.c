/* SPDX-License-Identifier: GPL-2.0-only */

#define __SIMPLE_DEVICE__

#include <bootmode.h>
#include <console/console.h>
#include <device/device.h>
#include <device/pci.h>
#include <device/pci_def.h>
#include <device/pci_ops.h>
#include <soc/iomap.h>
#include <soc/pci_devs.h>
#include <soc/pm.h>
#include <soc/txe.h>
#include <cf9_reset.h>
#include <delay.h>
#include <timer.h>

#define SLOT_SIZE			sizeof(uint32_t)
/* Circular buffer on TXEI has only 16 slots */
#define TXEI_CIRCULAR_BUFFER_SIZE	16

#define TXEI_HDR_IS_COMPLETE		(1 << 31)
#define TXEI_HDR_LENGTH_START		16
#define TXEI_HDR_LENGTH_SIZE		9
#define TXEI_HDR_LENGTH			(((1 << TXEI_HDR_LENGTH_SIZE) - 1) << TXEI_HDR_LENGTH_START)
#define TXEI_HDR_HOST_ADDR_START	8
#define TXEI_HDR_HOST_ADDR		(((1 << 8) - 1) << TXEI_HDR_HOST_ADDR_START)
#define TXEI_HDR_TXE_ADDR_START		0
#define TXEI_HDR_TXE_ADDR		(((1 << 8) - 1) << TXEI_HDR_TXE_ADDR_START)

enum txe_cmd_result {
	TXE_CMD_RESULT_GLOBAL_RESET_REQUESTED,
	TXE_CMD_RESULT_SUCCESS,
	TXE_CMD_RESULT_ERROR,
	TXE_CMD_RESULT_DISABLED,
	TXE_CMD_RESULT_RETRY,
};

bool is_txe_devfn_visible(void)
{
	if (!is_devfn_enabled(PCI_DEVFN(TXE_DEV, TXE_FUNC))) {
		printk(BIOS_WARNING, "TXE device %x.%x is disabled\n", TXE_DEV, TXE_FUNC);
		return false;
	}

	if (pci_read_config16(PCI_DEV(0, TXE_DEV, TXE_FUNC), PCI_VENDOR_ID) == 0xFFFF) {
		printk(BIOS_WARNING, "TXE device %x.%x is hidden\n", TXE_DEV, TXE_FUNC);
		return false;
	}

	return true;
}

/* Get HECI BAR 0 from PCI configuration space */
static uintptr_t get_txei_bar(pci_devfn_t dev)
{
	uintptr_t bar;

	bar = pci_read_config32(dev, PCI_BASE_ADDRESS_0);
	assert(bar != 0);
	return bar & ~PCI_BASE_ADDRESS_MEM_ATTR_MASK;
}

static void txei_assign_resource(pci_devfn_t dev, uintptr_t tempbar0, uintptr_t tempbar1)
{
	u16 pcireg;

	/* Assign Resources */
	/* Clear BIT 1-2 of Command Register */
	pcireg = pci_read_config16(dev, PCI_COMMAND);
	pcireg &= ~(PCI_COMMAND_MASTER | PCI_COMMAND_MEMORY);
	pci_write_config16(dev, PCI_COMMAND, pcireg);

	/* Program Temporary BAR for TXEI device */
	if (tempbar0 == 0)
		pci_write_config32(dev, PCI_BASE_ADDRESS_0, TXEI_BASE_ADDRESS);
	else
		pci_write_config32(dev, PCI_BASE_ADDRESS_0, tempbar0);

	if (tempbar1 == 0)
		pci_write_config32(dev, PCI_BASE_ADDRESS_1, TXEI2_BASE_ADDRESS);
	else
		pci_write_config32(dev, PCI_BASE_ADDRESS_1, tempbar1);

	/* Enable Bus Master and MMIO Space */
	pci_or_config16(dev, PCI_COMMAND, PCI_COMMAND_MASTER | PCI_COMMAND_MEMORY);
}

int txe_aliveness_request(uintptr_t tempbar, bool request)
{
	struct stopwatch sw;

	if (request)
		write32p(tempbar + TXE_MMIO_ALIVENESS_REQ,
			 read32p(tempbar + TXE_MMIO_ALIVENESS_REQ) | 1);
	else
		write32p(tempbar + TXE_MMIO_ALIVENESS_REQ,
			 read32p(tempbar + TXE_MMIO_ALIVENESS_REQ) & (uint32_t)~1);

	stopwatch_init_usecs_expire(&sw, HECI_READY_TIMEOUT);
	while ((read32p(tempbar + TXE_MMIO_ALIVENESS_RESP) & 1) != request) {
		// Check if init timeout has expired.
		if (stopwatch_expired(&sw)) {
			printk(BIOS_ERR, "Timeout when sending TXE aliveness request: %d\n", request);
			return -1;
		}

		udelay(100);
	}

	return 0;
}

void txe_handle_bios_action(uint32_t bios_action)
{
	switch (bios_action) {
	/* Do nothing for the below actions */
	case NO_DID_ACK_RECEIVED:
	case ACK_DATA_GO_TO_S3:
	case ACK_DATA_GO_TO_S4:
	case ACK_DATA_GO_TO_S5:
		return;
	case ACK_DATA_CONTINUE_BOOT:
		printk(BIOS_INFO, "TXE requested to continue boot\n");
		break;
	case NON_PWR_CYCLE_RESET:
		printk(BIOS_INFO, "TXE requested a reset\n");
		system_reset();
		break;
	case PWR_CYCLE_RESET:
		printk(BIOS_INFO, "TXE requested a power cycle reset\n");
		full_reset();
		break;
	case PERFORM_GLOBAL_RESET:
		printk(BIOS_INFO, "TXE requested a global reset\n");
		printk(BIOS_INFO, "Global reset will be performed in ramstage\n");
		break;
	default:
		printk(BIOS_INFO, "TXE invalid BIOS action %x\n",
		       bios_action >> DID_ACK_DATA_SHIFT);
		break;
	}
}

/*
 * Initialize the TXE device with provided temporary BAR. If BAR is 0 use a
 * default. This is intended for pre-mem usage only where BARs haven't been
 * assigned yet and devices are not enabled.
 */
void txe_txei_init(void)
{
	pci_devfn_t dev = PCI_DEV(0, TXE_DEV, TXE_FUNC);
	uint32_t aliveness_req, aliveness_resp;
	uint32_t txe_opmode;
	uintptr_t tempbar0, tempbar;
	struct stopwatch sw;

	/* Check if device enabled */
	if (!is_txe_devfn_visible())
		return;

	tempbar0 =  pci_read_config32(dev, PCI_BASE_ADDRESS_0);
	tempbar0 &= ~PCI_BASE_ADDRESS_MEM_ATTR_MASK;
	tempbar =  pci_read_config32(dev, PCI_BASE_ADDRESS_1);
	tempbar &= ~PCI_BASE_ADDRESS_MEM_ATTR_MASK;

	txe_opmode = pci_read_config32(dev, TXE_FWSTS0);
	txe_opmode &= TXE_CURR_OPMODE_MASK;
	txe_opmode >>= TXE_CURR_OPMODE_SHIFT;

	if (txe_opmode == TXE_FWSTS0_COM_DEBUG ||
	    txe_opmode == TXE_FWSTS0_COM_SOFT_TEMP_DISABLE ||
	    txe_opmode == TXE_FWSTS0_COM_SECOVER_JMPR ||
	    txe_opmode == TXE_FWSTS0_COM_SECOVER_TXEI_MSG)
		return;

	/* Assign TXEI resource and enable the resource */
	txei_assign_resource(dev, tempbar0, tempbar);

	/* Configure interrupt */
	pci_and_config8(dev, TXE_MSI_CAP, 0xfc);

	stopwatch_init_usecs_expire(&sw, HECI_READY_TIMEOUT);
	do {
		aliveness_req = read32p(tempbar + TXE_MMIO_ALIVENESS_REQ) & 1;
		aliveness_resp = read32p(tempbar + TXE_MMIO_ALIVENESS_RESP) & 1;
		if (stopwatch_expired(&sw)) {
			printk(BIOS_ERR, "Timeout waiting for TXE aliveness early sync\n");
			return;
		}

		udelay(1000);
	} while(aliveness_req != aliveness_resp);

	if ((read32p(tempbar + TXE_MMIO_ALIVENESS_REQ) & 1) == 1)
		txe_aliveness_request(tempbar, false);

	write32p(tempbar + TXE_MMIO_HOST_IPC_READINESS,
		 read32p(tempbar + TXE_MMIO_HOST_IPC_READINESS) | IPC_READINESS_RDY_CLR);

	stopwatch_init_usecs_expire(&sw, HECI_READY_TIMEOUT);
	do {
		if (stopwatch_expired(&sw)) {
			printk(BIOS_ERR, "Timeout waiting for TXE IPC readiness\n");
			return;
		}

		udelay(1000);
	} while(!(read32p(tempbar + TXE_MMIO_SEC_IPC_READINESS) & IPC_READINESS_SEC_RDY));

	write32p(tempbar + TXE_MMIO_HHISR,
		 read32p(tempbar + TXE_MMIO_HHISR) & ~(HHISR_INT_BAR0_STS | HHISR_INT_BAR0_STS));

	write32p(tempbar + TXE_MMIO_SEC_IPC_OUTPUT_STATUS,
		 read32p(tempbar + TXE_MMIO_SEC_IPC_OUTPUT_STATUS) | IPC_OUTPUT_READY);


	write32p(tempbar + TXE_MMIO_HOST_IPC_READINESS,
		 read32p(tempbar + TXE_MMIO_HOST_IPC_READINESS) | IPC_READINESS_HOST_RDY);

	txe_aliveness_request(tempbar, true);
}

bool txe_is_excluded_from_bios_flows(void)
{
	uint32_t reg;

	if (read32p(PMC_BASE_ADDRESS + 0xc) & 0x10) {
		reg = read32p(PMC_BASE_ADDRESS + 0x2c) & 0xf;
		switch (reg) {
		case 0:
		case 4:
		case 5:
		case 6:
			return true;
		}
	}

	return false;
}

bool txe_in_recovery(void)
{
	if (!is_txe_devfn_visible())
		return false;

	uint32_t reg = pci_read_config32(PCI_DEV(0, TXE_DEV, TXE_FUNC), TXE_FWSTS0);

	return ((reg & TXE_CWS_MASK) == TXE_FWSTS0_CWS_RECOVERY);
}

bool txe_has_error(void)
{
	if (!is_txe_devfn_visible())
		return false;

	uint32_t reg = pci_read_config32(PCI_DEV(0, TXE_DEV, TXE_FUNC), TXE_FWSTS0);

	return ((reg & TXE_ERR_CODE_MASK) != 0);
}

bool txe_fw_update_in_progress(void)
{
	if (!is_txe_devfn_visible())
		return false;

	uint32_t reg = pci_read_config32(PCI_DEV(0, TXE_DEV, TXE_FUNC), TXE_FWSTS0);

	return !!(reg & TXE_FW_UPD_IN_PROGRESS);
}

void txe_hide_device(void)
{
	write32p(PMC_BASE_ADDRESS + FUNC_DIS,
		read32p(PMC_BASE_ADDRESS + FUNC_DIS) | TXE_DIS);
}

void txe_fw_shadow_done(void)
{
	if (!is_txe_devfn_visible())
		return;

	printk(BIOS_DEBUG, "TXE: sending FW SHADOW DONE\n");
	pci_or_config32(PCI_DEV(0, TXE_DEV, TXE_FUNC), TXE_HOST_TO_SEC0, 1);
}

static uint32_t read_bar(pci_devfn_t dev, uint32_t offset)
{
	return read32p(get_txei_bar(dev) + offset);
}

static void write_bar(pci_devfn_t dev, uint32_t offset, uint32_t val)
{
	return write32p(get_txei_bar(dev) + offset, val);
}

static uint32_t read_txe_csr(void)
{
	return read_bar(PCI_DEV(0, TXE_DEV, TXE_FUNC), TXE_MMIO_SEC_CSR);
}

static uint32_t read_host_csr(void)
{
	return read_bar(PCI_DEV(0, TXE_DEV, TXE_FUNC), TXE_MMIO_HOST_CSR);
}

static void write_host_csr(uint32_t data)
{
	write_bar(PCI_DEV(0, TXE_DEV, TXE_FUNC), TXE_MMIO_HOST_CSR, data);
}

static int txe_ready(void)
{
	uint32_t csr;
	csr = read_txe_csr();
	return csr & CSR_READY;
}

static int txe_ipc_ready(void)
{
	uint32_t ipc_input_sts;
	ipc_input_sts = read_bar(PCI_DEV(0, TXE_DEV, TXE_FUNC), TXE_MMIO_SEC_IPC_INPUT_STS);
	return ipc_input_sts & 1;
}

static int host_ready(void)
{
	uint32_t csr;
	csr = read_host_csr();
	return csr & CSR_READY;
}

static int host_interrupt(void)
{
	uint32_t csr;
	csr = read_host_csr();
	return csr & CSR_IS;
}

static int wait_heci_ipc_ready(void)
{
	struct stopwatch sw;

	stopwatch_init_usecs_expire(&sw, HECI_READY_TIMEOUT);
	while (!txe_ipc_ready()) {
		udelay(HECI_WAIT_DELAY);
		if (stopwatch_expired(&sw))
			return 0;
	}

	return 1;
}

static int wait_heci_ready(void)
{
	struct stopwatch sw;

	stopwatch_init_usecs_expire(&sw, HECI_INIT_TIMEOUT);
	while (!txe_ready()) {
		udelay(HECI_WAIT_DELAY);
		if (stopwatch_expired(&sw))
			return 0;
	}

	return 1;
}

static int wait_host_reset_started(void)
{
	struct stopwatch sw;

	stopwatch_init_usecs_expire(&sw, HECI_INIT_TIMEOUT);
	while (host_ready()) {
		udelay(HECI_WAIT_DELAY);
		if (stopwatch_expired(&sw))
			return 0;
	}

	return 1;
}

static int wait_host_interrupt(void)
{
	struct stopwatch sw;

	stopwatch_init_usecs_expire(&sw, HECI_INIT_TIMEOUT);
	while (!host_interrupt()) {
		udelay(HECI_WAIT_DELAY);
		if (stopwatch_expired(&sw))
			return 0;
	}

	return 1;
}

static void host_gen_interrupt(void)
{
	uint32_t csr;
	csr = read_host_csr();
	csr |= CSR_IG;
	write_host_csr(csr);
}

static void txe_set_host_ready(void)
{
	uint32_t csr;
	csr = read_host_csr();
	csr &= ~CSR_RESET;
	csr |= (CSR_IG | CSR_READY);
	write_host_csr(csr);
}

static bool check_heci_reset(void)
{
	return (read_bar(PCI_DEV(0, TXE_DEV, TXE_FUNC), TXE_MMIO_SEC_IPC_READINESS) == 0);
}

static int heci_reset(void)
{
	uint32_t csr;

	/* Send reset request */
	csr = read_host_csr();
	csr |= (CSR_RESET | CSR_IG);
	write_host_csr(csr);

	if (!wait_host_reset_started()) {
		printk(BIOS_CRIT, "HECI: reset failed\n");
		return 0;
	}

	if (wait_heci_ready()) {
		if (!wait_host_interrupt()) {
			/* Device is back on its imaginary feet, clear reset */
			txe_set_host_ready();
			return 1;
		}
	}

	printk(BIOS_CRIT, "HECI: reset failed\n");

	return 0;
}

static size_t filled_slots(uint32_t data)
{
	uint8_t wp, rp;
	rp = data >> CSR_RP_START;
	wp = data >> CSR_WP_START;
	return (uint8_t)(wp - rp);
}

static size_t txe_filled_slots(void)
{
	return filled_slots(read_txe_csr());
}

static size_t host_empty_slots(void)
{
	uint32_t csr;
	csr = read_host_csr();

	return ((csr & CSR_CBD) >> CSR_CBD_START) - filled_slots(csr);
}

static uint32_t read_slot(uint32_t idx)
{
	return read_bar(PCI_DEV(0, TXE_DEV, TXE_FUNC), TXE_MMIO_IPC_INPUT_PAYLOAD + idx);
}

static void write_slot(uint32_t idx, uint32_t val)
{
	write_bar(PCI_DEV(0, TXE_DEV, TXE_FUNC), TXE_MMIO_IPC_INPUT_PAYLOAD + idx, val);
}

static void set_ipc_input_doorbell(void)
{
	uint32_t ipc_input_doorbell;
	ipc_input_doorbell = read_bar(PCI_DEV(0, TXE_DEV, TXE_FUNC), TXE_MMIO_SEC_IPC_INPUT_DRBELL);
	write_bar(PCI_DEV(0, TXE_DEV, TXE_FUNC), TXE_MMIO_SEC_IPC_INPUT_DRBELL, ipc_input_doorbell | 1);
}

static int wait_write_slots(size_t cnt)
{
	struct stopwatch sw;

	stopwatch_init_usecs_expire(&sw, HECI_SEND_TIMEOUT);
	while (host_empty_slots() < cnt) {
		udelay(HECI_WAIT_DELAY);
		if (stopwatch_expired(&sw)) {
			printk(BIOS_ERR, "HECI: timeout, buffer not drained\n");
			return 0;
		}
	}
	return 1;
}

static int ipc_output_ready(void)
{
	uint32_t ipc_output_sts;
	ipc_output_sts = read_bar(PCI_DEV(0, TXE_DEV, TXE_FUNC), TXE_MMIO_SEC_IPC_OUTPUT_STS);
	return ipc_output_sts & 1;
}

static void set_ipc_output_ready(void)
{
	uint32_t ipc_output_sts;
	ipc_output_sts = read_bar(PCI_DEV(0, TXE_DEV, TXE_FUNC), TXE_MMIO_SEC_IPC_OUTPUT_STS);
	write_bar(PCI_DEV(0, TXE_DEV, TXE_FUNC), TXE_MMIO_SEC_IPC_OUTPUT_STS, ipc_output_sts | 1);
}

static int wait_read_slots(size_t cnt)
{
	struct stopwatch sw;

	stopwatch_init_usecs_expire(&sw, HECI_READ_TIMEOUT);
	while (true) {
		udelay(HECI_WAIT_DELAY);
		if (stopwatch_expired(&sw)) {
			printk(BIOS_ERR, "HECI: timed out reading answer!\n");
			return 0;
		}

		if (!ipc_output_ready())
			continue;

		if (txe_filled_slots() >= cnt)
			break;

	}
	return 1;
}


static size_t hdr_get_length(uint32_t hdr)
{
	return (hdr & TXEI_HDR_LENGTH) >> TXEI_HDR_LENGTH_START;
}

/* get number of full 4-byte slots */
static size_t bytes_to_slots(size_t bytes)
{
	return ALIGN_UP(bytes, SLOT_SIZE) / SLOT_SIZE;
}

static int
send_one_message(uint32_t hdr, const void *buff)
{
	size_t pend_len, pend_slots, remainder, i;
	uint32_t tmp = 0;
	const uint32_t *p = buff;

	if (!wait_heci_ipc_ready())
		return 0;

	/* Get space for the header */
	if (!wait_write_slots(1))
		return 0;

	/* First, write header */
	write_slot(0, hdr);

	pend_len = hdr_get_length(hdr);
	pend_slots = bytes_to_slots(pend_len);

	if (!wait_write_slots(pend_slots))
		return 0;

	/* Write the body in whole slots */
	i = 0;
	while (i < ALIGN_DOWN(pend_len, SLOT_SIZE)) {
		write_slot(i + 4, *p++);
		i += SLOT_SIZE;
	}

	remainder = pend_len % SLOT_SIZE;
	/* Pad to 4 bytes not touching caller's buffer */
	if (remainder) {
		memcpy(&tmp, p, remainder);
		write_slot(i + 4, tmp);
	}

	set_ipc_input_doorbell();
	host_gen_interrupt();

	/* Make sure nothing bad happened during transmission */
	if (!txe_ready())
		return 0;

	return pend_len;
}

static bool heci_can_send(void)
{
	pci_devfn_t dev = PCI_DEV(0, TXE_DEV, TXE_FUNC);
	uint32_t fw_sts0;

	if (!is_txe_devfn_visible())
		return false;

	if (txe_is_excluded_from_bios_flows()) {
		printk(BIOS_INFO, "TXE excluded from platform flows\n");
		printk(BIOS_ERR, "Can not send HECI messages\n");
		return false;
	}

	if (txe_has_error()) {
		printk(BIOS_ERR, "TXE Error occurred, not sending HECI message\n");
		return false;
	}

	if (txe_fw_update_in_progress()) {
		printk(BIOS_INFO, "TXE FW is updating, not sending HECI message\n");
		return false;
	}

	fw_sts0 = pci_read_config32(dev, TXE_FWSTS0);

	switch ((fw_sts0 & TXE_CURR_OPMODE_MASK) >> TXE_CURR_OPMODE_SHIFT) {
	case TXE_FWSTS0_COM_NORMAL:
	case TXE_FWSTS0_COM_SECOVER_TXEI_MSG:
		break;
	case TXE_FWSTS0_COM_DEBUG:
	case TXE_FWSTS0_COM_SOFT_TEMP_DISABLE:
	case TXE_FWSTS0_COM_SECOVER_JMPR:
		printk(BIOS_INFO, "Current TXE operation mode does not allow"
				  " sending HECI messages\n");
		return false;
	}

	return true;
}

static enum txe_tx_rx_status
heci_send(const void *msg, size_t len, uint8_t host_addr, uint8_t client_addr)
{
	uint8_t retry;
	uint32_t hdr;
	size_t sent, remaining, cb_size, max_length;
	const uint8_t *p;

	if (!msg || !len)
		return TXE_TX_ERR_INPUT;

	if (!heci_can_send())
		return TXE_TX_ERR_INPUT;

	for (retry = 0; retry < HECI_MAX_RETRY; retry++) {
		p = msg;

		if (check_heci_reset())
			txe_txei_init();

		if (!wait_heci_ipc_ready()) {
			printk(BIOS_ERR, "HECI: not ready\n");
			continue;
		}

		cb_size = TXEI_CIRCULAR_BUFFER_SIZE;
		/*
		 * Reserve one slot for the header. Limit max message
		 * length by 9 bits that are available in the header.
		 */
		max_length = MIN(cb_size, (1 << TXEI_HDR_LENGTH_SIZE) - 1)
				- SLOT_SIZE;
		remaining = len;

		/*
		 * Fragment the message into smaller messages not exceeding
		 * useful circular buffer length. Mark last message complete.
		 */
		do {
			hdr = MIN(max_length, remaining)
				<< TXEI_HDR_LENGTH_START;
			hdr |= client_addr << TXEI_HDR_TXE_ADDR_START;
			hdr |= host_addr << TXEI_HDR_HOST_ADDR_START;
			hdr |= (MIN(max_length, remaining) == remaining) ?
						TXEI_HDR_IS_COMPLETE : 0;
			sent = send_one_message(hdr, p);
			p += sent;
			remaining -= sent;
		} while (remaining > 0 && sent != 0);

		if (!remaining)
			return TXE_TX_RX_SUCCESS;
	}

	printk(BIOS_DEBUG, "HECI: Trigger HECI reset\n");
	heci_reset();
	return TXE_TX_ERR_TXE_NOT_READY;
}

static enum txe_tx_rx_status
recv_one_message(uint32_t *hdr, void *buff, size_t maxlen, size_t *recv_len)
{
	uint32_t reg, *p = buff;
	size_t recv_slots, remainder, i;

	/* first get the header */
	if (!wait_read_slots(1))
		return TXE_RX_ERR_TIMEOUT;

	*hdr = read_slot(0);
	*recv_len = hdr_get_length(*hdr);

	if (!*recv_len)
		printk(BIOS_WARNING, "HECI: message is zero-sized\n");

	recv_slots = bytes_to_slots(*recv_len);

	i = 0;
	if (*recv_len > maxlen) {
		printk(BIOS_ERR, "HECI: response is too big\n");
		return TXE_RX_ERR_RESP_LEN_MISMATCH;
	}

	/* wait for the rest of messages to arrive */
	wait_read_slots(recv_slots);

	/* fetch whole slots first */
	while (i < ALIGN_DOWN(*recv_len, SLOT_SIZE)) {
		*p++ = read_slot(i + 4);
		i += SLOT_SIZE;
	}

	/*
	 * If TXE is not ready, something went wrong and
	 * we received junk
	 */
	if (!txe_ready())
		return TXE_RX_ERR_TXE_NOT_READY;

	remainder = *recv_len % SLOT_SIZE;

	if (remainder) {
		reg = read_slot(i + 4);
		memcpy(p, &reg, remainder);
	}
	set_ipc_output_ready();
	return TXE_TX_RX_SUCCESS;
}

static enum txe_tx_rx_status heci_receive(void *buff, size_t *maxlen)
{
	uint8_t retry;
	size_t left, received;
	uint32_t hdr = 0;
	uint8_t *p;
	enum txe_tx_rx_status ret = TXE_RX_ERR_TIMEOUT;

	if (!buff || !maxlen || !*maxlen)
		return TXE_RX_ERR_INPUT;

	for (retry = 0; retry < HECI_MAX_RETRY; retry++) {
		p = buff;
		left = *maxlen;

		if (check_heci_reset())
			txe_txei_init();

		if (!wait_heci_ipc_ready()) {
			printk(BIOS_ERR, "HECI: not ready\n");
			break;
		}

		/*
		 * Receive multiple packets until we meet one marked
		 * complete or we run out of space in caller-provided buffer.
		 */
		do {
			ret = recv_one_message(&hdr, p, left, &received);
			if (ret) {
				printk(BIOS_ERR, "HECI: Failed to receive!\n");
				goto TXE_RX_ERR_HANDLE;
			}
			left -= received;
			p += received;
			/* If we read out everything ping to send more */
			if (!(hdr & TXEI_HDR_IS_COMPLETE) && !txe_filled_slots())
				host_gen_interrupt();
		} while (received && !(hdr & TXEI_HDR_IS_COMPLETE) && left > 0);

		if ((hdr & TXEI_HDR_IS_COMPLETE) && received) {
			*maxlen = p - (uint8_t *)buff;
			return TXE_TX_RX_SUCCESS;
		}
	}

TXE_RX_ERR_HANDLE:
	printk(BIOS_DEBUG, "HECI: Trigger HECI Reset\n");
	heci_reset();
	return TXE_RX_ERR_TXE_NOT_READY;
}

static enum txe_tx_rx_status heci_send_receive(const void *snd_msg, size_t snd_sz, void *rcv_msg,
					size_t *rcv_sz, uint8_t txe_addr)
{
	enum txe_tx_rx_status ret;

	ret = heci_send(snd_msg, snd_sz, BIOS_HOST_ADDR, txe_addr);
	if (ret) {
		printk(BIOS_ERR, "HECI: send Failed\n");
		return ret;
	}

	if (rcv_msg != NULL) {
		ret = heci_receive(rcv_msg, rcv_sz);
		if (ret) {
			printk(BIOS_ERR, "HECI: receive Failed\n");
			return ret;
		}
	}
	return ret;
}

static bool txe_cws_normal(void)
{
	if (!is_txe_devfn_visible())
		return false;

	uint32_t reg = pci_read_config32(PCI_DEV(0, TXE_DEV, TXE_FUNC), TXE_FWSTS0);

	return ((reg & TXE_CWS_MASK) == TXE_FWSTS0_CWS_NORMAL);
}

static bool txe_com_normal(void)
{
	if (!is_txe_devfn_visible())
		return false;

	uint32_t reg = pci_read_config32(PCI_DEV(0, TXE_DEV, TXE_FUNC), TXE_FWSTS0);
	reg &= TXE_CURR_OPMODE_MASK;
	reg >>= TXE_CURR_OPMODE_SHIFT;

	return (reg == TXE_FWSTS0_COM_NORMAL);
}

static bool txe_did_done(void)
{
	if (!is_txe_devfn_visible())
		return false;

	uint32_t reg = pci_read_config32(PCI_DEV(0, TXE_DEV, TXE_FUNC), TXE_DID_MSG);

	return ((reg & DID_MSG_DID_DONE) == DID_MSG_DID_DONE);
}

int txe_global_reset(void)
{
	int status;
	struct mkhi_hdr reply;
	struct reset_message {
		struct mkhi_hdr hdr;
		uint8_t req_origin;
		uint8_t reset_type;
	} __packed;
	struct reset_message msg = {
		.hdr = {
			.group_id = MKHI_CBM_GROUP_ID,
			.command = MKHI_CBM_GLOBAL_RESET,
		},
		.req_origin = GR_ORIGIN_BIOS_POST,
		.reset_type = 1
	};
	size_t reply_size;

	printk(BIOS_DEBUG, "HECI: Global Reset Command\n");

	if (!txe_cws_normal()) {
		printk(BIOS_ERR, "HECI: TXE does not meet required prerequisites\n");
		return 0;
	}

	heci_reset();

	reply_size = sizeof(reply);
	memset(&reply, 0, reply_size);

	status = heci_send_receive(&msg, sizeof(msg), &reply, &reply_size, HECI_MKHI_ADDR);

	printk(BIOS_DEBUG, "HECI: Global Reset %s!\n", !status ? "success" : "failure");
	return status;
}

static enum txe_cmd_result decode_heci_send_receive_error(enum txe_tx_rx_status ret)
{
	switch (ret) {
	case TXE_TX_ERR_TXE_NOT_READY:
	case TXE_RX_ERR_TXE_NOT_READY:
	case TXE_RX_ERR_RESP_LEN_MISMATCH:
	case TXE_RX_ERR_TIMEOUT:
		return TXE_CMD_RESULT_RETRY;
	default:
		return TXE_CMD_RESULT_ERROR;
	}
}


static enum txe_cmd_result txe_receive_eop(void)
{
	enum {
		EOP_REQUESTED_ACTION_CONTINUE = 0,
		EOP_REQUESTED_ACTION_GLOBAL_RESET = 1,
	};
	enum txe_tx_rx_status ret;
	struct end_of_post_resp {
		/* FIXME: why this volatile is neded to read the hdr fields properly? */
		volatile struct mkhi_hdr hdr;
		uint32_t requested_actions;
	} __packed resp = {};
	size_t resp_size = sizeof(resp);

	ret = heci_receive(&resp, &resp_size);
	if (ret)
		return decode_heci_send_receive_error(ret);

	if (resp.hdr.group_id != MKHI_GEN_GROUP_ID ||
	    resp.hdr.command != MKHI_END_OF_POST) {
		printk(BIOS_ERR, "HECI: EOP Unexpected response group or command.\n");
		return TXE_CMD_RESULT_ERROR;
	}

	if (resp.hdr.result) {
		printk(BIOS_ERR, "HECI: EOP Resp Failed: %u\n", resp.hdr.result);
		return TXE_CMD_RESULT_ERROR;
	}

	printk(BIOS_INFO, "TXE: EOP requested action: ");

	switch (resp.requested_actions) {
	case EOP_REQUESTED_ACTION_GLOBAL_RESET:
		printk(BIOS_INFO, "global reset\n");
		return TXE_CMD_RESULT_GLOBAL_RESET_REQUESTED;
	case EOP_REQUESTED_ACTION_CONTINUE:
		printk(BIOS_INFO, "continue boot\n");
		return TXE_CMD_RESULT_SUCCESS;
	default:
		printk(BIOS_INFO, "unknown %u\n", resp.requested_actions);
		return TXE_CMD_RESULT_ERROR;
	}
}

static enum txe_cmd_result txe_send_eop(void)
{
	enum txe_tx_rx_status ret;
	struct end_of_post_msg {
		struct mkhi_hdr hdr;
	} __packed msg = {
		.hdr = {
			.group_id = MKHI_GEN_GROUP_ID,
			.command = MKHI_END_OF_POST,
		},
	};

	/*
	 * Prerequisites:
	 * 1) fwsts0 CWS is Normal
	 * 2) fwsts0 COM is Normal
	 * 3) Only sent after DID
	 */
	if (!txe_cws_normal() || !txe_com_normal() || !txe_did_done()) {
		printk(BIOS_ERR, "HECI: Prerequisites not met for sending EOP\n");
		return TXE_CMD_RESULT_ERROR;
	}

	printk(BIOS_INFO, "HECI: Sending End-of-Post\n");

	ret = heci_send(&msg, sizeof(msg), BIOS_HOST_ADDR, HECI_MKHI_ADDR);
	if (ret)
		return decode_heci_send_receive_error(ret);

	return TXE_CMD_RESULT_SUCCESS;
}

static enum txe_cmd_result txe_send_and_receive_eop(void)
{
	enum txe_cmd_result ret;

	ret = txe_send_eop();
	if (ret != TXE_CMD_RESULT_SUCCESS)
		return ret;

	return txe_receive_eop();
}

static enum txe_cmd_result txe_send_cmd_retries(enum txe_cmd_result (*txe_send_command)(void))
{
	size_t retry;
	enum txe_cmd_result ret;
	for (retry = 0; retry < HECI_MAX_RETRY; retry++) {
		ret = txe_send_command();
		if (ret != TXE_CMD_RESULT_RETRY)
			break;
	}
	return ret;
}

/*
 * On EOP error, the BIOS is required to send an MEI bus disable message to the
 * TXE, followed by disabling all TXEI devices. After successfully completing
 * this, it is safe to boot.
 */
static void txe_handle_eop_error(void)
{
	txe_hide_device();
}

static void handle_txe_eop_result(enum txe_cmd_result result)
{
	switch (result) {
	case TXE_CMD_RESULT_GLOBAL_RESET_REQUESTED:
		printk(BIOS_INFO, "TXE requested global reset in EOP response, resetting...\n");
		global_reset();
		break;
	case TXE_CMD_RESULT_SUCCESS:
		printk(BIOS_INFO, "TXE EOP successful, continuing boot\n");
		break;
	case TXE_CMD_RESULT_DISABLED:
		printk(BIOS_INFO, "TXE is disabled, continuing boot\n");
		break;
	case TXE_CMD_RESULT_ERROR: /* fallthrough */
	default:
		printk(BIOS_ERR, "Failed to send EOP to TXE, %d\n", result);

		/* In non-vboot builds or recovery mode, follow the BWG in order
		   to continue to boot securely. */
		txe_handle_eop_error();
		break;
	}
}

void txe_do_send_end_of_post(void)
{
	enum txe_cmd_result ret;

	if (platform_is_resuming()) {
		printk(BIOS_INFO, "Skip sending EOP during S3 resume\n");
		return;
	}

	ret = txe_send_cmd_retries(txe_send_and_receive_eop);
	handle_txe_eop_result(ret);
}

void txe_print_secure_boot_status(void)
{
	union txe_sbsts sb_sts;

	if (!is_txe_devfn_visible())
		return;

	sb_sts.data = pci_read_config32(PCI_DEV(0, TXE_DEV, TXE_FUNC), TXE_SECURE_BOOT_STS);

	printk(BIOS_INFO, "TXE: Secure Boot flow executed: %s "
			  "(valid only if Global Valid Fuse is set)\n",
			  sb_sts.fields.sb_executed ? "YES" : "NO");
	printk(BIOS_INFO, "TXE: Recovery flow executed:    %s "
			  "(valid only if Global Valid Fuse is set)\n",
			  sb_sts.fields.recovery ? "YES" : "NO");
	printk(BIOS_INFO, "TXE: Debug was enabled:         %s\n",
			  sb_sts.fields.debug_was_enabled ? "YES" : "NO");
	printk(BIOS_INFO, "TXE: Debug is enabled:          %s\n",
			  sb_sts.fields.debug_is_enabled ? "YES" : "NO");
	printk(BIOS_INFO, "TXE: Emulation mode:            %s\n",
			  sb_sts.fields.emulation_mode ? "YES" : "NO");
	printk(BIOS_INFO, "TXE: Secure Boot Manifest SVN:  0x%x\n",
			  sb_sts.fields.sbm_svn);
	printk(BIOS_INFO, "TXE: Key Manifest ID:           0x%x\n",
			  sb_sts.fields.km_id);
	printk(BIOS_INFO, "TXE: Key Manifest SVN:          0x%x\n",
			  sb_sts.fields.km_svn);
	printk(BIOS_INFO, "TXE: Alternative BIOS Limit:    %08x\n",
			  sb_sts.fields.alt_bios_limit << 13);
}

static const char * const txe_bios_actions[] =
{
	"No DRAM Init Done ACK received",
	"Non power cycle reset",
	"Power cycle reset",
	"Go to S3",
	"Go to S4",
	"Go to S5",
	"Perform Global Reset",
	"Continue Boot"
};

static const char * const txe_cws_values[] = {
	[0 ... 15 ] = "Reserved",
	[TXE_FWSTS0_CWS_RESET] = "RESET",
	[TXE_FWSTS0_CWS_INIT] = "INIT",
	[TXE_FWSTS0_CWS_RECOVERY] = "RECOVERY",
	[TXE_FWSTS0_CWS_NORMAL] = "NORMAL",
};

static const char * const txe_com_values[] = {
	[0 ... 15 ] = "Reserved",
	[TXE_FWSTS0_COM_NORMAL]			= "Normal",
	[TXE_FWSTS0_COM_DEBUG]			= "Debug",
	[TXE_FWSTS0_COM_SOFT_TEMP_DISABLE]	= "Soft Temporary Disable",
	[TXE_FWSTS0_COM_SECOVER_JMPR]		= "Security Override via Jumper",
	[TXE_FWSTS0_COM_SECOVER_TXEI_MSG]	= "Security Override via MEI Message"
};

static const char * const txe_opstate_values[] = {
	[0 ... 7] = "Reserved",
	[TXE_FWSTS0_STATE_PREBOOT]	= "Preboot",
	[TXE_FWSTS0_STATE_M0_UMA]	= "M0 with UMA",
	[TXE_FWSTS0_STATE_MOFF]		= "MOff",
	[TXE_FWSTS0_STATE_M0]		= "M0 without UMA",
	[TXE_FWSTS0_STATE_BRINGUP]	= "Bring up",
	[TXE_FWSTS0_STATE_ERROR]	= "M0 without UMA but with error"
};

static const char * const txe_pm_event_values[] = {
	[0 ... 15] = "Reserved",
	[0] = "Clean Moff --> M0 wake",
	[1] = "Moff --> M0 wake after error",
	[2] = "Clean global reset",
	[3] = "Global Reset after an error",
	[4] = "Clean SeC reset",
	[5] = "SeC reset due to exception",
	[6] = "Pseudo-global reset",
	[11] = "Power cycle reset through MOff",
	[13] = "S0/M0 --> S0/MOff"
};

static const char * const txe_error_code_values[] = {
	[0 ... 15] = "Reserved",
	[0] = "No Error",
	[1] = "Uncategorized failure (see Extended Error Code)",
	[3] = "Image Failure",
	[4] = "Debug Failure",
	[5] = "Internal ROM Error",
	[6] = "Invalid fuse configuration",
	[7] = "Failed to access SPI flash",
};

static const char * const txe_progress_code_values[] = {
	[0 ... 15] = "Reserved",
	[0] = "ROM",
	[1] = "Bringup",
	[2] = "uKernel",
	[3] = "Policy module",
	[4] = "Module loading",
	[6] = "Host Communication",
};

void dump_txe_status(void)
{
	union txe_fwsts0 fwsts0;
	union txe_fwsts1 fwsts1;

	if (!is_txe_devfn_visible())
		return;

	fwsts0.data = pci_read_config32(PCI_DEV(0, TXE_DEV, TXE_FUNC), TXE_FWSTS0);
	fwsts1.data = pci_read_config32(PCI_DEV(0, TXE_DEV, TXE_FUNC), TXE_FWSTS1);


	printk(BIOS_DEBUG, "TXE: FWSTS0                      : 0x%08X\n", fwsts0.data);
	printk(BIOS_DEBUG, "TXE: FWSTS1                      : 0x%08X\n", fwsts1.data);

	printk(BIOS_DEBUG, "TXE: Manufacturing Mode          : %s\n",
		fwsts0.fields.mfg_mode ? "YES" : "NO");
	printk(BIOS_DEBUG, "TXE: FW Partition Table          : %s\n",
		fwsts0.fields.fpt_bad ? "BAD" : "OK");
	printk(BIOS_DEBUG, "TXE: Bringup Loader Failure      : %s\n",
		fwsts0.fields.ft_bup_ld_flr ? "YES" : "NO");
	printk(BIOS_DEBUG, "TXE: Firmware Init Complete      : %s\n",
		fwsts0.fields.fw_init_complete ? "YES" : "NO");
	printk(BIOS_DEBUG, "TXE: Update In Progress          : %s\n",
		fwsts0.fields.update_in_progress ? "YES" : "NO");
	printk(BIOS_DEBUG, "TXE: Current Working State       : %u (%s)\n",
		fwsts0.fields.working_state,
		txe_cws_values[fwsts0.fields.working_state]);
	printk(BIOS_DEBUG, "TXE: Current Operation State     : %u (%s)\n",
		fwsts0.fields.operation_state,
		txe_opstate_values[fwsts0.fields.operation_state]);
	printk(BIOS_DEBUG, "TXE: Current Operation Mode      : %u (%s)\n",
		fwsts0.fields.operation_mode,
		txe_com_values[fwsts0.fields.operation_mode]);
	printk(BIOS_DEBUG, "TXE: DRAM Init Done ACK          : %u\n",
		fwsts0.fields.bios_msg_ack);
	printk(BIOS_DEBUG, "TXE: BIOS Action                 : %s\n",
		txe_bios_actions[fwsts0.fields.ack_data]);
	printk(BIOS_DEBUG, "TXE: Progress Code               : %u (%s)\n",
		fwsts1.fields.progress_code,
		txe_progress_code_values[fwsts1.fields.progress_code]);
	printk(BIOS_DEBUG, "TXE: PM Event                    : %u (%s)\n",
		fwsts1.fields.current_pmevent,
		txe_pm_event_values[fwsts1.fields.current_pmevent]);
	printk(BIOS_DEBUG, "TXE: Error Code                  : %u (%s)\n",
		fwsts0.fields.error_code,
		txe_error_code_values[fwsts0.fields.error_code]);
	printk(BIOS_DEBUG, "TXE: Extended Error Code         : %u\n",
		fwsts1.fields.current_state);
}
