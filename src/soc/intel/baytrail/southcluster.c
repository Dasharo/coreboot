/* SPDX-License-Identifier: GPL-2.0-only */

#include <stdint.h>
#include <arch/io.h>
#include <arch/ioapic.h>
#include <cpu/x86/smm.h>
#include <device/mmio.h>
#include <device/pci_ops.h>
#include <acpi/acpi.h>
#include <bootstate.h>
#include <console/console.h>
#include <device/device.h>
#include <device/pci.h>
#include <device/pci_ids.h>
#include <pc80/mc146818rtc.h>
#include <pc80/i8254.h>
#include <pc80/i8259.h>
#include <pc80/isa-dma.h>
#include <drivers/uart/uart8250reg.h>

#include <soc/iomap.h>
#include <soc/irq.h>
#include <soc/lpc.h>
#include <soc/pci_devs.h>
#include <soc/pm.h>
#include <soc/ramstage.h>
#include <soc/spi.h>
#include "chip.h"
#include <acpi/acpigen.h>

static void sc_add_mmio_resources(struct device *dev)
{
	mmio_range(dev, 0xfeb, ABORT_BASE_ADDRESS, ABORT_BASE_SIZE);
	mmio_range(dev, PBASE, PMC_BASE_ADDRESS, PMC_BASE_SIZE);
	mmio_range(dev, IOBASE, IO_BASE_ADDRESS, IO_BASE_SIZE);
	mmio_range(dev, IBASE, ILB_BASE_ADDRESS, ILB_BASE_SIZE);
	mmio_range(dev, SBASE, SPI_BASE_ADDRESS, SPI_BASE_SIZE);
	mmio_range(dev, MPBASE, MPHY_BASE_ADDRESS, MPHY_BASE_SIZE);
	mmio_range(dev, PUBASE, PUNIT_BASE_ADDRESS, PUNIT_BASE_SIZE);
	mmio_range(dev, RCBA, RCBA_BASE_ADDRESS, RCBA_BASE_SIZE);
}

/* Default IO range claimed by the LPC device. The upper bound is exclusive. */
#define LPC_DEFAULT_IO_RANGE_LOWER 0
#define LPC_DEFAULT_IO_RANGE_UPPER 0x1000

static inline int io_range_in_default(int base, int size)
{
	/* Does it start above the range? */
	if (base >= LPC_DEFAULT_IO_RANGE_UPPER)
		return 0;

	/* Is it entirely contained? */
	if (base >= LPC_DEFAULT_IO_RANGE_LOWER && (base + size) < LPC_DEFAULT_IO_RANGE_UPPER)
		return 1;

	/* This will return not in range for partial overlaps */
	return 0;
}

/*
 * Note: this function assumes there is no overlap with the default LPC device's
 * claimed range: LPC_DEFAULT_IO_RANGE_LOWER -> LPC_DEFAULT_IO_RANGE_UPPER.
 */
static void sc_add_io_resource(struct device *dev, int base, int size, int index)
{
	struct resource *res;

	if (io_range_in_default(base, size))
		return;

	res = new_resource(dev, index);
	res->base = base;
	res->size = size;
	res->flags = IORESOURCE_IO | IORESOURCE_ASSIGNED | IORESOURCE_FIXED;
}

static void sc_add_io_resources(struct device *dev)
{
	struct resource *res;

	/* Add the default claimed IO range for the LPC device. */
	res = new_resource(dev, 0);
	res->base = LPC_DEFAULT_IO_RANGE_LOWER;
	res->size = LPC_DEFAULT_IO_RANGE_UPPER - LPC_DEFAULT_IO_RANGE_LOWER;
	res->flags = IORESOURCE_IO | IORESOURCE_ASSIGNED | IORESOURCE_FIXED;

	/* GPIO */
	sc_add_io_resource(dev, GPIO_BASE_ADDRESS, GPIO_BASE_SIZE, GBASE);

	/* ACPI */
	sc_add_io_resource(dev, ACPI_BASE_ADDRESS, ACPI_BASE_SIZE, ABASE);
}

static void sc_read_resources(struct device *dev)
{
	/* Get the normal PCI resources of this device. */
	pci_dev_read_resources(dev);

	/* Add non-standard MMIO resources. */
	sc_add_mmio_resources(dev);

	/* Add IO resources. */
	sc_add_io_resources(dev);
}

/*
 * The UART hardware loses power while in suspend. Because of this the kernel
 * can hang because it doesn't re-initialize serial ports it is using for
 * consoles at resume time. The following function configures the UART
 * if the hardware is enabled though it may not be the correct baud rate
 * or configuration.  This is definitely a hack, but it helps the kernel
 * along.
 */
static void com1_configure_resume(struct device *dev)
{
	const uint16_t port = 0x3f8;

	/* Is the UART I/O port enabled? */
	if (!(pci_read_config32(dev, UART_CONT) & 1))
		return;

	/* Disable interrupts */
	outb(0x0, port + UART8250_IER);

	/* Enable FIFOs */
	outb(UART8250_FCR_FIFO_EN, port + UART8250_FCR);

	/* assert DTR and RTS so the other end is happy */
	outb(UART8250_MCR_DTR | UART8250_MCR_RTS, port + UART8250_MCR);

	/* DLAB on */
	outb(UART8250_LCR_DLAB | 3, port + UART8250_LCR);

	/* Set Baud Rate Divisor. 1 ==> 115200 Baud */
	outb(1, port + UART8250_DLL);
	outb(0, port + UART8250_DLM);

	/* Set to 3 for 8N1 */
	outb(3, port + UART8250_LCR);
}


static void sc_enable_ioapic(struct device *dev)
{
	int i;
	u32 reg32;
	volatile u32 *ioapic_index = (u32 *)(IO_APIC_ADDR);
	volatile u32 *ioapic_data = (u32 *)(IO_APIC_ADDR + 0x10);
	u8 *ilb_base = (u8 *)(pci_read_config32(dev, IBASE) & ~0x0f);

	/*
	 * Enable ACPI I/O and power management.
	 * Set SCI IRQ to IRQ9
	 */
	write32(ilb_base + ILB_OIC, 0x100);  /* AEN */
	reg32 = read32(ilb_base + ILB_OIC); /* Read back per BWG */
	write32(ilb_base + ACTL, 0);  /* ACTL bit 2:0 SCIS IRQ9 */

	*ioapic_index = 0;
	*ioapic_data = (1 << 25);

	/* affirm full set of redirection table entries ("write once") */
	*ioapic_index = 1;
	reg32 = *ioapic_data;
	*ioapic_index = 1;
	*ioapic_data = reg32;

	*ioapic_index = 0;
	reg32 = *ioapic_data;
	printk(BIOS_DEBUG, "Southbridge APIC ID = %x\n", (reg32 >> 24) & 0x0f);
	if (reg32 != (1 << 25))
		die("APIC Error\n");

	printk(BIOS_SPEW, "Dumping IOAPIC registers\n");
	for (i=0; i<3; i++) {
		*ioapic_index = i;
		printk(BIOS_SPEW, "  reg 0x%04x:", i);
		reg32 = *ioapic_data;
		printk(BIOS_SPEW, " 0x%08x\n", reg32);
	}

	*ioapic_index = 3; /* Select Boot Configuration register. */
	*ioapic_data = 1; /* Use Processor System Bus to deliver interrupts. */
}

static void sc_enable_serial_irqs(struct device *dev)
{
#ifdef SETUPSERIQ /* NOT defined. Remove when the TODO is done. */
	/*
	 * TODO: SERIRQ seems to have a number of problems on baytrail.
	 * With it enabled, we get some spurious interrupts (ps2)
	 * in seabios. It also caused IOCHK# NMIs. Remove it
	 * until we understand how it needs to be configured.
	 */
	u8 reg8;
	u8 *ibase = (u8 *)(pci_read_config32(dev, IBASE) & ~0xF);

	/*
	 * Disable the IOCHK# NMI. Let the NMI handler enable it if it needs.
	 */
	reg8 = inb(0x61);
	reg8 &= 0x0f; /* Higher Nibble must be 0 */
	reg8 |= (1 << 3); /* IOCHK# NMI  Disable for now */
	outb(reg8, 0x61);

	write32(ibase + ILB_OIC, read32(ibase + ILB_OIC) | SIRQEN);
	write8(ibase + ILB_SERIRQ_CNTL, SCNT_CONTINUOUS_MODE);

	if (!CONFIG(SERIRQ_CONTINUOUS_MODE)) {
		/*
		 * SoC requires that the System BIOS first set the SERIRQ logic to
		 * continuous mode operation for at least one frame before switching
		 *  it into quiet mode operation.
		 */
		outb(0x00, 0xED); /* I/O Delay to get the 1 frame */
		write8(ibase + ILB_SERIRQ_CNTL, SCNT_QUIET_MODE);
	}
#endif  /* DON'T SET UP IRQS */
}

/*
 * Write PCI config space IRQ assignments.  PCI devices have the INT_LINE
 * (0x3C) and INT_PIN (0x3D) registers which report interrupt routing
 * information to operating systems and drivers.  The INT_PIN register is
 * generally read only and reports which interrupt pin A - D it uses.  The
 * INT_LINE register is configurable and reports which IRQ (generally the
 * PIC IRQs 1 - 15) it will use.  This needs to take interrupt pin swizzling
 * on devices that are downstream on a PCI bridge into account.
 *
 * This function will loop through all enabled PCI devices and program the
 * INT_LINE register with the correct PIC IRQ number for the INT_PIN that it
 * uses.  It then configures each interrupt in the pic to be level triggered.
 */
static void write_pci_config_irqs(void)
{
	struct device *irq_dev;
	struct device *targ_dev;
	uint8_t int_line = 0;
	uint8_t original_int_pin = 0;
	uint8_t new_int_pin = 0;
	uint16_t current_bdf = 0;
	uint16_t parent_bdf = 0;
	uint8_t pirq = 0;
	uint8_t device_num = 0;
	const struct baytrail_irq_route *ir = &global_baytrail_irq_route;

	if (ir == NULL) {
		printk(BIOS_WARNING, "Warning: Can't write PCI IRQ assignments because"
				" 'global_baytrail_irq_route' structure does not exist\n");
		return;
	}

	/*
	 * Loop through all enabled devices and program their
	 * INT_LINE, INT_PIN registers from values taken from
	 * the Interrupt Route registers in the ILB
	 */
	printk(BIOS_DEBUG, "PCI_CFG IRQ: Write PCI config space IRQ assignments\n");
	for (irq_dev = all_devices; irq_dev; irq_dev = irq_dev->next) {

		if ((irq_dev->path.type != DEVICE_PATH_PCI) ||
			(!irq_dev->enabled))
			continue;

		current_bdf = irq_dev->path.pci.devfn |
			irq_dev->upstream->secondary << 8;

		/*
		 * Step 1: Get the INT_PIN and device structure to look for
		 * in the pirq_data table defined in the mainboard directory.
		 */
		targ_dev = NULL;
		new_int_pin = get_pci_irq_pins(irq_dev, &targ_dev);
		if (targ_dev == NULL || new_int_pin < 1)
			continue;

		/*
		 * Adjust the INT routing for the PCIe root ports
		 * See 'Interrupt Generated for INT[A-D] Interrupts'
		 * Table 241 in Document Number: 538136, Rev. 3.9
		 */
		if (PCI_SLOT(targ_dev->path.pci.devfn) == PCIE_DEV &&
				targ_dev != irq_dev)
			new_int_pin = ((new_int_pin - 1 +
				PCI_FUNC(targ_dev->path.pci.devfn)) % 4) + 1;

		/* Get the original INT_PIN for record keeping */
		original_int_pin = pci_read_config8(irq_dev, PCI_INTERRUPT_PIN);

		parent_bdf = targ_dev->path.pci.devfn
			| targ_dev->upstream->secondary << 8;
		device_num = PCI_SLOT(parent_bdf);

		/* Find the PIRQ that is attached to the INT_PIN this device uses */
		pirq = (ir->pcidev[device_num] >> ((new_int_pin - 1) * 4)) & 0xF;

		/* Get the INT_LINE this device/function will use */
		int_line = ir->pic[pirq];

		if (int_line != PIRQ_PIC_IRQDISABLE) {
			/* Set this IRQ to level triggered since it is used by a PCI device */
			i8259_configure_irq_trigger(int_line, IRQ_LEVEL_TRIGGERED);
			/* Set the Interrupt Line register in PCI config space */
			pci_write_config8(irq_dev, PCI_INTERRUPT_LINE, int_line);
		} else {
			/* Set the Interrupt line register as "unknown or unused" */
			pci_write_config8(irq_dev, PCI_INTERRUPT_LINE,
				PIRQ_PIC_UNKNOWN_UNUSED);
		}

		printk(BIOS_SPEW, "\tINT_PIN\t\t: %d (%s)\n",
			original_int_pin, pin_to_str(original_int_pin));
		if (parent_bdf != current_bdf)
			printk(BIOS_SPEW, "\tSwizzled to\t: %d (%s)\n",
							new_int_pin, pin_to_str(new_int_pin));
		printk(BIOS_SPEW, "\tPIRQ\t\t: %c\n"
						"\tINT_LINE\t: 0x%X (IRQ %d)\n",
						'A' + pirq, int_line, int_line);
	}
	printk(BIOS_DEBUG, "PCI_CFG IRQ: Finished writing PCI config space IRQ assignments\n");
}

static void sc_pirq_init(struct device *dev)
{
	int i, j;
	int pirq;
	u8 *pr_base = (u8 *)(ILB_BASE_ADDRESS + 0x08);
	u16 *ir_base = (u16 *)(ILB_BASE_ADDRESS + 0x20);
	u32 *actl = (u32 *)(ILB_BASE_ADDRESS + ACTL);
	const struct baytrail_irq_route *ir = &global_baytrail_irq_route;

	/* Set up the PIRQ PIC routing based on static config. */
	printk(BIOS_SPEW, "Start writing IRQ assignments\n"
			"PIRQ\tA\tB\tC\tD\tE\tF\tG\tH\n"
			"IRQ ");
	for (i = 0; i < NUM_PIRQS; i++) {
		write8(pr_base + i, ir->pic[i]);
		printk(BIOS_SPEW, "\t%d", ir->pic[i]);
	}
	printk(BIOS_SPEW, "\n\n");

	/* Set up the per device PIRQ routing based on static config. */
	printk(BIOS_SPEW, "\t\t\tPIRQ[A-H] routed to each INT_PIN[A-D]\n"
			"Dev\tINTA (IRQ)\tINTB (IRQ)\tINTC (IRQ)\tINTD (IRQ)\n");
	for (i = 0; i < 32; i++) {
		write16(ir_base + i, ir->pcidev[i]);

		/* If the entry is more than just 0, print it out */
		if (ir->pcidev[i]) {
			printk(BIOS_SPEW, " %d: ", i);
			for (j = 0; j < 4; j++) {
				pirq = (ir->pcidev[i] >> (j * 4)) & 0xF;
				printk(BIOS_SPEW, "\t%-4c (%d)", 'A' + pirq, ir->pic[pirq]);
			}
			printk(BIOS_SPEW, "\n");
		}
	}

	/* Route SCI to IRQ9 */
	write32(actl, (read32(actl) & ~SCIS_MASK) | SCIS_IRQ9);
	printk(BIOS_SPEW, "Finished writing IRQ assignments\n");

	/* Write IRQ assignments to PCI config space */
	write_pci_config_irqs();
}

static void sc_init(struct device *dev)
{
	struct soc_intel_baytrail_config *config = config_of(dev);
	u32 *gen_pmcon1 = (u32 *)(PMC_BASE_ADDRESS + GEN_PMCON1);
	u8 *ibase;

	printk(BIOS_DEBUG, "soc: southcluster_init\n");

	ibase = (u8 *)(pci_read_config32(dev, IBASE) & ~0xF);

	write8(ibase + ILB_MC, 0);

	cmos_init(rtc_failure());

	/* Set the value for PCI command register. */
	pci_write_config16(dev, PCI_COMMAND,
		PCI_COMMAND_IO | PCI_COMMAND_MEMORY |
		PCI_COMMAND_MASTER | PCI_COMMAND_SPECIAL);

	/* IO APIC initialization. */
	sc_enable_ioapic(dev);

	sc_enable_serial_irqs(dev);

	/* Setup the PIRQ. */
	sc_pirq_init(dev);

	/* Initialize ISA DMA. */
	isa_dma_init();

	setup_i8259();

	setup_i8254();

	if (config->disable_slp_x_stretch_sus_fail) {
		printk(BIOS_DEBUG, "Disabling slp_x stretching.\n");
		write32(gen_pmcon1, read32(gen_pmcon1) | DIS_SLP_X_STRCH_SUS_UP);

	} else {
		write32(gen_pmcon1, read32(gen_pmcon1) & ~DIS_SLP_X_STRCH_SUS_UP);
	}

	if (acpi_is_wakeup_s3())
		com1_configure_resume(dev);
}

/*
 * Common code for the south cluster devices.
 */

#define SET_DIS_MASK(name_) \
	case PCI_DEVFN(name_ ## _DEV, name_ ## _FUNC): \
		mask |= name_ ## _DIS
#define SET_DIS_MASK2(name_) \
	case PCI_DEVFN(name_ ## _DEV, name_ ## _FUNC): \
		mask2 |= name_ ## _DIS


/* Set bit in function disable register to hide this device. */
static void sc_disable_devfn(struct device *dev)
{
	void *func_dis  = (void *)(PMC_BASE_ADDRESS + FUNC_DIS);
	void *func_dis2 = (void *)(PMC_BASE_ADDRESS + FUNC_DIS2);
	uint32_t mask  = 0;
	uint32_t mask2 = 0;

	switch (dev->path.pci.devfn) {
	SET_DIS_MASK(MIPI);
		break;
	SET_DIS_MASK(SDIO);
		break;
	SET_DIS_MASK(SD);
		break;
	SET_DIS_MASK(SATA);
		break;
	SET_DIS_MASK(XHCI);
		/* Disable super speed PHY when XHCI is not available. */
		mask2 |= USH_SS_PHY_DIS;
		break;
	SET_DIS_MASK(LPE);
		break;
	SET_DIS_MASK(MMC);
		break;
	SET_DIS_MASK(MMC45);
		break;
	SET_DIS_MASK(SIO_DMA1);
		break;
	SET_DIS_MASK(I2C1);
		break;
	SET_DIS_MASK(I2C2);
		break;
	SET_DIS_MASK(I2C3);
		break;
	SET_DIS_MASK(I2C4);
		break;
	SET_DIS_MASK(I2C5);
		break;
	SET_DIS_MASK(I2C6);
		break;
	SET_DIS_MASK(I2C7);
		break;
	SET_DIS_MASK(TXE);
		break;
	SET_DIS_MASK(HDA);
		break;
	SET_DIS_MASK(PCIE_PORT1);
		break;
	SET_DIS_MASK(PCIE_PORT2);
		break;
	SET_DIS_MASK(PCIE_PORT3);
		break;
	SET_DIS_MASK(PCIE_PORT4);
		break;
	SET_DIS_MASK2(SMBUS);
		break;
	SET_DIS_MASK(EHCI);
		break;
	SET_DIS_MASK(SIO_DMA2);
		break;
	SET_DIS_MASK(PWM1);
		break;
	SET_DIS_MASK(PWM2);
		break;
	SET_DIS_MASK(HSUART1);
		break;
	SET_DIS_MASK(HSUART2);
		break;
	SET_DIS_MASK(SPI);
		break;

	SET_DIS_MASK(OTG);
		/* Disable OTG PHY when OTG is not available. */
		mask2 |= OTG_SS_PHY_DIS;
		break;
	}

	if (mask != 0) {
		write32(func_dis, read32(func_dis) | mask);
		/* Ensure posted write hits */
		read32(func_dis);
	}

	if (mask2 != 0) {
		write32(func_dis2, read32(func_dis2) | mask2);
		/* Ensure posted write hits */
		read32(func_dis2);
	}
}

static inline void set_d3hot_bits(struct device *dev, int offset)
{
	uint32_t reg8;
	printk(BIOS_DEBUG, "Power management CAP offset 0x%x.\n", offset);
	reg8 = pci_read_config8(dev, offset + 4);
	reg8 |= 0x3;
	pci_write_config8(dev, offset + 4, reg8);
}

/*
 * Parts of the audio subsystem are powered by the HDA device. Thus, one cannot put HDA into
 * D3Hot. Instead, perform this workaround to make some of the audio paths work for LPE audio.
 */
static void hda_work_around(struct device *dev)
{
	void *gctl = (void *)(TEMP_BASE_ADDRESS + 0x8);

	/* Need to set magic register 0x43 to 0xd7 in config space. */
	pci_write_config8(dev, 0x43, 0xd7);

	/*
	 * Need to set bit 0 of GCTL to take the device out of reset.
	 * However, that requires setting up the 64-bit BAR.
	 */
	pci_write_config32(dev, PCI_BASE_ADDRESS_0, TEMP_BASE_ADDRESS);
	pci_write_config32(dev, PCI_BASE_ADDRESS_1, 0);
	pci_write_config16(dev, PCI_COMMAND, PCI_COMMAND_MEMORY);
	write32(gctl, read32(gctl) | 0x1);
	pci_write_config16(dev, PCI_COMMAND, 0);
	pci_write_config32(dev, PCI_BASE_ADDRESS_0, 0);
}

static int place_device_in_d3hot(struct device *dev)
{
	unsigned int offset;

	/*
	 * Parts of the HDA block are used for LPE audio as well.
	 * Therefore assume the HDA will never be put into D3Hot.
	 */
	if (dev->path.pci.devfn == PCI_DEVFN(HDA_DEV, HDA_FUNC)) {
		hda_work_around(dev);
		return 0;
	}

	offset = pci_find_capability(dev, PCI_CAP_ID_PM);

	if (offset != 0) {
		set_d3hot_bits(dev, offset);
		return 0;
	}

	/*
	 * For some reason some of the devices don't have the capability pointer set correctly.
	 * Work around this by hard coding the offset.
	 */
	switch (dev->path.pci.devfn) {
	case PCI_DEVFN(MMC_DEV, MMC_FUNC):
		offset = 0x80;
		break;
	case PCI_DEVFN(SDIO_DEV, SDIO_FUNC):
		offset = 0x80;
		break;
	case PCI_DEVFN(SD_DEV, SD_FUNC):
		offset = 0x80;
		break;
	case PCI_DEVFN(MMC45_DEV, MMC45_FUNC):
		offset = 0x80;
		break;
	case PCI_DEVFN(LPE_DEV, LPE_FUNC):
		offset = 0x80;
		break;
	case PCI_DEVFN(SIO_DMA1_DEV, SIO_DMA1_FUNC):
		offset = 0x80;
		break;
	case PCI_DEVFN(I2C1_DEV, I2C1_FUNC):
		offset = 0x80;
		break;
	case PCI_DEVFN(I2C2_DEV, I2C2_FUNC):
		offset = 0x80;
		break;
	case PCI_DEVFN(I2C3_DEV, I2C3_FUNC):
		offset = 0x80;
		break;
	case PCI_DEVFN(I2C4_DEV, I2C4_FUNC):
		offset = 0x80;
		break;
	case PCI_DEVFN(I2C5_DEV, I2C5_FUNC):
		offset = 0x80;
		break;
	case PCI_DEVFN(I2C6_DEV, I2C6_FUNC):
		offset = 0x80;
		break;
	case PCI_DEVFN(I2C7_DEV, I2C7_FUNC):
		offset = 0x80;
		break;
	case PCI_DEVFN(SIO_DMA2_DEV, SIO_DMA2_FUNC):
		offset = 0x80;
		break;
	case PCI_DEVFN(PWM1_DEV, PWM1_FUNC):
		offset = 0x80;
		break;
	case PCI_DEVFN(PWM2_DEV, PWM2_FUNC):
		offset = 0x80;
		break;
	case PCI_DEVFN(HSUART1_DEV, HSUART1_FUNC):
		offset = 0x80;
		break;
	case PCI_DEVFN(HSUART2_DEV, HSUART2_FUNC):
		offset = 0x80;
		break;
	case PCI_DEVFN(SPI_DEV, SPI_FUNC):
		offset = 0x80;
		break;
	case PCI_DEVFN(SATA_DEV, SATA_FUNC):
		offset = 0x70;
		break;
	case PCI_DEVFN(XHCI_DEV, XHCI_FUNC):
		offset = 0x70;
		break;
	case PCI_DEVFN(EHCI_DEV, EHCI_FUNC):
		offset = 0x70;
		break;
	case PCI_DEVFN(HDA_DEV, HDA_FUNC):
		offset = 0x50;
		break;
	case PCI_DEVFN(SMBUS_DEV, SMBUS_FUNC):
		offset = 0x50;
		break;
	case PCI_DEVFN(TXE_DEV, TXE_FUNC):
		/* TXE cannot be placed in D3Hot. */
		return 0;
	case PCI_DEVFN(PCIE_PORT1_DEV, PCIE_PORT1_FUNC):
		offset = 0xa0;
		break;
	case PCI_DEVFN(PCIE_PORT2_DEV, PCIE_PORT2_FUNC):
		offset = 0xa0;
		break;
	case PCI_DEVFN(PCIE_PORT3_DEV, PCIE_PORT3_FUNC):
		offset = 0xa0;
		break;
	case PCI_DEVFN(PCIE_PORT4_DEV, PCIE_PORT4_FUNC):
		offset = 0xa0;
		break;
	}

	if (offset != 0) {
		set_d3hot_bits(dev, offset);
		return 0;
	}

	return -1;
}

/* Common PCI device function disable. */
void southcluster_enable_dev(struct device *dev)
{
	uint16_t reg16;

	if (!dev->enabled) {
		int slot = PCI_SLOT(dev->path.pci.devfn);
		int func = PCI_FUNC(dev->path.pci.devfn);
		printk(BIOS_DEBUG, "%s: Disabling device: %02x.%01x\n",
		       dev_path(dev), slot, func);

		/* Ensure memory, io, and bus master are all disabled */
		reg16 = pci_read_config16(dev, PCI_COMMAND);
		reg16 &= ~(PCI_COMMAND_MASTER | PCI_COMMAND_MEMORY | PCI_COMMAND_IO);
		pci_write_config16(dev, PCI_COMMAND, reg16);

		/* Place device in D3Hot */
		if (place_device_in_d3hot(dev) < 0) {
			printk(BIOS_WARNING,
			       "Could not place %02x.%01x into D3Hot. "
			       "Keeping device visible.\n", slot, func);
			return;
		}
		/* Disable this device if possible */
		sc_disable_devfn(dev);
	} else {
		/* Enable SERR */
		pci_or_config16(dev, PCI_COMMAND, PCI_COMMAND_SERR);
	}
}

static struct device_operations device_ops = {
	.read_resources		= sc_read_resources,
	.set_resources		= pci_dev_set_resources,
	.write_acpi_tables	= acpi_write_hpet,
	.init			= sc_init,
	.enable			= southcluster_enable_dev,
	.scan_bus		= scan_static_bus,
	.ops_pci		= &soc_pci_ops,
};

static const struct pci_driver southcluster __pci_driver = {
	.ops		= &device_ops,
	.vendor		= PCI_VID_INTEL,
	.device		= LPC_DEVID,
};

int __weak mainboard_get_spi_config(struct spi_config *cfg)
{
	return -1;
}

static void finalize_chipset(void *unused)
{
	void *bcr = (void *)(SPI_BASE_ADDRESS + BCR);
	void *gcs = (void *)(RCBA_BASE_ADDRESS + GCS);
	void *gen_pmcon2 = (void *)(PMC_BASE_ADDRESS + GEN_PMCON2);
	void *etr = (void *)(PMC_BASE_ADDRESS + ETR);
	uint8_t *spi = (uint8_t *)SPI_BASE_ADDRESS;
	struct spi_config cfg;

	/* Set the lock enable on the BIOS control register */
	write32(bcr, read32(bcr) | BCR_LE);

	/* Set BIOS lock down bit controlling boot block size and swapping */
	write32(gcs, read32(gcs) | BILD);

	/* Lock sleep stretching policy and set SMI lock */
	write32(gen_pmcon2, read32(gen_pmcon2) | SLPSX_STR_POL_LOCK | SMI_LOCK);

	/*  Set the CF9 lock */
	write32(etr, read32(etr) | CF9LOCK);

	if (mainboard_get_spi_config(&cfg) < 0) {
		printk(BIOS_DEBUG, "No SPI lockdown configuration.\n");
	} else {
		write16(spi + PREOP, cfg.preop);
		write16(spi + OPTYPE, cfg.optype);
		write32(spi + OPMENU0, cfg.opmenu[0]);
		write32(spi + OPMENU1, cfg.opmenu[1]);
		write16(spi + HSFSTS, read16(spi + HSFSTS) | FLOCKDN);
		write32(spi + UVSCC, cfg.uvscc);
		write32(spi + LVSCC, cfg.lvscc | VCL);
	}

	apm_control(APM_CNT_FINALIZE);
}

BOOT_STATE_INIT_ENTRY(BS_OS_RESUME, BS_ON_ENTRY, finalize_chipset, NULL);
BOOT_STATE_INIT_ENTRY(BS_PAYLOAD_LOAD, BS_ON_EXIT, finalize_chipset, NULL);
