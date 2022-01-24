/* SPDX-License-Identifier: GPL-2.0-only */

#include "comlib.h"

#include <device/pci.h>
#include <device/pci_ops.h>
#include <cpu/amd/msr.h>
#include <device/pci_def.h>


/*
 *---------------------------------------------------------------------------
 * EXPORTED FUNCTIONS
 *
 *---------------------------------------------------------------------------
 */

void CALLCONV amd_pci_read_bits(SBDFO loc, u8 highbit, u8 lowbit, u32 *p_value)
{
	ASSERT(highbit < 32 && lowbit < 32 && highbit >= lowbit && (loc & 3) == 0);

	AmdPCIRead(loc, p_value);
	*p_value = *p_value >> lowbit;  /* Shift */

	/* A 1<<32 == 1<<0 due to x86 SHL instruction, so skip if that is the case */
	if ((highbit-lowbit) != 31)
		*p_value &= (((u32)1 << (highbit - lowbit + 1)) - 1);
}


void CALLCONV amd_pci_write_bits(SBDFO loc, u8 highbit, u8 lowbit, u32 *p_value)
{
	u32 temp, mask;

	ASSERT(highbit < 32 && lowbit < 32 && highbit >= lowbit && (loc & 3) == 0);

	/* A 1<<32 == 1<<0 due to x86 SHL instruction, so skip if that is the case */
	if ((highbit-lowbit) != 31)
		mask = (((u32)1 << (highbit - lowbit + 1)) - 1);
	else
		mask = (u32)0xFFFFFFFF;

	AmdPCIRead(loc, &temp);
	temp &= ~(mask << lowbit);
	temp |= (*p_value & mask) << lowbit;
	AmdPCIWrite(loc, &temp);
}


/*
 *  Given a SBDFO this routine will find the next PCI capabilities list entry.
 *   If the end of the list of reached, or if a problem is detected, then
 *   ILLEGAL_SBDFO is returned.
 *
 *   To start a new search from the beginning of head of the list, specify a
 *   SBDFO with a offset of zero.
 */
void CALLCONV AmdPCIFindNextCap(SBDFO *pCurrent)
{
	SBDFO base;
	u32 offset;
	u32 temp;

	if (*pCurrent == ILLEGAL_SBDFO)
		return;

	offset = SBDFO_OFF(*pCurrent);
	base = *pCurrent - offset;
	*pCurrent = ILLEGAL_SBDFO;

	/* Verify that the SBDFO points to a valid PCI device SANITY CHECK */
	AmdPCIRead(base, &temp);
	if (temp == 0xFFFFFFFF)
		return; /* There is no device at this address */

	/* Verify that the device supports a capability list */
	amd_pci_read_bits(base + 0x04, 20, 20, &temp);
	if (temp == 0)
		return; /* This PCI device does not support capability lists */

	if (offset != 0)
	{
		/* If we are continuing on an existing list */
		amd_pci_read_bits(base + offset, 15, 8, &temp);
	}
	else
	{
		/* We are starting on a new list */
		amd_pci_read_bits(base + 0x34, 7, 0, &temp);
	}

	if (temp == 0)
		return; /* We have reached the end of the capabilties list */

	/* Error detection and recovery- The statement below protects against
		PCI devices with broken PCI capabilities lists.	 Detect a pointer
		that is not u32 aligned, points into the first 64 reserved DWORDs
		or points back to itself.
	*/
	if (((temp & 3) != 0) || (temp == offset) || (temp < 0x40))
		return;

	*pCurrent = base + temp;
	return;
}


void CALLCONV Amdmemcpy(void *pDst, const void *pSrc, u32 length)
{
	ASSERT(length <= 32768);
	ASSERT(pDst != NULL);
	ASSERT(pSrc != NULL);

	while (length--) {
	//	*(((u8*)pDst)++) = *(((u8*)pSrc)++);
		*((u8*)pDst) = *((u8*)pSrc);
		pDst++;
		pSrc++;
	}
}


void CALLCONV Amdmemset(void *pBuf, u8 val, u32 length)
{
	ASSERT(length <= 32768);
	ASSERT(pBuf != NULL);

	while (length--) {
		//*(((u8*)pBuf)++) = val;
		*(((u8*)pBuf)) = val;
		pBuf++;
	}
}


u8 CALLCONV AmdBitScanReverse(u32 value)
{
	u8 i;

	for (i = 31; i != 0xFF; i--)
	{
		if (value & ((u32)1 << i))
			break;
	}

	return i;
}


u32 CALLCONV AmdRotateRight(u32 value, u8 size, u32 count)
{
	u32 msb, mask;
	ASSERT(size > 0 && size <= 32);

	msb = (u32)1 << (size-1);
	mask = ((msb-1) << 1) + 1;

	value = value & mask;

	while (count--)
	{
		if (value & 1)
			value = (value >> 1) | msb;
		else
			value = value >> 1;
	}

	return value;
}


u32 CALLCONV AmdRotateLeft(u32 value, u8 size, u32 count)
{
	u32 msb, mask;
	ASSERT(size > 0 && size <= 32);

	msb = (u32)1 << (size-1);
	mask = ((msb-1) << 1) + 1;

	value = value & mask;

	while (count--)
	{
		if (value & msb)
			value = ((value << 1) & mask) | (u32)1;
		else
			value = ((value << 1) & mask);
	}

	return value;
}


void CALLCONV AmdPCIRead(SBDFO loc, u32 *Value)
{
	/* Use coreboot PCI functions */
	*Value = pci_read_config32((loc & 0xFFFFF000), SBDFO_OFF(loc));
}


void CALLCONV AmdPCIWrite(SBDFO loc, u32 *Value)
{
	/* Use coreboot PCI functions */
	pci_write_config32((loc & 0xFFFFF000), SBDFO_OFF(loc), *Value);
}


void CALLCONV AmdMSRRead(uint32 Address, uint64 *Value)
{
	msr_t msr;

	msr = rdmsr(Address);
	Value->lo = msr.lo;
	Value->hi = msr.hi;
}


void CALLCONV AmdMSRWrite(uint32 Address, uint64 *Value)
{
	msr_t msr;

	msr.lo = Value->lo;
	msr.hi = Value->hi;
	wrmsr(Address, msr);
}
