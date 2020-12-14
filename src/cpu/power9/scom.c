/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <cpu/power/scom.h>
#include <cpu/power/spr.h>		// HMER
#include <console/console.h>

#define XSCOM_DATA_IND_READ			PPC_BIT(0)
#define XSCOM_DATA_IND_COMPLETE			PPC_BIT(32)
#define XSCOM_DATA_IND_ERR			PPC_BITMASK(33,35)
#define XSCOM_DATA_IND_DATA			PPC_BITMASK(48,63)
#define XSCOM_DATA_IND_FORM1_DATA		PPC_BITMASK(12,63)
#define XSCOM_IND_MAX_RETRIES			10

#define XSCOM_RCVED_STAT_REG			0x00090018
#define XSCOM_LOG_REG				0x00090012
#define XSCOM_ERR_REG				0x00090013

void write_scom_indirect(uint64_t reg_address, uint64_t value)
{
	uint64_t addr;
	uint64_t data;
	addr = reg_address & 0x7FFFFFFF;
	data = reg_address & XSCOM_ADDR_IND_ADDR;
	data |= value & XSCOM_ADDR_IND_DATA;

	write_scom_direct(addr, data);

	for (int retries = 0; retries < XSCOM_IND_MAX_RETRIES; ++retries) {
		data = read_scom_direct(addr);
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

uint64_t read_scom_indirect(uint64_t reg_address)
{
	uint64_t addr;
	uint64_t data;
	addr = reg_address & 0x7FFFFFFF;
	data = XSCOM_DATA_IND_READ | (reg_address & XSCOM_ADDR_IND_ADDR);

	write_scom_direct(addr, data);

	for (int retries = 0; retries < XSCOM_IND_MAX_RETRIES; ++retries) {
		data = read_scom_direct(addr);
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

/* This function should be rarely called, don't make it inlined */
void reset_scom_engine(void)
{
	/*
	 * With cross-CPU SCOM accesses, first register should be cleared on the
	 * executing CPU, the other two on target CPU. In that case it may be
	 * necessary to do the remote writes in assembly directly to skip checking
	 * HMER and possibly end in a loop.
	 */
	write_scom_direct(XSCOM_RCVED_STAT_REG, 0);
	write_scom_direct(XSCOM_LOG_REG, 0);
	write_scom_direct(XSCOM_ERR_REG, 0);
	clear_hmer();
	eieio();
}
