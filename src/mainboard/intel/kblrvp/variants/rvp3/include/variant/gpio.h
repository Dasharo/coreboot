/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef MAINBOARD_GPIO_H
#define MAINBOARD_GPIO_H

#include <soc/gpe.h>
#include <soc/gpio.h>

/* TCA6424A  I/O Expander */
#define IO_EXPANDER_BUS		4
#define IO_EXPANDER_0_ADDR	0x22
#define IO_EXPANDER_P0CONF	0x0C	/* Port 0 conf offset */
#define IO_EXPANDER_P0DOUT	0x04	/* Port 0 data offset */
#define IO_EXPANDER_P1CONF	0x0D
#define IO_EXPANDER_P1DOUT	0x05
#define IO_EXPANDER_P2CONF	0x0E
#define IO_EXPANDER_P2DOUT	0x06
#define IO_EXPANDER_1_ADDR	0x23

/* EC wake is LAN_WAKE# which is a special DeepSX wake pin */
#define GPE_EC_WAKE		GPE0_LAN_WAK

/* GPP_E16 is EC_SCI_L. GPP_E group is routed to DW2 in the GPE0 block */
#define EC_SCI_GPI		GPE0_DW2_16
#define EC_SMI_GPI		GPP_E15

#ifndef __ACPI__
/* Pad configuration in ramstage. */
static const struct pad_config gpio_table[] = {
/* PCH_RCIN */		PAD_CFG_NF(GPP_A0, NONE, DEEP, NF1),
/* LPC_LAD_0 */		PAD_CFG_NF(GPP_A1, DN_20K, DEEP, NF1),
/* LPC_LAD_1 */		PAD_CFG_NF(GPP_A2, DN_20K, DEEP, NF1),
/* LPC_LAD_2 */		PAD_CFG_NF(GPP_A3, DN_20K, DEEP, NF1),
/* LPC_LAD_3 */		PAD_CFG_NF(GPP_A4, DN_20K, DEEP, NF1),
/* LPC_FRAME */		PAD_CFG_NF(GPP_A5, NONE, DEEP, NF1),
/* LPC_SERIRQ */	PAD_CFG_NF(GPP_A6, NONE, DEEP, NF1),
/* PM_SLP_S0ix_N */	PAD_CFG_GPI_GPIO_DRIVER(GPP_A7, UP_20K, DEEP),
/* LPC_CLKRUN */	PAD_CFG_NF(GPP_A8, NONE, DEEP, NF1),
/* LPC_CLK */		PAD_CFG_NF(GPP_A9, DN_20K, DEEP, NF1),
/* PCH_LPC_CLK */	PAD_CFG_NF(GPP_A10, DN_20K, DEEP, NF1),
/* EC_HID_INT */	PAD_NC(GPP_A11, NONE),
/* ISH_KB_PROX_INT */	PAD_NC(GPP_A12, NONE),
/* PCH_SUSPWRACB */	PAD_CFG_NF(GPP_A13, NONE, DEEP, NF1),
/* PM_SUS_STAT */	PAD_NC(GPP_A14, NONE),
/* SUSACK_R_N */	PAD_CFG_NF(GPP_A15, DN_20K, DEEP, NF1),
/* SD_1P8_SEL */	PAD_CFG_NF(GPP_A16, NONE, DEEP, NF1),
/* SD_PWR_EN */		PAD_CFG_NF(GPP_A17, NONE, DEEP, NF1),
/* ISH_GP0 */		PAD_NC(GPP_A18, NONE),
/* ISH_GP1 */		PAD_NC(GPP_A19, NONE),
/* ISH_GP2 */		PAD_NC(GPP_A20, NONE),
/* ISH_GP3 */		PAD_NC(GPP_A21, NONE),
/* ISH_GP4 */		PAD_NC(GPP_A22, NONE),
/* ISH_GP5 */		PAD_NC(GPP_A23, NONE),
/* V0.85A_VID0 */	PAD_NC(GPP_B0, NONE),
/* V0.85A_VID1 */	PAD_NC(GPP_B1, NONE),
/* GP_VRALERTB */	PAD_CFG_GPI_GPIO_DRIVER(GPP_B2, NONE, DEEP),
/* TCH_PAD_INTR */	PAD_CFG_GPI_APIC_HIGH(GPP_B3, NONE, PLTRST),
/* BT_RF_KILL */	PAD_CFG_GPO(GPP_B4, 1, DEEP),
/* CLK_REQ_SLOT0 */	PAD_NC(GPP_B5, NONE),
/* CLK_REQ_SLOT1 */	PAD_CFG_NF(GPP_B6, NONE, DEEP, NF1),
/* CLK_REQ_SLOT2 */	PAD_CFG_NF(GPP_B7, NONE, DEEP, NF1),
/* CLK_REQ_SLOT3 */	PAD_CFG_NF(GPP_B8, NONE, DEEP, NF1),
/* CLK_REQ_SLOT4 */	PAD_CFG_NF(GPP_B9, NONE, DEEP, NF1),
/* CLK_REQ_SLOT5 */	PAD_CFG_NF(GPP_B10, NONE, DEEP, NF1),
/* MPHY_EXT_PWR_GATE */	PAD_CFG_NF(GPP_B11, NONE, DEEP, NF1),
/* PM_SLP_S0 */		PAD_CFG_NF(GPP_B12, NONE, DEEP, NF1),
/* PCH_PLT_RST */	PAD_CFG_NF(GPP_B13, NONE, DEEP, NF1),
/* TCH_PNL_PWREN */	PAD_CFG_GPO(GPP_B14, 1, DEEP),
/* GSPI0_CS# */		/* GPP_B15 */
/* WLAN_PCIE_WAKE */	PAD_CFG_GPI_SCI(GPP_B16, NONE, DEEP, EDGE_SINGLE, INVERT),
/* TBT_CIO */		PAD_NC(GPP_B17, NONE),
/* SLOT1_WAKE */	PAD_CFG_GPI_SCI(GPP_B18, UP_20K, DEEP, EDGE_SINGLE, INVERT),
/* GSPI1_CS */		PAD_CFG_NF(GPP_B19, NONE, DEEP, NF1),
/* GSPI1_CLK */		PAD_CFG_NF(GPP_B20, DN_20K, DEEP, NF1),
/* GSPI1_MISO */	PAD_CFG_NF(GPP_B21, DN_20K, DEEP, NF1),
/* GSPI1_MOSI */	PAD_CFG_NF(GPP_B22, DN_20K, DEEP, NF1),
/* GNSS_RESET */	PAD_CFG_GPO(GPP_B23, 1, DEEP),
/* SMB_CLK */		PAD_CFG_NF(GPP_C0, NONE, DEEP, NF1),
/* SMB_DATA */		PAD_CFG_NF(GPP_C1, DN_20K, DEEP, NF1),
/* SMBALERT# */		PAD_CFG_GPO(GPP_C2, 1, DEEP),
/* SML0_CLK */		PAD_CFG_NF(GPP_C3, NONE, DEEP, NF1),
/* SML0DATA */		PAD_CFG_NF(GPP_C4, NONE, DEEP, NF1),
/* SML0ALERT# */	PAD_CFG_GPI_APIC_HIGH(GPP_C5, DN_20K, PLTRST),
/* SML1_CLK */		PAD_CFG_NF(GPP_C6, NONE, DEEP, NF1),
/* SML1_DATA */		PAD_CFG_NF(GPP_C7, DN_20K, DEEP, NF1),
/* UART0_RXD */		PAD_CFG_NF(GPP_C8, NONE, DEEP, NF1),
/* UART0_TXD */		PAD_CFG_NF(GPP_C9, NONE, DEEP, NF1),
/* UART0_RTS */		PAD_CFG_NF(GPP_C10, NONE, DEEP, NF1),
/* UART0_CTS */		PAD_CFG_NF(GPP_C11, NONE, DEEP, NF1),
/* UART1_RXD */		PAD_CFG_NF(GPP_C12, NONE, DEEP, NF1),
/* UART1_TXD */		PAD_CFG_NF(GPP_C13, NONE, DEEP, NF1),
/* UART1_RTS */		PAD_CFG_NF(GPP_C14, NONE, DEEP, NF1),
/* UART1_CTS */		PAD_CFG_NF(GPP_C15, NONE, DEEP, NF1),
/* I2C0_SDA */		PAD_CFG_NF(GPP_C16, UP_5K, DEEP, NF1),
/* I2C0_SCL */		PAD_CFG_NF(GPP_C17, UP_5K, DEEP, NF1),
/* I2C1_SDA */		PAD_CFG_NF(GPP_C18, NONE, DEEP, NF1),
/* I2C1_SCL */		PAD_CFG_NF(GPP_C19, NONE, DEEP, NF1),
/* UART2_RXD */		PAD_CFG_NF(GPP_C20, NONE, DEEP, NF1),
/* UART2_TXD */		PAD_CFG_NF(GPP_C21, NONE, DEEP, NF1),
/* UART2_RTS */		PAD_CFG_NF(GPP_C22, NONE, DEEP, NF1),
/* UART2_CTS */		PAD_CFG_NF(GPP_C23, NONE, DEEP, NF1),
/* SPI1_CS */		PAD_CFG_NF(GPP_D0, NONE, DEEP, NF1),
/* SPI1_CLK */		PAD_CFG_NF(GPP_D1, NONE, DEEP, NF1),
/* SPI1_MISO */		PAD_CFG_NF(GPP_D2, NONE, DEEP, NF1),
/* SPI1_MOSI */		PAD_CFG_NF(GPP_D3, NONE, DEEP, NF1),
/* CAM_FLASH_STROBE */	PAD_CFG_NF(GPP_D4, NONE, DEEP, NF1),
/* ISH_I2C0_SDA */	PAD_CFG_NF(GPP_D5, NONE, DEEP, NF1),
/* ISH_I2C0_SCL */	PAD_CFG_NF(GPP_D6, NONE, DEEP, NF1),
/* ISH_I2C1_SDA */	PAD_CFG_NF(GPP_D7, NONE, DEEP, NF1),
/* ISH_I2C1_SCL */	PAD_CFG_NF(GPP_D8, NONE, DEEP, NF1),
/* HOME_BTN */		PAD_CFG_GPI_GPIO_DRIVER(GPP_D9, NONE, DEEP),
/* SCREEN_LOCK */	PAD_CFG_GPI_GPIO_DRIVER(GPP_D10, NONE, DEEP),
/* VOL_UP_PCH */	PAD_CFG_GPI_GPIO_DRIVER(GPP_D11, NONE, DEEP),
/* VOL_DOWN_PCH */	PAD_CFG_GPI_GPIO_DRIVER(GPP_D12, NONE, DEEP),
/* ISH_UART0_RXD */	PAD_CFG_NF(GPP_D13, NONE, DEEP, NF1),
/* ISH_UART0_TXD */	PAD_CFG_NF(GPP_D14, NONE, DEEP, NF1),
/* ISH_UART0_RTS */	PAD_CFG_NF(GPP_D15, NONE, DEEP, NF1),
/* ISH_UART0_CTS */	PAD_CFG_NF(GPP_D16, NONE, DEEP, NF1),
/* DMIC_CLK_1 */	PAD_CFG_NF(GPP_D17, NONE, DEEP, NF1),
/* DMIC_DATA_1 */	PAD_CFG_NF(GPP_D18, DN_20K, DEEP, NF1),
/* DMIC_CLK_0 */	PAD_CFG_NF(GPP_D19, NONE, DEEP, NF1),
/* DMIC_DATA_0 */	PAD_CFG_NF(GPP_D20, DN_20K, DEEP, NF1),
/* SPI1_D2 */		PAD_CFG_NF(GPP_D21, NONE, DEEP, NF1),
/* SPI1_D3 */		PAD_CFG_NF(GPP_D22, NONE, DEEP, NF1),
/* I2S_MCLK */		PAD_CFG_NF(GPP_D23, NONE, DEEP, NF1),
/* SPI_TPM_IRQ */	PAD_CFG_GPI_APIC_HIGH(GPP_E0, NONE, PLTRST),
/* SATAXPCIE1 */	PAD_CFG_NF(GPP_E1, NONE, DEEP, NF1),
/* SSD_PEDET */		PAD_CFG_GPI_GPIO_DRIVER(GPP_E2, NONE, DEEP),
/* EINK_SSR_DFU_N */	PAD_CFG_GPO(GPP_E3, 1, DEEP),
/* SSD_SATA_DEVSLP */	PAD_CFG_GPO(GPP_E4, 0, DEEP),
/* SATA_DEVSLP1 */	PAD_CFG_NF(GPP_E5, NONE, DEEP, NF1),
/* SATA_DEVSLP2 */	/* GPP_E6 */
/* TCH_PNL_INTR* */	PAD_CFG_GPI_APIC_HIGH(GPP_E7, NONE, PLTRST),
/* SATALED# */		PAD_CFG_NF(GPP_E8, NONE, DEEP, NF1),
/* USB2_OC_0 */		PAD_CFG_NF(GPP_E9, NONE, DEEP, NF1),
/* USB2_OC_1 */		PAD_CFG_NF(GPP_E10, NONE, DEEP, NF1),
/* USB2_OC_2 */		PAD_CFG_NF(GPP_E11, NONE, DEEP, NF1),
/* USB2_OC_3 */		PAD_CFG_GPI_APIC_HIGH(GPP_E12, NONE, PLTRST),
/* DDI1_HPD */		PAD_CFG_NF(GPP_E13, NONE, DEEP, NF1),
/* DDI2_HPD */		PAD_CFG_NF(GPP_E14, NONE, DEEP, NF1),
/* EC_SMI */		PAD_CFG_GPI_SMI(GPP_E15, NONE, DEEP, EDGE_SINGLE, INVERT),
/* EC_SCI */		PAD_CFG_GPI_SCI(GPP_E16, NONE, DEEP, EDGE_SINGLE, INVERT),
/* EDP_HPD */		PAD_CFG_NF(GPP_E17, NONE, DEEP, NF1),
/* DDPB_CTRLCLK */	PAD_CFG_NF(GPP_E18, NONE, DEEP, NF1),
/* DDPB_CTRLDATA */	PAD_CFG_NF(GPP_E19, DN_20K, DEEP, NF1),
/* DDPC_CTRLCLK */	PAD_CFG_NF(GPP_E20, NONE, DEEP, NF1),
/* DDPC_CTRLDATA */	PAD_CFG_NF(GPP_E21, DN_20K, DEEP, NF1),
/* DDPD_CTRLCLK */	PAD_CFG_GPI_APIC_HIGH(GPP_E22, NONE, DEEP),
/* TCH_PNL_RST */	PAD_CFG_GPO(GPP_E23, 1, DEEP),
/* I2S2_SCLK */		PAD_CFG_NF(GPP_F0, NONE, DEEP, NF1),
/* I2S2_SFRM */		PAD_CFG_NF(GPP_F1, NONE, DEEP, NF1),
/* I2S2_TXD */		PAD_CFG_NF(GPP_F2, NONE, DEEP, NF1),
/* I2S2_RXD */		PAD_CFG_NF(GPP_F3, NONE, DEEP, NF1),
/* I2C2_SDA */		PAD_CFG_NF_1V8(GPP_F4, NONE, DEEP, NF1),
/* I2C2_SCL */		PAD_CFG_NF_1V8(GPP_F5, NONE, DEEP, NF1),
/* I2C3_SDA */		PAD_CFG_NF_1V8(GPP_F6, NONE, DEEP, NF1),
/* I2C3_SCL */		PAD_CFG_NF_1V8(GPP_F7, NONE, DEEP, NF1),
/* I2C4_SDA */		PAD_CFG_NF_1V8(GPP_F8, NONE, DEEP, NF1),
/* I2C4_SDA */		PAD_CFG_NF_1V8(GPP_F9, NONE, DEEP, NF1),
/* ISH_I2C2_SDA */	PAD_CFG_NF_1V8(GPP_F10, NONE, DEEP, NF2),
/* ISH_I2C2_SCL */	PAD_CFG_NF_1V8(GPP_F11, NONE, DEEP, NF2),
/* EMMC_CMD */		PAD_CFG_NF(GPP_F12, NONE, DEEP, NF1),
/* EMMC_DATA0 */	PAD_CFG_NF(GPP_F13, NONE, DEEP, NF1),
/* EMMC_DATA1 */	PAD_CFG_NF(GPP_F14, NONE, DEEP, NF1),
/* EMMC_DATA2 */	PAD_CFG_NF(GPP_F15, NONE, DEEP, NF1),
/* EMMC_DATA3 */	PAD_CFG_NF(GPP_F16, NONE, DEEP, NF1),
/* EMMC_DATA4 */	PAD_CFG_NF(GPP_F17, NONE, DEEP, NF1),
/* EMMC_DATA5 */	PAD_CFG_NF(GPP_F18, NONE, DEEP, NF1),
/* EMMC_DATA6 */	PAD_CFG_NF(GPP_F19, NONE, DEEP, NF1),
/* EMMC_DATA7 */	PAD_CFG_NF(GPP_F20, NONE, DEEP, NF1),
/* EMMC_RCLK */		PAD_CFG_NF(GPP_F21, NONE, DEEP, NF1),
/* EMMC_CLK */		PAD_CFG_NF(GPP_F22, NONE, DEEP, NF1),
/* UIM_SIM_DET */	PAD_CFG_GPI_APIC_HIGH(GPP_F23, NONE, DEEP),
/* SD_CMD */		PAD_CFG_NF(GPP_G0, NONE, DEEP, NF1),
/* SD_DATA0 */		PAD_CFG_NF(GPP_G1, NONE, DEEP, NF1),
/* SD_DATA1 */		PAD_CFG_NF(GPP_G2, NONE, DEEP, NF1),
/* SD_DATA2 */		PAD_CFG_NF(GPP_G3, NONE, DEEP, NF1),
/* SD_DATA3 */		PAD_CFG_NF(GPP_G4, NONE, DEEP, NF1),
/* SD_CD# */		PAD_CFG_NF(GPP_G5, NONE, DEEP, NF1),
/* SD_CLK */		PAD_CFG_NF(GPP_G6, NONE, DEEP, NF1),
/* SD_WP */		PAD_CFG_NF(GPP_G7, NONE, DEEP, NF1),
/* PCH_BATLOW */	PAD_CFG_NF(GPD0, NONE, DEEP, NF1),
/* AC_PRESENT */	PAD_CFG_NF(GPD1, NONE, DEEP, NF1),
/* PCH_WAKE */		PAD_CFG_NF(GPD2, NONE, DEEP, NF1),
/* PCH_PWRBTN */	PAD_CFG_NF(GPD3, NONE, DEEP, NF1),
/* PM_SLP_S3# */	PAD_CFG_NF(GPD4, NONE, DEEP, NF1),
/* PM_SLP_S4# */	PAD_CFG_NF(GPD5, NONE, DEEP, NF1),
/* PM_SLP_SA# */	PAD_CFG_NF(GPD6, NONE, DEEP, NF1),
/* GPD7 */		PAD_NC(GPD7, NONE),
/* PM_SUSCLK */		PAD_CFG_NF(GPD8, NONE, DEEP, NF1),
/* PCH_SLP_WLAN# */	PAD_CFG_NF(GPD9, NONE, DEEP, NF1),
/* PM_SLP_S5# */	PAD_CFG_NF(GPD10, NONE, DEEP, NF1),
/* LANPHYC */		PAD_CFG_NF(GPD11, NONE, DEEP, NF1),
};

/* Early pad configuration in bootblock */
static const struct pad_config early_gpio_table[] = {
/* UART2_RXD */		PAD_CFG_NF(GPP_C20, NONE, DEEP, NF1),
/* UART2_TXD */		PAD_CFG_NF(GPP_C21, NONE, DEEP, NF1),
};

#endif

#endif
