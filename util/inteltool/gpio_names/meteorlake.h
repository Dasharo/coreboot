/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef GPIO_NAMES_METEORLAKE
#define GPIO_NAMES_METEORLAKE

#include "gpio_groups.h"

/* ----------------------------- Meteor Lake ----------------------------- */

const char *const meteorlake_pch_group_a_names[] = {
	"GPP_A0",		"ESPI_IO0",         "n/a",                      "n/a",          "n/a",              "n/a",  "n/a",
	"GPP_A1",		"ESPI_IO1",         "n/a",                      "n/a",          "n/a",              "n/a",  "n/a",
	"GPP_A2",       "ESPI_IO2",         "SUSWARN# / SUSPWRDNACK",   "n/a",          "n/a",              "n/a",  "n/a",
	"GPP_A3",       "ESPI_IO3",         "SUSACK#",                  "n/a",          "n/a",              "n/a",  "n/a",
	"GPP_A4",       "ESPI_CS0#",        "n/a",                      "n/a",          "n/a",              "n/a",  "n/a",
	"GPP_A5",       "ESPI_ALERT0#",     "n/a",                      "n/a",          "n/a",              "n/a",  "n/a",
	"GPP_A6",       "ESPI_ALERT1#",     "n/a",                      "n/a",          "n/a",              "n/a",  "n/a",
	"GPP_A7",       "SRCCLK_OE7#",      "n/a",                      "n/a",          "n/a",              "n/a",  "n/a",
	"GPP_A8",       "SRCCLKREQ7#",      "n/a",                      "n/a",          "n/a",              "n/a",  "n/a",
	"GPP_A9",       "ESPI_CLK",         "n/a",                      "n/a",          "n/a",              "n/a",  "n/a",
	"GPP_A10",      "ESPI_RESET#",      "n/a",                      "n/a",          "n/a",              "n/a",  "n/a",
	"GPP_A11",      "n/a",              "n/a",                      "n/a",          "n/a",              "n/a",  "n/a",
	"GPP_A12",      "SATAXPCIE1",       "SATAGP1",                  "n/a",          "SRCCLKREQ9B#",     "n/a",  "n/a",
	"GPP_A13",      "n/a",              "n/a",                      "n/a",          "n/a",              "n/a",  "n/a",
	"GPP_A14",      "USB_OC1#",         "DDSP_HPD3",                "n/a",          "DISP_MISC3",       "n/a",  "n/a",
	"GPP_A15",      "USB_OC2#",         "DDSP_HPD4",                "n/a",          "DISP_MISC4",       "n/a",  "n/a",
	"GPP_A16",      "USB_OC3#",         "n/a",                      "n/a",          "ISH_GP5",          "n/a",  "n/a",
	"GPP_A17",      "n/a",              "n/a",                      "n/a",          "DISP_MISCC",       "n/a",  "n/a",
	"GPP_A18",      "DDSP_HPDB",        "n/a",                      "n/a",          "DISP_MISCB",       "n/a",  "n/a",
	"GPP_A19",      "DDSP_HPD1",        "n/a",                      "n/a",          "DISP_MISC1",       "n/a",  "n/a",
	"GPP_A20",      "DDSP_HPD2",        "n/a",                      "n/a",          "DISP_MISC2",       "n/a",  "n/a",
	"GPP_A21",      "DDPC_CTRLCLK",     "n/a",                      "n/a",          "n/a",              "n/a",  "n/a",
	"GPP_A22",      "DDPC_CTRLDATA",    "n/a",                      "n/a",          "n/a",              "n/a",  "n/a",
	"GPP_A23",      "ESPI_CS1#",        "n/a",                      "n/a",          "n/a",              "n/a",  "n/a",
};

const struct gpio_group meteorlake_pch_group_a = {
	.display	= "------- GPIO Group GPP_A -------",
	.pad_count	= ARRAY_SIZE(meteorlake_pch_group_a_names) / 7,
	.func_count	= 7,
	.pad_names	= meteorlake_pch_group_a_names,
};

/*
"n/a",                 "n/a",                    "n/a",             "n/a",                "n/a", "n/a",  "n/a",
"n/a",                 "n/a",                    "n/a",             "n/a",                "n/a", "n/a",  "n/a",
*/

const char *const meteorlake_pch_group_b_names[] = {
	"GPP_B0",              "CORE_VID0",              "n/a",             "n/a",                "n/a",            "n/a",  "n/a",
	"GPP_B1",              "CORE_VID1",              "n/a",             "n/a",                "n/a",            "n/a",  "n/a",
	"GPP_B2",              "VRALERT#",               "n/a",             "n/a",                "n/a",            "n/a",  "n/a",
	"GPP_B3",              "PROC_GP2",               "n/a",             "n/a",                "ISH_GP4B",       "n/a",  "n/a",
	"GPP_B4",              "PROC_GP3",               "n/a",             "n/a",                "ISH_GP5B",       "n/a",  "n/a",
	"GPP_B5",              "ISH_I2C0_SDA",           "I2C2_SDA",        "n/a",                "n/a",            "n/a",  "n/a",
	"GPP_B6",              "ISH_I2C0_SCL",           "I2C2_SCL",        "n/a",                "n/a",            "n/a",  "n/a",
	"GPP_B7",              "ISH_I2C1_SDA",           "I2C3_SDA",        "n/a",                "n/a",            "n/a",  "n/a",
	"GPP_B8",              "ISH_I2C1_SCL",           "I2C3_SCL",        "n/a",                "n/a",            "n/a",  "n/a",

	"n/a",                 "n/a",                    "n/a",             "n/a",                "n/a",            "n/a",  "n/a",
	"n/a",                 "n/a",                    "n/a",             "n/a",                "n/a",            "n/a",  "n/a",

	"GPP_B11",             "PMCALERT#",              "n/a",             "n/a",                "n/a",            "n/a",  "n/a",
	"GPP_B12",             "SLP_S0#",                "n/a",             "n/a",                "n/a",            "n/a",  "n/a",
	"GPP_B13",             "PLTRST#",                "n/a",             "n/a",                "n/a",            "n/a",  "n/a",
	"GPP_B14",             "SPKR",                   "TIME_SYNC1",      "n/a",                "SATA_LED#",      "n/a",  "ISH_GP6",
	"GPP_B15",             "n/a",                    "TIME_SYNC0",      "n/a",                "n/a",            "n/a",  "ISH_GP7",
	"GPP_B16",             "n/a",                    "I2C5_SDA",        "n/a",                "ISH_I2C2_SDA",   "n/a",  "n/a",
	"GPP_B17",             "n/a",                    "I2C5_SCL",        "n/a",                "ISH_I2C2_SCL",   "n/a",  "n/a",
	"GPP_B18",             "n/a",                    "n/a",             "n/a",                "n/a",            "n/a",  "n/a",

	"n/a",                 "n/a",                    "n/a",             "n/a",                "n/a", "n/a",  "n/a",
	"n/a",                 "n/a",                    "n/a",             "n/a",                "n/a", "n/a",  "n/a",
	"n/a",                 "n/a",                    "n/a",             "n/a",                "n/a", "n/a",  "n/a",
	"n/a",                 "n/a",                    "n/a",             "n/a",                "n/a", "n/a",  "n/a",

	"GPP_B23",             "SML1ALERT#",             "PCHHOT#",         "n/a",                "n/a", "n/a",  "n/a",

};

const struct gpio_group meteorlake_pch_group_b = {
	.display	= "------- GPIO Group GPP_B -------",
	.pad_count	= ARRAY_SIZE(meteorlake_pch_group_b_names) / 7,
	.func_count	= 7,
	.pad_names	= meteorlake_pch_group_b_names,
};


const char *const meteorlake_pch_group_c_names[] = {
	"GPP_C0",   "SMBCLK",
	"GPP_C1",   "SMBDATA",
	"GPP_C2",   "SMBALERT#",
	"GPP_C3",   "SML0CLK",
	"GPP_C4",   "SML0DATA",
	"GPP_C5",   "SML0ALERT#",
	"GPP_C6",   "SML1CLK",
	"GPP_C7",   "SML1DATA",
};

const struct gpio_group meteorlake_pch_group_c = {
	.display	= "------- GPIO Group GPP_C -------",
	.pad_count	= ARRAY_SIZE(meteorlake_pch_group_c_names) / 2,
	.func_count	= 2,
	.pad_names	= meteorlake_pch_group_c_names,
};


const char *const meteorlake_pch_group_d_names[] = {
	"GPP_D0",       "ISH_GP0",              "BK0",              "n/a",          "n/a",              "n/a",  "SBK0",
	"GPP_D1",       "ISH_GP1",              "BK1",              "n/a",          "n/a",              "n/a",  "SBK1",
	"GPP_D2",       "ISH_GP2",              "BK2",              "n/a",          "n/a",              "n/a",  "SBK2",
	"GPP_D3",       "ISH_GP3",              "BK3",              "n/a",          "n/a",              "n/a",  "SBK3",
	"GPP_D4",       "IMGCLKOUT0",           "BK4",              "n/a",          "n/a",              "n/a",  "SBK4",
	"GPP_D5",       "SRCCLKREQ0#",          "n/a",              "n/a",          "n/a",              "n/a",  "n/a",
	"GPP_D6",       "SRCCLKREQ1#",          "n/a",              "n/a",          "n/a",              "n/a",  "n/a",
	"GPP_D7",       "SRCCLKREQ2#",          "n/a",              "n/a",          "n/a",              "n/a",  "n/a",
	"GPP_D8",       "SRCCLKREQ3#",          "n/a",              "n/a",          "n/a",              "n/a",  "n/a",
	"GPP_D9",       "ISH_SPI_CS#",          "DDP3_CTRLCLK",     "n/a",          "TBT_LSX2_TXD",     "n/a",  "BSSB_LS2_RX",
	"GPP_D10",      "ISH_SPI_CLK",          "DDP3_CTRLDATA",    "n/a",          "TBT_LSX2_RXD",     "n/a",  "BSSB_LS2_TX",
	"GPP_D11",      "ISH_SPI_MISO",         "DDP4_CTRLCLK",     "n/a",          "TBT_LSX3_TXD",     "n/a",  "BSSB_LS3_RX",
	"GPP_D12",      "ISH_SPI_MOSI",         "DDP4_CTRLDATA",    "n/a",          "TBT_LSX3_RXD",     "n/a",  "BSSB_LS3_TX",
	"GPP_D13",      "ISH_UART0_RXD",        "n/a",              "I2C6_SDA",     "n/a",              "n/a",  "n/a",
	"GPP_D14",      "ISH_UART0_TXD",        "n/a",              "I2C6_SCL",     "n/a",              "n/a",  "n/a",
	"GPP_D15",      "ISH_UART0_RTS#",       "n/a",              "n/a",          "I2C7B_SDA",        "n/a",  "n/a",
	"GPP_D16",      "ISH_UART0_CTS#",       "n/a",              "n/a",          "I2C7B_SCL",        "n/a",  "n/a",
	"GPP_D17",      "UART1_RXD",            "ISH_UART1_RXD",    "n/a",          "n/a",              "n/a",  "n/a",
	"GPP_D18",      "UART1_TXD",            "ISH_UART1_TXD",    "n/a",          "n/a",              "n/a",  "n/a",
	"GPP_D19",      "I2S_MCLK1_OUT",        "n/a",              "n/a",          "n/a",              "n/a",  "n/a",
};

const struct gpio_group meteorlake_pch_group_d = {
	.display	= "------- GPIO Group GPP_D -------",
	.pad_count	= ARRAY_SIZE(meteorlake_pch_group_d_names) / 7,
	.func_count	= 7,
	.pad_names	= meteorlake_pch_group_d_names,
};


const char *const meteorlake_pch_group_e_names[] = {
	"GPP_E0",   "SATAXPCIE0",       "SATAGP0",         "n/a",   "n/a",          "n/a",  "n/a",

	"n/a",        "n/a",           "n/a",               "n/a",  "n/a",          "n/a",  "n/a",
	"n/a",        "n/a",           "n/a",               "n/a",  "n/a",          "n/a",  "n/a",

	"GPP_E1",   "n/a",              "THC0_SPI1_IO2",    "n/a",  "n/a",          "n/a",  "n/a",
	"GPP_E2",   "n/a",              "THC0_SPI1_IO3",    "n/a",  "n/a",          "n/a",  "n/a",
	"GPP_E3",   "PROC_GP0",         "n/a",              "n/a",  "n/a",          "n/a",  "n/a",
	"GPP_E4",   "DEVSLP0",          "n/a",              "n/a",  "n/a",          "n/a",  "n/a",
	"GPP_E5",   "DEVSLP1",          "n/a",              "n/a",  "n/a",          "n/a",  "n/a",
	"GPP_E6",   "n/a",              "THC0_SPI1_RST#",   "n/a",  "n/a",          "n/a",  "n/a",
	"GPP_E7",   "PROC_GP1",         "n/a",              "n/a",  "n/a",          "n/a",  "n/a",
	"GPP_E8",   "n/a",              "n/a",              "n/a",  "n/a",          "n/a",  "n/a",
	"GPP_E9",   "USB_OC0#",         "ISH_GP4",          "n/a",  "n/a",          "n/a",  "n/a",
	"GPP_E10",  "n/a",              "THC0_SPI1_CS#",    "n/a",  "n/a",          "n/a",  "n/a",
	"GPP_E11",  "n/a",              "THC0_SPI1_CLK",    "n/a",  "n/a",          "n/a",  "n/a",
	"GPP_E12",  "n/a",              "THC0_SPI1_IO1",    "n/a",  "n/a",          "n/a",  "I2C0A_SDA",
	"GPP_E13",  "n/a",              "THC0_SPI1_IO0",    "n/a",  "n/a",          "n/a",  "I2C0A_SCL",
	"GPP_E14",  "DDSP_HPDA",        "DISP_MISC_A",      "n/a",  "n/a",          "n/a",  "n/a",
	"GPP_E15",  "n/a",              "Reserved",         "n/a",  "n/a",          "n/a",  "n/a",
	"GPP_E16",  "n/a",              "Reserved",         "n/a",  "n/a",          "n/a",  "n/a",
	"GPP_E17",  "n/a",              "THC0_SPI1_INT#",   "n/a",  "n/a",          "n/a",  "n/a",
	"GPP_E18",  "DDP1_CTRLCLK",     "n/a",              "n/a",  "TBT_LSX0_TXD", "n/a",  "BSSB_LS0_RX",
	"GPP_E19",  "DDP1_CTRLDATA",    "n/a",              "n/a",  "TBT_LSX0_RXD", "n/a",  "BSSB_LS0_TX",
	"GPP_E20",  "DDP2_CTRLCLK",     "n/a",              "n/a",  "TBT_LSX1_TXD", "n/a",  "BSSB_LS1_RX",
	"GPP_E21",  "DDP2_CTRLDATA",    "n/a",              "n/a",  "TBT_LSX1_RXD", "n/a",  "BSSB_LS1_TX",
	"GPP_E22",  "DDPA_CTRLCLK",     "DNX_FORCE_RELOAD", "n/a",  "n/a",          "n/a",  "n/a",
	"GPP_E23",  "DDPA_CTRLDATA",    "n/a",              "n/a",  "n/a",          "n/a",  "n/a",
};

const struct gpio_group meteorlake_pch_group_e = {
	.display	= "------- GPIO Group GPP_E -------",
	.pad_count	= ARRAY_SIZE(meteorlake_pch_group_e_names) / 7,
	.func_count	= 7,
	.pad_names	= meteorlake_pch_group_e_names,
};


const char *const meteorlake_pch_group_f_names[] = {
	"GPP_F0",   "CNV_BRI_DT",       "UART2_RTS#",   "n/a",              "n/a",          "n/a",  "n/a",
	"GPP_F1",   "CNV_BRI_RSP",      "UART2_RXD",    "n/a",              "n/a",          "n/a",  "n/a",
	"GPP_F2",   "CNV_RGI_DT",       "UART2_TXD",    "n/a",              "n/a",          "n/a",  "n/a",
	"GPP_F3",   "CNV_RGI_RSP",      "UART2_CTS#",   "n/a",              "n/a",          "n/a",  "n/a",
	"GPP_F4",   "CNV_RF_RESET#",    "n/a",          "n/a",              "n/a",          "n/a",  "n/a",
	"GPP_F5",   "n/a",              "MODEM_CLKREQ", "CRF_XTAL_CLKREQ",  "n/a",          "n/a",  "n/a",
	"GPP_F6",   "CNV_PA_BLANKING",  "n/a",          "n/a",              "n/a",          "n/a",  "n/a",
	"GPP_F7",   "n/a",              "n/a",          "n/a",              "n/a",          "n/a",  "n/a",
	"n/a",      "n/a",              "n/a",          "n/a",              "n/a",          "n/a",  "n/a",
	"GPP_F9",   "BOOTMPC",          "n/a",          "n/a",              "n/a",          "n/a",  "n/a",
	"GPP_F10",  "n/a",              "n/a",          "n/a",              "n/a",          "n/a",  "n/a",
	"GPP_F11",  "n/a",              "n/a",          "THC1_SPI2_CLK",    "GSPI1_CLK",    "n/a",  "n/a",
	"GPP_F12",  "GSXDOUT",          "n/a",          "THC1_SPI2_IO0",    "GSPI1_MOSI",   "n/a",  "I2C1A_SCL",
	"GPP_F13",  "GSXSLOAD",         "n/a",          "THC1_SPI2_IO1",    "GSPI1_MISO",   "n/a",  "I2C1A_SDA",
	"GPP_F14",  "GSXDIN",           "n/a",          "THC1_SPI2_IO2",    "n/a",          "n/a",  "n/a",
	"GPP_F15",  "GSXSRESET#",       "n/a",          "THC1_SPI2_IO3",    "n/a",          "n/a",  "n/a",
	"GPP_F16",  "GSXCLK",           "n/a",          "THC1_SPI2_CS#",    "GSPI_CS0#",    "n/a",  "n/a",
	"GPP_F17",  "n/a",              "n/a",          "THC1_SPI2_RST#",   "n/a",          "n/a",  "n/a",
	"GPP_F18",  "n/a",              "n/a",          "THC1_SPI2_INT#",   "n/a",          "n/a",  "n/a",
	"GPP_F19",  "SRCCLKREQ6#",      "n/a",          "n/a",              "n/a",          "n/a",  "n/a",
	"GPP_F20",  "Reserved",         "n/a",          "n/a",              "n/a",          "n/a",  "n/a",
	"GPP_F21",  "Reserved",         "n/a",          "n/a",              "n/a",          "n/a",  "n/a",
	"GPP_F22",  "VNN_CTRL",         "n/a",          "n/a",              "n/a",          "n/a",  "n/a",
	"GPP_F23",  "V1P05_CTRL",       "n/a",          "n/a",              "n/a",          "n/a",  "n/a"
};

const struct gpio_group meteorlake_pch_group_f = {
	.display	= "------- GPIO Group GPP_F -------",
	.pad_count	= ARRAY_SIZE(meteorlake_pch_group_f_names) / 7,
	.func_count	= 7,
	.pad_names	= meteorlake_pch_group_f_names,
};


const char *const meteorlake_pch_group_h_names[] = {
	"GPP_H0",	"n/a",				"n/a",				"n/a",				"n/a",		"n/a",	"n/a",
	"GPP_H1",	"n/a",				"n/a",				"n/a",				"n/a",		"n/a",	"n/a",
	"GPP_H2",	"n/a",				"n/a",				"n/a",				"n/a",		"n/a",	"n/a",
	"GPP_H3",	"SX_EXIT_HOLDOFF#",	"n/a",				"n/a",				"n/a",		"n/a",	"n/a",
	"GPP_H4",	"I2C0_SDA",			"n/a",				"n/a",				"n/a",		"n/a",	"n/a",
	"GPP_H5",	"I2C0_SCL",			"n/a",				"n/a",				"n/a",		"n/a",	"n/a",
	"GPP_H6",	"I2C1_SDA",			"n/a",				"n/a",				"n/a",		"n/a",	"n/a",
	"GPP_H7",	"I2C1_SCL",			"n/a",				"n/a",				"n/a",		"n/a",	"n/a",
	"GPP_H8",	"I2C4_SDA",			"CNV_MFUART2_RXD",	"n/a",				"n/a",		"n/a",	"n/a",
	"GPP_H9",	"I2C4_SCL",			"CNV_MFUART2_TXD",	"n/a",				"n/a",		"n/a",	"n/a",
	"GPP_H10",	"n/a",				"UART0_RXD",		"M2_SKT2_CFG0",		"n/a",		"n/a",	"n/a",
	"GPP_H11",	"n/a",				"UART0_TXD",		"M2_SKT2_CFG1",		"n/a", 		"n/a",	"n/a",
	"GPP_H12",	"I2C7_SDA",			"UART0_RTS#",		"M2_SKT2_CFG2",		"ISH_GP6B",	"n/a",	"DEVSLP0B",
	"GPP_H13",	"I2C7_SCL",			"UART0_CTS#",		"M2_SKT2_CFG3",		"ISH_GP7B",	"n/a",	"DEVSLP1B",

	"n/a","n/a","n/a","n/a","n/a","n/a","n/a",

	"GPP_H15",	"DDPB_CTRLCLK",		"n/a",				"PCIE_LINK_DOWN",	"n/a",		"n/a",	"n/a",

	"n/a","n/a","n/a","n/a","n/a","n/a","n/a",

	"GPP_H17",	"DDPB_CTRLDATA",	"n/a",				"n/a",				"n/a",		"n/a",	"n/a",
	"GPP_H18",	"PROC_C10_GATE#",	"n/a",				"n/a",				"n/a",		"n/a",	"n/a",
	"GPP_H19",	"SRCCLKREQ4#",		"n/a",				"n/a",				"n/a",		"n/a",	"n/a",
	"GPP_H20",	"IMGCLKOUT1",		"n/a",				"n/a",				"n/a",		"n/a",	"n/a",
	"GPP_H21",	"IMGCLKOUT2",		"n/a",				"n/a",				"n/a",		"n/a",	"n/a",
	"GPP_H22",	"IMGCLKOUT3",		"n/a",				"n/a",				"n/a",		"n/a",	"n/a",
	"GPP_H23",	"n/a",				"SRCCLKREQ5#",		"n/a",				"n/a",		"n/a",	"n/a",
};

const struct gpio_group meteorlake_pch_group_h = {
	.display	= "------- GPIO Group GPP_H -------",
	.pad_count	= ARRAY_SIZE(meteorlake_pch_group_h_names) / 7,
	.func_count	= 7,
	.pad_names	= meteorlake_pch_group_h_names,
};


const char *const meteorlake_pch_group_r_names[] = {
	"GPP_R0",	"HDA_BCLK",	"I2S0_SCLK",	"DMIC_CLK_B0",	"HDAPROC_BCLK",
	"GPP_R1",	"HDA_SYNC",	"I2S0_SFRM",	"DMIC_CLK_B1",	"n/a",
	"GPP_R2",	"HDA_SDO",	"I2S0_TXD",		"n/a",			"HDAPROC_SDO",
	"GPP_R3",	"HDA_SDI0",	"I2S0_RXD",		"n/a",			"HDAPROC_SDI",
	"GPP_R4",	"HDA_RST#",	"I2S2_SCLK",	"DMIC_CLK_A0",	"n/a",
	"GPP_R5",	"HDA_SDI1",	"I2S2_SFRM",	"DMIC_DATA0",	"n/a",
	"GPP_R6",	"n/a",		"I2S2_TXD",		"DMIC_CLK_A1",	"n/a",
	"GPP_R7",	"n/a",		"I2S2_RXD",		"DMIC_DATA1",	"n/a",
};

const struct gpio_group meteorlake_pch_group_r = {
	.display	= "------- GPIO Group GPP_R -------",
	.pad_count	= ARRAY_SIZE(meteorlake_pch_group_r_names) / 5,
	.func_count	= 5,
	.pad_names	= meteorlake_pch_group_r_names,
};


const char *const meteorlake_pch_group_s_names[] = {
	"GPP_S0",	"SNDW0_CLK",	"n/a",			"n/a",	"I2S1_SCLK",
	"GPP_S1",	"SNDW0_DATA",	"n/a",			"n/a",	"I2S1_SFRM",
	"GPP_S2",	"SNDW1_CLK",	"DMIC_CKL_A0",	"n/a",	"I2S1_TXD",
	"GPP_S3",	"SNDW1_DATA",	"DMIC_DATA0",	"n/a",	"I2S1_RXD",
	"GPP_S4",	"SNDW2_CLK",	"DMIC_CLK_B0",	"n/a",	"n/a",
	"GPP_S5",	"SNDW2_DATA",	"DMIC_CLK_B1",	"n/a",	"n/a",
	"GPP_S6",	"SNDW3_CLK",	"DMIC_CLK_A1",	"n/a",	"n/a",
	"GPP_S7",	"SNDW3_DATA",	"DMIC_DATA1",	"n/a",	"n/a",
}

const struct gpio_group meteorlake_pch_group_s = {
	.display	= "------- GPIO Group GPP_S -------",
	.pad_count	= ARRAY_SIZE(meteorlake_pch_group_s_names) / 5,
	.func_count	= 5,
	.pad_names	= meteorlake_pch_group_s_names,
};


const char *const meteorlake_pch_group_t_names[] = {
	"GPP_T2",	"n/a",	"Reserved",
	"GPP_T3",	"n/a",	"Reserved",
};

const struct gpio_group meteorlake_pch_group_t = {
	.display	= "------- GPIO Group GPP_T -------",
	.pad_count	= ARRAY_SIZE(meteorlake_pch_group_t_names) / 3,
	.func_count	= 3,
	.pad_names	= meteorlake_pch_group_t_names,
};


const char *const meteorlake_pch_group_gpd_names[] = {
	"GPD0",		"BATLOW#",
	"GPD1",		"ACPRESENT",
	"GPD2",		"LAN_WAKE#",
	"GPD3",		"PWRBTN#",
	"GPD4",		"SLP_S3#",
	"GPD5",		"SLP_S4#",
	"GPD6",		"SLP_A#",
	"GPD7",		"n/a",
	"GPD8",		"SUSCLK",
	"GPD9",		"SLP_WLAN#",
	"GPD10",	"SLP_S5#",
	"GPD11",	"LANPHYPC",
}

const struct gpio_group meteorlake_pch_group_gpd = {
	.display	= "------- GPIO Group GPP_GPD -------",
	.pad_count	= ARRAY_SIZE(meteorlake_pch_group_gpd_names) / 2,
	.func_count	= 2,
	.pad_names	= meteorlake_pch_group_gpd_names,
};


/*
    COMMUNITIES

    community 0: 8h - 944h
        GPP_V
        GPP_C

    community 1: 8h - 8f4h
        GPP_A
        GPP_E

    community 2: ????

    community 3: 8h - 914h
        GPP_H
        GPP_F
        + gpio expander capability

    community 4: 8h - 674h
        GPP_S

    community 5: 8h - 904h
        GPP_B
        GPP_D
        + blink, serial blink, and PWM control
*/

const struct gpio_group *const meteorlake_pch_community_0_groups[] = {
	&meteorlake_pch_group_v,
	&meteorlake_pch_group_c,
};

const struct gpio_community meteorlake_pch_community_0 = {
	.name		= "------- GPIO Community 0 -------",
	.pcr_port_id	= 0x6e,
	.group_count	= ARRAY_SIZE(meteorlake_pch_community_0_groups),
	.groups		= meteorlake_pch_community_0_groups,
};


const struct gpio_group *const meteorlake_pch_community_1_groups[] = {
	&meteorlake_pch_group_a,
	&meteorlake_pch_group_e,
};

const struct gpio_community meteorlake_pch_community_1 = {
	.name		= "------- GPIO Community 1 -------",
	.pcr_port_id	= 0x6e,
	.group_count	= ARRAY_SIZE(meteorlake_pch_community_1_groups),
	.groups		= meteorlake_pch_community_1_groups,
};


const struct gpio_group *const meteorlake_pch_community_3_groups[] = {
	&meteorlake_pch_group_h,
	&meteorlake_pch_group_f,
    // and gpio expander?
};

const struct gpio_community meteorlake_pch_community_3 = {
	.name		= "------- GPIO Community 3 -------",
	.pcr_port_id	= 0x6e,
	.group_count	= ARRAY_SIZE(meteorlake_pch_community_3_groups),
	.groups		= meteorlake_pch_community_3_groups,
};


const struct gpio_group *const meteorlake_pch_community_4_groups[] = {
	&meteorlake_pch_group_s,
};

const struct gpio_community meteorlake_pch_community_4 = {
	.name		= "------- GPIO Community 4 -------",
	.pcr_port_id	= 0x6e,
	.group_count	= ARRAY_SIZE(meteorlake_pch_community_4_groups),
	.groups		= meteorlake_pch_community_4_groups,
};


const struct gpio_group *const meteorlake_pch_community_5_groups[] = {
	&meteorlake_pch_group_b,
    &meteorlake_pch_group_d,
    // and blink, serial blink and pwm?
};

const struct gpio_community meteorlake_pch_community_5 = {
	.name		= "------- GPIO Community 5 -------",
	.pcr_port_id	= 0x6e,
	.group_count	= ARRAY_SIZE(meteorlake_pch_community_5_groups),
	.groups		= meteorlake_pch_community_5_groups,
};

const struct gpio_community *const meteorlake_pch_communities[] = {
	&meteorlake_pch_community_0,
	&meteorlake_pch_community_1,
	&meteorlake_pch_community_3,
	&meteorlake_pch_community_4,
	&meteorlake_pch_community_5,
};


/*
"n/a",                 "Deglitch Legend",        "n/a",             "n/a",                "n/a", "n/a",  "n/a",
"n/a",                 "n/a"yes(1) - o/p Hi-Z",  "n/a",             "n/a",                "n/a", "n/a",  "n/a",
"n/a",                 "n/a"yes(2) - o/p Hi-Z",  "n/a",             "n/a",                "n/a", "n/a",  "n/a",
"n/a",                 "n/a",                    "n/a",             "n/a",                "n/a", "n/a",  "n/a",
" UART",               "n/a",                    "n/a",             "n/a",                "n/a", "n/a",  "n/a",
*/
