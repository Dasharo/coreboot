/* SPDX-License-Identifier: GPL-2.0-only */

#include <console/console.h>
#include <cpu/power/scom.h>
#include <cpu/power/occ.h>
#include <cpu/power/proc.h>

#include "ops.h"

#define OCB_PIB_OCBCSR0_OCB_STREAM_MODE (4)
#define OCB_PIB_OCBCSR0_OCB_STREAM_TYPE (5)

#define OCB_OCI_OCBSHCS0_PUSH_ENABLE (31)
#define OCB_OCI_OCBSHCS0_PUSH_FULL   (0)

#define PU_OCB_PIB_OCBCSR0_RO (0x0006D011)
#define PU_OCB_PIB_OCBCSR1_RO (0x0006D031)

#define PU_OCB_OCI_OCBSHCS0_SCOM (0x0006C204)
#define PU_OCB_OCI_OCBSHCS1_SCOM (0x0006C214)

#define EX_PPM_SPWKUP_OCC (0x200F010C)
#define PU_OCB_PIB_OCBAR0 (0x0006D010)

#define PU_OCB_PIB_OCBDR0 (0x0006D015)
#define PU_OCB_PIB_OCBDR1 (0x0006D035)

#define PU_OCB_PIB_OCBCSR0_OR (0x0006D013)
#define PU_OCB_PIB_OCBCSR0_CLEAR (0x0006D012)

/* FIR register offset from base */
enum fir_offset {
	BASE_WAND_INCR  = 1,
	BASE_WOR_INCR   = 2,
	MASK_INCR       = 3,
	MASK_WAND_INCR  = 4,
	MASK_WOR_INCR   = 5,
	ACTION0_INCR    = 6,
	ACTION1_INCR    = 7
};

static void pm_ocb_setup(uint8_t chip, uint32_t ocb_bar)
{
	write_scom(chip, PU_OCB_PIB_OCBCSR0_OR, PPC_BIT(OCB_PIB_OCBCSR0_OCB_STREAM_MODE));
	write_scom(chip, PU_OCB_PIB_OCBCSR0_CLEAR, PPC_BIT(OCB_PIB_OCBCSR0_OCB_STREAM_TYPE));
	write_scom(chip, PU_OCB_PIB_OCBAR0, (uint64_t)ocb_bar << 32);
}

static void check_ocb_mode(uint8_t chip, uint64_t OCBCSR_address, uint64_t OCBSHCS_address)
{
	uint64_t ocb_pib = read_scom(chip, OCBCSR_address);

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
		uint64_t stream_push_control = read_scom(chip, OCBSHCS_address);

		if (stream_push_control & PPC_BIT(OCB_OCI_OCBSHCS0_PUSH_ENABLE)) {
			uint8_t counter = 0;
			for (counter = 0; counter < 4; counter++) {
				/* Proceed if the OCB_OCI_OCBSHCS0_PUSH_FULL is clear */
				if (!(stream_push_control & PPC_BIT(OCB_OCI_OCBSHCS0_PUSH_FULL)))
					break;

				stream_push_control = read_scom(chip, OCBSHCS_address);
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

void writeOCCSRAM(uint8_t chip, uint32_t address, uint64_t *buffer, size_t data_length)
{
	pm_ocb_setup(chip, address);
	put_ocb_indirect(chip, data_length / 8, address, buffer);
}

void readOCCSRAM(uint8_t chip, uint32_t address, uint64_t *buffer, size_t data_length)
{
	pm_ocb_setup(chip, address);
	get_ocb_indirect(chip, data_length / 8, address, buffer);
}

void write_occ_command(uint8_t chip, uint64_t write_data)
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
	writeOCCSRAM(chip, OCC_SRAM_BOOT_ADDR, sram_program, sizeof(sram_program));

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
