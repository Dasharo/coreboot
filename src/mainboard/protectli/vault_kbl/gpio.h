/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef _GPIOFW6B_H
#define _GPIOFW6B_H

#include <soc/gpe.h>
#include <soc/gpio.h>

#ifndef __ACPI__

/* Pad configuration in ramstage. */
static const struct pad_config gpio_table[] = {
/* RCIN# */		PAD_CFG_NF(GPP_A0, NONE, DEEP, NF1),
/* LPC_LAD_0 */		PAD_CFG_NF(GPP_A1, UP_20K, DEEP, NF1),
/* LPC_LAD_1 */		PAD_CFG_NF(GPP_A2, UP_20K, DEEP, NF1),
/* LPC_LAD_2 */		PAD_CFG_NF(GPP_A3, UP_20K, DEEP, NF1),
/* LPC_LAD_3 */		PAD_CFG_NF(GPP_A4, UP_20K, DEEP, NF1),
/* LPC_FRAME */		PAD_CFG_NF(GPP_A5, NONE, DEEP, NF1),
/* LPC_SERIRQ */	PAD_CFG_NF(GPP_A6, NONE, DEEP, NF1),
/* PIRQA_N*/		PAD_CFG_TERM_GPO(GPP_A7, 1, NONE, DEEP),
/* LPC_CLKRUN */	PAD_NC(GPP_A8, NONE),
/* PCH_LPC_CLK0 */	PAD_CFG_NF(GPP_A9, DN_20K, DEEP, NF1),
/* PCH_LPC_CLK1 */	PAD_CFG_NF(GPP_A10, DN_20K, DEEP, NF1),
/* PME# */		PAD_CFG_NF(GPP_A11, UP_20K, DEEP, NF1),
/* ISH_GP6 */		PAD_NC(GPP_A12, NONE),
/* PCH_SUSPWRACB */	PAD_CFG_NF(GPP_A13, NONE, DEEP, NF1),
/* PCH_SUSSTAT */	PAD_NC(GPP_A14, NONE),
/* PCH_SUSACK */	PAD_CFG_NF(GPP_A15, DN_20K, DEEP, NF1),
/* SD_1P8_SEL */	PAD_NC(GPP_A16, NONE),
/* SD_PWR_EN */		PAD_NC(GPP_A17, NONE),
/* ISH_GP0 */		PAD_NC(GPP_A18, NONE),
/* ISH_GP1 */		PAD_NC(GPP_A19, NONE),
/* ISH_GP2 */		PAD_NC(GPP_A20, NONE),
/* ISH_GP3 */		PAD_NC(GPP_A21, NONE),
/* ISH_GP4 */		PAD_NC(GPP_A22, NONE),
/* ISH_GP5 */		PAD_NC(GPP_A23, NONE),
/* CORE_VID0 */		PAD_NC(GPP_B0, NONE),
/* CORE_VID1 */		PAD_NC(GPP_B1, NONE),
/* VRALERT_N */		PAD_CFG_NF(GPP_B2, NONE, DEEP, NF1),
/* CPU_GP2 */		PAD_NC(GPP_B3, NONE),
/* CPU_GP3 */		PAD_NC(GPP_B4, NONE),
/* SRCCLKREQ0_N */	PAD_NC(GPP_B5, NONE),
/* SRCCLKREQ1_N*/	PAD_NC(GPP_B6, NONE),
/* SRCCLKREQ2_N*/	PAD_NC(GPP_B7, NONE),
/* SRCCLKREQ3_N*/	PAD_NC(GPP_B8, NONE),
/* SRCCLKREQ4_N*/	PAD_NC(GPP_B9, NONE),
/* SRCCLKREQ5_N*/	PAD_NC(GPP_B10, NONE),
/* EXT_PWR_GATE_N */	PAD_CFG_NF(GPP_B11, NONE, DEEP, NF1),
/* SLP_S0# */		PAD_CFG_NF(GPP_B12, NONE, DEEP, NF1),
/* PCH_PLT_RST */	PAD_CFG_NF(GPP_B13, NONE, DEEP, NF1),
/* SPKR */		PAD_CFG_NF(GPP_B14, DN_20K, PLTRST, NF1),
/* GSPI0_CS_N */	PAD_NC(GPP_B15, NONE),
/* GSPI0_CLK */		PAD_NC(GPP_B16, NONE),
/* GSPI0_MISO */	PAD_NC(GPP_B17, NONE),
/* GSPI0_MOSI */	PAD_NC(GPP_B18, NONE),
/* GSPI1_CS_N */	PAD_NC(GPP_B19, NONE),
/* GSPI1_CLK */		PAD_NC(GPP_B20, NONE),
/* GSPI1_MISO */	PAD_NC(GPP_B21, NONE),
/* GSPI1_MOSI */	PAD_NC(GPP_B22, NONE),
/* SM1ALERT# */		PAD_NC(GPP_B23, NONE),
/* SMB_CLK */		PAD_CFG_NF(GPP_C0, NONE, DEEP, NF1),
/* SMB_DATA */		PAD_CFG_NF(GPP_C1, NONE, DEEP, NF1),
/* SMBALERT# */		PAD_CFG_NF(GPP_C2, NONE, DEEP, NF1),
/* SML0_CLK */		PAD_NC(GPP_C3, NONE),
/* SML0DATA */		PAD_NC(GPP_C4, NONE),
/* SML0ALERT# */	PAD_NC(GPP_C5, NONE),
/* UART0_RXD */		PAD_NC(GPP_C8, NONE),
/* UART0_TXD */		PAD_NC(GPP_C9, NONE),
/* UART0_CTS_N */	PAD_NC(GPP_C10, NONE),
/* UART0_RTS_N */	PAD_NC(GPP_C11, NONE),
/* UART1_RXD */		PAD_NC(GPP_C12, NONE),
/* UART1_TXD */		PAD_NC(GPP_C13, NONE),
/* UART1_CTS_N */	PAD_NC(GPP_C14, NONE),
/* UART1_RTS_N */	PAD_NC(GPP_C15, NONE),
/* I2C0_SDA */		PAD_NC(GPP_C16, NONE),
/* I2C0_SCL */		PAD_NC(GPP_C17, NONE),
/* I2C1_SDA */		PAD_NC(GPP_C18, NONE),
/* I2C1_SCL */		PAD_NC(GPP_C19, NONE),
/* UART2_RXD */		PAD_NC(GPP_C20, NONE),
/* UART2_TXD */		PAD_NC(GPP_C21, NONE),
/* UART2_CTS_N */	PAD_NC(GPP_C22, NONE),
/* UART2_RTS_N */	PAD_NC(GPP_C23, NONE),
/* ITCH_SPI_CS */	PAD_NC(GPP_D0, NONE),
/* ITCH_SPI_CLK */	PAD_NC(GPP_D1, NONE),
/* ITCH_SPI_MISO_1 */	PAD_NC(GPP_D2, NONE),
/* ITCH_SPI_MISO_0 */	PAD_NC(GPP_D3, NONE),
/* FLASHTRIG */		PAD_NC(GPP_D4, NONE),
/* ISH_I2C0_SDA */	PAD_NC(GPP_D5, NONE),
/* ISH_I2C0_SCL */	PAD_NC(GPP_D6, NONE),
/* ISH_I2C1_SDA */	PAD_NC(GPP_D7, NONE),
/* ISH_I2C1_SCL */	PAD_NC(GPP_D8, NONE),
/* GPP_D9 */		PAD_NC(GPP_D9, NONE),
/* GPP_D10 */		PAD_NC(GPP_D10, NONE),
/* GPP_D11 */		PAD_NC(GPP_D11, NONE),
/* GPP_D12 */		PAD_NC(GPP_D12, NONE),
/* ISH_UART0_RXD */	PAD_NC(GPP_D13, NONE),
/* ISH_UART0_TXD */	PAD_NC(GPP_D14, NONE),
/* ISH_UART0_RTS */	PAD_NC(GPP_D15, NONE),
/* ISH_UART0_CTS */	PAD_NC(GPP_D16, NONE),
/* DMIC_CLK_1 */	PAD_NC(GPP_D17, NONE),
/* DMIC_DATA_1 */	PAD_NC(GPP_D18, NONE),
/* DMIC_CLK_0 */	PAD_NC(GPP_D19, NONE),
/* DMIC_DATA_0 */	PAD_NC(GPP_D20, NONE),
/* ITCH_SPI_D2 */	PAD_NC(GPP_D21, NONE),
/* ITCH_SPI_D3 */	PAD_NC(GPP_D22, NONE),
/* I2S_MCLK */		PAD_NC(GPP_D23, NONE),
/* SATAXPCIE0 (TP8) */	PAD_NC(GPP_E0, NONE),
/* SATAXPCIE1 (TP9)*/	PAD_NC(GPP_E1, NONE),
/* SATAXPCIE2 (TP10) */	PAD_NC(GPP_E2, NONE),
/* CPU_GP0 */		PAD_NC(GPP_E3, NONE),
/* SATA_DEVSLP0 */	PAD_NC(GPP_E4, NONE),
/* SATA_DEVSLP1 */	PAD_NC(GPP_E5, NONE),
/* SATA_DEVSLP2 */	PAD_NC(GPP_E6, NONE),
/* CPU_GP1 */		PAD_NC(GPP_E7, NONE),
/* SATA_LED */		PAD_CFG_NF(GPP_E8, NONE, DEEP, NF1),
/* USB2_OC_0 */		PAD_NC(GPP_E9, NONE),
/* USB2_OC_1 */		PAD_NC(GPP_E10, NONE),
/* USB2_OC_2 */		PAD_NC(GPP_E11, NONE),
/* USB2_OC_3 */		PAD_NC(GPP_E12, NONE),
/* DDI1_HPD */		PAD_CFG_NF(GPP_E13, NONE, DEEP, NF1),
/* DDI2_HPD */		PAD_NC(GPP_E14, NONE),
/* DDI3_HPD */		PAD_NC(GPP_E15, NONE),
/* DDI4_HPD */		PAD_NC(GPP_E16, NONE),
/* EDP_HPD */		PAD_NC(GPP_E17, NONE),
/* DDPB_CTRLCLK */	PAD_CFG_NF(GPP_E18, NONE, DEEP, NF1),
/* DDPB_CTRLDATA */	PAD_CFG_NF(GPP_E19, DN_20K, DEEP, NF1),
/* DDPC_CTRLCLK */	PAD_NC(GPP_E20, NONE),
/* DDPC_CTRLDATA */	PAD_NC(GPP_E21, NONE),
/* DDPD_CTRLCLK */	PAD_NC(GPP_E22, NONE),
/* DDPD_CTRLDATA */	PAD_NC(GPP_E23, NONE),
/* I2S2_SCLK */		PAD_NC(GPP_F0, NONE),
/* I2S2_SFRM */		PAD_NC(GPP_F1, NONE),
/* I2S2_TXD */		PAD_NC(GPP_F2, NONE),
/* I2S2_RXD */		PAD_NC(GPP_F3, NONE),
/* I2C2_SDA */		PAD_NC(GPP_F4, NONE),
/* I2C2_SCL */		PAD_NC(GPP_F5, NONE),
/* I2C3_SDA */		PAD_NC(GPP_F6, NONE),
/* I2C3_SCL */		PAD_NC(GPP_F7, NONE),
/* I2C4_SDA */		PAD_NC(GPP_F8, NONE),
/* I2C4_SDA */		PAD_NC(GPP_F9, NONE),
/* I2C5_SDA */		PAD_NC(GPP_F10, NONE),
/* I2C5_SCL */		PAD_NC(GPP_F11, NONE),
/* EMMC_CMD */		PAD_NC(GPP_F12, NONE),
/* EMMC_DATA0 */	PAD_NC(GPP_F13, NONE),
/* EMMC_DATA1 */	PAD_NC(GPP_F14, NONE),
/* EMMC_DATA2 */	PAD_NC(GPP_F15, NONE),
/* EMMC_DATA3 */	PAD_NC(GPP_F16, NONE),
/* EMMC_DATA4 */	PAD_NC(GPP_F17, NONE),
/* EMMC_DATA5 */	PAD_NC(GPP_F18, NONE),
/* EMMC_DATA6 */	PAD_NC(GPP_F19, NONE),
/* EMMC_DATA7 */	PAD_NC(GPP_F20, NONE),
/* EMMC_RCLK */		PAD_NC(GPP_F21, NONE),
/* EMMC_CLK */		PAD_NC(GPP_F22, NONE),
/* GPP_F23 */		PAD_NC(GPP_F23, NONE),
/* SD_CMD */		PAD_NC(GPP_G0, NONE),
/* SD_DATA0 */		PAD_NC(GPP_G1, NONE),
/* SD_DATA1 */		PAD_NC(GPP_G2, NONE),
/* SD_DATA2 */		PAD_NC(GPP_G3, NONE),
/* SD_DATA3 */		PAD_NC(GPP_G4, NONE),
/* SD_CD# */		PAD_NC(GPP_G5, NONE),
/* SD_CLK */		PAD_NC(GPP_G6, NONE),
/* SD_WP */		PAD_NC(GPP_G7, NONE),
/* PCH_BATLOW */	PAD_NC(GPD0, NONE),
/* ACPRESENT */		PAD_CFG_NF(GPD1, NONE, DEEP, NF1),
/* LAN_WAKE_N */	PAD_NC(GPD2, NONE),
/* PWRBTN */		PAD_CFG_NF(GPD3, UP_20K, DEEP, NF1),
/* PM_SLP_S3# */	PAD_CFG_NF(GPD4, NONE, DEEP, NF1),
/* PM_SLP_S4# */	PAD_CFG_NF(GPD5, NONE, DEEP, NF1),
/* PM_SLP_SA# (TP7) */	PAD_CFG_NF(GPD6, NONE, DEEP, NF1),
/* GPD7_RSVD */		PAD_NC(GPD7, NONE),
/* PM_SUSCLK */		PAD_CFG_NF(GPD8, NONE, DEEP, NF1),
/* SLP_WLAN# (TP6) */	PAD_NC(GPD9, NONE),
/* SLP_S5# (TP3) */	PAD_CFG_NF(GPD10, NONE, DEEP, NF1),
/* LANPHYC */		PAD_NC(GPD11, NONE),
};

#endif

#endif
