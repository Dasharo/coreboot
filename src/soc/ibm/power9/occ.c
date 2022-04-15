/* SPDX-License-Identifier: GPL-2.0-only */

#include <console/console.h>
#include <cpu/power/mvpd.h>
#include <cpu/power/scom.h>
#include <cpu/power/occ.h>
#include <delay.h>
#include <lib.h>
#include <string.h>		// memset, memcpy
#include <timer.h>
#include <vendorcode/ibm/power9/pstates/p9_pstates_occ.h>

#include <cpu/power/proc.h>

#include "homer.h"
#include "ops.h"

#define OCB_PIB_OCBCSR0_OCB_STREAM_MODE 4
#define OCB_PIB_OCBCSR0_OCB_STREAM_TYPE 5

#define OCB_OCI_OCBSHCS0_PUSH_ENABLE 31
#define OCB_OCI_OCBSHCS0_PUSH_FULL   0

#define PU_OCB_PIB_OCBCSR0_RO 0x0006D011
#define PU_OCB_PIB_OCBCSR1_RO 0x0006D031

#define PU_OCB_OCI_OCBSHCS0_SCOM 0x0006C204
#define PU_OCB_OCI_OCBSHCS1_SCOM 0x0006C214

#define EX_PPM_SPWKUP_OCC 0x200F010C
#define PU_OCB_PIB_OCBAR0 0x0006D010

#define PU_OCB_PIB_OCBDR0 0x0006D015
#define PU_OCB_PIB_OCBDR1 0x0006D035

#define PU_OCB_PIB_OCBCSR0_OR 0x0006D013
#define PU_OCB_PIB_OCBCSR0_CLEAR 0x0006D012

#define OCC_CMD_ADDR 0x000E0000
#define OCC_RSP_ADDR 0x000E1000

#define OCC_CMD_POLL            0x00
#define OCC_CMD_CLEAR_ERROR_LOG 0x12
#define OCC_CMD_SET_STATE       0x20
#define OCC_CMD_SETUP_CFG_DATA  0x21
#define OCC_CMD_SET_POWER_CAP   0x22

#define OCC_RC_SUCCESS             0x00
#define OCC_RC_INIT_FAILURE        0xE5
#define OCC_RC_OCC_INIT_CHECKPOINT 0xE1

#define OCC_CFGDATA_FREQ_POINT    0x02
#define OCC_CFGDATA_OCC_ROLE      0x03
#define OCC_CFGDATA_APSS_CONFIG   0x04
#define OCC_CFGDATA_MEM_CONFIG    0x05
#define OCC_CFGDATA_PCAP_CONFIG   0x07
#define OCC_CFGDATA_SYS_CONFIG    0x0F
#define OCC_CFGDATA_TCT_CONFIG    0x13
#define OCC_CFGDATA_AVSBUS_CONFIG 0x14
#define OCC_CFGDATA_GPU_CONFIG    0x15

/* FIR register offset from base */
enum fir_offset {
	BASE_WAND_INCR = 1,
	BASE_WOR_INCR  = 2,
	MASK_INCR      = 3,
	MASK_WAND_INCR = 4,
	MASK_WOR_INCR  = 5,
	ACTION0_INCR   = 6,
	ACTION1_INCR   = 7
};

struct occ_cfg_inputs {
	struct homer_st *homer;
	uint8_t chip;
	bool is_master_occ;
};

struct occ_cfg_info {
	const char *name;
	void (*func)(const struct occ_cfg_inputs *inputs, uint8_t *data, uint16_t *size);
	bool to_master_only;
};

struct occ_poll_response {
	uint8_t  status;
	uint8_t  ext_status;
	uint8_t  occs_present;
	uint8_t  requested_cfg;
	uint8_t  state;
	uint8_t  mode;
	uint8_t  ips_status;
	uint8_t  error_id;
	uint32_t error_address;
	uint16_t error_length;
	uint8_t  error_source;
	uint8_t  gpu_cfg;
	uint8_t  code_level[16];
	uint8_t  sensor[6];
	uint8_t  num_blocks;
	uint8_t  version;
	uint8_t  sensor_data[];	// 4049 bytes
} __attribute__((packed));

static void pm_ocb_setup(uint8_t chip, uint32_t ocb_bar)
{
	write_scom(chip, PU_OCB_PIB_OCBCSR0_OR, PPC_BIT(OCB_PIB_OCBCSR0_OCB_STREAM_MODE));
	write_scom(chip, PU_OCB_PIB_OCBCSR0_CLEAR, PPC_BIT(OCB_PIB_OCBCSR0_OCB_STREAM_TYPE));
	write_scom(chip, PU_OCB_PIB_OCBAR0, (uint64_t)ocb_bar << 32);
}

static void check_ocb_mode(uint8_t chip, uint64_t ocb_csr_address, uint64_t ocb_shcs_address)
{
	uint64_t ocb_pib = read_scom(chip, ocb_csr_address);

	/*
	 * The following check for circular mode is an additional check
	 * performed to ensure a valid data access.
	 */
	if ((ocb_pib & PPC_BIT(OCB_PIB_OCBCSR0_OCB_STREAM_MODE)) &&
	    (ocb_pib & PPC_BIT(OCB_PIB_OCBCSR0_OCB_STREAM_TYPE))) {
		/*
		 * Check if push queue is enabled. If not, let the store occur
		 * anyway to let the PIB error response return occur. (That is
		 * what will happen if this checking code were not here.)
		 */
		uint64_t stream_push_ctrl = read_scom(chip, ocb_shcs_address);

		if (stream_push_ctrl & PPC_BIT(OCB_OCI_OCBSHCS0_PUSH_ENABLE)) {
			uint8_t counter = 0;
			for (counter = 0; counter < 4; counter++) {
				/* Proceed if the OCB_OCI_OCBSHCS0_PUSH_FULL is clear */
				if (!(stream_push_ctrl & PPC_BIT(OCB_OCI_OCBSHCS0_PUSH_FULL)))
					break;

				stream_push_ctrl = read_scom(chip, ocb_shcs_address);
			}

			if (counter == 4)
				die("Failed to write to circular buffer.\n");
		}
	}
}

static void put_ocb_indirect(uint8_t chip, uint32_t ocb_req_length,
			     uint32_t oci_address, uint64_t *ocb_buffer)
{
	write_scom(chip, PU_OCB_PIB_OCBAR0, (uint64_t)oci_address << 32);

	check_ocb_mode(chip, PU_OCB_PIB_OCBCSR0_RO, PU_OCB_OCI_OCBSHCS0_SCOM);

	for (uint32_t index = 0; index < ocb_req_length; index++)
		write_scom(chip, PU_OCB_PIB_OCBDR0, ocb_buffer[index]);
}

static void get_ocb_indirect(uint8_t chip, uint32_t ocb_req_length,
			     uint32_t oci_address, uint64_t *ocb_buffer)
{
	write_scom(chip, PU_OCB_PIB_OCBAR0, (uint64_t)oci_address << 32);
	for (uint32_t loopCount = 0; loopCount < ocb_req_length; loopCount++)
		ocb_buffer[loopCount] = read_scom(chip, PU_OCB_PIB_OCBDR0);
}

static void write_occ_sram(uint8_t chip, uint32_t address, uint64_t *buffer, size_t data_length)
{
	pm_ocb_setup(chip, address);
	put_ocb_indirect(chip, data_length / 8, address, buffer);
}

static void read_occ_sram(uint8_t chip, uint32_t address, uint64_t *buffer, size_t data_length)
{
	pm_ocb_setup(chip, address);
	get_ocb_indirect(chip, data_length / 8, address, buffer);
}

static void write_occ_command(uint8_t chip, uint64_t write_data)
{
	check_ocb_mode(chip, PU_OCB_PIB_OCBCSR1_RO, PU_OCB_OCI_OCBSHCS1_SCOM);
	write_scom(chip, PU_OCB_PIB_OCBDR1, write_data);
}

void clear_occ_special_wakeups(uint8_t chip, uint64_t cores)
{
	for (size_t i = 0; i < MAX_CORES_PER_CHIP; i += 2) {
		if (!IS_EX_FUNCTIONAL(i, cores))
			continue;
		scom_and_for_chiplet(chip, EC00_CHIPLET_ID + i, EX_PPM_SPWKUP_OCC,
				     ~PPC_BIT(0));
	}
}

void special_occ_wakeup_disable(uint8_t chip, uint64_t cores)
{
	enum { PPM_SPWKUP_FSP = 0x200F010B };

	for (int i = 0; i < MAX_CORES_PER_CHIP; ++i) {
		if (!IS_EC_FUNCTIONAL(i, cores))
			continue;

		write_scom_for_chiplet(chip, EC00_CHIPLET_ID + i, PPM_SPWKUP_FSP, 0);
		/* This puts an inherent delay in the propagation of the reset transition */
		(void)read_scom_for_chiplet(chip, EC00_CHIPLET_ID + i, PPM_SPWKUP_FSP);
	}
}

/* Sets up boot loader in SRAM and returns 32-bit jump instruction to it */
static uint64_t setup_memory_boot(uint8_t chip)
{
	enum {
		OCC_BOOT_OFFSET = 0x40,
		CTR = 9,
		OCC_SRAM_BOOT_ADDR = 0xFFF40000,
		OCC_SRAM_BOOT_ADDR2 = 0xFFF40002,
	};

	uint64_t sram_program[2];

	/* lis r1, 0x8000 */
	sram_program[0] = ((uint64_t)ppc_lis(1, 0x8000) << 32);

	/* ori r1, r1, OCC_BOOT_OFFSET */
	sram_program[0] |= ppc_ori(1, 1, OCC_BOOT_OFFSET);

	/* mtctr (mtspr r1, CTR) */
	sram_program[1] = ((uint64_t)ppc_mtspr(1, CTR) << 32);

	/* bctr */
	sram_program[1] |= ppc_bctr();

	/* Write to SRAM */
	write_occ_sram(chip, OCC_SRAM_BOOT_ADDR, sram_program, sizeof(sram_program));

	return ((uint64_t)ppc_b(OCC_SRAM_BOOT_ADDR2) << 32);
}

void occ_start_from_mem(uint8_t chip)
{
	enum {
		OCB_PIB_OCR_CORE_RESET_BIT = 0,
		JTG_PIB_OJCFG_DBG_HALT_BIT = 6,

		PU_SRAM_SRBV0_SCOM = 0x0006A004,

		PU_JTG_PIB_OJCFG_AND = 0x0006D005,
		PU_OCB_PIB_OCR_CLEAR = 0x0006D001,
		PU_OCB_PIB_OCR_OR    = 0x0006D002,
	};

	write_scom(chip, PU_OCB_PIB_OCBCSR0_OR, PPC_BIT(OCB_PIB_OCBCSR0_OCB_STREAM_MODE));

	/*
	 * Set up Boot Vector Registers in SRAM:
	 *  - set bv0-2 to all 0's (illegal instructions)
	 *  - set bv3 to proper branch instruction
	 */
	write_scom(chip, PU_SRAM_SRBV0_SCOM, 0);
	write_scom(chip, PU_SRAM_SRBV0_SCOM + 1, 0);
	write_scom(chip, PU_SRAM_SRBV0_SCOM + 2, 0);
	write_scom(chip, PU_SRAM_SRBV0_SCOM + 3, setup_memory_boot(chip));

	write_scom(chip, PU_JTG_PIB_OJCFG_AND, ~PPC_BIT(JTG_PIB_OJCFG_DBG_HALT_BIT));
	write_scom(chip, PU_OCB_PIB_OCR_OR, PPC_BIT(OCB_PIB_OCR_CORE_RESET_BIT));
	write_scom(chip, PU_OCB_PIB_OCR_CLEAR, PPC_BIT(OCB_PIB_OCR_CORE_RESET_BIT));
}

/* Wait for OCC to reach communications checkpoint */
static void wait_for_occ_checkpoint(uint8_t chip)
{
	enum {
		/* Wait up to 15 seconds for OCC to be ready (1500 * 10ms = 15s) */
		US_BETWEEN_READ  = 10000,
		READ_RETRY_LIMIT = 1500,

		OCC_COMM_INIT_COMPLETE = 0x0EFF,
		OCC_INIT_FAILURE       = 0xE000,

		OCC_RSP_SRAM_ADDR = 0xFFFBF000,
	};

	int retry_count = 0;

	while (retry_count++ < READ_RETRY_LIMIT) {
		uint8_t response[8] = { 0x0 };
		uint8_t status;
		uint16_t checkpoint;

		udelay(US_BETWEEN_READ);

		/* Read SRAM response buffer to check for OCC checkpoint */
		read_occ_sram(chip, OCC_RSP_SRAM_ADDR, (uint64_t *)response, sizeof(response));

		/* Pull status from response (byte 2) */
		status = response[2];

		/* Pull checkpoint from response (bytes 6-7) */
		checkpoint = (response[6] << 8) | response[7];

		if (status == OCC_RC_OCC_INIT_CHECKPOINT &&
		    checkpoint == OCC_COMM_INIT_COMPLETE)
			/* Success */
			return;

		if ((checkpoint & OCC_INIT_FAILURE) == OCC_INIT_FAILURE ||
		    status == OCC_RC_INIT_FAILURE)
			die("OCC initialization has failed\n");
	}

	die("Waiting for OCC initialization checkpoint has timed out.\n");
}

static void build_occ_cmd(struct homer_st *homer, uint8_t occ_cmd, uint8_t seq_num,
			  const uint8_t *data, uint16_t data_len)
{
	uint8_t *cmd_buf = &homer->occ_host_area[OCC_CMD_ADDR];
	uint16_t cmd_len = 0;
	uint16_t checksum = 0;
	uint16_t i = 0;

	cmd_buf[cmd_len++] = seq_num;
	cmd_buf[cmd_len++] = occ_cmd;
	cmd_buf[cmd_len++] = (data_len >> 8) & 0xFF;
	cmd_buf[cmd_len++] = data_len & 0xFF;
	memcpy(&cmd_buf[cmd_len], data, data_len);
	cmd_len += data_len;

	for (i = 0; i < cmd_len; ++i)
		checksum += cmd_buf[i];
	cmd_buf[cmd_len++] = (checksum >> 8) & 0xFF;
	cmd_buf[cmd_len++] = checksum & 0xFF;

	/*
	 * When the P8 processor writes to memory (such as the HOMER) there is
	 * no certainty that the writes happen in order or that they have
	 * actually completed by the time the instructions complete. 'sync'
	 * is a memory barrier to ensure the HOMER data has actually been made
	 * consistent with respect to memory, so that if the OCC were to read
	 * it they would see all of the data. Otherwise, there is potential
	 * for them to get stale or incomplete data.
	 */
	asm volatile("sync" ::: "memory");
}

static void wait_for_occ_response(struct homer_st *homer, uint32_t timeout_sec,
				  uint8_t seq_num)
{
	enum {
		/*
		 * With two CPUs OCC polls were failing with this set to 10 or 20 us.
		 * Apparently, checks performed by the code might not guarantee
		 * that poll data is available in full (checksum doesn't match).
		 *
		 * With one CPU wait_for_occ_status() reports OCC is asking for PCAP
		 * configuration data if *this* delay (not the one in wait_for_occ_status)
		 * is small (50 us or smaller), 100 us seems fine.
		 *
		 * Something is wrong with synchronization and huge delays in Hostboot
		 * might be hiding the issue.
		 */
		OCC_RSP_SAMPLE_TIME_US = 100,
		OCC_COMMAND_IN_PROGRESS = 0xFF,
	};

	const uint8_t *rsp_buf = &homer->occ_host_area[OCC_RSP_ADDR];

	long timeout_us = timeout_sec * USECS_PER_SEC;
	if (timeout_sec == 0)
		timeout_us = OCC_RSP_SAMPLE_TIME_US;

	while (timeout_us >= 0) {
		/*
		 * 1. When OCC receives the command, it will set the status to
		 *    COMMAND_IN_PROGRESS.
		 * 2. When the response is ready OCC will update the full
		 *    response buffer (except the status)
		 * 3. The status field is updated last to indicate response ready
		 *
		 * Note: Need to check the sequence number to be sure we are
		 *       processing the expected response
		 */
		if (rsp_buf[2] != OCC_COMMAND_IN_PROGRESS && rsp_buf[0] == seq_num) {
			/*
			 * Need an 'isync' here to ensure that previous instructions
			 * have completed before the code continues on. This is a type
			 * of read-barrier.  Without this the processor can do
			 * speculative reads of the HOMER data and you can actually
			 * get stale data as part of the instructions that happen
			 * afterwards. Another 'weak consistency' issue.
			 */
			asm volatile("isync" ::: "memory");

			/* OCC must have processed the command */
			break;
		}

		if (timeout_us > 0) {
			/* Delay before the next check */
			long sleep_us = OCC_RSP_SAMPLE_TIME_US;
			if (timeout_us < sleep_us)
				sleep_us = timeout_us;

			udelay(sleep_us);
			timeout_us -= sleep_us;
		} else {
			/* Time expired */
			die("Timed out while waiting for OCC response\n");
		}
	}
}

static bool parse_occ_response(struct homer_st *homer, uint8_t occ_cmd,
			       uint8_t *status, uint8_t *seq_num,
			       uint8_t *response, uint32_t *response_len)
{
	uint16_t index = 0;
	uint16_t data_len = 0;
	uint16_t checksum = 0;
	uint16_t i = 0;

	const uint8_t *rsp_buf = &homer->occ_host_area[OCC_RSP_ADDR];

	*seq_num = rsp_buf[index++];
	index += 1; /* command */
	*status = rsp_buf[index++];

	data_len = *(uint16_t *)&rsp_buf[index];
	index += 2;

	if (data_len > 0) {
		uint16_t copy_size = data_len;
		if (copy_size > *response_len)
			copy_size = *response_len;

		memcpy(response, &rsp_buf[index], copy_size);
		*response_len = copy_size;

		index += data_len;
	}

	for (i = 0; i < index; ++i)
		checksum += rsp_buf[i];

	if (checksum != *(uint16_t *)&rsp_buf[index]) {
		printk(BIOS_WARNING, "OCC response for 0x%02x has invalid checksum\n",
		       occ_cmd);
		return false;
	}

	if (*status != OCC_RC_SUCCESS) {
		printk(BIOS_WARNING, "0x%02x OCC command failed with an error code: 0x%02x\n",
		       occ_cmd, *status);
		return false;
	}

	return true;
}

static bool write_occ_cmd(uint8_t chip, struct homer_st *homer, uint8_t occ_cmd,
			  const uint8_t *data, uint16_t data_len,
			  uint8_t *response, uint32_t *response_len)
{
	static uint8_t cmd_seq_num;

	uint8_t status = 0;
	uint8_t rsp_seq_num = 0;

	++cmd_seq_num;
	/* Do not use 0 for sequence number */
	if (cmd_seq_num == 0)
		++cmd_seq_num;

	build_occ_cmd(homer, occ_cmd, cmd_seq_num, data, data_len);
	/* Sender: HTMGT; command: Command Write Attention */
	write_occ_command(chip, 0x1001000000000000);

	/* Wait for OCC to process command and send response (timeout is the
	   same for all commands) */
	wait_for_occ_response(homer, 20, cmd_seq_num);

	if (!parse_occ_response(homer, occ_cmd, &status, &rsp_seq_num, response,
				response_len)) {
		/* Statuses of 0xE0-EF are reserved for OCC exceptions */
		if ((status & 0xF0) == 0xE0) {
			printk(BIOS_WARNING,
			       "OCC exception occurred while running 0x%02x command\n",
			       occ_cmd);
		}

		printk(BIOS_WARNING, "Received OCC response:\n");
		hexdump(response, *response_len);
		printk(BIOS_WARNING, "Failed to parse OCC response\n");
		return false;
	}

	if (rsp_seq_num != cmd_seq_num) {
		printk(BIOS_WARNING,
		       "Received OCC response for a wrong command while running 0x%02x\n",
		       occ_cmd);
		return false;
	}

	return true;
}

static void send_occ_cmd(uint8_t chip, struct homer_st *homer, uint8_t occ_cmd,
			 const uint8_t *data, uint16_t data_len,
			 uint8_t *response, uint32_t *response_len)
{
	enum { MAX_TRIES = 2 };

	uint8_t i = 0;

	for (i = 0; i < MAX_TRIES; ++i) {
		if (write_occ_cmd(chip, homer, occ_cmd, data, data_len, response, response_len))
			break;

		if (i < MAX_TRIES - 1)
			printk(BIOS_WARNING, "Retrying running OCC command 0x%02x\n", occ_cmd);
	}

	if (i == MAX_TRIES)
		die("Failed running OCC command 0x%02x %d times\n", occ_cmd, MAX_TRIES);
}

/* Reports OCC error to the user and clears it on OCC's side */
static void handle_occ_error(uint8_t chip, struct homer_st *homer,
			     const struct occ_poll_response *response)
{
	static uint8_t error_log_buf[4096];

	uint16_t error_length = response->error_length;

	const uint8_t clear_log_data[4] = {
		0x01, // Version
		response->error_id,
		response->error_source,
		0x00  // Reserved
	};
	uint32_t response_len = 0;

	if (error_length > sizeof(error_log_buf)) {
		printk(BIOS_WARNING, "Truncating OCC error log from %d to %ld bytes\n",
		       error_length, sizeof(error_log_buf));
		error_length = sizeof(error_log_buf);
	}

	read_occ_sram(chip, response->error_address, (uint64_t *)error_log_buf, error_length);

	printk(BIOS_WARNING, "OCC error log:\n");
	hexdump(error_log_buf, error_length);

	/* Confirm to OCC that we've read the log */
	send_occ_cmd(chip, homer, OCC_CMD_CLEAR_ERROR_LOG,
		     clear_log_data, sizeof(clear_log_data),
		     NULL, &response_len);
}

static void poll_occ(uint8_t chip, struct homer_st *homer, bool flush_all_errors,
		     struct occ_poll_response *response)
{
	enum { OCC_POLL_DATA_MIN_SIZE = 40 };

	uint8_t max_more_errors = 10;
	while (true) {
		const uint8_t poll_data[1] = { 0x20 /*version*/ };
		uint32_t response_len = sizeof(*response);

		send_occ_cmd(chip, homer, OCC_CMD_POLL, poll_data, sizeof(poll_data),
			     (uint8_t *)response, &response_len);

		if (response_len < OCC_POLL_DATA_MIN_SIZE)
			die("Invalid data length");

		if (!flush_all_errors)
			break;

		if (response->error_id == 0)
			break;

		handle_occ_error(chip, homer, response);

		--max_more_errors;
		if (max_more_errors == 0) {
			printk(BIOS_WARNING, "Last OCC poll response:\n");
			hexdump(response, response_len);
			die("Hit too many errors on polling OCC\n");
		}
	}
}

static void get_freq_point_msg_data(const struct occ_cfg_inputs *inputs,
				    uint8_t *data, uint16_t *size)
{
	enum { OCC_CFGDATA_FREQ_POINT_VERSION = 0x20 };
	OCCPstateParmBlock *oppb = (void *)inputs->homer->ppmr.occ_parm_block;

	const struct voltage_bucket_data *bucket = get_voltage_data(inputs->chip);

	uint16_t index = 0;
	uint16_t min_freq = 0;

	data[index++] = OCC_CFGDATA_FREQ_POINT;
	data[index++] = OCC_CFGDATA_FREQ_POINT_VERSION;

	/* Nominal Frequency in MHz */
	memcpy(&data[index], &bucket->nominal.freq, 2);
	index += 2;

	/* Turbo Frequency in MHz */
	memcpy(&data[index], &bucket->turbo.freq, 2);
	index += 2;

	/* Minimum Frequency in MHz */
	min_freq = oppb->frequency_min_khz / 1000;
	memcpy(&data[index], &min_freq, 2);
	index += 2;

	/* Ultra Turbo Frequency in MHz */
	memcpy(&data[index], &bucket->ultra_turbo.freq, 2);
	index += 2;

	/* Reserved (Static Power Save in PowerVM) */
	memset(&data[index], 0, 2);
	index += 2;

	/* Reserved (FFO in PowerVM) */
	memset(&data[index], 0, 2);
	index += 2;

	*size = index;
}

static void get_occ_role_msg_data(const struct occ_cfg_inputs *inputs,
				  uint8_t *data, uint16_t *size)
{
	enum {
		OCC_ROLE_SLAVE  = 0x00,
		OCC_ROLE_MASTER = 0x01,
	};

	data[0] = OCC_CFGDATA_OCC_ROLE;
	data[1] = (inputs->is_master_occ ? OCC_ROLE_MASTER : OCC_ROLE_SLAVE);

	*size = 2;
}

static void get_apss_msg_data(const struct occ_cfg_inputs *inputs,
			      uint8_t *data, uint16_t *size)
{
	enum { OCC_CFGDATA_APSS_VERSION = 0x20 };

	/* ATTR_APSS_GPIO_PORT_PINS */
	uint8_t function[16] = { 0x0 };

	/* ATTR_ADC_CHANNEL_GNDS */
	uint8_t ground[16] = { 0x0 };

	/* ATTR_ADC_CHANNEL_GAINS */
	uint32_t gain[16] = { 0x0 };

	/* ATTR_ADC_CHANNEL_OFFSETS */
	uint32_t offset[16] = { 0x0 };

	uint16_t index = 0;

	data[index++] = OCC_CFGDATA_APSS_CONFIG;
	data[index++] = OCC_CFGDATA_APSS_VERSION;
	data[index++] = 0;
	data[index++] = 0;

	for (uint64_t channel = 0; channel < sizeof(function); ++channel) {
		data[index++] = function[channel];         // ADC Channel assignment

		memset(&data[index], 0, sizeof(uint32_t)); // Sensor ID
		index += 4;

		data[index++] = ground[channel];           // Ground Select

		memcpy(&data[index], &gain[channel], sizeof(uint32_t));
		index += 4;

		memcpy(&data[index], &offset[channel], sizeof(uint32_t));
		index += 4;
	}

	/* ATTR_APSS_GPIO_PORT_MODES */
	uint8_t gpio_mode[2] = { 0x0 };
	/* ATTR_APSS_GPIO_PORT_PINS */
	uint8_t gpio_pin[16] = { 0x0 };

	uint64_t pins_per_port = sizeof(gpio_pin) / sizeof(gpio_mode);
	uint64_t pin_idx = 0;

	for (uint64_t port = 0; port < sizeof(gpio_mode); ++port) {
		data[index++] = gpio_mode[port];
		data[index++] = 0;

		memcpy(&data[index], gpio_pin + pin_idx, pins_per_port);
		index += pins_per_port;

		pin_idx += pins_per_port;
	}

	*size = index;
}

static void get_mem_cfg_msg_data(const struct occ_cfg_inputs *inputs,
				 uint8_t *data, uint16_t *size)
{
	enum { OCC_CFGDATA_MEM_CONFIG_VERSION = 0x21 };

	uint16_t index = 0;

	data[index++] = OCC_CFGDATA_MEM_CONFIG;
	data[index++] = OCC_CFGDATA_MEM_CONFIG_VERSION;

	/* If OPAL then no "Power Control Default" support */

	/* Byte 3: Memory Power Control Default */
	data[index++] = 0xFF;
	/* Byte 4: Idle Power Memory Power Control */
	data[index++] = 0xFF;

	/* Byte 5: Number of data sets */
	data[index++] = 0; // Monitoring is disabled

	*size = index;
}

static void get_sys_cfg_msg_data(const struct occ_cfg_inputs *inputs,
				 uint8_t *data, uint16_t *size)
{
	/* TODO: all sensor IDs are zero, because we don't have IPMI messaging,
	 *       which seems to be required to get them */

	enum {
		OCC_CFGDATA_SYS_CONFIG_VERSION = 0x21,

		/* KVM or OPAL mode + single node */
		OCC_CFGDATA_OPENPOWER_OPALVM = 0x81,

		OCC_CFGDATA_NON_REDUNDANT_PS      = 0x02,
		OCC_REPORT_THROTTLE_BELOW_NOMINAL = 0x08,
	};

	uint8_t system_type = OCC_CFGDATA_OPENPOWER_OPALVM;
	uint16_t index = 0;
	uint8_t i = 0;

	data[index++] = OCC_CFGDATA_SYS_CONFIG;
	data[index++] = OCC_CFGDATA_SYS_CONFIG_VERSION;

	/* System Type */

	/* ATTR_REPORT_THROTTLE_BELOW_NOMINAL == 0 */

	/* 0 = OCC report throttling when max frequency lowered below turbo */
	system_type &= ~OCC_REPORT_THROTTLE_BELOW_NOMINAL;
	/* Power supply policy is redundant */
	system_type &= ~OCC_CFGDATA_NON_REDUNDANT_PS;
	data[index++] = system_type;

	/* Processor Callout Sensor ID */
	memset(&data[index], 0, 4);
	index += 4;

	/* Next 12*4 bytes are for core sensors */
	for (i = 0; i < MAX_CORES_PER_CHIP; ++i) {
		/* Core Temp Sensor ID */
		memset(&data[index], 0, 4);
		index += 4;

		/* Core Frequency Sensor ID */
		memset(&data[index], 0, 4);
		index += 4;
	}

	/* Backplane Callout Sensor ID */
	memset(&data[index], 0, 4);
	index += 4;

	/* APSS Callout Sensor ID */
	memset(&data[index], 0, 4);
	index += 4;

	/* Format 21 - VRM VDD Callout Sensor ID */
	memset(&data[index], 0, 4);
	index += 4;

	/* Format 21 - VRM VDD Temperature Sensor ID */
	memset(&data[index], 0, 4);
	index += 4;

	*size = index;
}

static void get_thermal_ctrl_msg_data(const struct occ_cfg_inputs *inputs,
				      uint8_t *data, uint16_t *size)
{
	enum {
		OCC_CFGDATA_TCT_CONFIG_VERSION = 0x20,

		CFGDATA_FRU_TYPE_PROC       = 0x00,
		CFGDATA_FRU_TYPE_MEMBUF     = 0x01,
		CFGDATA_FRU_TYPE_DIMM       = 0x02,
		CFGDATA_FRU_TYPE_VRM        = 0x03,
		CFGDATA_FRU_TYPE_GPU_CORE   = 0x04,
		CFGDATA_FRU_TYPE_GPU_MEMORY = 0x05,
		CFGDATA_FRU_TYPE_VRM_VDD    = 0x06,

		OCC_NOT_DEFINED = 0xFF,
	};

	uint16_t index = 0;

	data[index++] = OCC_CFGDATA_TCT_CONFIG;
	data[index++] = OCC_CFGDATA_TCT_CONFIG_VERSION;

	/* Processor Core Weight, ATTR_OPEN_POWER_PROC_WEIGHT, from talos.xml */
	data[index++] = 9;

	/* Processor Quad Weight, ATTR_OPEN_POWER_QUAD_WEIGHT, from talos.xml */
	data[index++] = 1;

	/* Data sets following (proc, DIMM, etc.), and each will get a FRU type,
	   DVS temp, error temp and max read timeout */
	data[index++] = 5;

	/*
	 * Note: Bytes 4 and 5 of each data set represent the PowerVM DVFS and ERROR
	 * Resending the regular DVFS and ERROR for now.
	 */

	/* Processor */
	data[index++] = CFGDATA_FRU_TYPE_PROC;
	data[index++] = 85; // DVFS, ATTR_OPEN_POWER_PROC_DVFS_TEMP_DEG_C, from talos.xml
	data[index++] = 95; // ERROR, ATTR_OPEN_POWER_PROC_ERROR_TEMP_DEG_C, from talos.xml
	data[index++] = OCC_NOT_DEFINED; // PM_DVFS
	data[index++] = OCC_NOT_DEFINED; // PM_ERROR
	data[index++] = 5;  // ATTR_OPEN_POWER_PROC_READ_TIMEOUT_SEC, from talos.xml

	/* DIMM */
	data[index++] = CFGDATA_FRU_TYPE_DIMM;
	data[index++] = 84; // DVFS, ATTR_OPEN_POWER_DIMM_THROTTLE_TEMP_DEG_C, from talos.xml
	data[index++] = 84; // ERROR, ATTR_OPEN_POWER_DIMM_ERROR_TEMP_DEG_C, from talos.xml
	data[index++] = OCC_NOT_DEFINED; // PM_DVFS
	data[index++] = OCC_NOT_DEFINED; // PM_ERROR
	data[index++] = 30; // TIMEOUT, ATTR_OPEN_POWER_DIMM_READ_TIMEOUT_SEC, from talos.xml

	/* VRM OT monitoring is disabled, because ATTR_OPEN_POWER_VRM_READ_TIMEOUT_SEC == 0
	   (default) */

	/* GPU Cores */
	data[index++] = CFGDATA_FRU_TYPE_GPU_CORE;
	// DVFS
	data[index++] = OCC_NOT_DEFINED;
	// ERROR, ATTR_OPEN_POWER_GPU_ERROR_TEMP_DEG_C, not set
	data[index++] = OCC_NOT_DEFINED;
	// PM_DVFS
	data[index++] = OCC_NOT_DEFINED;
	// PM_ERROR
	data[index++] = OCC_NOT_DEFINED;
	// TIMEOUT, ATTR_OPEN_POWER_GPU_READ_TIMEOUT_SEC, default
	data[index++] = OCC_NOT_DEFINED;

	/* GPU Memory */
	data[index++] = CFGDATA_FRU_TYPE_GPU_MEMORY;
	data[index++] = OCC_NOT_DEFINED; // DVFS
	// ERROR, ATTR_OPEN_POWER_GPU_MEM_ERROR_TEMP_DEG_C, not set
	data[index++] = OCC_NOT_DEFINED;
	// PM_DVFS
	data[index++] = OCC_NOT_DEFINED;
	// PM_ERROR
	data[index++] = OCC_NOT_DEFINED;
	// TIMEOUT, ATTR_OPEN_POWER_GPU_MEM_READ_TIMEOUT_SEC, not set
	data[index++] = OCC_NOT_DEFINED;

	/* VRM Vdd */
	data[index++] = CFGDATA_FRU_TYPE_VRM_VDD;
	// DVFS, ATTR_OPEN_POWER_VRM_VDD_DVFS_TEMP_DEG_C, default
	data[index++] = OCC_NOT_DEFINED;
	// ERROR, ATTR_OPEN_POWER_VRM_VDD_ERROR_TEMP_DEG_C, default
	data[index++] = OCC_NOT_DEFINED;
	// PM_DVFS
	data[index++] = OCC_NOT_DEFINED;
	// PM_ERROR
	data[index++] = OCC_NOT_DEFINED;
	// TIMEOUT, ATTR_OPEN_POWER_VRM_VDD_READ_TIMEOUT_SEC, default
	data[index++] = OCC_NOT_DEFINED;

	*size = index;
}

static void get_power_cap_msg_data(const struct occ_cfg_inputs *inputs,
				   uint8_t *data, uint16_t *size)
{
	enum { OCC_CFGDATA_PCAP_CONFIG_VERSION = 0x20 };

	uint16_t index = 0;

	/* Values of the following attributes were taken from Hostboot's log */

	/* Minimum HARD Power Cap (ATTR_OPEN_POWER_MIN_POWER_CAP_WATTS) */
	uint16_t min_pcap = 2000;

	/* Minimum SOFT Power Cap (ATTR_OPEN_POWER_SOFT_MIN_PCAP_WATTS) */
	uint16_t soft_pcap = 2000;

	/* Quick Power Drop Power Cap (ATTR_OPEN_POWER_N_BULK_POWER_LIMIT_WATTS) */
	uint16_t qpd_pcap = 2000;

	/* System Maximum Power Cap (ATTR_OPEN_POWER_N_PLUS_ONE_HPC_BULK_POWER_LIMIT_WATTS) */
	uint16_t max_pcap = 3000;

	data[index++] = OCC_CFGDATA_PCAP_CONFIG;
	data[index++] = OCC_CFGDATA_PCAP_CONFIG_VERSION;

	memcpy(&data[index], &soft_pcap, 2);
	index += 2;

	memcpy(&data[index], &min_pcap, 2);
	index += 2;

	memcpy(&data[index], &max_pcap, 2);
	index += 2;

	memcpy(&data[index], &qpd_pcap, 2);
	index += 2;

	*size = index;
}

static void get_avs_bus_cfg_msg_data(const struct occ_cfg_inputs *inputs,
				     uint8_t *data, uint16_t *size)
{
	enum { OCC_CFGDATA_AVSBUS_CONFIG_VERSION = 0x01 };

	/* ATTR_NO_APSS_PROC_POWER_VCS_VIO_WATTS, from talos.xml */
	const uint16_t power_adder = 19;

	uint16_t index = 0;

	data[index++] = OCC_CFGDATA_AVSBUS_CONFIG;
	data[index++] = OCC_CFGDATA_AVSBUS_CONFIG_VERSION;
	data[index++] = 0;    // Vdd Bus, ATTR_VDD_AVSBUS_BUSNUM
	data[index++] = 0;    // Vdd Rail Sel, ATTR_VDD_AVSBUS_RAIL
	data[index++] = 0xFF; // reserved
	data[index++] = 0xFF; // reserved
	data[index++] = 1;    // Vdn Bus, ATTR_VDN_AVSBUS_BUSNUM, from talos.xml
	data[index++] = 0;    // Vdn Rail sel, ATTR_VDN_AVSBUS_RAIL, from talos.xml

	data[index++] = (power_adder >> 8) & 0xFF;
	data[index++] = power_adder & 0xFF;

	/* ATTR_VDD_CURRENT_OVERFLOW_WORKAROUND_ENABLE == 0 */

	*size = index;
}

static void get_power_data(const struct occ_cfg_inputs *inputs,
			   uint16_t *power_max, uint16_t *power_drop)
{
	const struct voltage_bucket_data *bucket = get_voltage_data(inputs->chip);

	/* All processor chips (do not have to be functional) */
	const uint8_t num_procs = 2; // from Hostboot log

	const uint16_t proc_socket_power = 250; // ATTR_PROC_SOCKET_POWER_WATTS, default
	const uint16_t misc_power = 0; // ATTR_MISC_SYSTEM_COMPONENTS_MAX_POWER_WATTS, default

	const uint16_t mem_power_min_throttles = 36; // from Hostboot log
	const uint16_t mem_power_max_throttles = 23; // from Hostboot log

	/*
	 * Calculate Total non-GPU maximum power (Watts):
	 *   Maximum system power excluding GPUs when CPUs are at maximum frequency
	 *   (ultra turbo) and memory at maximum power (least throttled) plus
	 *   everything else (fans...) excluding GPUs.
	 */
	*power_max = proc_socket_power * num_procs;
	*power_max += mem_power_min_throttles + misc_power;

	OCCPstateParmBlock *oppb = (void *)inputs->homer->ppmr.occ_parm_block;
	uint16_t min_freq_mhz = oppb->frequency_min_khz / 1000;
	const uint16_t mhz_per_watt = 28; // ATTR_PROC_MHZ_PER_WATT, from talos.xml
	/* Drop is always calculated from Turbo to Min (not ultra) */
	uint32_t proc_drop = (bucket->turbo.freq - min_freq_mhz) / mhz_per_watt;
	proc_drop *= num_procs;
	const uint16_t memory_drop = mem_power_min_throttles - mem_power_max_throttles;

	*power_drop = proc_drop + memory_drop;
}

static void get_gpu_msg_data(const struct occ_cfg_inputs *inputs, uint8_t *data, uint16_t *size)
{
	enum {
		OCC_CFGDATA_GPU_CONFIG_VERSION = 0x01,
		MAX_GPUS = 3,
	};

	uint16_t power_max = 0;
	uint16_t power_drop = 0;

	uint16_t index = 0;

	data[index++] = OCC_CFGDATA_GPU_CONFIG;
	data[index++] = OCC_CFGDATA_GPU_CONFIG_VERSION;

	get_power_data(inputs, &power_max, &power_drop);

	memcpy(&data[index], &power_max, 2);   // Total non-GPU max power (W)
	index += 2;

	memcpy(&data[index], &power_drop, 2);  // Total proc/mem power drop (W)
	index += 2;
	data[index++] = 0; // reserved
	data[index++] = 0; // reserved

	/* No sensors ID.  Might require OBus or just be absent. */
	uint32_t gpu_func_sensors[MAX_GPUS] = {0};
	uint32_t gpu_temp_sensors[MAX_GPUS] = {0};
	uint32_t gpu_memtemp_sensors[MAX_GPUS] = {0};

	/* GPU0 */
	memcpy(&data[index], &gpu_temp_sensors[0], 4);
	index += 4;
	memcpy(&data[index], &gpu_memtemp_sensors[0], 4);
	index += 4;
	memcpy(&data[index], &gpu_func_sensors[0], 4);
	index += 4;

	/* GPU1 */
	memcpy(&data[index], &gpu_temp_sensors[1], 4);
	index += 4;
	memcpy(&data[index], &gpu_memtemp_sensors[1], 4);
	index += 4;
	memcpy(&data[index], &gpu_func_sensors[1], 4);
	index += 4;

	/* GPU2 */
	memcpy(&data[index], &gpu_temp_sensors[2], 4);
	index += 4;
	memcpy(&data[index], &gpu_memtemp_sensors[2], 4);
	index += 4;
	memcpy(&data[index], &gpu_func_sensors[2], 4);
	index += 4;

	*size = index;
}

static void send_occ_config_data(uint8_t chip, struct homer_st *homer)
{
	enum {
		TO_ALL = 0,    /* to_master_only = false */
		TO_MASTER = 1, /* to_master_only = true */
	};

	/*
	 * Order in which these are sent is important!
	 * Not every order works.
	 */
	struct occ_cfg_info cfg_info[] = {
		{ "System config",    &get_sys_cfg_msg_data,      TO_ALL    },
		{ "APSS config",      &get_apss_msg_data,         TO_ALL    },
		{ "OCC role",         &get_occ_role_msg_data,     TO_ALL    },
		{ "Frequency points", &get_freq_point_msg_data,   TO_MASTER },
		{ "Memory config",    &get_mem_cfg_msg_data,      TO_ALL    },
		{ "Power cap",        &get_power_cap_msg_data,    TO_MASTER },
		{ "Thermal control",  &get_thermal_ctrl_msg_data, TO_ALL    },
		{ "AVS",              &get_avs_bus_cfg_msg_data,  TO_ALL    },
		{ "GPU",              &get_gpu_msg_data,          TO_ALL    },
	};

	const struct occ_cfg_inputs inputs = {
		.homer = homer,
		.chip = chip,
		.is_master_occ = (chip == 0),
	};

	uint8_t i;

	for (i = 0; i < ARRAY_SIZE(cfg_info); ++i) {
		/* All our messages are short */
		uint8_t data[256];
		uint16_t data_len = 0;
		uint32_t response_len = 0;

		/* Poll is sent between configuration packets to flush errors */
		struct occ_poll_response poll_response;

		/*
		 * Certain kinds of configuration data is broadcasted to slave
		 * OCCs by the master and must not be sent to them directly
		 */
		if (cfg_info[i].to_master_only && !inputs.is_master_occ)
			continue;

		cfg_info[i].func(&inputs, data, &data_len);
		if (data_len > sizeof(data))
			die("Buffer for OCC data is too small!\n");

		send_occ_cmd(chip, homer, OCC_CMD_SETUP_CFG_DATA, data, data_len,
			     NULL, &response_len);
		poll_occ(chip, homer, /*flush_all_errors=*/false, &poll_response);
	}
}

static void send_occ_user_power_cap(uint8_t chip, struct homer_st *homer)
{
	/* No power limit */
	const uint8_t data[2] = { 0x00, 0x00 };
	uint32_t response_len = 0;
	send_occ_cmd(chip, homer, OCC_CMD_SET_POWER_CAP, data, sizeof(data),
		     NULL, &response_len);
}

static void wait_for_occ_status(uint8_t chip, struct homer_st *homer, uint8_t status_bit)
{
	enum {
		MAX_POLLS = 200,
		DELAY_BETWEEN_POLLS_US = 50000,
	};

	uint8_t num_polls = 0;
	struct occ_poll_response poll_response;

	for (num_polls = 0; num_polls < MAX_POLLS; ++num_polls) {
		poll_occ(chip, homer, /*flush_all_errors=*/false, &poll_response);
		if (poll_response.status & status_bit)
			break;

		if (poll_response.requested_cfg != 0x00) {
			die("OCC requests 0x%02x configuration data\n",
			    poll_response.requested_cfg);
		}

		if (num_polls < MAX_POLLS)
			udelay(DELAY_BETWEEN_POLLS_US);
	}

	if (num_polls == MAX_POLLS)
		die("Failed to wait until OCC has reached state 0x%02x\n", status_bit);
}

static void set_occ_state(uint8_t chip, struct homer_st *homer, uint8_t state)
{
	struct occ_poll_response poll_response;

	/* Fields: version, state, reserved */
	const uint8_t data[3] = { 0x00, state, 0x00 };
	uint32_t response_len = 0;

	/* Send poll cmd to confirm comm has been established and flush old errors */
	poll_occ(chip, homer, /*flush_all_errors=*/true, &poll_response);

	/* Try to switch to a new state */
	send_occ_cmd(chip, homer, OCC_CMD_SET_STATE, data, sizeof(data), NULL, &response_len);

	/* Send poll to query state of all OCC and flush any errors */
	poll_occ(chip, homer, /*flush_all_errors=*/true, &poll_response);

	if (poll_response.state != state)
		die("State of OCC is 0x%02x instead of 0x%02x.\n", poll_response.state, state);
}

static void set_occ_active_state(uint8_t chip, struct homer_st *homer)
{
	enum {
		OCC_STATUS_ACTIVE_READY = 0x01,
		OCC_STATE_ACTIVE = 0x03,
	};

	wait_for_occ_status(chip, homer, OCC_STATUS_ACTIVE_READY);
	set_occ_state(chip, homer, OCC_STATE_ACTIVE);
}

void activate_occ(uint8_t chips, struct homer_st *homers)
{
	/* Make sure OCCs are ready for communication */
	for (uint8_t chip = 0; chip < MAX_CHIPS; chip++) {
		if (chips & (1 << chip))
			wait_for_occ_checkpoint(chip);
	}

	/* Send initial poll to all OCCs to establish communication */
	for (uint8_t chip = 0; chip < MAX_CHIPS; chip++) {
		if (chips & (1 << chip)) {
			struct occ_poll_response poll_response;
			poll_occ(chip, &homers[chip], /*flush_all_errors=*/false,
				 &poll_response);
		}
	}

	/* Send OCC's config data */
	for (uint8_t chip = 0; chip < MAX_CHIPS; chip++) {
		if (chips & (1 << chip))
			send_occ_config_data(chip, &homers[chip]);
	}

	/* Set the User PCAP (sent only to master OCC) */
	send_occ_user_power_cap(/*chip=*/0, &homers[0]);

	/* Switch for OCC to active state (sent only to master OCC) */
	set_occ_active_state(/*chip=*/0, &homers[0]);

	/* TODO: Hostboot sets active sensors for all OCCs here, so BMC can start
	   communication with OCCs. */
}

void pm_occ_fir_init(uint8_t chip)
{
	enum {
		PERV_TP_OCC_SCOM_OCCLFIR = 0x01010800,

		/* Bits of OCC LFIR */
		OCC_FW0                    = 0,
		OCC_FW1                    = 1,
		CME_ERR_NOTIFY             = 2,
		STOP_RCV_NOTIFY_PRD        = 3,
		OCC_HB_NOTIFY              = 4,
		GPE0_WD_TIMEOUT            = 5,
		GPE1_WD_TIMEOUT            = 6,
		GPE2_WD_TIMEOUT            = 7,
		GPE3_WD_TIMEOUT            = 8,
		GPE0_ERR                   = 9,
		GPE1_ERR                   = 10,
		GPE2_ERR                   = 11,
		GPE3_ERR                   = 12,
		OCB_ERR                    = 13,
		SRAM_UE                    = 14,
		SRAM_CE                    = 15,
		SRAM_READ_ERR              = 16,
		SRAM_WRITE_ERR             = 17,
		SRAM_DATAOUT_PERR          = 18,
		SRAM_OCI_WDATA_PARITY      = 19,
		SRAM_OCI_BE_PARITY_ERR     = 20,
		SRAM_OCI_ADDR_PARITY_ERR   = 21,
		GPE0_HALTED                = 22,
		GPE1_HALTED                = 23,
		GPE2_HALTED                = 24,
		GPE3_HALTED                = 25,
		EXT_TRAP                   = 26,
		PPC405_CORE_RESET          = 27,
		PPC405_CHIP_RESET          = 28,
		PPC405_SYS_RESET           = 29,
		PPC405_WAIT_STATE          = 30,
		PPC405_DBGSTOPACK          = 31,
		OCB_DB_OCI_TIMEOUT         = 32,
		OCB_DB_OCI_RDATA_PARITY    = 33,
		OCB_DB_OCI_SLVERR          = 34,
		OCB_PIB_ADDR_PARITY_ERR    = 35,
		OCB_DB_PIB_DATA_PARITY_ERR = 36,
		OCB_IDC0_ERR               = 37,
		OCB_IDC1_ERR               = 38,
		OCB_IDC2_ERR               = 39,
		OCB_IDC3_ERR               = 40,
		SRT_FSM_ERR                = 41,
		JTAGACC_ERR                = 42,
		SPARE_ERR_38               = 43,
		C405_ECC_UE                = 44,
		C405_ECC_CE                = 45,
		C405_OCI_MC_CHK            = 46,
		SRAM_SPARE_DIRERR0         = 47,
		SRAM_SPARE_DIRERR1         = 48,
		SRAM_SPARE_DIRERR2         = 49,
		SRAM_SPARE_DIRERR3         = 50,
		GPE0_OCISLV_ERR            = 51,
		GPE1_OCISLV_ERR            = 52,
		GPE2_OCISLV_ERR            = 53,
		GPE3_OCISLV_ERR            = 54,
		C405ICU_M_TIMEOUT          = 55,
		C405DCU_M_TIMEOUT          = 56,
		OCC_CMPLX_FAULT            = 57,
		OCC_CMPLX_NOTIFY           = 58,
		SPARE_59                   = 59,
		SPARE_60                   = 60,
		SPARE_61                   = 61,
		FIR_PARITY_ERR_DUP         = 62,
		FIR_PARITY_ERR             = 63,
	};

	const uint64_t action0_bits = 0;
	const uint64_t action1_bits =
		  PPC_BIT(C405_ECC_CE)             | PPC_BIT(C405_OCI_MC_CHK)
		| PPC_BIT(C405DCU_M_TIMEOUT)       | PPC_BIT(GPE0_ERR)
		| PPC_BIT(GPE0_OCISLV_ERR)         | PPC_BIT(GPE1_ERR)
		| PPC_BIT(GPE1_OCISLV_ERR)         | PPC_BIT(GPE2_OCISLV_ERR)
		| PPC_BIT(GPE3_OCISLV_ERR)         | PPC_BIT(JTAGACC_ERR)
		| PPC_BIT(OCB_DB_OCI_RDATA_PARITY) | PPC_BIT(OCB_DB_OCI_SLVERR)
		| PPC_BIT(OCB_DB_OCI_TIMEOUT)      | PPC_BIT(OCB_DB_PIB_DATA_PARITY_ERR)
		| PPC_BIT(OCB_IDC0_ERR)            | PPC_BIT(OCB_IDC1_ERR)
		| PPC_BIT(OCB_IDC2_ERR)            | PPC_BIT(OCB_IDC3_ERR)
		| PPC_BIT(OCB_PIB_ADDR_PARITY_ERR) | PPC_BIT(OCC_CMPLX_FAULT)
		| PPC_BIT(OCC_CMPLX_NOTIFY)        | PPC_BIT(SRAM_CE)
		| PPC_BIT(SRAM_DATAOUT_PERR)       | PPC_BIT(SRAM_OCI_ADDR_PARITY_ERR)
		| PPC_BIT(SRAM_OCI_BE_PARITY_ERR)  | PPC_BIT(SRAM_OCI_WDATA_PARITY)
		| PPC_BIT(SRAM_READ_ERR)           | PPC_BIT(SRAM_SPARE_DIRERR0)
		| PPC_BIT(SRAM_SPARE_DIRERR1)      | PPC_BIT(SRAM_SPARE_DIRERR2)
		| PPC_BIT(SRAM_SPARE_DIRERR3)      | PPC_BIT(SRAM_UE)
		| PPC_BIT(SRAM_WRITE_ERR)          | PPC_BIT(SRT_FSM_ERR)
		| PPC_BIT(STOP_RCV_NOTIFY_PRD)     | PPC_BIT(C405_ECC_UE);

	uint64_t mask = read_scom(chip, PERV_TP_OCC_SCOM_OCCLFIR + MASK_INCR);
	mask &= ~action0_bits;
	mask &= ~action1_bits;

	write_scom(chip, PERV_TP_OCC_SCOM_OCCLFIR, 0);
	write_scom(chip, PERV_TP_OCC_SCOM_OCCLFIR + ACTION0_INCR, action0_bits);
	write_scom(chip, PERV_TP_OCC_SCOM_OCCLFIR + ACTION1_INCR, action1_bits);
	write_scom(chip, PERV_TP_OCC_SCOM_OCCLFIR + MASK_WOR_INCR, mask);
	write_scom(chip, PERV_TP_OCC_SCOM_OCCLFIR + MASK_WAND_INCR, mask);
}

void pm_pba_fir_init(uint8_t chip)
{
	enum {
		PU_PBAFIR = 0x05012840,

		/* Bits of PBA LFIR. */
		PBAFIR_OCI_APAR_ERR      = 0,
		PBAFIR_PB_RDADRERR_FW    = 1,
		PBAFIR_PB_RDDATATO_FW    = 2,
		PBAFIR_PB_SUE_FW         = 3,
		PBAFIR_PB_UE_FW          = 4,
		PBAFIR_PB_CE_FW          = 5,
		PBAFIR_OCI_SLAVE_INIT    = 6,
		PBAFIR_OCI_WRPAR_ERR     = 7,
		PBAFIR_SPARE             = 8,
		PBAFIR_PB_UNEXPCRESP     = 9,
		PBAFIR_PB_UNEXPDATA      = 10,
		PBAFIR_PB_PARITY_ERR     = 11,
		PBAFIR_PB_WRADRERR_FW    = 12,
		PBAFIR_PB_BADCRESP       = 13,
		PBAFIR_PB_ACKDEAD_FW_RD  = 14,
		PBAFIR_PB_CRESPTO        = 15,
		PBAFIR_BCUE_SETUP_ERR    = 16,
		PBAFIR_BCUE_PB_ACK_DEAD  = 17,
		PBAFIR_BCUE_PB_ADRERR    = 18,
		PBAFIR_BCUE_OCI_DATERR   = 19,
		PBAFIR_BCDE_SETUP_ERR    = 20,
		PBAFIR_BCDE_PB_ACK_DEAD  = 21,
		PBAFIR_BCDE_PB_ADRERR    = 22,
		PBAFIR_BCDE_RDDATATO_ERR = 23,
		PBAFIR_BCDE_SUE_ERR      = 24,
		PBAFIR_BCDE_UE_ERR       = 25,
		PBAFIR_BCDE_CE           = 26,
		PBAFIR_BCDE_OCI_DATERR   = 27,
		PBAFIR_INTERNAL_ERR      = 28,
		PBAFIR_ILLEGAL_CACHE_OP  = 29,
		PBAFIR_OCI_BAD_REG_ADDR  = 30,
		PBAFIR_AXPUSH_WRERR      = 31,
		PBAFIR_AXRCV_DLO_ERR     = 32,
		PBAFIR_AXRCV_DLO_TO      = 33,
		PBAFIR_AXRCV_RSVDATA_TO  = 34,
		PBAFIR_AXFLOW_ERR        = 35,
		PBAFIR_AXSND_DHI_RTYTO   = 36,
		PBAFIR_AXSND_DLO_RTYTO   = 37,
		PBAFIR_AXSND_RSVTO       = 38,
		PBAFIR_AXSND_RSVERR      = 39,
		PBAFIR_PB_ACKDEAD_FW_WR  = 40,
		PBAFIR_RESERVED_41       = 41,
		PBAFIR_RESERVED_42       = 42,
		PBAFIR_RESERVED_43       = 43,
		PBAFIR_FIR_PARITY_ERR2   = 44,
		PBAFIR_FIR_PARITY_ERR    = 45,
	};

	const uint64_t action0_bits = 0;
	const uint64_t action1_bits =
		  PPC_BIT(PBAFIR_OCI_APAR_ERR)     | PPC_BIT(PBAFIR_PB_UE_FW)
		| PPC_BIT(PBAFIR_PB_CE_FW)         | PPC_BIT(PBAFIR_OCI_SLAVE_INIT)
		| PPC_BIT(PBAFIR_OCI_WRPAR_ERR)    | PPC_BIT(PBAFIR_PB_UNEXPCRESP)
		| PPC_BIT(PBAFIR_PB_UNEXPDATA)     | PPC_BIT(PBAFIR_PB_PARITY_ERR)
		| PPC_BIT(PBAFIR_PB_WRADRERR_FW)   | PPC_BIT(PBAFIR_PB_BADCRESP)
		| PPC_BIT(PBAFIR_PB_CRESPTO)       | PPC_BIT(PBAFIR_INTERNAL_ERR)
		| PPC_BIT(PBAFIR_ILLEGAL_CACHE_OP) | PPC_BIT(PBAFIR_OCI_BAD_REG_ADDR);

	uint64_t mask = PPC_BITMASK(0, 63);
	mask &= ~action0_bits;
	mask &= ~action1_bits;

	write_scom(chip, PU_PBAFIR, 0);
	write_scom(chip, PU_PBAFIR + ACTION0_INCR, action0_bits);
	write_scom(chip, PU_PBAFIR + ACTION1_INCR, action1_bits);
	write_scom(chip, PU_PBAFIR + MASK_WOR_INCR, mask);
	write_scom(chip, PU_PBAFIR + MASK_WAND_INCR, mask);
}
