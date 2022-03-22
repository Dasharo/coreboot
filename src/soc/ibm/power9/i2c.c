/* SPDX-License-Identifier: GPL-2.0-only */

/* Debugging every access takes too much time */
#define SKIP_SCOM_DEBUG

#include <device/i2c_simple.h>
#include <console/console.h>
#include <cpu/power/proc.h>
#include <cpu/power/scom.h>
#include <spd_bin.h>

#include "fsi.h"

/* Base FSI address for registers of an FSI I2C master */
#define I2C_HOST_MASTER_BASE_ADDR 0xA0004

#define FIFO_REG		0
#define CMD_REG			1
#define MODE_REG		2
#define STATUS_REG		7
#define RESET_REG		7
#define RES_ERR_REG		8

// CMD register
#define LEN_PLACE(x)		PPC_PLACE((x), 16, 16)
#define ADDR_PLACE(x)		PPC_PLACE((x), 8, 7)
#define READ_NOT_WRITE		0x0001000000000000
#define START			0x8000000000000000
#define WITH_ADDR		0x4000000000000000
#define READ_CONT		0x2000000000000000
#define STOP			0x1000000000000000

// STATUS register
#define DATA_REQUEST		0x0200000000000000
#define CMD_COMPLETE		0x0100000000000000
#define FIFO_COUNT_FLD		0x0000000F00000000
#define BUSY			0x0000030000000000
#define SCL			0x0000080000000000
#define SDA			0x0000040000000000
#define UNRECOVERABLE		0xFC80000000000000

#define I2C_MAX_FIFO_CAPACITY	8

enum i2c_type {
	HOST_I2C_CPU0, // I2C via XSCOM (first CPU)
	HOST_I2C_CPU1, // I2C via XSCOM (second CPU)
	FSI_I2C,       // I2C via FSI (second CPU)
};

/* return -1 if SMBus errors otherwise return 0 */
static int get_spd(uint8_t bus, u8 *spd, u8 addr)
{
	/*
	 * Second half of DIMMs is on the second I2C port. platform_i2c_transfer()
	 * changes this automatically for SPD and RCD, but not for SPD page select.
	 * For those commands, set MSB that is later masked out.
	 */
	uint8_t fix = addr & 0x80;

	if (i2c_read_bytes(bus, addr, 0, spd, SPD_PAGE_LEN) < 0) {
		printk(BIOS_INFO, "No memory DIMM at address %02X\n", addr);
		return -1;
	}

	/* DDR4 spd is 512 byte. Switch to page 1 */
	i2c_writeb(bus, SPD_PAGE_1 | fix, 0, 0);

	/* No need to check again if DIMM is present */
	i2c_read_bytes(bus, addr, 0, spd + SPD_PAGE_LEN, SPD_PAGE_LEN);
	/* Restore to page 0 */
	i2c_writeb(bus, SPD_PAGE_0 | fix, 0, 0);

	return 0;
}

static u8 spd_data[MAX_CHIPS][CONFIG_DIMM_MAX][CONFIG_DIMM_SPD_SIZE];

void get_spd_i2c(uint8_t bus, struct spd_block *blk)
{
	u8 i;
	u8 chip = bus / I2C_BUSES_PER_CPU;

	for (i = 0 ; i < CONFIG_DIMM_MAX; i++) {
		if (blk->addr_map[i] == 0) {
			blk->spd_array[i] = NULL;
			continue;
		}

		if (get_spd(bus, spd_data[chip][i], blk->addr_map[i]) == 0)
			blk->spd_array[i] = spd_data[chip][i];
		else
			blk->spd_array[i] = NULL;
	}

	blk->len = SPD_PAGE_LEN_DDR4;
}

/* The four functions below use 64-bit address and data as for SCOM and do
 * translation for FSI which is 32-bit (as is actual I2C interface). They also
 * interpret address as register number for FSI I2C. */

static void write_i2c(enum i2c_type type, uint64_t addr, uint64_t data)
{
	if (type != FSI_I2C)
		write_rscom(type == HOST_I2C_CPU0 ? 0 : 1, addr, data);
	else
		write_fsi_i2c(/*chip=*/1, addr, data >> 32, /*size=*/4);
}

static uint64_t read_i2c(enum i2c_type type, uint64_t addr)
{
	if (type != FSI_I2C)
		return read_rscom(type == HOST_I2C_CPU0 ? 0 : 1, addr);
	else
		return (uint64_t)read_fsi_i2c(/*chip=*/1, addr, /*size=*/4) << 32;
}

static void write_i2c_byte(enum i2c_type type, uint64_t addr, uint8_t data)
{
	if (type != FSI_I2C)
		write_rscom(type == HOST_I2C_CPU0 ? 0 : 1, addr, (uint64_t)data << 56);
	else
		write_fsi_i2c(/*chip=*/1, addr, (uint32_t)data << 24, /*size=*/1);
}

static uint8_t read_i2c_byte(enum i2c_type type, uint64_t addr)
{
	if (type != FSI_I2C)
		return read_rscom(type == HOST_I2C_CPU0 ? 0 : 1, addr) >> 56;
	else
		return read_fsi_i2c(/*chip=*/1, addr, /*size=*/1) >> 24;
}

/*
 * There are 4 buses/engines, but the function accepts bus [0-8] in order to
 * allow specifying buses of the second CPU and I2C FSI bus while still
 * following coreboot's prototype for this function. [0-3] are buses of the
 * first CPU, [4-7] of the second one (0-3 correspondingly) and 8 is FSI I2C of
 * the second CPU.
 */
int platform_i2c_transfer(unsigned int bus, struct i2c_msg *segment,
			  int seg_count)
{
	int bytes_transfered = 0;
	int i;
	uint64_t r;

	enum i2c_type type = HOST_I2C_CPU0;
	if (bus >= I2C_BUSES_PER_CPU) {
		bus -= I2C_BUSES_PER_CPU;
		type = HOST_I2C_CPU1;
	}
	if (bus >= I2C_BUSES_PER_CPU) {
		bus -= I2C_BUSES_PER_CPU;
		type = FSI_I2C;

		/* There seems to be only one engine on FSI I2C */
		if (bus != 0) {
			printk(BIOS_ERR, "FSI I2C bus out of range (%d)\n", bus);
			return -1;
		}
	}

	if (bus >= I2C_BUSES_PER_CPU) {
		printk(BIOS_ERR, "I2C bus out of range (%d)\n", bus);
		return -1;
	}

	uint32_t base = (type == FSI_I2C ? 0 : I2C_HOST_MASTER_BASE_ADDR | (bus << 12));
	/* Addition is fine, because there will be no carry in bus number bits */
	uint32_t fifo_reg = base + FIFO_REG;
	uint32_t cmd_reg = base + CMD_REG;
	uint32_t mode_reg = base + MODE_REG;
	uint32_t status_reg = base + STATUS_REG;
	uint32_t res_err_reg = base + RES_ERR_REG;

	uint64_t clear_err = (type != FSI_I2C ? PPC_BIT(0) : 0);

	write_i2c(type, res_err_reg, clear_err);

	for (i = 0; i < seg_count; i++) {
		unsigned int len;
		uint64_t read_not_write, stop, read_cont, port;

		/* Only read for now, implement different flags when needed */
		if (segment[i].flags & ~I2C_M_RD) {
			printk(BIOS_ERR, "Unsupported I2C flags (0x%4.4x)\n", segment[i].flags);
			return -1;
		}

		read_not_write = (segment[i].flags & I2C_M_RD) ? READ_NOT_WRITE : 0;
		stop = (i == seg_count - 1) ? STOP : 0;
		read_cont = (!stop && !read_not_write) ? READ_CONT : 0;
		port = segment[i].slave & 0x80 ? 1 : 0;

		/*
		 * Divisor fields in this register are poorly documented:
		 *
		 *	 Bits     SCOM   Field Mnemonic: Description
		 *	0:7       RWX    BIT_RATE_DIVISOR_3: Decides the speed on the I2C bus.
		 *	8:9       RWX    BIT_RATE_DIVISOR_3: Decides the speed on the I2C bus.
		 *	10:15     RWX    BIT_RATE_DIVISOR_3: Decides the speed on the I2C bus.
		 *
		 * After issuing a fast command (SCOM A3000) they change like this:
		 * -  100 kHz - previous value is not changed
		 * -   50 kHz - 0x000B
		 * - 3400 kHz - 0x005E
		 * -  400 kHz - 0x0177
		 *
		 * Use value for 400 kHz as it is the one used by Hostboot.
		 */
		write_i2c(type, mode_reg,
			  0x0177000000000000 | PPC_PLACE(port, 16, 6));	// 400kHz

		write_i2c(type, res_err_reg, clear_err);
		write_i2c(type, cmd_reg,
			  START | stop | WITH_ADDR | read_not_write | read_cont |
			  ADDR_PLACE(segment[i].slave) |
			  LEN_PLACE(segment[i].len));

		for (len = 0; len < segment[i].len; len++, bytes_transfered++) {
			r = read_i2c(type, status_reg);

			if (read_not_write) {
				/* Read */
				while ((r & (DATA_REQUEST | FIFO_COUNT_FLD)) == 0) {
					if (r & UNRECOVERABLE) {
						/* This may be DIMM not present so use low verbosity */
						printk(BIOS_INFO, "I2C transfer failed (0x%16.16llx)\n", r);
						return -1;
					}
					r = read_i2c(type, status_reg);
				}

				segment[i].buf[len] = read_i2c_byte(type, fifo_reg);
			}
			else
			{
				/* Write */
				while ((r & DATA_REQUEST) == 0) {
					if (r & UNRECOVERABLE) {
						printk(BIOS_INFO, "I2C transfer failed (0x%16.16llx)\n", r);
						return -1;
					}
					r = read_i2c(type, status_reg);
				}

				write_i2c_byte(type, fifo_reg, segment[i].buf[len]);
			}
		}

		r = read_i2c(type, status_reg);
		while ((r & CMD_COMPLETE) == 0) {
			if (r & UNRECOVERABLE) {
				printk(BIOS_INFO, "I2C transfer failed to complete (0x%16.16llx)\n", r);
				return -1;
			}
			r = read_i2c(type, status_reg);
		}

	}

	return bytes_transfered;
}

/* Defined in fsi.h */
void fsi_i2c_init(uint8_t chips)
{
	uint64_t status;

	/* Nothing to do if second CPU isn't present */
	if (!(chips & 0x02))
		return;

	/*
	 * Sometimes I2C status looks like 0x_____8__ (i.e., SCL is set, but
	 * not SDA), which indicates I2C hardware is in a messed up state that
	 * it won't leave on its own.  Sending an additional STOP *before* reset
	 * addresses this and doesn't hurt when I2C isn't broken.
	 */
	write_i2c(FSI_I2C, CMD_REG, STOP);

	/* Reset I2C */
	write_i2c(FSI_I2C, RESET_REG, 0);

	/* Wait for SCL */
	status = read_i2c(FSI_I2C, STATUS_REG);
	while ((status & SCL) == 0) {
		if (status & UNRECOVERABLE)
			die("Unrecoverable I2C error while waiting for SCL: 0x%016llx\n",
			    status);
		status = read_i2c(FSI_I2C, STATUS_REG);
	}

	/* Send STOP command */
	write_i2c(FSI_I2C, CMD_REG, STOP);

	status = read_i2c(FSI_I2C, STATUS_REG);
	while ((status & CMD_COMPLETE) == 0) {
		if (status & UNRECOVERABLE)
			die("Unrecoverable I2C error on STOP: 0x%016llx\n", status);
		status = read_i2c(FSI_I2C, STATUS_REG);
	}

	if ((status & (SCL | SDA | BUSY)) != (SCL | SDA))
		die("Invalid I2C state after initialization: 0x%016llx\n", status);
}
