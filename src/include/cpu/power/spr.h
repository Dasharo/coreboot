#ifndef CPU_PPC64_SPR_H
#define CPU_PPC64_SPR_H

#include <arch/byteorder.h>	// PPC_BIT()

#define SPR_DEC					0x16
#define SPR_DEC_IMPLEMENTED_BITS		56
#define SPR_DEC_LONGEST_TIME			((1ull << (SPR_DEC_IMPLEMENTED_BITS - 1)) - 1)

#define SPR_DAWR				0xB4
#define SPR_CIABR				0xBB
#define SPR_DAWRX				0xBC
#define SPR_TB					0x10C
#define SPR_HSPRG0				0x130
#define SPR_HDEC				0x136
#define SPR_HRMOR				0x139

#define SPR_LPCR				0x13E
#define SPR_LPCR_LD				PPC_BIT(46)
#define SPR_LPCR_HEIC				PPC_BIT(59)
#define SPR_LPCR_HDICE				PPC_BIT(63)

#define SPR_HMER				0x150
#define SPR_HMEER				0x151
/* Bits in HMER/HMEER */
#define SPR_HMER_MALFUNCTION_ALERT		PPC_BIT(0)
#define SPR_HMER_PROC_RECV_DONE			PPC_BIT(2)
#define SPR_HMER_PROC_RECV_ERROR_MASKED		PPC_BIT(3)
#define SPR_HMER_TFAC_ERROR			PPC_BIT(4)
#define SPR_HMER_TFMR_PARITY_ERROR		PPC_BIT(5)
#define SPR_HMER_XSCOM_FAIL			PPC_BIT(8)
#define SPR_HMER_XSCOM_DONE			PPC_BIT(9)
#define SPR_HMER_PROC_RECV_AGAIN		PPC_BIT(11)
#define SPR_HMER_WARN_RISE			PPC_BIT(14)
#define SPR_HMER_WARN_FALL			PPC_BIT(15)
#define SPR_HMER_SCOM_FIR_HMI			PPC_BIT(16)
#define SPR_HMER_TRIG_FIR_HMI			PPC_BIT(17)
#define SPR_HMER_HYP_RESOURCE_ERR		PPC_BIT(20)
#define SPR_HMER_XSCOM_STATUS			PPC_BITMASK(21,23)
#define SPR_HMER_XSCOM_OCCUPIED			PPC_BIT(23)

#define SPR_PTCR				0x1D0
#define SPR_PSSCR				0x357
#define SPR_PMCR				0x374

#ifndef __ASSEMBLER__
#include <types.h>

static inline uint64_t read_spr(int spr)
{
	uint64_t val;
	asm volatile("mfspr %0,%1" : "=r"(val) : "i"(spr) : "memory");
	return val;
}

static inline void write_spr(int spr, uint64_t val)
{
	asm volatile("mtspr %0, %1" :: "i"(spr), "r"(val) : "memory");
}


static inline uint64_t read_hmer(void)
{
	return read_spr(SPR_HMER);
}

static inline void clear_hmer(void)
{
	write_spr(SPR_HMER, 0);
}

static inline uint64_t read_msr(void)
{
	uint64_t val;
	asm volatile("mfmsr %0" : "=r"(val) :: "memory");
	return val;
}

static inline void write_msr(uint64_t val)
{
	asm volatile("mtmsrd %0" :: "r"(val) : "memory");
}

#endif /* __ASSEMBLER__ */
#endif /* CPU_PPC64_SPR_H */
