/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <delay.h>
#include <timer.h>

#include "nct6687d_hwm.h"
#include "nct6687d_smbus.h"

#if !ENV_RAMSTAGE
uint16_t nct6687d_hwm_base = 0;
#endif

static void prepare_smbus_transaction(uint8_t operation, uint8_t address, uint8_t command, uint8_t length)
{
	hwm_reg_write(SMBUS_MASTER_PROTOCOL_SEL_REG, operation);

	if (operation == SMBUS_MASTER_BLOCK_WRITE)
			hwm_reg_write(SMBUS_MASTER_BLOCK_WR_LEN_REG, length);

	hwm_reg_write(SMBUS_MASTER_DEV_ADDR_REG, address);
	hwm_reg_write(SMBUS_MASTER_CMD_REG, command);
}

static bool wait_smbus_status_bit_clear(uint8_t bit)
{
	struct stopwatch timer;

	stopwatch_init_msecs_expire(&timer, 500);

	while (true) {
		if ((hwm_reg_read(SMBUS_MASTER_CFG1_REG) & bit) == 0)
			return true;

		/* On timeout, assume that the TPM is not working */
		if (stopwatch_expired(&timer))
			return false;

		mdelay(1);
	}
}

static void start_smbus_transaction(void)
{
	hwm_reg_write(SMBUS_MASTER_CFG1_REG, SMB_MASTER_EN | SMB_MASTER_START);
}


void nct6687d_smbus_init(uint16_t hwmbase)
{
	if (hwmbase)
		nct6687d_hwm_base = hwmbase;

	hwm_reg_write(SMBUS_MASTER_CFG2_REG, SMB_MASTER_PORT(4));
	hwm_reg_write(SMBUS_MASTER_BAUD_RATE_SEL_REG, SMB_MASTER_BAUD_100K);
}

int nct6687d_smbus_read_byte(uint8_t address, uint8_t command, uint8_t *data)
{
	uint8_t error;

	if (!data)
		return -1;

	prepare_smbus_transaction(SMBUS_MASTER_READ_BYTE, address, command, 0);

	hwm_reg_write(SMBUS_MASTER_CFG1_REG, SMB_MASTER_EN | SMB_MASTER_CLEAR_BUF);
	if (!wait_smbus_status_bit_clear(SMB_MASTER_CLEAR_BUF))
		return -2;

	start_smbus_transaction();

	if (!wait_smbus_status_bit_clear(SMB_MASTER_START))
		return -3;

	error = hwm_reg_read(SMBUS_MASTER_ERR_STS_REG);

	if (error != 0)
		return (int)error;

	*data = hwm_reg_read(SMBUS_MASTER_READ_BUFFER(0));

	return 0;
}

int nct6687d_smbus_read_word(uint8_t address, uint8_t command, uint16_t *data)
{
	uint8_t error;

	if (!data)
		return -1;

	prepare_smbus_transaction(SMBUS_MASTER_READ_WORD, address, command, 0);

	hwm_reg_write(SMBUS_MASTER_CFG1_REG, SMB_MASTER_EN | SMB_MASTER_CLEAR_BUF);
	if (!wait_smbus_status_bit_clear(SMB_MASTER_CLEAR_BUF))
		return -2;

	start_smbus_transaction();

	if (!wait_smbus_status_bit_clear(SMB_MASTER_START))
		return -3;

	error = hwm_reg_read(SMBUS_MASTER_ERR_STS_REG);

	if (error != 0)
		return (int)error;

	*data = hwm_reg_read(SMBUS_MASTER_READ_BUFFER(0));
	*data |= hwm_reg_read(SMBUS_MASTER_READ_BUFFER(1)) << 8;

	return 0;
}

int nct6687d_smbus_write_byte(uint8_t address, uint8_t command, uint8_t data)
{
	uint8_t error;

	prepare_smbus_transaction(SMBUS_MASTER_WRITE_BYTE, address, command, 0);

	hwm_reg_write(SMBUS_MASTER_WRITE_BUFFER(0), data);

	start_smbus_transaction();

	if (!wait_smbus_status_bit_clear(SMB_MASTER_START))
		return -3;

	error = hwm_reg_read(SMBUS_MASTER_ERR_STS_REG);

	if (error != 0)
		return (int)error;

	return 0;
}

int nct6687d_smbus_write_word(uint8_t address, uint8_t command, uint16_t data)
{
	uint8_t error;

	prepare_smbus_transaction(SMBUS_MASTER_WRITE_WORD, address, command, 0);

	hwm_reg_write(SMBUS_MASTER_WRITE_BUFFER(0), data & 0xff);
	hwm_reg_write(SMBUS_MASTER_WRITE_BUFFER(1), data >> 8);

	start_smbus_transaction();

	if (!wait_smbus_status_bit_clear(SMB_MASTER_START))
		return -3;

	error = hwm_reg_read(SMBUS_MASTER_ERR_STS_REG);

	if (error != 0)
		return (int)error;

	return 0;
}

int nct6687d_smbus_block_read(uint8_t address, uint8_t command, uint8_t *data, uint8_t *length)
{
	uint8_t error;
	uint8_t read_len;

	if (!data || !length || *length >= SMBUS_MASTER_READ_BUFFER_LEN || *length == 0)
		return -1;

	prepare_smbus_transaction(SMBUS_MASTER_BLOCK_READ, address, command, 0);

	if (!wait_smbus_status_bit_clear(SMB_MASTER_CLEAR_BUF))
		return -2;

	start_smbus_transaction();

	if (!wait_smbus_status_bit_clear(SMB_MASTER_START))
		return -3;

	error = hwm_reg_read(SMBUS_MASTER_ERR_STS_REG);

	if (error != 0)
		return (int)error;

	read_len = hwm_reg_read(SMBUS_MASTER_READ_BUFFER(0));

	if (read_len > *length || read_len >= SMBUS_MASTER_READ_BUFFER_LEN)
		return -4;

	for (int i = 0; i <= read_len; i++)
		data[i] = hwm_reg_read(SMBUS_MASTER_READ_BUFFER(i + 1));

	return 0;
}

int nct6687d_smbus_block_write(uint8_t address, uint8_t command, uint8_t *data, uint8_t length)
{
	uint8_t error;

	if (!data || length > SMBUS_MASTER_WRITE_BUFFER_LEN || length == 0)
		return -1;

	prepare_smbus_transaction(SMBUS_MASTER_BLOCK_WRITE, address, command, length);

	hwm_reg_write(SMBUS_MASTER_CFG1_REG, SMB_MASTER_EN | SMB_MASTER_CLEAR_BUF);
	if (!wait_smbus_status_bit_clear(SMB_MASTER_CLEAR_BUF))
		return -2;

	for (int i = 0; i < length; i++)
		hwm_reg_write(SMBUS_MASTER_WRITE_BUFFER(i), data[i]);

	start_smbus_transaction();

	if (!wait_smbus_status_bit_clear(SMB_MASTER_START))
		return -3;

	error = hwm_reg_read(SMBUS_MASTER_ERR_STS_REG);

	if (error != 0)
		return (int)error;

	return 0;
}
