/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef MAINBOARD_GPIO_H
#define MAINBOARD_GPIO_H

#include <soc/gpe.h>
#include <soc/gpio.h>

#ifndef __ACPI__

/* Pad configuration in ramstage. */
static const struct pad_config gpio_table[] = {
/* RCIN# */			PAD_CFG_NF(GPP_A0, NONE, DEEP, NF1),
/* LAD0 */			PAD_CFG_NF(GPP_A1, NONE, DEEP, NF1),
/* LAD1 */			PAD_CFG_NF(GPP_A2, NONE, DEEP, NF1),
/* LAD2 */			PAD_CFG_NF(GPP_A3, NONE, DEEP, NF1),
/* LAD3 */			PAD_CFG_NF(GPP_A4, NONE, DEEP, NF1),
/* LFRAME# */		PAD_CFG_NF(GPP_A5, NONE, DEEP, NF1),
/* SERIRQ */		PAD_CFG_NF(GPP_A6, NONE, DEEP, NF1),
/* PIRQA# */		PAD_CFG_NC(GPP_A7),
/* CLKRUN# */		PAD_CFG_NF(GPP_A8, NONE, DEEP, NF1),
/* CLKOUT_LPC0 */	PAD_CFG_NF(GPP_A9, NONE, DEEP, NF1),
/* CLKOUT_LPC1 */	PAD_CFG_NF(GPP_A10, NONE, DEEP, NF1),
/* PME# */			PAD_CFG_NC(GPP_A11),
/* BM_BUSY# */		PAD_CFG_NC(GPP_A12),
/* SUSWARN# */		PAD_CFG_NF(GPP_A13, NONE, DEEP, NF1),
/* SUS_STAT# */		PAD_CFG_NF(GPP_A14, NONE, DEEP, NF1),
/* SUSACK# */		PAD_CFG_NF(GPP_A15, DN_20K, DEEP, NF1),
/* SD_1P8_SEL */	PAD_CFG_NC(GPP_A16),
/* SD_PWR_EN# */	PAD_CFG_NF(GPP_A17, NONE, DEEP, NF1),
/* ISH_GP0 */		PAD_CFG_GPI_GPIO_DRIVER(GPP_A18, NONE, DEEP),
/* ISH_GP1 */		PAD_CFG_GPI_GPIO_DRIVER(GPP_A19, NONE, DEEP),
/* ISH_GP2 */		PAD_CFG_GPI_GPIO_DRIVER(GPP_A20, NONE, DEEP),
/* ISH_GP3 */		PAD_CFG_NC(GPP_A21),
/* ISH_GP4 */		PAD_CFG_NC(GPP_A22),
/* ISH_GP5 */		PAD_CFG_NC(GPP_A23),

/* CORE_VID0 */		PAD_CFG_NC(GPP_B0),
/* CORE_VID1 */		PAD_CFG_NC(GPP_B1),
/* VRALERT# */		PAD_CFG_NC(GPP_B2),
/* CPU_GP2 */		PAD_CFG_NC(GPP_B3),
/* CPU_GP3 */		PAD_CFG_NC(GPP_B4),
/* SRCCLKREQ0# */	PAD_CFG_NF(GPP_B5, NONE, DEEP, NF1),
/* SRCCLKREQ1# */	PAD_CFG_NF(GPP_B6, NONE, DEEP, NF1),
/* SRCCLKREQ2# */	PAD_CFG_NF(GPP_B7, NONE, DEEP, NF1),
/* SRCCLKREQ3# */	PAD_CFG_NF(GPP_B8, NONE, DEEP, NF1),
/* SRCCLKREQ4# */	PAD_CFG_NF(GPP_B9, NONE, DEEP, NF1),
/* SRCCLKREQ5# */	PAD_CFG_NF(GPP_B10, NONE, DEEP, NF1),
/* EXT_PWR_GATE# */	PAD_CFG_NC(GPP_B11),
/* SLP_S0# */		PAD_CFG_NF(GPP_B12, NONE, DEEP, NF1),
/* PLTRST# */		PAD_CFG_NF(GPP_B13, NONE, DEEP, NF1),
/* SPKR */			PAD_CFG_TERM_GPO(GPP_B14, 1, DN_20K, DEEP),
/* GSPI0_CS# */		PAD_CFG_NC(GPP_B15),
/* GSPI0_CLK */		PAD_CFG_NC(GPP_B16),
/* GSPI0_MISO */	PAD_CFG_NC(GPP_B17),
/* GSPI0_MOSI */	PAD_CFG_GPI_SCI(GPP_B18, UP_20K, PLTRST, LEVEL,
					INVERT),
/* GSPI1_CS# */		PAD_CFG_NC(GPP_B19),
/* GSPI1_CLK */		PAD_CFG_NC(GPP_B20),
/* GSPI1_MISO */	PAD_CFG_NC(GPP_B21),
/* GSPI1_MOSI */	PAD_CFG_NF(GPP_B22, DN_20K, DEEP, NF1),
/* SM1ALERT# */		PAD_CFG_TERM_GPO(GPP_B23, 1, DN_20K, DEEP),

/* SMBCLK */		PAD_CFG_NF(GPP_C0, NONE, DEEP, NF1),
/* SMBDATA */		PAD_CFG_NF(GPP_C1, DN_20K, DEEP, NF1),
/* SMBALERT# */		PAD_CFG_TERM_GPO(GPP_C2, 1, DN_20K, DEEP),
/* SML0CLK */		PAD_CFG_NF(GPP_C3, NONE, DEEP, NF1),
/* SML0DATA */		PAD_CFG_NF(GPP_C4, NONE, DEEP, NF1),
/* SML0ALERT# */	PAD_CFG_GPI_APIC_INVERT(GPP_C5, DN_20K, DEEP),
/* SML1CLK */		PAD_CFG_NC(GPP_C6), /* RESERVED */
/* SML1DATA */		PAD_CFG_NC(GPP_C7), /* RESERVED */
/* UART0_RXD */		PAD_CFG_NF(GPP_C8, NONE, DEEP, NF1),
/* UART0_TXD */		PAD_CFG_NF(GPP_C9, NONE, DEEP, NF1),
/* UART0_RTS# */	PAD_CFG_NF(GPP_C10, NONE, DEEP, NF1),
/* UART0_CTS# */	PAD_CFG_NF(GPP_C11, NONE, DEEP, NF1),
/* UART1_RXD */		PAD_CFG_NC(GPP_C12),
/* UART1_TXD */		PAD_CFG_NC(GPP_C13),
/* UART1_RTS# */	PAD_CFG_NC(GPP_C14),
/* UART1_CTS# */	PAD_CFG_NC(GPP_C15),
/* I2C0_SDA */		PAD_CFG_GPI_GPIO_DRIVER(GPP_C16, NONE, DEEP),
/* I2C0_SCL */		PAD_CFG_GPI_GPIO_DRIVER(GPP_C17, NONE, DEEP),
/* I2C1_SDA */		PAD_CFG_GPI_GPIO_DRIVER(GPP_C18, NONE, DEEP),
/* I2C1_SCL */		PAD_CFG_NC(GPP_C19),
/* UART2_RXD */		PAD_CFG_NC(GPP_C20),
/* UART2_TXD */		PAD_CFG_NC(GPP_C21),
/* UART2_RTS# */	PAD_CFG_NC(GPP_C22),
/* UART2_CTS# */	PAD_CFG_NC(GPP_C23),

/* SPI1_CS# */			PAD_CFG_NC(GPP_D0),
/* SPI1_CLK */			PAD_CFG_NC(GPP_D1),
/* SPI1_MISO */			PAD_CFG_NC(GPP_D2),
/* SPI1_MOSI */			PAD_CFG_NC(GPP_D3),
/* FASHTRIG */			PAD_CFG_NC(GPP_D4),
/* ISH_I2C0_SDA */		PAD_CFG_NC(GPP_D5),
/* ISH_I2C0_SCL */		PAD_CFG_NC(GPP_D6),
/* ISH_I2C1_SDA */		PAD_CFG_NC(GPP_D7),
/* ISH_I2C1_SCL */		PAD_CFG_NC(GPP_D8),
/* ISH_SPI_CS# */		PAD_CFG_TERM_GPO(GPP_D9, 0, NONE, DEEP),
/* ISH_SPI_CLK */		PAD_CFG_GPI_GPIO_DRIVER(GPP_D10, NONE, DEEP),
/* ISH_SPI_MISO */		PAD_CFG_TERM_GPO(GPP_D11, 1, NONE, DEEP),
/* ISH_SPI_MOSI */		PAD_CFG_NC(GPP_D12),
/* ISH_UART0_RXD */		PAD_CFG_NC(GPP_D13),
/* ISH_UART0_TXD */		PAD_CFG_NC(GPP_D14),
/* ISH_UART0_RTS# */	PAD_CFG_NC(GPP_D15),
/* ISH_UART0_CTS# */	PAD_CFG_NC(GPP_D16),
/* DMIC_CLK1 */			PAD_CFG_NF(GPP_D17, NONE, DEEP, NF1),
/* DMIC_DATA1 */		PAD_CFG_NF(GPP_D18, DN_20K, DEEP, NF1),
/* DMIC_CLK0 */			PAD_CFG_NF(GPP_D19, NONE, DEEP, NF1),
/* DMIC_DATA0 */		PAD_CFG_NF(GPP_D20, DN_20K, DEEP, NF1),
/* SPI1_IO2 */			PAD_CFG_NC(GPP_D21),
/* SPI1_IO3 */			PAD_CFG_NC(GPP_D22),
/* I2S_MCLK */			PAD_CFG_NC(GPP_D23),

/* SATAXPCI0 */		PAD_CFG_NC(GPP_E0),
/* SATAXPCIE1 */	PAD_CFG_NC(GPP_E1),
/* SATAXPCIE2 */	PAD_CFG_NF(GPP_E2, UP_20K, DEEP, NF1),
/* CPU_GP0 */		PAD_CFG_NC(GPP_E3),
/* SATA_DEVSLP0 */	PAD_CFG_NC(GPP_E4),
/* SATA_DEVSLP1 */	PAD_CFG_NC(GPP_E5),
/* SATA_DEVSLP2 */	PAD_CFG_NC(GPP_E6),
/* CPU_GP1 */		PAD_CFG_NC(GPP_E7),
/* SATALED# */		PAD_CFG_NC(GPP_E8),
/* USB2_OCO# */		PAD_CFG_NF(GPP_E9, NONE, DEEP, NF1),
/* USB2_OC1# */		PAD_CFG_NF(GPP_E10, NONE, DEEP, NF1),
/* USB2_OC2# */		PAD_CFG_NF(GPP_E11, NONE, DEEP, NF1),
/* USB2_OC3# */		PAD_CFG_NC(GPP_E12),
/* DDPB_HPD0 */		PAD_CFG_NF(GPP_E13, NONE, DEEP, NF1),
/* DDPC_HPD1 */		PAD_CFG_NF(GPP_E14, NONE, DEEP, NF1),
/* DDPD_HPD2 */		PAD_CFG_NC(GPP_E15),
/* DDPE_HPD3 */		PAD_CFG_GPI_ACPI_SCI(GPP_E16, NONE, PLTRST, NONE),
/* EDP_HPD */		PAD_CFG_NF(GPP_E17, NONE, DEEP, NF1),
/* DDPB_CTRLCLK */	PAD_CFG_NF(GPP_E18, NONE, DEEP, NF1),
/* DDPB_CTRLDATA */	PAD_CFG_NF(GPP_E19, DN_20K, DEEP, NF1),
/* DDPC_CTRLCLK */	PAD_CFG_NF(GPP_E20, NONE, DEEP, NF1),
/* DDPC_CTRLDATA */	PAD_CFG_NF(GPP_E21, DN_20K, DEEP, NF1),
/* DDPD_CTRLCLK */	PAD_CFG_GPI_APIC(GPP_E22, NONE, DEEP),
/* DDPD_CTRLDATA */	PAD_CFG_TERM_GPO(GPP_E23, 1, DN_20K, DEEP),

/* I2S2_SCLK */		PAD_CFG_NC(GPP_F0),
/* I2S2_SFRM */		PAD_CFG_NC(GPP_F1),
/* I2S2_TXD */		PAD_CFG_NC(GPP_F2),
/* I2S2_RXD */		PAD_CFG_NC(GPP_F3),
/* I2C2_SDA */		PAD_CFG_NC(GPP_F4),
/* I2C2_SCL */		PAD_CFG_NC(GPP_F5),
/* I2C3_SDA */		PAD_CFG_NC(GPP_F6),
/* I2C3_SCL */		PAD_CFG_NC(GPP_F7),
/* I2C4_SDA */		PAD_CFG_NF_1V8(GPP_F8, NONE, DEEP, NF1),
/* I2C4_SCL */		PAD_CFG_NF_1V8(GPP_F9, NONE, DEEP, NF1),
/* I2C5_SDA */		PAD_CFG_NC(GPP_F10),
/* I2C5_SCL */		PAD_CFG_NC(GPP_F11),
/* EMMC_CMD */		PAD_CFG_NC(GPP_F12),
/* EMMC_DATA0 */	PAD_CFG_NC(GPP_F13),
/* EMMC_DATA1 */	PAD_CFG_NC(GPP_F14),
/* EMMC_DATA2 */	PAD_CFG_NC(GPP_F15),
/* EMMC_DATA3 */	PAD_CFG_NC(GPP_F16),
/* EMMC_DATA4 */	PAD_CFG_NC(GPP_F17),
/* EMMC_DATA5 */	PAD_CFG_NC(GPP_F18),
/* EMMC_DATA6 */	PAD_CFG_NC(GPP_F19),
/* EMMC_DATA7 */	PAD_CFG_NC(GPP_F20),
/* EMMC_RCLK */		PAD_CFG_NC(GPP_F21),
/* EMMC_CLK */		PAD_CFG_NC(GPP_F22),
/* RSVD */			PAD_CFG_NC(GPP_F23),

/* SD_CMD */		PAD_CFG_NF(GPP_G0, NONE, DEEP, NF1),
/* SD_DATA0 */		PAD_CFG_NF(GPP_G1, NONE, DEEP, NF1),
/* SD_DATA1 */		PAD_CFG_NF(GPP_G2, NONE, DEEP, NF1),
/* SD_DATA2 */		PAD_CFG_NF(GPP_G3, NONE, DEEP, NF1),
/* SD_DATA3 */		PAD_CFG_NF(GPP_G4, NONE, DEEP, NF1),
/* SD_CD# */		PAD_CFG_NF(GPP_G5, NONE, DEEP, NF1),
/* SD_CLK */		PAD_CFG_NF(GPP_G6, NONE, DEEP, NF1),
/* SD_WP */			PAD_CFG_NF(GPP_G7, UP_20K, DEEP, NF1),

/* BATLOW# */		PAD_CFG_NC(GPD0),
/* ACPRESENT */		PAD_CFG_NF(GPD1, NONE, PWROK, NF1),
/* LAN_WAKE# */		PAD_CFG_NC(GPD2),
/* PWRBTN# */		PAD_CFG_NF(GPD3, UP_20K, PWROK, NF1),
/* SLP_S3# */		PAD_CFG_NF(GPD4, NONE, PWROK, NF1),
/* SLP_S4# */		PAD_CFG_NF(GPD5, NONE, PWROK, NF1),
/* SLP_A# */		PAD_CFG_NF(GPD6, NONE, PWROK, NF1),
/* RSVD */			PAD_CFG_NC(GPD7),
/* SUSCLK */		PAD_CFG_NF(GPD8, NONE, PWROK, NF1),
/* SLP_WLAN# */		PAD_CFG_NF(GPD9, NONE, PWROK, NF1),
/* SLP_S5# */		PAD_CFG_NF(GPD10, NONE, PWROK, NF1),
/* LANPHYC */		PAD_CFG_NF(GPD11, NONE, DEEP, NF1),
};

#endif

#endif
