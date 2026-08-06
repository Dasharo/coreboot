/* SPDX-License-Identifier: BSD-2-Clause */

#include <stdint.h>
#include <security/intel/stm/StmApi.h>
#include <security/intel/stm/SmmStm.h>
#include <security/intel/stm/StmPlatformResource.h>

#if CONFIG(SOUTHBRIDGE_INTEL_COMMON_PMCLIB)
#include <southbridge/intel/common/pmutil.h>
#else
#include <soc/pm.h>
#endif

#if CONFIG(SOC_INTEL_COMMON_BLOCK_FAST_SPI)
#include <intelblocks/fast_spi.h>
#include <soc/intel/common/block/fast_spi/fast_spi_def.h>
#endif

#if CONFIG(SOC_INTEL_COMMON_BLOCK_P2SB)
#include <soc/iomap.h>
#endif

#include <arch/pci_io_cfg.h>
#include <cbmem.h>
#include <cpu/x86/msr.h>
#include <cpu/intel/msr.h>
#include <console/console.h>
#include <console/uart.h>
#include <security/intel/txt/txt_register.h>

#define RDWR_ACCS 3
#define FULL_ACCS 7

// Fixed memory ranges
//
// TSEG memory!
static STM_RSC_MEM_DESC rsc_tseg_memory = {{MEM_RANGE, sizeof(STM_RSC_MEM_DESC)},
				    0,
				    0,
				    FULL_ACCS};

// SMMSTORE communication buffer
static STM_RSC_MEM_DESC rsc_smmstore_comm_buffer_memory = {
				{MEM_RANGE, sizeof(STM_RSC_MEM_DESC)},
				0,
				0,
				RDWR_ACCS};

// cbmem console buffer
static STM_RSC_MEM_DESC rsc_cbmemc_memory = {
				{MEM_RANGE, sizeof(STM_RSC_MEM_DESC)},
				0,
				0,
				RDWR_ACCS};

// TPM PPI buffer
static STM_RSC_MEM_DESC rsc_tpm_ppi_memory = {
				{MEM_RANGE, sizeof(STM_RSC_MEM_DESC)},
				0,
				0,
				RDWR_ACCS};

// Flash part
static STM_RSC_MEM_DESC rsc_spi_memory = {
				{MEM_RANGE, sizeof(STM_RSC_MEM_DESC)},
				0xFF000000,
				0x01000000,
				FULL_ACCS};

static STM_RSC_MEM_DESC rsc_ext_spi_memory = {
				{MEM_RANGE, sizeof(STM_RSC_MEM_DESC)},
				CONFIG_EXT_BIOS_WIN_BASE,
				CONFIG_EXT_BIOS_WIN_SIZE,
				FULL_ACCS};

// P2SB
#if CONFIG(SOC_INTEL_COMMON_BLOCK_P2SB)
static STM_RSC_MMIO_DESC rsc_p2sb_mmio = {{MMIO_RANGE, sizeof(STM_RSC_MMIO_DESC)},
					   P2SB_BAR,
					   P2SB_SIZE, // Length
					   RDWR_ACCS};
#endif

#if CONFIG(SOC_INTEL_COMMON_BLOCK_P2SB2)
static STM_RSC_MMIO_DESC rsc_p2sb2_mmio = {{MMIO_RANGE, sizeof(STM_RSC_MMIO_DESC)},
					   P2SB2_BAR,
					   P2SB2_SIZE, // Length
					   RDWR_ACCS};
#endif

#if CONFIG(SOC_INTEL_COMMON_BLOCK_IOE_P2SB)
static STM_RSC_MMIO_DESC rsc_ioe_p2sb_mmio = {{MMIO_RANGE, sizeof(STM_RSC_MMIO_DESC)},
					      IOE_P2SB_BAR,
					      IOE_P2SB_SIZE, // Length
					      RDWR_ACCS};
#endif

// ACPI
static STM_RSC_IO_DESC rsc_pm_io = {{IO_RANGE, sizeof(STM_RSC_IO_DESC)}, 0, 128};

// PCI IO
static STM_RSC_IO_DESC rsc_pci_io = {{IO_RANGE, sizeof(STM_RSC_IO_DESC)},
				     PCI_IO_CONFIG_INDEX, PCI_IO_CONFIG_PORT_COUNT};

// UART
static STM_RSC_IO_DESC rsc_uart_io = {{IO_RANGE, sizeof(STM_RSC_IO_DESC)}, 0, 8};
static STM_RSC_IO_DESC rsc_uart_mmio = {{MMIO_RANGE, sizeof(STM_RSC_IO_DESC)}, 0, 8, RDWR_ACCS};

// PCIE MMIO
static STM_RSC_MMIO_DESC rsc_pcie_mmio = {{MMIO_RANGE, sizeof(STM_RSC_MMIO_DESC)},
				   CONFIG_ECAM_MMCONF_BASE_ADDRESS,
				   CONFIG_ECAM_MMCONF_LENGTH, // Length
				   RDWR_ACCS};

// Local APIC
static STM_RSC_MMIO_DESC rsc_apic_mmio = {{MMIO_RANGE, sizeof(STM_RSC_MMIO_DESC)},
				   0,
				   0x400,
				   RDWR_ACCS};

// Software SMI
static STM_RSC_TRAPPED_IO_DESC rsc_sw_smi_trap_io = {
				{TRAPPED_IO_RANGE, sizeof(STM_RSC_TRAPPED_IO_DESC)},
				0xB2,
				2};

// SPI MMIO
static STM_RSC_MMIO_DESC rsc_spi_mmio = {{MMIO_RANGE, sizeof(STM_RSC_MMIO_DESC)},
				0,
				0x1000, // Length
				RDWR_ACCS};

// TXT MMIO
static STM_RSC_MMIO_DESC rsc_txt_mmio = {{MMIO_RANGE, sizeof(STM_RSC_MMIO_DESC)},
				TXT_PRIVATE_SPACE,
				0x20000, // Length
				RDWR_ACCS};

// End of list
static STM_RSC_END rsc_list_end __attribute__((used)) = {
			{END_OF_RESOURCES, sizeof(STM_RSC_END)}, 0};

// Common PCI devices
//
// LPC bridge
STM_RSC_PCI_CFG_DESC rsc_lpc_bridge_pci = {
	{PCI_CFG_RANGE, sizeof(STM_RSC_PCI_CFG_DESC)},
	RDWR_ACCS,
	0,
	0,
	0x1000,
	0,
	0,
	{
		{1, 1, sizeof(STM_PCI_DEVICE_PATH_NODE), LPC_FUNCTION,
		 LPC_DEVICE},
	},
};

// SPI controller
STM_RSC_PCI_CFG_DESC rsc_spi_pci = {
	{PCI_CFG_RANGE, sizeof(STM_RSC_PCI_CFG_DESC)},
	RDWR_ACCS,
	0,
	0,
	0x1000,
	0,
	0,
	{
		{1, 1, sizeof(STM_PCI_DEVICE_PATH_NODE), SPI_FUNCTION,
		 LPC_DEVICE},
	},
};

// Template for MSR resources.
STM_RSC_MSR_DESC rsc_msr_tpl = {
	{MACHINE_SPECIFIC_REG, sizeof(STM_RSC_MSR_DESC)},
};

// MSR indices to register
typedef struct {
	uint32_t msr_index;
	uint64_t read_mask;
	uint64_t write_mask;
} MSR_TABLE_ENTRY;

MSR_TABLE_ENTRY msr_table[] = {
	// Index Read Write
	// MASK64 means need access, MASK0 means no need access.
	{SMRR_PHYSBASE_MSR, MASK64, MASK0},
	{SMRR_PHYSMASK_MSR, MASK64, MASK0},
#if !CONFIG(SOC_INTEL_COMMON_BLOCK_SMM_NO_MSR_SPCL_CHIPSET_USAGE)
	{MSR_SPCL_CHIPSET_USAGE, MASK64, BIT(0)},
#endif
};

static int fixup_cbmem_resource(uint32_t cbmem_id, uint64_t *base, uint64_t *length)
{
	const struct cbmem_entry *e;

	e = cbmem_entry_find(cbmem_id);
	if (!e)
		return -1;

	*base = (uintptr_t)cbmem_entry_start(e);
	*length = cbmem_entry_size(e);

	return 0;
}

static int add_fast_spi_resource(void)
{
	int Status = 0;

	rsc_spi_mmio.base = (uintptr_t)fast_spi_get_bar();

	Status |= add_pi_resource((void *)&rsc_spi_mmio, 1);
	Status |= add_pi_resource((void *)&rsc_spi_pci, 1);

	if (CONFIG(FAST_SPI_SUPPORTS_EXT_BIOS_WINDOW))
		Status |= add_pi_resource((void *)&rsc_ext_spi_memory, 1);

	return Status;
}

static int add_smm_debug_resource(void)
{
	int Status = 0;

	if (CONFIG(CONSOLE_CBMEM) &&
		!fixup_cbmem_resource(CBMEM_ID_CONSOLE,
					&rsc_cbmemc_memory.base,
					&rsc_cbmemc_memory.length)) {
		Status |= add_pi_resource((void *)&rsc_cbmemc_memory, 1);
	}

	if (CONFIG(CONSOLE_SERIAL) && CONFIG(DRIVERS_UART_8250IO)) {
		Status |= add_pi_resource((void *)&rsc_uart_io, 1);
	} else if (CONFIG(CONSOLE_SERIAL) && CONFIG(DRIVERS_UART_8250IO)) {
		rsc_uart_mmio.base = uart_platform_base(CONFIG_UART_FOR_CONSOLE);
		if (rsc_uart_mmio.base != 0)
			Status |= add_pi_resource((void *)&rsc_uart_mmio, 1);
	}

	return Status;
}

/*
 *  Add basic resources to BIOS resource database.
 */
static void add_simple_resources(void)
{
	int Status = 0;
	msr_t ReadMsr;

	ReadMsr = rdmsr(SMRR_PHYSBASE_MSR);
	rsc_tseg_memory.base = ReadMsr.lo & 0xFFFFF000;

	ReadMsr = rdmsr(SMRR_PHYSMASK_MSR);
	rsc_tseg_memory.length = (~(ReadMsr.lo & 0xFFFFF000) + 1);

	rsc_pm_io.base = (uint16_t)get_pmbase();

	// Local APIC. We assume that all threads are programmed identically
	// despite that it is possible to have individual APIC address for
	// each of the threads. If this is the case this programming should
	// be corrected.
	ReadMsr = rdmsr(IA32_APIC_BASE_MSR_INDEX);
	rsc_apic_mmio.base = ((uint64_t)ReadMsr.lo & 0xFFFFF000) |
				((uint64_t)(ReadMsr.hi & 0x0000000F) << 32);

	Status |= add_pi_resource((void *)&rsc_tseg_memory, 1);
	Status |= add_pi_resource((void *)&rsc_spi_memory, 1);

	Status |= add_pi_resource((void *)&rsc_pm_io, 1);
	Status |= add_pi_resource((void *)&rsc_pci_io, 1);
	Status |= add_pi_resource((void *)&rsc_pcie_mmio, 1);
	Status |= add_pi_resource((void *)&rsc_apic_mmio, 1);
	Status |= add_pi_resource((void *)&rsc_txt_mmio, 1);
	Status |= add_pi_resource((void *)&rsc_sw_smi_trap_io, 1);

	Status |= add_pi_resource((void *)&rsc_lpc_bridge_pci, 1);

#if CONFIG(SOC_INTEL_COMMON_BLOCK_P2SB)
	Status |= add_pi_resource((void *)&rsc_p2sb_mmio, 1);
#endif
#if CONFIG(SOC_INTEL_COMMON_BLOCK_P2SB2)
	Status |= add_pi_resource((void *)&rsc_p2sb2_mmio, 1);
#endif
#if CONFIG(SOC_INTEL_COMMON_BLOCK_IOE_P2SB)
	Status |= add_pi_resource((void *)&rsc_ioe_p2sb_mmio, 1);
#endif

	/* FIXME: Handle SOUTHBRIDGE_INTEL_COMMON_SPI */
	if (CONFIG(SOC_INTEL_COMMON_BLOCK_FAST_SPI))
		Status |= add_fast_spi_resource();

	if (CONFIG(SMMSTORE_V2) &&
	    !fixup_cbmem_resource(CBMEM_ID_SMM_COMBUFFER,
				  &rsc_smmstore_comm_buffer_memory.base,
				  &rsc_smmstore_comm_buffer_memory.length)) {
		Status |= add_pi_resource((void *)&rsc_smmstore_comm_buffer_memory, 1);
	}

	if (CONFIG(TPM_PPI_UEFIVAR_BACKED) &&
	    !fixup_cbmem_resource(CBMEM_ID_TPM_PPI,
				  &rsc_tpm_ppi_memory.base,
				  &rsc_tpm_ppi_memory.length)) {
		Status |= add_pi_resource((void *)&rsc_tpm_ppi_memory, 1);
	}

	if (CONFIG(DEBUG_SMI))
		Status |= add_smm_debug_resource();

	if (Status != 0)
		printk(BIOS_DEBUG, "STM - Error in adding simple resources\n");
}

/*
 * Add MSR resources to BIOS resource database.
 */
static void add_msr_resources(void)
{
	uint32_t Status = 0;
	uint32_t Index;

	for (Index = 0; Index < ARRAY_SIZE(msr_table); Index++) {
		rsc_msr_tpl.msr_index = (uint32_t)msr_table[Index].msr_index;
		rsc_msr_tpl.read_mask = (uint64_t)msr_table[Index].read_mask;
		rsc_msr_tpl.write_mask = (uint64_t)msr_table[Index].write_mask;

		Status |= add_pi_resource((void *)&rsc_msr_tpl, 1);
	}

	if (Status != 0)
		printk(BIOS_DEBUG, "STM - Error in adding MSR resources\n");
}

__weak int mainboard_stm_add_resources(void)
{
	return 0;
}

/*
 * Add resources to BIOS resource database.
 */
extern uint8_t *m_stm_resources_ptr;

void add_resources_cmd(void)
{
	m_stm_resources_ptr = NULL;

	add_simple_resources();

	add_msr_resources();

	if (mainboard_stm_add_resources() != 0)
		printk(BIOS_DEBUG, "STM - Error in adding mainboard resources\n");
}
