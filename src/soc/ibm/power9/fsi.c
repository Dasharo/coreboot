/* SPDX-License-Identifier: GPL-2.0-only */

/* FSI is used to read MVPD, logging which takes too much time */
#define SKIP_SCOM_DEBUG

#include "fsi.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <console/console.h>
#include <cpu/power/scom.h>
#include <delay.h>

/*
 * Some of the code relies on the fact that we're only interested in the current
 * CPU (its MFSI) and the other CPU on port #1. Nothing is actually connected to
 * any other ports, so using chip==0 for MFSI of CPU and chip==1 (port #1) and
 * not being able to work with port #0 is OK. Getting rid of this requires
 * passing both chip and port around (possibly encoded into a single variable),
 * which is unnecessary otherwise.
 */

enum {
	MAX_SLAVE_PORTS = 8,

	FSI2OPB_OFFSET_0 = 0x00020000, // SCOM address for FSI interactions

	MFSI_CONTROL_REG = 0x003400, // MFSI Control Register

	OPB_REG_CMD  = 0x0000, // Command Register
	OPB_REG_STAT = 0x0001, // Status Register
	OPB_REG_RES  = 0x0004, // Reset Register

	/* FSI Control Registers */
	FSI_MMODE_000  = 0x000,
	FSI_MDLYR_004  = 0x004,
	FSI_MLEVP0_018 = 0x018,
	FSI_MSENP0_018 = 0x018,
	FSI_MCENP0_020 = 0x020,
	FSI_MSIEP0_030 = 0x030,
	FSI_MAEB_070   = 0x070, // MREFP0
	FSI_MRESP0_0D0 = 0x0D0,
	FSI_MESRB0_1D0 = 0x1D0,
	FSI_MECTRL_2E0 = 0x2E0,

	/* FSI2PIB Engine (SCOM) */
	FSI2PIB_ENGINE   = 0x001000,
	FSI2PIB_RESET    = FSI2PIB_ENGINE | 0x18, // see CFAM 1006
	FSI2PIB_COMPMASK = FSI2PIB_ENGINE | 0x30, // see CFAM 100C
	FSI2PIB_TRUEMASK = FSI2PIB_ENGINE | 0x34, // see CFAM 100D

	/* MFSI Ports (512KB for each of 8 slaves) */
	MFSI_PORT_0 = 0x080000,

	/* FSI Slave Register */
	SLAVE_REGS = 0x000800,
	SMODE_00   = SLAVE_REGS | 0x00,
	SLRES_34   = SLAVE_REGS | 0x34,

	/* Bitmasks for OPB status register */
	OPB_STAT_ANYERR     = 0x8000000000000000, // 0 is Any error
	OPB_STAT_ERR_OPB    = 0x7FEC000000000000, // 1:10,12:13 are OPB errors
	OPB_STAT_ERRACK     = 0x0010000000000000, // 11 is OPB errAck
	OPB_STAT_READ_VALID = 0x0002000000000000, // 14 is the Valid Read bit
	OPB_STAT_BUSY       = 0x0001000000000000, // 15 is the Busy bit
	OPB_STAT_ERR_CMFSI  = 0x0000FC0000000000, // 16:21 are cMFSI errors
	OPB_STAT_ERR_MFSI   = 0x000000FC00000000, // 24:29 are MFSI errors

	OPB_STAT_NON_MFSI_ERR = (OPB_STAT_ERR_OPB |
				 OPB_STAT_ERRACK |
				 OPB_STAT_ANYERR),
	OPB_STAT_ERR_ANY      = (OPB_STAT_NON_MFSI_ERR |
				 OPB_STAT_ERR_CMFSI |
				 OPB_STAT_ERR_MFSI),
};

void fsi_reset_pib2opb(uint8_t chip)
{
	write_rscom(chip, FSI2OPB_OFFSET_0 | OPB_REG_RES, 0x8000000000000000);
	write_rscom(chip, FSI2OPB_OFFSET_0 | OPB_REG_STAT, 0x8000000000000000);
}

static void cleanup_port_maeb_error(uint8_t port)
{
	/* See comment at the top of the file */
	const uint8_t master_chip = 0;
	const uint8_t slave_chip = port;

	uint32_t compmask;
	uint32_t truemask;

	/*
	 * Reset the bridge to clear up the residual errors
	 * 0=Bridge: General reset
	 */
	write_fsi(master_chip, MFSI_CONTROL_REG | FSI_MESRB0_1D0, 0x80000000);

	/*
	 * Perform error reset on Centaur FSI slave: write 0x4000000 to addr=834
	 *
	 * Hostboot does this unconditionally, even though not all Power9 models
	 * have Centaur chips. Kept here just in case.
	 */
	write_fsi(slave_chip, SLRES_34, 0x4000000);

	/* Need to save/restore the true/comp masks or the FSP will get annoyed */
	compmask = read_fsi(slave_chip, FSI2PIB_COMPMASK);
	truemask = read_fsi(slave_chip, FSI2PIB_TRUEMASK);

	/* Then, write arbitrary data to 1018 (putcfam 1006) to reset any
	 * pending FSI2PIB errors */
	write_fsi(slave_chip, FSI2PIB_RESET, 0xFFFFFFFF);

	/* Restore the true/comp masks */
	write_fsi(slave_chip, FSI2PIB_COMPMASK, compmask);
	write_fsi(slave_chip, FSI2PIB_TRUEMASK, truemask);
}

static void init_fsi_port(uint8_t port)
{
	/* See comment at the top of the file */
	const uint8_t master_chip = 0;
	const uint8_t slave_chip = port;

	uint8_t port_bit = (0x80 >> port);

	/* Write the port enable (enables clocks for FSI link) */
	write_fsi(master_chip, MFSI_CONTROL_REG | FSI_MSENP0_018, (uint32_t)port_bit << 24);

	/* Hostboot reads FSI_MESRB0_1D0 and does nothing to it, skipped here
	 * with the assumption that it has no effect */

	/*
	 * Send the BREAK command to all slaves on this port (target slave0)
	 * part of FSI definition, write magic string into address zero.
	 */
	write_fsi(slave_chip, 0x00, 0xC0DE0000);

	if (read_fsi(master_chip, MFSI_CONTROL_REG | FSI_MAEB_070) != 0) {
		/* Alternative is to pretend this slave doesn't exist */
		die("Detected MAEB error on FSI port #%d.\n", port);
	}

	/*
	 * Setup the FSI slave to enable HW recovery, lbus ratio
	 * 2= Enable HW error recovery (bit 2)
	 * 6:7= Slave ID: 3 (default)
	 * 8:11= Echo delay: 0xF (default)
	 * 12:15= Send delay cycles: 0xF
	 * 20:23= Local bus ratio: 0x1
	 */
	write_fsi(slave_chip, SMODE_00, 0x23FF0100);

	/* Wait for a little bit to be sure everything is done */
	udelay(1000); // 1ms

	/*
	 * Reset the port to clear up any previous error state (using idec reg
	 * as arbitrary address for lookups). Note, initial cfam reset should
	 * have cleaned up everything but this makes sure we're in a consistent
	 * state.
	 */
	cleanup_port_maeb_error(port);
}

static void basic_master_init(void)
{
	const uint8_t chip = 0;
	const uint32_t ctrl_reg = MFSI_CONTROL_REG;

	uint64_t tmp;

	/* Cleanup any initial error states */
	fsi_reset_pib2opb(chip);

	/* Ensure we don't have any errors before we even start */
	tmp = read_scom(FSI2OPB_OFFSET_0 | OPB_REG_STAT);
	if (tmp & OPB_STAT_NON_MFSI_ERR)
		die("Unclearable errors on MFSI initialization: 0x%016llx\n", tmp);

	/*
	 * Setup clock ratios and some error checking
	 * 1= Enable hardware error recovery
	 * 3= Enable parity checking
	 * 4:13= FSI clock ratio 0 is 1:1
	 * 14:23= FSI clock ratio 1 is 4:1
	 */
	write_fsi(chip, ctrl_reg | FSI_MMODE_000, 0x50040400);

	/*
	 * Setup error control reg to do nothing
	 * 16= Enable OPB_errAck [=1]
	 * 18= Freeze FSI port on FSI/OPB bridge error [=0]
	 */
	write_fsi(chip, ctrl_reg | FSI_MECTRL_2E0, 0x00008000);

	/*
	 * Note that this actually resets 4 ports twice rather than 8 ports
	 * once: OR makes 0x01XX equivalent to 0x00XX due to 0xD being
	 * 0b00001101 and ORing 0b00000001 to it changes nothing. Hostboot does
	 * it this way...
	 */
	for (uint8_t port = 0; port < MAX_SLAVE_PORTS; port++) {
		/*
		 * 0= Port: General reset
		 * 1= Port: Error reset
		 * 2= General reset to all bridges
		 * 3= General reset to all port controllers
		 * 4= Reset all FSI Master control registers
		 * 5= Reset parity error source latch
		 */
		write_fsi(chip, ctrl_reg | FSI_MRESP0_0D0 | (port * 4), 0xFC000000);
	}

	/* Wait a little bit to be sure the reset is done */
	udelay(1000); // 1ms delay

	/*
	 * Setup error control reg for regular use
	 * (somehow this is the same as "to do nothing", a bug in Hostboot?)
	 * 16= Enable OPB_errAck [=1]
	 * 18= Freeze FSI port on FSI/OPB bridge error [=0]
	 */
	write_fsi(chip, ctrl_reg | FSI_MECTRL_2E0, 0x00008000);

	/*
	 * Set MMODE reg to enable HW recovery, parity checking, setup clock
	 * ratio
	 * 1= Enable hardware error recovery
	 * 3= Enable parity checking
	 * 4:13= FSI clock ratio 0 is 1:1
	 * 14:23= FSI clock ratio 1 is 4:1
	 */
	tmp = 0x50040400;
	/*
	 * Setup timeout so that:
	 *   code(10ms) > masterproc (0.9ms) > remote fsi master (0.8ms)
	 */
	tmp |= 0x00000010; // 26:27= Timeout (b01) = 0.9ms
	write_fsi(chip, ctrl_reg | FSI_MMODE_000, tmp);
}

static void basic_slave_init(void)
{
	const uint8_t chip = 0;
	const uint32_t ctrl_reg = MFSI_CONTROL_REG;

	uint64_t tmp;

	/* Clear FSI Slave Interrupt on ports 0-7 */
	write_fsi(chip, ctrl_reg | FSI_MSIEP0_030, 0x00000000);

	/*
	 * Set the delay rates:
	 * 0:3,8:11= Echo delay cycles is 15
	 * 4:7,12:15= Send delay cycles is 15
	 */
	write_fsi(chip, ctrl_reg | FSI_MDLYR_004, 0xFFFF0000);

	/* Enable the ports */
	write_fsi(chip, ctrl_reg | FSI_MSENP0_018, 0xFF000000);

	udelay(1000); // 1ms

	/* Clear the port enable */
	write_fsi(chip, ctrl_reg | FSI_MCENP0_020, 0xFF000000);

	/*
	 * Reset all bridges and ports (again?).
	 * Line above is from Hostboot. Actually this seems to reset only port
	 * 0 and with a bit different mask (0xFC000000 above).
	 */
	write_fsi(chip, ctrl_reg | FSI_MRESP0_0D0, 0xF0000000);

	/* Wait a little bit to be sure reset is done */
	udelay(1000); // 1ms

	/* Note: not enabling IPOLL because hotplug is not supported */

	/* Turn off Legacy mode */
	tmp = read_fsi(chip, ctrl_reg | FSI_MMODE_000);
	tmp &= ~0x00000040; // bit 25: clock/4 mode
	write_fsi(chip, ctrl_reg | FSI_MMODE_000, tmp);
}

void fsi_init(void)
{
	uint8_t chips;

	basic_master_init();
	basic_slave_init();

	chips = fsi_get_present_chips();
	if (chips & 0x2)
		init_fsi_port(/*port=*/1);

	fsi_i2c_init(chips);
}

uint8_t fsi_get_present_chips(void)
{
	const uint8_t chip = 0;
	const uint32_t ctrl_reg = MFSI_CONTROL_REG;

	uint8_t chips;
	uint8_t present_slaves;

	present_slaves = (read_fsi(chip, ctrl_reg | FSI_MLEVP0_018) >> 24);

	/* First CPU is always there (it executes this code) */
	chips = 0x01;
	/* Status of the second CPU (connected to port #1) */
	chips |= ((present_slaves & 0x40) >> 5);

	return chips;
}

/* Polls OPB dying on error or timeout */
static inline uint64_t poll_opb(uint8_t chip)
{
	enum {
		MAX_WAIT_LOOPS = 1000,
		TIMEOUT_STEP_US = 10,
	};

	const uint64_t stat_addr = FSI2OPB_OFFSET_0 | OPB_REG_STAT;

	int i;
	uint64_t tmp;

	uint64_t err_mask;

	/* MFSI are irrelevant for access to the chip we're running on, only
	 * OPB bits are of interest */
	err_mask = OPB_STAT_NON_MFSI_ERR;
	if (chip == 1) {
		/* Second CPU is routed through MFSI of the first CPU */
		err_mask |= OPB_STAT_ERR_MFSI;
	}

	/* Timeout after 10ms, check every 10us, supposedly there is hardware
	 * timeout after 1ms */
	tmp = read_scom(stat_addr);
	for (i = 0; (tmp & OPB_STAT_BUSY) && !(tmp & err_mask) && i < MAX_WAIT_LOOPS; i++) {
		udelay(TIMEOUT_STEP_US);
		tmp = read_scom(stat_addr);
	}

	if (tmp & err_mask)
		die("Detected an error while polling OPB for chip #%d: 0x%016llx\n", chip, tmp);

	if (i == MAX_WAIT_LOOPS) {
		die("Timed out while polling OPB for chip #%d, last response: 0x%016llx\n",
		    chip, tmp);
	}

	return tmp;
}

uint32_t fsi_op(uint8_t chip, uint32_t addr, uint32_t data, bool is_read, uint8_t size)
{
	enum {
		WRITE_NOT_READ = PPC_BIT(0),
		SIZE_1B = PPC_PLACE(0, 1, 2),
		SIZE_4B = PPC_PLACE(3, 1, 2),
	};

	uint64_t cmd;
	uint64_t response;

	assert(size == 1 || size == 4);

	/* See comment at the top of the file */
	if (chip != 0) {
		const uint8_t port = chip;
		addr |= MFSI_PORT_0 * (port + 1);
	}

	/* Make sure there are no other ops running before we start. The
	 * function will die on error, so not handling return value. */
	(void)poll_opb(chip);

	/*
	 * Register is mentioned in the docs, but contains mostly reserved
	 * fields. This is what can be decoded from code:
	 * [0]     WRITE_NOT_READ = 1 for write, 0 for read
	 * [1-2]   size           = 3      // 0b00 - 1B; 0b01 - 2B; 0b11 - 4B
	 * [3-31]  FSI address    = addr   // FSI spec says address is 23 bits
	 * [32-63] data to write  = data   // don't care for read
	 */
	cmd = (size == 4 ? SIZE_4B : SIZE_1B) | PPC_PLACE(addr, 3, 29) | data;
	if (!is_read)
		cmd |= WRITE_NOT_READ;

	write_scom(FSI2OPB_OFFSET_0 | OPB_REG_CMD, cmd);

	/* Poll for complete and get the data back. */
	response = poll_opb(chip);

	/* A write operation is done if poll_opb hasn't died */
	if (!is_read)
		return 0;

	if (!(response & OPB_STAT_READ_VALID))
		die("FSI read has failed.\n");
	return (response & 0xFFFFFFFF);
}
