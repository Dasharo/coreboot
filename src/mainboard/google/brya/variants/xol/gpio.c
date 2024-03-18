/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <baseboard/variants.h>
#include <soc/gpio.h>

static const struct pad_config gpio_overrides[] = {
	/* A6  : ESPI_ALERT1# ==> NC */
	PAD_NC(GPP_A6, NONE),
	/* A7  : SRCCLK_OE7# ==> PU 100K 3.3V */
	PAD_CFG_GPI(GPP_A7, NONE, DEEP),
	/* A8  : SRCCLKREQ7# ==> NC */
	PAD_NC(GPP_A8, NONE),
	/* A12 : SATAXPCIE1 ==> NC */
	PAD_NC(GPP_A12, NONE),
	/* A14 : USB_OC1# ==> NC */
	PAD_NC(GPP_A14, NONE),
	/* A15 : USB_OC2# ==> NC */
	PAD_NC(GPP_A15, NONE),
	/* A16 : USB_OC3# ==> NC */
	PAD_NC(GPP_A16, NONE),
	/* A17 : DISP_MISCC ==> EN_WCAM_PWR */
	PAD_CFG_GPO(GPP_A17, 1, DEEP),
	/* A21 : DDPC_CTRCLK ==> NC */
	PAD_NC(GPP_A21, NONE),
	/* A22 : DDPC_CTRLDATA ==> NC */
	PAD_NC(GPP_A22, NONE),

	/* B2  : VRALERT# ==> NC */
	PAD_NC(GPP_B2, NONE),
	/* B3  : PROC_GP2 ==> PU 100K 1.8V */
	PAD_CFG_GPI(GPP_B3, NONE, DEEP),
	/* B4  : PROC_GP3 ==> NC */
	PAD_NC(GPP_B4, NONE),
	/* B5  : ISH_I2C0_SDA ==> NC */
	PAD_NC(GPP_B5, NONE),
	/* B6  : ISH_I2C0_SCL ==> NC */
	PAD_NC(GPP_B6, NONE),
	/* B7  : ISH_12C1_SDA ==> NC */
	PAD_NC(GPP_B7, NONE),
	/* B8  : ISH_I2C1_SCL ==> NC */
	PAD_NC(GPP_B8, NONE),
	/* B9  : NC */
	PAD_NC(GPP_B9, NONE),
	/* B10 : NC */
	PAD_NC(GPP_B10, NONE),
	/* B11 : PMCALERT# ==> NC */
	PAD_NC(GPP_B11, NONE),
	/* B15 : TIME_SYNC0 ==> PU 100K 3.3V */
	PAD_CFG_GPI(GPP_B15, NONE, DEEP),

	/* C0  : SMBCLK ==> NC */
	PAD_NC(GPP_C0, NONE),
	/* C1  : SMBDATA ==> NC */
	PAD_NC(GPP_C1, NONE),
	/* C3  : SML0CLK ==> NC */
	PAD_NC(GPP_C3, NONE),
	/* C4  : SML0DATA ==> NC */
	PAD_NC(GPP_C4, NONE),
	/* C6  : SML1CLK ==> NC */
	PAD_NC(GPP_C6, NONE),
	/* C7  : SML1DATA ==> NC */
	PAD_NC(GPP_C7, NONE),

	/* D0  : ISH_GP0 ==> NC */
	PAD_NC(GPP_D0, NONE),
	/* D1  : ISH_GP1 ==> NC */
	PAD_NC(GPP_D1, NONE),
	/* D2  : ISH_GP2 ==> NC */
	PAD_NC(GPP_D2, NONE),
	/* D3  : ISH_GP3 ==> NC */
	PAD_NC(GPP_D3, NONE),
	/* D5  : SRCCLKREQ0# ==> PU 100K 1.8V */
	PAD_CFG_GPI(GPP_D5, NONE, DEEP),
	/* D7  : SRCCLKREQ2# ==> NC */
	PAD_NC(GPP_D7, NONE),
	/* D8  : SRCCLKREQ3# ==> PU 100K 3.3V */
	PAD_CFG_GPI(GPP_D8, NONE, DEEP),
	/* D9  : ISH_SPI_CS# ==> NC */
	PAD_NC(GPP_D9, NONE),
	/* D13 : ISH_UART0_RXD ==> NC */
	PAD_NC(GPP_D13, NONE),
	/* D14 : ISH_UART0_TXD ==> NC */
	PAD_NC(GPP_D14, NONE),
	/* D15 : ISH_UART0_RTS# ==> NC */
	PAD_NC(GPP_D15, NONE),
	/* D16 : ISH_UART0_CTS# ==> NC */
	PAD_NC(GPP_D16, NONE),
	/* D17 : UART1_RXD ==> PU 100K 3.3V */
	PAD_CFG_GPI(GPP_D17, NONE, DEEP),
	/* D18 : UART1_TXD ==> NC */
	PAD_NC(GPP_D18, NONE),

	/* E0 : SATAXPCIE0 ==> NC */
	PAD_NC(GPP_E0, NONE),
	/* E3  : PROC_GP0 ==> PU 100K 1.8V */
	PAD_CFG_GPI(GPP_E3, NONE, DEEP),
	/* E4  : SATA_DEVSLP0 ==> NC */
	PAD_NC(GPP_E4, NONE),
	/* E5  : SATA_DEVSLP1 ==> NC */
	PAD_NC(GPP_E5, NONE),
	/* E7  : PROC_GP1 ==> PU 100K 1.8V */
	PAD_CFG_GPI(GPP_E7, NONE, DEEP),
	/* E10 : THC0_SPI1_CS# ==> PU 100K 1.8V */
	PAD_CFG_GPI(GPP_E10, NONE, DEEP),
	/* E16 : RSVD_TP ==> NC */
	PAD_NC(GPP_E16, NONE),
	/* E17 : THC0_SPI1_INT# ==> PU 100K 1.8V */
	PAD_CFG_GPI(GPP_E17, NONE, DEEP),
	/* E18 : DDP1_CTRLCLK ==> NC */
	PAD_NC(GPP_E18, NONE),
	/* E20 : DDP2_CTRLCLK ==> NC */
	PAD_NC(GPP_E20, NONE),

	/* F6  : CNV_PA_BLANKING ==> NC */
	PAD_NC(GPP_F6, NONE),
	/* F11 : THC1_SPI2_CLK ==> NC */
	PAD_NC(GPP_F11, NONE),
	/* F12 : GSXDOUT ==> NC */
	PAD_NC(GPP_F12, NONE),
	/* F13 : GSXDOUT ==> NC */
	PAD_NC(GPP_F13, NONE),
	/* F15 : GSXSRESET# ==> PU 100K 3.3V */
	PAD_CFG_GPI(GPP_F15, NONE, DEEP),
	/* F16 : GSXCLK ==> NC */
	PAD_NC(GPP_F16, NONE),
	/* F19 : SRCCLKREQ6# ==> NC */
	PAD_NC(GPP_F19, NONE),
	/* F20 : EXT_PWR_GATE# ==> NC */
	PAD_NC(GPP_F20, NONE),
	/* F21 : EXT_PWR_GATE2# ==> PU 100K 1.8V */
	PAD_CFG_GPI(GPP_F21, NONE, DEEP),

	/* H3  : SX_EXIT_HOLDOFF# ==> NC */
	PAD_NC(GPP_H3, NONE),
	/* H8  : I2C4_SDA ==> NC */
	PAD_NC(GPP_H8, NONE),
	/* H9  : I2C4_SCL ==> NC */
	PAD_NC(GPP_H9, NONE),
	/* H12 : I2C7_SDA ==> PU 100K 3.3V */
	PAD_CFG_GPI(GPP_H12, NONE, DEEP),
	/* H13 : I2C7_SCL ==> NC */
	PAD_NC(GPP_H13, NONE),
	/* H19 : SRCCLKREQ4# ==> PU 100K 1.8V */
	PAD_CFG_GPI(GPP_H19, NONE, DEEP),
	/* H20 : IMGCLKOUT1 ==> NC */
	PAD_NC(GPP_H20, NONE),
	/* H21 : IMGCLKOUT2 ==> NC */
	PAD_NC(GPP_H21, NONE),
	/* H22 : IMGCLKOUT3 ==> NC */
	PAD_NC(GPP_H22, NONE),
	/* H23 : SRCCLKREQ5# ==> PU 100K 3.3V */
	PAD_CFG_GPI(GPP_H23, NONE, DEEP),

	/* R4 : HDA_RST# ==> DMIC_CLK0 */
	PAD_CFG_NF(GPP_R4, NONE, DEEP, NF3),
	/* R5 : HDA_SDI1 ==> DMIC_DATA0 */
	PAD_CFG_NF(GPP_R5, NONE, DEEP, NF3),
	/* R6 : I2S2_TXD ==> NC */
	PAD_NC(GPP_R6, NONE),
	/* R7 : I2S2_RXD ==> NC */
	PAD_NC(GPP_R7, NONE),

	/* S0 : SNDW0_CLK ==> SDW_HP_CLK_R */
	PAD_CFG_NF(GPP_S0, NONE, DEEP, NF4),
	/* S1 : SNDW0_DATA ==> SDW_HP_DATA_R */
	PAD_CFG_NF(GPP_S1, NONE, DEEP, NF4),
	/* S2 : SNDW1_CLK ==> DMIC_CLK0_R */
	PAD_CFG_NF(GPP_S2, NONE, DEEP, NF4),
	/* S3 : SNDW1_DATA ==> NC */
	PAD_NC(GPP_S3, NONE),
	/* S4 : SNDW2_CLK ==> NC */
	PAD_NC(GPP_S4, NONE),
	/* S5 : SNDW2_DATA ==> NC */
	PAD_NC(GPP_S5, NONE),
	/* S6 : SNDW3_CLK ==> NC */
	PAD_NC(GPP_S6, NONE),
	/* S7 : SNDW3_DATA ==> NC */
	PAD_NC(GPP_S7, NONE),

	/* GPD6: SLP_A# ==> NC */
	PAD_NC(GPD6, NONE),
	/* GPD8: SUSCLK ==> NC */
	PAD_NC(GPD8, NONE),
	/* GPD9: SLP_WLAN# ==> NC */
	PAD_NC(GPD9, NONE),
	/* GPD11: LANPHYC ==> NC */
	PAD_NC(GPD11, NONE),
};

/* Early pad configuration in bootblock */
static const struct pad_config early_gpio_table[] = {
	/* A13 : PMC_I2C_SCL ==> GSC_PCH_INT_ODL */
	PAD_CFG_GPI_APIC(GPP_A13, NONE, PLTRST, LEVEL, INVERT),
	/* E13 : THC0_SPI1_IO2 ==> MEM_CH_SEL */
	PAD_CFG_GPI(GPP_E13, NONE, DEEP),
	/* E15 : RSVD_TP ==> PCH_WP_OD */
	PAD_CFG_GPI_GPIO_DRIVER(GPP_E15, NONE, DEEP),
	/* F18 : EC_IN_RW_OD ==> EC_IN_RW_OD */
	PAD_CFG_GPI(GPP_F18, NONE, DEEP),
	/* H6  : I2C1_SDA ==> PCH_I2C_TPM_SDA */
	PAD_CFG_NF(GPP_H6, NONE, DEEP, NF1),
	/* H7  : I2C1_SCL ==> PCH_I2C_TPM_SCL */
	PAD_CFG_NF(GPP_H7, NONE, DEEP, NF1),
	/* H10 : UART0_RXD ==> UART_PCH_RX_DBG_TX */
	PAD_CFG_NF(GPP_H10, NONE, DEEP, NF2),
	/* H11 : UART0_TXD ==> UART_PCH_TX_DBG_RX */
	PAD_CFG_NF(GPP_H11, NONE, DEEP, NF2),

	/* Add virtual GPIOs for CPU PCIe RP */
	PAD_CFG_NF_VWEN(GPP_vGPIO_PCIE_48, NONE, PLTRST, NF1),
	PAD_CFG_NF_VWEN(GPP_vGPIO_PCIE_49, NONE, PLTRST, NF1),
	PAD_CFG_NF_VWEN(GPP_vGPIO_PCIE_50, NONE, PLTRST, NF1),
	PAD_CFG_NF_VWEN(GPP_vGPIO_PCIE_51, NONE, PLTRST, NF1),
	PAD_CFG_NF_VWEN(GPP_vGPIO_PCIE_52, NONE, PLTRST, NF1),
	PAD_CFG_NF_VWEN(GPP_vGPIO_PCIE_53, NONE, PLTRST, NF1),
	PAD_CFG_NF_VWEN(GPP_vGPIO_PCIE_54, NONE, PLTRST, NF1),
	PAD_CFG_NF_VWEN(GPP_vGPIO_PCIE_55, NONE, PLTRST, NF1),
	PAD_CFG_NF_VWEN(GPP_vGPIO_PCIE_56, NONE, PLTRST, NF1),
	PAD_CFG_NF_VWEN(GPP_vGPIO_PCIE_57, NONE, PLTRST, NF1),
	PAD_CFG_NF_VWEN(GPP_vGPIO_PCIE_58, NONE, PLTRST, NF1),
	PAD_CFG_NF_VWEN(GPP_vGPIO_PCIE_59, NONE, PLTRST, NF1),
	PAD_CFG_NF_VWEN(GPP_vGPIO_PCIE_60, NONE, PLTRST, NF1),
	PAD_CFG_NF_VWEN(GPP_vGPIO_PCIE_61, NONE, PLTRST, NF1),
	PAD_CFG_NF_VWEN(GPP_vGPIO_PCIE_62, NONE, PLTRST, NF1),
	PAD_CFG_NF_VWEN(GPP_vGPIO_PCIE_63, NONE, PLTRST, NF1),
	PAD_CFG_NF_VWEN(GPP_vGPIO_PCIE_76, NONE, PLTRST, NF1),
	PAD_CFG_NF_VWEN(GPP_vGPIO_PCIE_77, NONE, PLTRST, NF1),
	PAD_CFG_NF_VWEN(GPP_vGPIO_PCIE_78, NONE, PLTRST, NF1),
	PAD_CFG_NF_VWEN(GPP_vGPIO_PCIE_79, NONE, PLTRST, NF1),
};

const struct pad_config *variant_gpio_override_table(size_t *num)
{
	*num = ARRAY_SIZE(gpio_overrides);
	return gpio_overrides;
}

const struct pad_config *variant_early_gpio_table(size_t *num)
{
	*num = ARRAY_SIZE(early_gpio_table);
	return early_gpio_table;
}