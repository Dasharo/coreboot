/* SPDX-License-Identifier: GPL-2.0-only */

#include <memlayout.h>
#include <fmap_config.h>
#include <psp_verstage/psp_transfer.h>
#include <soc/psp_verstage_addr.h>

ENTRY(_psp_vs_start)
SECTIONS
{
	SRAM_START(PSP_SRAM_START)
	_verstage = .;

		.text : { *(PSP_HEADER_DATA) }
		.text : { *(.text._psp_vs_start) }
		.text : { *(.text.Main) }
		.text : { *(.text*) }
		.rodata : {
			*(.rodata*);

			. = ALIGN(ARCH_POINTER_ALIGN_SIZE);
			_tis_drivers = .;
			KEEP(*(.rodata.tis_driver));
			_etis_drivers = .;
			RECORD_SIZE(tis_drivers)
		}

		.data : {
			*(.data*);
			*(PSP_FOOTER_DATA);
#if CONFIG(CBFS_VERIFICATION)
			*(.metadata_hash_anchor);
			. = ALIGN(64);
#endif
		}

		_bss_start = .;
		.bss : { *(.bss*) }
		_bss_end = .;

	ALIGN_COUNTER(64)
	_everstage = .;

	#include "memlayout_transfer_buffer.inc"

#if defined(PSP_VERSTAGE_TEMP_STACK_START)
	PSP_VERSTAGE_TEMP_STACK_END = (PSP_VERSTAGE_TEMP_STACK_START + PSP_VERSTAGE_TEMP_STACK_SIZE );
#endif

	REGION(stack, PSP_VERSTAGE_STACK_START, PSP_VERSTAGE_STACK_SIZE, 64)
	PSP_VERSTAGE_STACK_BASE = _stack;
	PSP_VERSTAGE_STACK_END = _estack;
	SRAM_END(PSP_SRAM_START + PSP_SRAM_SIZE)
}
