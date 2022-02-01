/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef PORTING_H
#define PORTING_H

/* Create the Boolean type */
#define TRUE  1
#define FALSE 0
typedef unsigned char BOOL;

#define CALLCONV


typedef struct _uint64
{
	u32 lo;
	u32 hi;
} uint64;


/*
 *   SBDFO - Segment Bus Device Function Offset
 *   31:28   Segment (4-bits)
 *   27:20   Bus     (8-bits)
 *   19:15   Device  (5-bits)
 *   14:12   Function(3-bits)
 *   11:00   Offset  (12-bits)
 */
typedef u32 SBDFO;

#define MAKE_SBDFO(seg,bus,dev,fun,off) ((((u32)(seg)) << 28) | (((u32)(bus)) << 20) | \
		    (((u32)(dev)) << 15) | (((u32)(fun)) << 12) | ((u32)(off)))
#define SBDFO_SEG(x) (((u32)(x) >> 28) & 0x0F)
#define SBDFO_BUS(x) (((u32)(x) >> 20) & 0xFF)
#define SBDFO_DEV(x) (((u32)(x) >> 15) & 0x1F)
#define SBDFO_FUN(x) (((u32)(x) >> 12) & 0x07)
#define SBDFO_OFF(x) (((u32)(x)) & 0xFFF)
#define ILLEGAL_SBDFO 0xFFFFFFFF

void CALLCONV amd_msr_read(u32 address, uint64 *value);
void CALLCONV amd_msr_write(u32 address, uint64 *value);
void CALLCONV amd_io_read(u8 io_size, u16 address, u32 *value);
void CALLCONV amd_io_write(u8 io_size, u16 address, u32 *value);
void CALLCONV amd_mem_read(u8 mem_size, uint64 *address, u32 *value);
void CALLCONV amd_mem_write(u8 mem_size, uint64 *address, u32 *value);
void CALLCONV amd_pci_read(SBDFO loc, u32 *value);
void CALLCONV amd_pci_write(SBDFO loc, u32 *value);
void CALLCONV amd_cpuid_read(u32 address, u32 regs[4]);

#define BYTESIZE 1
#define WORDSIZE 2
#define DWORDSIZE 4

#endif /* PORTING_H */
