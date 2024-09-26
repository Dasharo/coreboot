/* SPDX-License-Identifier: GPL-2.0-only */

#include <stdint.h>
#include <acpi/acpi.h>
#include <acpi/acpi_gnvs.h>
#include <console/console.h>
#include <device/device.h>
#include <device/pci.h>
#include <device/pci_ids.h>
#include <reg_script.h>

#include <soc/iomap.h>
#include <soc/iosf.h>
#include <soc/irq.h>
#include <soc/nvs.h>
#include <soc/device_nvs.h>
#include <soc/pci_devs.h>
#include <soc/ramstage.h>

#include "chip.h"

#define LPSS_SPI_CLOCK_PARAMS			0x400
#define LPSS_UART_CLOCK_PARAMS			0x800
# define  LPSS_CLOCK_PARAMS_CLOCK_EN		(1 << 0)
# define  LPSS_CLOCK_PARAMS_CLOCK_UPDATE	(1 << 31)
#define LPSS_SPI_SOFTWARE_RESET			0x404
#define LPSS_SOFTWARE_RESET			0x804
# define   LPSS_SOFTWARE_RESET_FUNC		(1 << 0)
# define   LPSS_SOFTWARE_RESET_APB		(1 << 1)

/* Setting for UART 44.2368Mhz M/N = 0.442368 (0x1b00/0x3d09) x 100 Mhz */
#define UART_CLOCK_44P2368_MHZ			 ((0x3d09 << 16) | (0x1b00 << 1))
/* Setting for SPI 50Mhz M/N = 0.5 (0x1/0x2) x 100 Mhz */
#define SPI_CLOCK_50_MHZ			 ((0x2 << 16) | (0x1 << 1))

/* CSRT constants */
#define DMA1_GROUP_REVISION		2
#define DMA2_GROUP_REVISION		3
#define DMA1_NUM_CHANNELS		6
#define DMA2_NUM_CHANNELS		8
#define DMA1_BASE_REQ_LINE		0
#define DMA2_BASE_REQ_LINE		16
#define DMA_MAX_BLOCK_SIZE		0xfff
#define DMA_ADDRESS_WIDTH		32
#define DMA_NUM_HANDSHAKE_SIGNALS	16

#define CASE_DEV(name_) \
	case PCI_DEVFN(name_ ## _DEV, name_ ## _FUNC)

static void store_acpi_nvs(struct device *dev, int nvs_index)
{
	struct resource *bar;
	struct device_nvs *dev_nvs = acpi_get_device_nvs();

	/* Save BAR0 and BAR1 to ACPI NVS */
	bar = probe_resource(dev, PCI_BASE_ADDRESS_0);
	if (bar)
		dev_nvs->lpss_bar0[nvs_index] = (u32)bar->base;

	bar = probe_resource(dev, PCI_BASE_ADDRESS_1);
	if (bar)
		dev_nvs->lpss_bar1[nvs_index] = (u32)bar->base;
}

static void dev_enable_acpi_mode(struct device *dev, int iosf_reg, int nvs_index)
{
	struct reg_script ops[] = {
		/* Disable PCI interrupt, enable Memory and Bus Master */
		REG_PCI_OR16(PCI_COMMAND,
			     PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER | PCI_COMMAND_INT_DISABLE),
		/* Enable ACPI mode */
		REG_IOSF_OR(IOSF_PORT_LPSS, iosf_reg,
			    LPSS_CTL_PCI_CFG_DIS | LPSS_CTL_ACPI_INT_EN),

		REG_SCRIPT_END
	};
	struct resource *bar;
	struct device_nvs *dev_nvs = acpi_get_device_nvs();

	/* Save BAR0 and BAR1 to ACPI NVS */
	bar = probe_resource(dev, PCI_BASE_ADDRESS_0);
	if (bar)
		dev_nvs->lpss_bar0[nvs_index] = (u32)bar->base;

	bar = probe_resource(dev, PCI_BASE_ADDRESS_1);
	if (bar)
		dev_nvs->lpss_bar1[nvs_index] = (u32)bar->base;

	/* Device is enabled in ACPI mode */
	dev_nvs->lpss_en[nvs_index] = 1;

	/* Put device in ACPI mode */
	reg_script_run_on_dev(dev, ops);
}

static void dev_set_frequency(struct device *dev, u32 reg, u32 freq)
{
	struct reg_script ops[] = {
		REG_RES_WRITE32(PCI_BASE_ADDRESS_0, reg, freq),
		REG_RES_OR32(PCI_BASE_ADDRESS_0, reg, LPSS_CLOCK_PARAMS_CLOCK_EN),
		REG_RES_OR32(PCI_BASE_ADDRESS_0, reg, LPSS_CLOCK_PARAMS_CLOCK_UPDATE),
		REG_SCRIPT_END
	};

	reg_script_run_on_dev(dev, ops);
}

static void dev_configure_default_frequency(struct device *dev)
{
	switch (dev->path.pci.devfn) {
	CASE_DEV(HSUART1):
	CASE_DEV(HSUART2):
		dev_set_frequency(dev, LPSS_UART_CLOCK_PARAMS, UART_CLOCK_44P2368_MHZ);
		break;
	CASE_DEV(SPI):
		dev_set_frequency(dev, LPSS_SPI_CLOCK_PARAMS, SPI_CLOCK_50_MHZ);
		break;
	}
}

static void dev_enable_snoop_and_pm(struct device *dev, int iosf_reg)
{
	struct reg_script ops[] = {
		REG_IOSF_RMW(IOSF_PORT_LPSS, iosf_reg,
		             ~(LPSS_CTL_SNOOP | LPSS_CTL_NOSNOOP),
		             LPSS_CTL_SNOOP | LPSS_CTL_PM_CAP_PRSNT),
		REG_SCRIPT_END,
	};

	reg_script_run_on_dev(dev, ops);
}

#define SET_IOSF_REG(name_) \
	case PCI_DEVFN(name_ ## _DEV, name_ ## _FUNC): \
		do { \
			*iosf_reg = LPSS_ ## name_ ## _CTL; \
			*nvs_index = LPSS_NVS_ ## name_; \
		} while (0)

static void dev_ctl_reg(struct device *dev, int *iosf_reg, int *nvs_index)
{
	*iosf_reg = -1;
	*nvs_index = -1;

	switch (dev->path.pci.devfn) {
	SET_IOSF_REG(SIO_DMA1);
		break;
	SET_IOSF_REG(I2C1);
		break;
	SET_IOSF_REG(I2C2);
		break;
	SET_IOSF_REG(I2C3);
		break;
	SET_IOSF_REG(I2C4);
		break;
	SET_IOSF_REG(I2C5);
		break;
	SET_IOSF_REG(I2C6);
		break;
	SET_IOSF_REG(I2C7);
		break;
	SET_IOSF_REG(SIO_DMA2);
		break;
	SET_IOSF_REG(PWM1);
		break;
	SET_IOSF_REG(PWM2);
		break;
	SET_IOSF_REG(HSUART1);
		break;
	SET_IOSF_REG(HSUART2);
		break;
	SET_IOSF_REG(SPI);
		break;
	}
}

#define PIN_SET_FOR_I2C		0x2003C881
#define PIN_SET_FOR_PWM		0x2003CD01
#define PIN_SET_FOR_UART	0x2003CC81
#define PIN_SET_FOR_SPI		0x2003CC81
#define PIN_SET_FOR_SPI_CLK	0x2003CD01

static void dev_set_pins(struct device *dev)
{
	switch (dev->path.pci.devfn) {
	CASE_DEV(I2C1):
		write32((volatile void *)(IO_BASE_ADDRESS + 0x0210),
			PIN_SET_FOR_I2C);
		write32((volatile void *)(IO_BASE_ADDRESS + 0x0200),
			PIN_SET_FOR_I2C);
		break;
	CASE_DEV(I2C2):
		write32((volatile void *)(IO_BASE_ADDRESS + 0x01F0),
			PIN_SET_FOR_I2C);
		write32((volatile void *)(IO_BASE_ADDRESS + 0x01E0),
			PIN_SET_FOR_I2C);
		break;
	CASE_DEV(I2C3):
		write32((volatile void *)(IO_BASE_ADDRESS + 0x01D0),
			PIN_SET_FOR_I2C);
		write32((volatile void *)(IO_BASE_ADDRESS + 0x01B0),
			PIN_SET_FOR_I2C);
		break;
	CASE_DEV(I2C4):
		write32((volatile void *)(IO_BASE_ADDRESS + 0x0190),
			PIN_SET_FOR_I2C);
		write32((volatile void *)(IO_BASE_ADDRESS + 0x01C0),
			PIN_SET_FOR_I2C);
		break;
	CASE_DEV(I2C5):
		write32((volatile void *)(IO_BASE_ADDRESS + 0x01A0),
			PIN_SET_FOR_I2C);
		write32((volatile void *)(IO_BASE_ADDRESS + 0x0170),
			PIN_SET_FOR_I2C);
		break;
	CASE_DEV(I2C6):
		write32((volatile void *)(IO_BASE_ADDRESS + 0x0150),
			PIN_SET_FOR_I2C);
		write32((volatile void *)(IO_BASE_ADDRESS + 0x0140),
			PIN_SET_FOR_I2C);
		break;
	CASE_DEV(I2C7):
		write32((volatile void *)(IO_BASE_ADDRESS + 0x0180),
			PIN_SET_FOR_I2C);
		write32((volatile void *)(IO_BASE_ADDRESS + 0x0160),
			PIN_SET_FOR_I2C);
		break;
	CASE_DEV(PWM1):
		write32((volatile void *)(IO_BASE_ADDRESS + 0x00A0),
			PIN_SET_FOR_PWM);
		break;
	CASE_DEV(PWM2):
		write32((volatile void *)(IO_BASE_ADDRESS + 0x00B0),
			PIN_SET_FOR_PWM);
		break;
	CASE_DEV(HSUART1):
		write32((volatile void *)(IO_BASE_ADDRESS + 0x0020),
			PIN_SET_FOR_UART);
		write32((volatile void *)(IO_BASE_ADDRESS + 0x0010),
			PIN_SET_FOR_UART);
		write32((volatile void *)(IO_BASE_ADDRESS + 0x0000),
			PIN_SET_FOR_UART);
		write32((volatile void *)(IO_BASE_ADDRESS + 0x0040),
			PIN_SET_FOR_UART);
		break;
	CASE_DEV(HSUART2):
		write32((volatile void *)(IO_BASE_ADDRESS + 0x0090),
			PIN_SET_FOR_UART);
		write32((volatile void *)(IO_BASE_ADDRESS + 0x0080),
			PIN_SET_FOR_UART);
		write32((volatile void *)(IO_BASE_ADDRESS + 0x0060),
			PIN_SET_FOR_UART);
		write32((volatile void *)(IO_BASE_ADDRESS + 0x0070),
			PIN_SET_FOR_UART);
		break;
	CASE_DEV(SPI):
		write32((volatile void *)(IO_BASE_ADDRESS + 0x0110),
			PIN_SET_FOR_SPI);
		write32((volatile void *)(IO_BASE_ADDRESS + 0x0100),
			PIN_SET_FOR_SPI_CLK);
		write32((volatile void *)(IO_BASE_ADDRESS + 0x0130),
			PIN_SET_FOR_SPI);
		write32((volatile void *)(IO_BASE_ADDRESS + 0x0120),
			PIN_SET_FOR_SPI);
		break;
	}
}

static void dev_set_int_pin(struct device *dev, int iosf_reg)
{
	u32 val;
	u8 pin = INTA;

	switch (dev->path.pci.devfn) {
	CASE_DEV(SIO_DMA1):
		pin = INTA;
		break;
	CASE_DEV(I2C1):
		pin = INTC;
		break;
	CASE_DEV(I2C2):
		pin = INTD;
		break;
	CASE_DEV(I2C3):
		pin = INTB;
		break;
	CASE_DEV(I2C4):
		pin = INTA;
		break;
	CASE_DEV(I2C5):
		pin = INTC;
		break;
	CASE_DEV(I2C6):
		pin = INTD;
		break;
	CASE_DEV(I2C7):
		pin = INTB;
		break;
	CASE_DEV(SIO_DMA2):
		pin = INTA;
		break;
	CASE_DEV(PWM1):
		pin = INTD;
		break;
	CASE_DEV(PWM2):
		pin = INTB;
		break;
	CASE_DEV(HSUART1):
		pin = INTC;
		break;
	CASE_DEV(HSUART2):
		pin = INTD;
		break;
	CASE_DEV(SPI):
		pin = INTB;
		break;
	default:
		return;
	}

	val = iosf_lpss_read(iosf_reg);
	val &= ~LPSS_CTL_INT_PIN_MASK;
	val |= (pin << LPSS_CTL_INT_PIN_SHIFT);
	iosf_lpss_write(iosf_reg, val);
}

static void dev_disable_resets(struct device *dev)
{
	/* Release the LPSS devices from reset. */
	static const struct reg_script ops[] = {
		REG_RES_WRITE32(PCI_BASE_ADDRESS_0, LPSS_SOFTWARE_RESET,
				LPSS_SOFTWARE_RESET_FUNC | LPSS_SOFTWARE_RESET_APB),
		REG_SCRIPT_END,
	};

	static const struct reg_script spi_ops[] = {
		REG_RES_WRITE32(PCI_BASE_ADDRESS_0, LPSS_SPI_SOFTWARE_RESET,
				LPSS_SOFTWARE_RESET_FUNC | LPSS_SOFTWARE_RESET_APB),
		REG_SCRIPT_END,
	};

	switch (dev->path.pci.devfn) {
	CASE_DEV(I2C1):
	CASE_DEV(I2C2):
	CASE_DEV(I2C3):
	CASE_DEV(I2C4):
	CASE_DEV(I2C5):
	CASE_DEV(I2C6):
	CASE_DEV(I2C7):
		printk(BIOS_DEBUG, "Releasing I2C device from reset.\n");
		break;
	CASE_DEV(PWM1):
	CASE_DEV(PWM2):
		printk(BIOS_DEBUG, "Releasing PWM device from reset.\n");
		break;
	CASE_DEV(HSUART1):
	CASE_DEV(HSUART2):
		printk(BIOS_DEBUG, "Releasing HSUART device from reset.\n");
		break;
	CASE_DEV(SPI):
		printk(BIOS_DEBUG, "Releasing SPI device from reset.\n");
		reg_script_run_on_dev(dev, spi_ops);
		return;
	default:
		return;
	}

	reg_script_run_on_dev(dev, ops);
}

static void lpss_init(struct device *dev)
{
	struct soc_intel_baytrail_config *config = config_of(dev);
	int iosf_reg, nvs_index;

	dev_ctl_reg(dev, &iosf_reg, &nvs_index);

	if (iosf_reg < 0) {
		int slot = PCI_SLOT(dev->path.pci.devfn);
		int func = PCI_FUNC(dev->path.pci.devfn);
		printk(BIOS_DEBUG, "Could not find iosf_reg for %02x.%01x\n", slot, func);
		return;
	}
	dev_enable_snoop_and_pm(dev, iosf_reg);
	dev_set_int_pin(dev, iosf_reg);
	dev_configure_default_frequency(dev);
	dev_disable_resets(dev);
	dev_set_pins(dev);

	if (config->lpss_acpi_mode)
		dev_enable_acpi_mode(dev, iosf_reg, nvs_index);
	else
		store_acpi_nvs(dev, nvs_index);
}

static void lpss_write_dma_group(struct device *dma, unsigned long *current, u16 revision,
				 const char cntlr_uid[4], u32 gsi, u8 num_channels,
				 u16 base_request_line)
{
	struct acpi_csrt_group *group = (struct acpi_csrt_group *)*current;

	acpi_write_csrt_group_hdr(current, "INTL", "\0\0\0\0",
				  0x9c60, 0, revision);

	acpi_write_csrt_shared_info(current, group, dma, PCI_BASE_ADDRESS_0,
				    1 /* major_ver */, 0 /* minor_ver */,
				    gsi, ACPI_ACTIVE_BOTH,
				    ACPI_LEVEL_SENSITIVE, num_channels,
				    DMA_ADDRESS_WIDTH, base_request_line,
				    DMA_NUM_HANDSHAKE_SIGNALS,
				    DMA_MAX_BLOCK_SIZE);

	acpi_write_csrt_descriptor(current, group, ACPI_CSRT_TYPE_DMA,
				   ACPI_CSRT_DMA_CONTROLLER, cntlr_uid);

	char uid[4] = "CHA";
	for (unsigned int ch = 0; ch < num_channels; ch++) {
		uid[3] = ch + 0x30;
		acpi_write_csrt_descriptor(current, group, ACPI_CSRT_TYPE_DMA,
					   ACPI_CSRT_DMA_CHANNEL, uid);
	}
}

static unsigned long lpss_fill_csrt(acpi_csrt_t *csrt_struct, unsigned long current)
{
	struct device *dma1 = pcidev_on_root(SIO_DMA1_DEV, SIO_DMA1_FUNC);
	struct device *dma2 = pcidev_on_root(SIO_DMA2_DEV, SIO_DMA2_FUNC);

	if (dma1 && dma1->enabled) {
		lpss_write_dma_group(dma1, &current, DMA1_GROUP_REVISION, "SPI ",
				     LPSS_DMA1_IRQ, DMA1_NUM_CHANNELS, DMA1_BASE_REQ_LINE);
	}
	if (dma2 && dma2->enabled) {
		lpss_write_dma_group(dma2, &current, DMA2_GROUP_REVISION, "I2C ",
				     LPSS_DMA2_IRQ, DMA2_NUM_CHANNELS, DMA2_BASE_REQ_LINE);
	}

	return current;
}

static unsigned long lpss_write_csrt(const struct device *device, unsigned long current,
				     acpi_rsdp_t *rsdp)
{
	acpi_csrt_t *csrt;
	static bool csrt_written = false;

	if (csrt_written)
		return current;

	printk(BIOS_DEBUG, "ACPI:    * CSRT\n");
	current = ALIGN_UP(current, 16);
	csrt = (acpi_csrt_t *)current;
	acpi_create_csrt(csrt, lpss_fill_csrt);
	acpi_add_table(rsdp, csrt);
	csrt_written = true;

	current += csrt->header.length;

	return current;
}

static struct device_operations device_ops = {
	.read_resources		= pci_dev_read_resources,
	.set_resources		= pci_dev_set_resources,
	.enable_resources	= pci_dev_enable_resources,
	.init			= lpss_init,
	.ops_pci		= &soc_pci_ops,
	.write_acpi_tables	= lpss_write_csrt,
};

static const unsigned short pci_device_ids[] = {
	SIO_DMA1_DEVID,
	I2C1_DEVID,
	I2C2_DEVID,
	I2C3_DEVID,
	I2C4_DEVID,
	I2C5_DEVID,
	I2C6_DEVID,
	I2C7_DEVID,
	SIO_DMA2_DEVID,
	PWM1_DEVID,
	PWM2_DEVID,
	HSUART1_DEVID,
	HSUART2_DEVID,
	SPI_DEVID,
	0,
};

static const struct pci_driver southcluster __pci_driver = {
	.ops		= &device_ops,
	.vendor		= PCI_VID_INTEL,
	.devices	= pci_device_ids,
};
