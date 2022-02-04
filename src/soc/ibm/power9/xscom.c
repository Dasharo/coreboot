/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <cpu/power/scom.h>
#include <cpu/power/spr.h>		// HMER
#include <console/console.h>

#define MMIO_GROUP0_CHIP0_SCOM_BASE_ADDR	0x800603FC00000000
#define MMIO_GROUP_SIZE				0x0000200000000000

#define XSCOM_ADDR_IND_FLAG			PPC_BIT(0)
#define XSCOM_ADDR_IND_ADDR			PPC_BITMASK(11,31)
#define XSCOM_ADDR_IND_DATA			PPC_BITMASK(48,63)

#define XSCOM_DATA_IND_READ			PPC_BIT(0)
#define XSCOM_DATA_IND_COMPLETE			PPC_BIT(32)
#define XSCOM_DATA_IND_ERR			PPC_BITMASK(33,35)
#define XSCOM_DATA_IND_DATA			PPC_BITMASK(48,63)
#define XSCOM_DATA_IND_FORM1_DATA		PPC_BITMASK(12,63)
#define XSCOM_IND_MAX_RETRIES			10

#define XSCOM_RCVED_STAT_REG			0x00090018
#define XSCOM_LOG_REG				0x00090012
#define XSCOM_ERR_REG				0x00090013

static void write_xscom_direct(uint8_t chip, uint64_t reg_address, uint64_t data);

static void reset_xscom_engine(uint8_t chip)
{
	/*
	 * With cross-CPU SCOM accesses, first register should be cleared on the
	 * executing CPU, the other two on target CPU. In that case it may be
	 * necessary to do the remote writes in assembly directly to skip checking
	 * HMER and possibly end in a loop.
	 */
	write_xscom_direct(0, XSCOM_RCVED_STAT_REG, 0);
	write_xscom_direct(chip, XSCOM_LOG_REG, 0);
	write_xscom_direct(chip, XSCOM_ERR_REG, 0);
	clear_hmer();
	eieio();
}

static uint64_t read_xscom_direct(uint8_t chip, uint64_t reg_address)
{
	uint64_t val;
	uint64_t hmer = 0;
	do {
		/*
		 * Clearing HMER on every SCOM access seems to slow down CCS up
		 * to a point where it starts hitting timeout on "less ideal"
		 * DIMMs for write centering. Clear it only if this do...while
		 * executes more than once.
		 */
		if ((hmer & SPR_HMER_XSCOM_STATUS) == SPR_HMER_XSCOM_OCCUPIED)
			clear_hmer();

		eieio();
		asm volatile(
			"ldcix %0, %1, %2":
			"=r"(val):
			"b"(MMIO_GROUP0_CHIP0_SCOM_BASE_ADDR + chip * MMIO_GROUP_SIZE ),
			"r"(reg_address << 3));
		eieio();
		hmer = read_hmer();
	} while ((hmer & SPR_HMER_XSCOM_STATUS) == SPR_HMER_XSCOM_OCCUPIED);

	if (hmer & SPR_HMER_XSCOM_STATUS) {
		reset_xscom_engine(chip);
		/*
		 * All F's are returned in case of error, but code polls for a set bit
		 * after changes that can make such error appear (e.g. clock settings).
		 * Return 0 so caller won't have to test for all F's in that case.
		 */
		return 0;
	}
	return val;
}

static void write_xscom_direct(uint8_t chip, uint64_t reg_address, uint64_t data)
{
	uint64_t hmer = 0;
	do {
		/* See comment in read_xscom_direct() */
		if ((hmer & SPR_HMER_XSCOM_STATUS) == SPR_HMER_XSCOM_OCCUPIED)
			clear_hmer();

		eieio();
		asm volatile(
			"stdcix %0, %1, %2"::
			"r"(data),
			"b"(MMIO_GROUP0_CHIP0_SCOM_BASE_ADDR + chip * MMIO_GROUP_SIZE ),
			"r"(reg_address << 3));
		eieio();
		hmer = read_hmer();
	} while ((hmer & SPR_HMER_XSCOM_STATUS) == SPR_HMER_XSCOM_OCCUPIED);

	if (hmer & SPR_HMER_XSCOM_STATUS)
		reset_xscom_engine(chip);
}

static void write_xscom_indirect(uint8_t chip, uint64_t reg_address, uint64_t value)
{
	uint64_t addr;
	uint64_t data;
	addr = reg_address & 0x7FFFFFFF;
	data = reg_address & XSCOM_ADDR_IND_ADDR;
	data |= value & XSCOM_ADDR_IND_DATA;

	write_xscom_direct(chip, addr, data);

	for (int retries = 0; retries < XSCOM_IND_MAX_RETRIES; ++retries) {
		data = read_xscom_direct(chip, addr);
		if((data & XSCOM_DATA_IND_COMPLETE) && ((data & XSCOM_DATA_IND_ERR) == 0)) {
			return;
		}
		else if(data & XSCOM_DATA_IND_COMPLETE) {
			printk(BIOS_EMERG, "SCOM WR error  %16.16llx = %16.16llx : %16.16llx\n",
			       reg_address, value, data);
		}
		// TODO: delay?
	}
}

static uint64_t read_xscom_indirect(uint8_t chip, uint64_t reg_address)
{
	uint64_t addr;
	uint64_t data;
	addr = reg_address & 0x7FFFFFFF;
	data = XSCOM_DATA_IND_READ | (reg_address & XSCOM_ADDR_IND_ADDR);

	write_xscom_direct(chip, addr, data);

	for (int retries = 0; retries < XSCOM_IND_MAX_RETRIES; ++retries) {
		data = read_xscom_direct(chip, addr);
		if((data & XSCOM_DATA_IND_COMPLETE) && ((data & XSCOM_DATA_IND_ERR) == 0)) {
			break;
		}
		else if(data & XSCOM_DATA_IND_COMPLETE) {
			printk(BIOS_EMERG, "SCOM RD error  %16.16llx : %16.16llx\n",
			       reg_address, data);
		}
		// TODO: delay?
	}

	return data & XSCOM_DATA_IND_DATA;
}

/* Private API used only by SCOM dispatcher, no need to expose it */
void write_xscom(uint8_t chip, uint64_t addr, uint64_t data);
uint64_t read_xscom(uint8_t chip, uint64_t addr);

void write_xscom(uint8_t chip, uint64_t addr, uint64_t data)
{
	if (addr & XSCOM_ADDR_IND_FLAG)
		write_xscom_indirect(chip, addr, data);
	else
		write_xscom_direct(chip, addr, data);
}

uint64_t read_xscom(uint8_t chip, uint64_t addr)
{
	if (addr & XSCOM_ADDR_IND_FLAG)
		return read_xscom_indirect(chip, addr);
	else
		return read_xscom_direct(chip, addr);
}
