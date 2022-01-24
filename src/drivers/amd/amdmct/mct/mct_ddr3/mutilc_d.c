/* SPDX-License-Identifier: GPL-2.0-only */

/* This file contains functions for common utility functions */

#include <arch/cpu.h>
#include <stdint.h>

#include <drivers/amd/amdmct/wrappers/mcti.h>
#include "mct_d_gcc.h"
#include "mwlc_d.h"

void amd_mem_pci_read_bits(SBDFO loc, u8 high_bit, u8 low_bit, u32 *p_value)
{
	/* ASSERT(high_bit < 32 && low_bit < 32 && high_bit >= low_bit && (loc & 3) == 0); */

	amd_mem_pci_read(loc, p_value);
	*p_value = *p_value >> low_bit;  /* Shift */

	/* A 1<<32 == 1<<0 due to x86 SHL instruction, so skip if that is the case */
	if ((high_bit-low_bit) != 31)
		*p_value &= (((u32)1 << (high_bit - low_bit + 1)) - 1);
}

void amd_mem_pci_write_bits(SBDFO loc, u8 high_bit, u8 low_bit, u32 *p_value)
{
	u32 temp, mask;

	/* ASSERT(high_bit < 32 && low_bit < 32 && high_bit >= low_bit && (loc & 3) == 0); */

	/* A 1<<32 == 1<<0 due to x86 SHL instruction, so skip if that is the case */
	if ((high_bit-low_bit) != 31)
		mask = (((u32)1 << (high_bit - low_bit + 1)) - 1);
	else
		mask = (u32)0xFFFFFFFF;

	amd_mem_pci_read(loc, &temp);
	temp &= ~(mask << low_bit);
	temp |= (*p_value & mask) << low_bit;
	amd_mem_cpi_write(loc, &temp);
}

/*-----------------------------------------------------------------------------
 * u32 bit_test_set(u32 cs_mask,u32 temp_d)
 *
 * Description:
 *     This routine sets a bit in a u32
 *
 * Parameters:
 *     IN        cs_mask = Target value in which the bit will be set
 *     IN        temp_d     =  Bit that will be set
 *     OUT    value     =  Target value with the bit set
 *-----------------------------------------------------------------------------
 */
u32 bit_test_set(u32 cs_mask,u32 temp_d)
{
	u32 local_temp;
	/* ASSERT(temp_d < 32); */
	local_temp = 1;
	cs_mask |= local_temp << temp_d;
	return cs_mask;
}

/*-----------------------------------------------------------------------------
 * u32 bit_test_reset(u32 cs_mask,u32 temp_d)
 *
 * Description:
 *     This routine re-sets a bit in a u32
 *
 * Parameters:
 *     IN        cs_mask = Target value in which the bit will be re-set
 *     IN        temp_d     =  Bit that will be re-set
 *     OUT    value     =  Target value with the bit re-set
 *-----------------------------------------------------------------------------
 */
u32 bit_test_reset(u32 cs_mask, u32 temp_d)
{
	u32 temp, local_temp;
	/* ASSERT(temp_d < 32); */
	local_temp = 1;
	temp = local_temp << temp_d;
	temp = ~temp;
	cs_mask &= temp;
	return cs_mask;
}

/*-----------------------------------------------------------------------------
 *  u32 get_bits(DCTStruct *DCTData, u8 DCT, u8 Node, u8 func, u16 offset,
 *                 u8 low, u8 high)
 *
 * Description:
 *     This routine Gets the PCT bits from the specified Node, DCT and PCI address
 *
 * Parameters:
 *   IN  OUT *DCTData - Pointer to buffer with information about each DCT
 *     IN        DCT - DCT number
 *              - 1 indicates DCT 1
 *              - 0 indicates DCT 0
 *              - 2 both DCTs
 *          Node - Node number
 *          Func - PCI Function number
 *          Offset - PCI register number
 *          Low - Low bit of the bit field
 *          High - High bit of the bit field
 *
 *     OUT    value     =  Value read from PCI space
 *-----------------------------------------------------------------------------
 */
u32 get_bits(sDCTStruct *p_dct_data,
		u8 dct, u8 node, u8 func,
		u16 offset, u8 low, u8 high)
{
	u32 temp;
	u32 dword;

	/* ASSERT(node < MAX_NODES); */
	if (dct == BOTH_DCTS)
	{
		/* Registers exist on DCT0 only */
		if (is_fam15h())
		{
			/* Select DCT 0 */
			amd_mem_pci_read(MAKE_SBDFO(0, 0, 24 + node, 1, 0x10c), &dword);
			dword &= ~0x1;
			amd_mem_cpi_write(MAKE_SBDFO(0, 0, 24 + node, 1, 0x10c), &dword);
		}

		amd_mem_pci_read_bits(MAKE_SBDFO(0, 0, 24 + node, func, offset), high, low, &temp);
	}
	else
	{
		if (is_fam15h())
		{
			/* Select DCT */
			amd_mem_pci_read(MAKE_SBDFO(0, 0, 24 + node, 1, 0x10c), &dword);
			dword &= ~0x1;
			dword |= (dct & 0x1);
			amd_mem_cpi_write(MAKE_SBDFO(0, 0, 24 + node, 1, 0x10c), &dword);

			/* Read from the selected DCT */
			amd_mem_pci_read_bits(MAKE_SBDFO(0, 0, 24 + node, func, offset), high, low, &temp);
		}
		else
		{
			if (dct == 1)
			{
				/* Read from dct 1 */
				offset += 0x100;
				amd_mem_pci_read_bits(MAKE_SBDFO(0, 0, 24 + node, func, offset), high, low, &temp);
			}
			else
			{
				/* Read from dct 0 */
				amd_mem_pci_read_bits(MAKE_SBDFO(0, 0, 24 + node, func, offset), high, low, &temp);
			}
		}
	}
	return temp;
}

/*-----------------------------------------------------------------------------
 *  void set_bits(DCTStruct *DCTData,u8 DCT,u8 Node,u8 func, u16 offset,
 *                u8 low, u8 high, u32 value)
 *
 * Description:
 *     This routine Sets the PCT bits from the specified Node, DCT and PCI address
 *
 * Parameters:
 *   IN  OUT *DCTData - Pointer to buffer with information about each DCT
 *     IN        DCT - DCT number
 *              - 1 indicates DCT 1
 *              - 0 indicates DCT 0
 *              - 2 both DCTs
 *          Node - Node number
 *          Func - PCI Function number
 *          Offset - PCI register number
 *          Low - Low bit of the bit field
 *          High - High bit of the bit field
 *
 *     OUT
 *-----------------------------------------------------------------------------
 */
void set_bits(sDCTStruct *p_dct_data,
		u8 dct, u8 node, u8 func,
		u16 offset, u8 low, u8 high, u32 value)
{
	u32 temp;
	u32 dword;

	temp = value;

	if (dct == BOTH_DCTS)
	{
		/* Registers exist on DCT0 only */
		if (is_fam15h())
		{
			/* Select DCT 0 */
			amd_mem_pci_read(MAKE_SBDFO(0, 0, 24 + node, 1, 0x10c), &dword);
			dword &= ~0x1;
			amd_mem_cpi_write(MAKE_SBDFO(0, 0, 24 + node, 1, 0x10c), &dword);
		}

		amd_mem_pci_write_bits(MAKE_SBDFO(0, 0, 24 + node, func, offset), high, low, &temp);
	}
	else
	{
		if (is_fam15h())
		{
			/* Select DCT */
			amd_mem_pci_read(MAKE_SBDFO(0, 0, 24 + node, 1, 0x10c), &dword);
			dword &= ~0x1;
			dword |= (dct & 0x1);
			amd_mem_cpi_write(MAKE_SBDFO(0, 0, 24 + node, 1, 0x10c), &dword);

			/* Write to the selected DCT */
			amd_mem_pci_write_bits(MAKE_SBDFO(0, 0, 24 + node, func, offset), high, low, &temp);
		}
		else
		{
			if (dct == 1)
			{
				/* Write to dct 1 */
				offset += 0x100;
				amd_mem_pci_write_bits(MAKE_SBDFO(0, 0, 24 + node, func, offset), high, low, &temp);
			}
			else
			{
				/* Write to dct 0 */
				amd_mem_pci_write_bits(MAKE_SBDFO(0, 0, 24 + node, func, offset), high, low, &temp);
			}
		}
	}
}

/*-------------------------------------------------
 *  u32 get_add_dct_bits(DCTStruct *DCTData,u8 DCT,u8 Node,u8 func,
 *                         u16 offset,u8 low, u8 high)
 *
 * Description:
 *     This routine gets the Additional PCT register from Function 2 by specified
 *   Node, DCT and PCI address
 *
 * Parameters:
 *   IN  OUT *DCTData - Pointer to buffer with information about each DCT
 *     IN        DCT - DCT number
 *              - 1 indicates DCT 1
 *              - 0 indicates DCT 0
 *              - 2 both DCTs
 *          Node - Node number
 *          Func - PCI Function number
 *          Offset - Additional PCI register number
 *          Low - Low bit of the bit field
 *          High - High bit of the bit field
 *
 *     OUT
 *-------------------------------------------------
 */
u32 get_add_dct_bits(sDCTStruct *p_dct_data,
		u8 dct, u8 node, u8 func,
		u16 offset, u8 low, u8 high)
{
	u32 temp_d;
	temp_d = offset;
	temp_d = bit_test_reset(temp_d,DCT_ACCESS_WRITE);
	set_bits(p_dct_data, dct, node, FUN_DCT, DRAM_CONTROLLER_ADD_DATA_OFFSET_REG,
		PCI_MIN_LOW, PCI_MAX_HIGH, offset);
	while ((get_bits(p_dct_data,dct, node, FUN_DCT, DRAM_CONTROLLER_ADD_DATA_OFFSET_REG,
			DCT_ACCESS_DONE, DCT_ACCESS_DONE)) == 0);
	return (get_bits(p_dct_data, dct, node, FUN_DCT, DRAM_CONTROLLER_ADD_DATA_PORT_REG,
			low, high));
}

/*-------------------------------------------------
 *  void set_dct_addr_bits(DCTStruct *DCTData, u8 DCT,u8 Node,u8 func,
 *                         u16 offset,u8 low, u8 high, u32 value)
 *
 * Description:
 *     This routine sets the Additional PCT register from Function 2 by specified
 *   Node, DCT and PCI address
 *
 * Parameters:
 *   IN  OUT *DCTData - Pointer to buffer with information about each DCT
 *     IN        DCT - DCT number
 *              - 1 indicates DCT 1
 *              - 0 indicates DCT 0
 *              - 2 both DCTs
 *          Node - Node number
 *          Func - PCI Function number
 *          Offset - Additional PCI register number
 *          Low - Low bit of the bit field
 *          High - High bit of the bit field
 *
 *     OUT
 *-------------------------------------------------
 */
void set_dct_addr_bits(sDCTStruct *p_dct_data,
		u8 dct, u8 node, u8 func,
		u16 offset, u8 low, u8 high, u32 value)
{
	u32 temp_d;

	set_bits(p_dct_data, dct, node, FUN_DCT, DRAM_CONTROLLER_ADD_DATA_OFFSET_REG,
		PCI_MIN_LOW, PCI_MAX_HIGH, offset);
	while ((get_bits(p_dct_data, dct, node, FUN_DCT, DRAM_CONTROLLER_ADD_DATA_OFFSET_REG,
			DCT_ACCESS_DONE, DCT_ACCESS_DONE)) == 0);

	set_bits(p_dct_data, dct, node, FUN_DCT, DRAM_CONTROLLER_ADD_DATA_PORT_REG,
		low, high, value);
	temp_d = offset;
	temp_d = bit_test_set(temp_d,DCT_ACCESS_WRITE);
	set_bits(p_dct_data, dct, node, FUN_DCT,DRAM_CONTROLLER_ADD_DATA_OFFSET_REG,
		PCI_MIN_LOW, PCI_MAX_HIGH, temp_d);
	while ((get_bits(p_dct_data, dct, p_dct_data->NodeId, FUN_DCT,
			DRAM_CONTROLLER_ADD_DATA_OFFSET_REG, DCT_ACCESS_DONE,
			DCT_ACCESS_DONE)) == 0);
}

/*-------------------------------------------------
 * bool bit_test(u32 value, u8 bit_loc)
 *
 * Description:
 *     This routine tests the value to determine if the bit_loc is set
 *
 * Parameters:
 *     IN        Value - value to be tested
 *          bit_loc - bit location to be tested
 *     OUT    TRUE - bit is set
 *          FALSE - bit is clear
 *-------------------------------------------------
 */
bool bit_test(u32 value, u8 bit_loc)
{
	u32 temp_d, comp_d;
	temp_d = value;
	comp_d = 0;
	comp_d = bit_test_set(comp_d,bit_loc);
	temp_d &= comp_d;
	if (comp_d == temp_d)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}
