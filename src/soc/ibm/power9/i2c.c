/* SPDX-License-Identifier: GPL-2.0-only */

#include <device/i2c_simple.h>
#include <console/console.h>
#include <cpu/power/scom.h>
#include <spd_bin.h>

#define FIFO_REG(bus)		(0xA0004 | ((bus) << 12))
#define CMD_REG(bus)		(0xA0005 | ((bus) << 12))
#define MODE_REG(bus)		(0xA0006 | ((bus) << 12))
#define STATUS_REG(bus)		(0xA000B | ((bus) << 12))
#define RES_ERR_REG(bus)	(0xA000C | ((bus) << 12))

// CMD register
#define LEN_SHIFT(x)		PPC_SHIFT((x), 31)
#define ADDR_SHIFT(x)		PPC_SHIFT((x), 14)
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
#define UNRECOVERABLE		0xFC80000000000000

#define CLEAR_ERR		0x8000000000000000

#define I2C_MAX_FIFO_CAPACITY	8
#define SPD_I2C_BUS		3

/* return -1 if SMBus errors otherwise return 0 */
static int get_spd(u8 *spd, u8 addr)
{
	/*
	 * Second half of DIMMs is on the second I2C port. platform_i2c_transfer()
	 * changes this automatically for SPD and RCD, but not for SPD page select.
	 * For those commands, set MSB that is later masked out.
	 */
	uint8_t fix = addr & 0x80;

	if (i2c_read_bytes(SPD_I2C_BUS, addr, 0, spd, SPD_PAGE_LEN) < 0) {
		printk(BIOS_INFO, "No memory DIMM at address %02X\n", addr);
		return -1;
	}

	/* DDR4 spd is 512 byte. Switch to page 1 */
	i2c_writeb(SPD_I2C_BUS, SPD_PAGE_1 | fix, 0, 0);

	/* No need to check again if DIMM is present */
	i2c_read_bytes(SPD_I2C_BUS, addr, 0, spd + SPD_PAGE_LEN, SPD_PAGE_LEN);
	/* Restore to page 0 */
	i2c_writeb(SPD_I2C_BUS, SPD_PAGE_0 | fix, 0, 0);

	return 0;
}

static u8 spd_data[CONFIG_DIMM_MAX * CONFIG_DIMM_SPD_SIZE];

void get_spd_smbus(struct spd_block *blk)
{
	u8 i;

	for (i = 0 ; i < CONFIG_DIMM_MAX; i++) {
		if (blk->addr_map[i] == 0) {
			blk->spd_array[i] = NULL;
			continue;
		}

		if (get_spd(&spd_data[i * CONFIG_DIMM_SPD_SIZE], blk->addr_map[i]) == 0)
			blk->spd_array[i] = &spd_data[i * CONFIG_DIMM_SPD_SIZE];
		else
			blk->spd_array[i] = NULL;
	}

	blk->len = SPD_PAGE_LEN_DDR4;
}

int platform_i2c_transfer(unsigned int bus, struct i2c_msg *segment,
			  int seg_count)
{
	int bytes_transfered = 0;
	int i;
	uint64_t r;

	if (bus > 3) {
		printk(BIOS_ERR, "I2C bus out of range (%d)\n", bus);
		return -1;
	}

	write_scom(RES_ERR_REG(bus), CLEAR_ERR);

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
		write_scom(MODE_REG(bus), 0x0177000000000000 | PPC_SHIFT(port, 21));	// 400kHz

		write_scom(RES_ERR_REG(bus), CLEAR_ERR);
		write_scom(CMD_REG(bus), START | stop | WITH_ADDR | read_not_write | read_cont |
		                         ADDR_SHIFT(segment[i].slave & 0x7F) |
		                         LEN_SHIFT(segment[i].len));

		for (len = 0; len < segment[i].len; len++, bytes_transfered++) {
			r = read_scom(STATUS_REG(bus));

			if (read_not_write) {
				/* Read */
				while ((r & (DATA_REQUEST | FIFO_COUNT_FLD)) == 0) {
					if (r & UNRECOVERABLE) {
						/* This may be DIMM not present so use low verbosity */
						printk(BIOS_INFO, "I2C transfer failed (0x%16.16llx)\n", r);
						return -1;
					}
					r = read_scom(STATUS_REG(bus));
				}

				r = read_scom(FIFO_REG(bus));
				segment[i].buf[len] = r >> 56;
			}
			else
			{
				/* Write */
				while ((r & DATA_REQUEST) == 0) {
					if (r & UNRECOVERABLE) {
						printk(BIOS_INFO, "I2C transfer failed (0x%16.16llx)\n", r);
						return -1;
					}
					r = read_scom(STATUS_REG(bus));
				}

				write_scom(FIFO_REG(bus), (uint64_t) segment[i].buf[len] << 56);
			}
		}

		r = read_scom(STATUS_REG(bus));
		while ((r & CMD_COMPLETE) == 0) {
			if (r & UNRECOVERABLE) {
				printk(BIOS_INFO, "I2C transfer failed (0x%16.16llx)\n", r);
				return -1;
			}
			r = read_scom(STATUS_REG(bus));
		}

	}

	return bytes_transfered;
}
