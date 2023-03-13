#ifndef GPIO_NAMES_ELKHARTLAKE
#define GPIO_NAMES_ELKHARTLAKE

#include "gpio_groups.h"

/* ----------------------------- Elkhart Lake ----------------------------- */

const char *const elkhartlake_pch_group_a_names[] = {
"GPP_A00",  "ESPI_IO_0",    "n/a",
"GPP_A01",  "ESPI_IO_1",    "n/a",
"GPP_A02",  "ESPI_IO_2",    "n/a",
"GPP_A03",  "ESPI_IO_3",    "n/a",
"GPP_A04",  "ESPI_CS_N",    "n/a",
"GPP_A05",  "ESPI_CLK",     "n/a",
"GPP_A06",  "ESPI_RESET_N", "n/a",
"GPP_A07",  "SMB_CLK",      "n/a",
"GPP_A08",  "SMB_DATA",     "n/a",
"GPP_A09",  "SMB_ALERT_N",  "n/a",
"GPP_A10",  "CPU_GP_0",     "n/a",
"GPP_A11",  "CPU_GP_1",     "n/a",
"GPP_A12",  "USB_OC1_N",    "n/a",
"GPP_A13",  "USB_OC2_N",    "n/a",
"GPP_A14",  "USB_OC3_N",    "n/a",
"GPP_A15",  "n/a",          "n/a",
"GPP_A16",  "DDI1_HPD",     "TIME_SYNC_1",
"GPP_A17",  "DDI0_HPD",     "n/a",
"GPP_A18",  "USB_OC0_N",    "n/a",
"GPP_A19",  "PCHHOT_N",     "n/a",
};

const struct gpio_group elkhartlake_pch_group_a = {
	.display	= "------- GPIO Group GPP_A -------",
	.pad_count	= ARRAY_SIZE(elkhartlake_pch_group_a_names) / 3,
	.func_count	= 3,
	.pad_names	= elkhartlake_pch_group_a_names,
};

const char *const elkhartlake_pch_group_b_names[] = {
"GP_B00", "PMC_CORE_VID0",  "n/a",         "n/a",
"GP_B01", "PMC_CORE_VID1",  "n/a",         "n/a",
"GP_B02", "PMC_VRALERT_N",  "n/a",         "n/a",
"GP_B03", "CPU_GP_2",       "n/a",         "n/a",
"GP_B04", "CPU_GP_3",       "n/a",         "n/a",
"GP_B05", "PCIE_CLKREQ0_N", "n/a",         "n/a",
"GP_B06", "PCIE_CLKREQ1_N", "n/a",         "n/a",
"GP_B07", "PCIE_CLKREQ2_N", "n/a",         "n/a",
"GP_B08", "PCIE_CLKREQ3_N", "n/a",         "n/a",
"GP_B09", "PCIE_CLKREQ4_N", "n/a",         "n/a",
"GP_B10", "PCIE_CLKREQ5_N", "n/a",         "n/a",
"GP_B11", "PMCALERT_N",     "n/a",         "n/a",
"GP_B12", "PMC_SLP_S0_N",   "n/a",         "n/a",
"GP_B13", "PMC_PLTRST_N",   "n/a",         "n/a",
"GP_B14", "SPKR",           "GSPI0_CS1_N", "n/a",
"GP_B15", "GSPI0_CS0_N",    "n/a",         "n/a",
"GP_B16", "GSPI0_CLK",      "n/a",         "n/a",
"GP_B17", "GSPI0_MISO",     "UART2A_RXD",  "n/a",
"GP_B18", "GSPI0_MOSI",     "UART2A_TXD",  "n/a",
"GP_B19", "GSPI1_CS0_N",    "n/a",         "n/a",
"GP_B20", "GSPI1_CLK",      "NFC_CLK",     "n/a",
"GP_B21", "GSPI1_MISO",     "NFC_CLKREQ",  "n/a",
"GP_B22", "GSPI1_MOSI",     "n/a",         "n/a",
"GP_B23", "DDI2_HPD",       "TIME_SYNC_0", "GSPI1_CS1_N",

};

const struct gpio_group elkhartlake_pch_group_b = {
	.display	= "------- GPIO Group GPP_B -------",
	.pad_count	= ARRAY_SIZE(elkhartlake_pch_group_b_names) / 9,
	.func_count	= 9,
	.pad_names	= elkhartlake_pch_group_b_names,
};

const char *const elkhartlake_pch_group_c_names[] = {
"GP_C00", "n/a",         "n/a",                           "n/a",
"GP_C01", "n/a",         "n/a",                           "n/a",
"GP_C02", "n/a",         "n/a",                           "n/a",
"GP_C03", "n/a",         "n/a",                           "n/a",
"GP_C04", "n/a",         "n/a",                           "n/a",
"GP_C05", "n/a",         "n/a",                           "n/a",
"GP_C06", "n/a",         "PMC_SUSWARN_N_PMC_SUSPWRDNACK", "n/a",
"GP_C07", "n/a",         "PMC_SUSACK_N",                  "n/a",
"GP_C08", "UART0_RXD",   "n/a",                           "n/a",
"GP_C09", "UART0_TXD",   "n/a",                           "n/a",
"GP_C10", "UART0_RTS_N", "n/a",                           "n/a",
"GP_C11", "UART0_CTS_N", "n/a",                           "n/a",
"GP_C12", "UART1_RXD",   "n/a",                           "n/a",
"GP_C13", "UART1_TXD",   "n/a",                           "n/a",
"GP_C14", "UART1_RTS_N", "n/a",                           "n/a",
"GP_C15", "UART1_CTS_N", "n/a",                           "n/a",
"GP_C16", "I2C0_SDA",    "n/a",                           "n/a",
"GP_C17", "I2C0_SCL",    "n/a",                           "n/a",
"GP_C18", "I2C1_SDA",    "n/a",                           "n/a",
"GP_C19", "I2C1_SCL",    "n/a",                           "n/a",
"GP_C20", "UART2_RXD",   "n/a",                           "CNV_MFUART0_RXD",
"GP_C21", "UART2_TXD",   "n/a",                           "CNV_MFUART0_TXD",
"GP_C22", "UART2_RTS_N", "n/a",                           "CNV_MFUART0_RTS_N",
"GP_C23", "UART2_CTS_N", "n/a",                           "CNV_MFUART0_CTS_N",

};

const struct gpio_group elkhartlake_pch_group_c = {
	.display	= "------- GPIO Group GPP_C -------",
	.pad_count	= ARRAY_SIZE(elkhartlake_pch_group_c_names) / 9,
	.func_count	= 9,
	.pad_names	= elkhartlake_pch_group_c_names,
};

const char *const elkhartlake_pch_group_d_names[] = {
"GP_D00", "n/a",             "BK_0",             "SBK_0",        "n/a",
"GP_D01", "n/a",             "BK_1",             "SBK_1",        "n/a",
"GP_D02", "n/a",             "BK_2",             "SBK_2",        "n/a",
"GP_D03", "n/a",             "BK_3",             "SBK_3",        "n/a",
"GP_D04", "n/a",             "n/a",              "n/a",          "n/a",
"GP_D05", "n/a",             "n/a",              "n/a",          "n/a",
"GP_D06", "n/a",             "n/a",              "n/a",          "n/a",
"GP_D07", "n/a",             "n/a",              "n/a",          "n/a",
"GP_D08", "n/a",             "GSPI2_SPI2_CS0_N", "UART0A_RXD",   "n/a",
"GP_D09", "n/a",             "GSPI2_CLK",        "UART0A_TXD",   "n/a",
"GP_D10", "n/a",             "GSPI2_MISO",       "UART0A_RTS_N", "n/a",
"GP_D11", "n/a",             "GSPI2_MOSI",       "UART0A_CTS_N", "n/a",
"GP_D12", "n/a",             "n/a",              "I2C4B_SDA",    "n/a",
"GP_D13", "n/a",             "n/a",              "I2C4B_SCL",    "n/a",
"GP_D14", "n/a",             "GSPI2_CS1_N",      "n/a",          "n/a",
"GP_D15", "n/a",             "n/a",              "n/a",          "CNV_WCEN", 
"GP_D16", "n/a",             "n/a",              "n/a",          "n/a",
"GP_D17", "n/a",             "n/a",              "n/a",          "n/a",
"GP_D18", "AVS_I2S_MCLK",    "n/a",              "n/a",          "n/a",
"GP_D19", "CNV_MFUART2_RXD", "n/a",              "n/a",          "n/a",
"GP_D20", "CNV_MFUART2_TXD", "n/a",              "n/a",          "n/a",
"GP_D21", "CNV_PA_BLANKING", "n/a",              "n/a",          "n/a",
"GP_D22", "I2C5_SDA",        "n/a",              "n/a",          "n/a",
"GP_D23", "I2C5_SCL",        "n/a",              "n/a",          "n/a",
};

const struct gpio_group elkhartlake_pch_group_d = {
	.display	= "------- GPIO Group GPP_D -------",
	.pad_count	= ARRAY_SIZE(elkhartlake_pch_group_d_names) / 9,
	.func_count	= 9,
	.pad_names	= elkhartlake_pch_group_d_names,
};

const char *const elkhartlake_pch_group_gpd_names[] = {
	"GPD_00",	"PMC_BATLOW_N",
	"GPD_01",	"PMC_ACPRESENT",
	"GPD_02",	"RSVD",
	"GPD_03",	"PMC_PWRBTN_N",
	"GPD_04",	"PMC_SLP_S3_N",
	"GPD_05",	"PMC_SLP_S4_N",
	"GPD_07",	"RSVD",
	"GPD_08",	"PMC_SUSCLK",
	"GPD_09",	"RSVD",
	"GPD_10",	"PMC_SLP_S5_N",
	"GPD_11",	"RSVD",
	"GPIO_RSVD_8",	"n/a",
	"GPIO_RSVD_9",	"n/a",
	"GPIO_RSVD_10",	"n/a",
	"GPIO_RSVD_11",	"n/a",
	"GPIO_RSVD_12",	"n/a",
};

const struct gpio_group elkhartlake_pch_group_gpd = {
	.display	= "------- GPIO Group GPD -------",
	.pad_count	= ARRAY_SIZE(elkhartlake_pch_group_gpd_names) / 2,
	.func_count	= 2,
	.pad_names	= elkhartlake_pch_group_gpd_names,
};

const char *const elkhartlake_pch_group_e_names[] = {
	"GPP_E00",	"SATA_LED_N",		"SATAXPCIE_0",		"RSVD",	"RSVD",	"RSVD",			"RSVD",	"SATA_0_GP",	"n/a",
	"GPP_E01",	"n/a",			"RSVD",			"RSVD",	"RSVD",	"RSVD",			"RSVD",	"RSVD",		"n/a",
	"GPP_E02",	"n/a",			"RSVD",			"RSVD",	"RSVD",	"RSVD",			"RSVD",	"RSVD",		"n/a",
	"GPP_E03",	"DDI1_HPD",		"PNL_MISC_DDI1",	"RSVD",	"RSVD",	"RSVD",			"RSVD",	"CPU_GP0",	"PSE_TGPIO15",
	"GPP_E04",	"SATA_0_DEVSLP",	"n/a",			"RSVD",	"RSVD",	"RSVD",			"RSVD",	"PSE_PWM08",	"PSE_TGPIO44",
	"GPP_E05",	"DDI1_DDC_SDA",		"n/a",			"RSVD",	"RSVD",	"RSVD",			"RSVD",	"PSE_PWM09",	"PSE_TGPIO17",
	"GPP_E06",	"n/a",			"RSVD",			"RSVD",	"RSVD",	"RSVD",			"RSVD",	"PSE_PWM10",	"PSE_TGPIO18",
	"GPP_E07",	"DDI1_DDC_SCL",		"n/a",			"RSVD",	"RSVD",	"RSVD",			"RSVD",	"CPU_GP1",	"PSE_TGPIO16",
	"GPP_E08",	"n/a",			"SATA_1_DEVSLP",	"RSVD",	"RSVD",	"n/a",			"RSVD",	"RSVD",		"PSE_TGPIO45",
	"GPP_E09",	"USB2_OC0_N",		"n/a",			"RSVD",	"RSVD",	"n/a",			"RSVD",	"n/a",		"n/a",
	"GPP_E10",	"n/a",			"RSVD",			"RSVD",	"RSVD",	"n/a",			"RSVD",	"RSVD"	,	"n/a",
	"GPP_E11",	"n/a",			"RSVD",			"RSVD",	"RSVD",	"n/a",			"RSVD",	"RSVD",		"n/a",
	"GPP_E12",	"n/a",			"RSVD",			"RSVD",	"RSVD",	"n/a",			"RSVD",	"RSVD",		"n/a",
	"GPP_E13",	"n/a",			"RSVD",			"RSVD",	"RSVD",	"n/a",			"RSVD",	"RSVD",		"n/a",
	"GPP_E14",	"DDI0_HPD",		"PNL_MISC_DDI0",	"RSVD",	"RSVD",	"n/a",			"RSVD",	"n/a",		"PSE_TGPIO19",
	"GPP_E15",	"PSE_I2S0_RXD",		"RSVD",			"RSVD",	"RSVD",	"n/a",			"RSVD",	"PSE_CAN0_TX",	"PSE_TGPIO17",
	"GPP_E16",	"PSE_I2S0_TXD",		"RSVD",			"RSVD",	"RSVD",	"RSVD",			"RSVD",	"PSE_CAN0_RX",	"PSE_TGPIO16",
	"GPP_E17",	"PNL1_VDDEN",		"RSVD",			"RSVD",	"RSVD",	"PNL_MISC_DDI2",	"RSVD",	"PSE_PWM11",	"PSE_TGPIO46",
	"GPP_E18",	"DDI0_DDC_SDA",		"n/a",			"n/a",	"n/a",	"RSVD",			"n/a",	"PSE_PWM12",	"PSE_TGPIO23",
	"GPP_E19",	"DDI0_DDC_SCL",		"n/a",			"n/a",	"n/a",	"RSVD",			"n/a",	"PSE_PWM13",	"PSE_TGPIO24",
	"GPP_E20",	"PSE_I2S0_SCLK",	"n/a",			"n/a",	"n/a",	"n/a",			"n/a",	"PSE_CAN1_TX",	"PSE_TGPIO14",
	"GPP_E21",	"PSE_I2S0_SFRM",	"n/a",			"n/a",	"n/a",	"n/a",			"n/a",	"PSE_CAN1_RX",	"PSE_TGPIO15",
	"GPP_E22",	"PNL1_BKLTCTL",		"n/a",			"n/a",	"n/a",	"n/a",			"n/a",	"PSE_PWM14",	"PSE_TGPIO18",
	"GPP_E23",	"PNL1_BKLTEN",		"n/a",			"n/a",	"n/a",	"n/a",			"n/a",	"PSE_PWM15",	"PSE_TGPIO19",
};

const struct gpio_group elkhartlake_pch_group_e = {
	.display	= "------- GPIO Group GPP_E -------",
	.pad_count	= ARRAY_SIZE(elkhartlake_pch_group_e_names) / 9,
	.func_count	= 9,
	.pad_names	= elkhartlake_pch_group_e_names,
};

const char *const elkhartlake_pch_group_f_names[] = {
	"GPP_F00",	"RSVD",			"SIO_UART0_RTS_N",	"n/a",	"n/a",			"n/a",			"n/a",	"n/a",	"n/a",
	"GPP_F01",	"RSVD",			"SIO_UART0_RXD",	"n/a",	"n/a",			"n/a",			"n/a",	"n/a",	"n/a",
	"GPP_F02",	"RSVD",			"SIO_UART0_TXD",	"n/a",	"n/a",			"n/a",			"n/a",	"n/a",	"n/a",
	"GPP_F03",	"RSVD",			"SIO_UART0_CTS_N",	"n/a",	"n/a",			"n/a",			"n/a",	"n/a",	"n/a",
	"GPP_F04",	"RSVD",			"n/a",			"n/a",	"n/a",			"n/a",			"n/a",	"n/a",	"n/a",
	"GPP_F05",	"RSVD",			"RSVD",			"RSVD",	"RSVD",			"n/a",			"n/a",	"n/a",	"PSE_TGPIO14",
	"GPP_F06",	"RSVD",			"n/a",			"n/a",	"n/a",			"n/a",			"n/a",	"n/a",	"PSE_TGPIO47",
	"GPP_F07",	"PSE_I2S1_SCLK",	"n/a",			"n/a",	"n/a",			"AVS_I2S4_SCLK",	"n/a",	"n/a",	"PSE_TGPIO14",
	"GPP_F08",	"AVS_I2S_MCLK2",	"n/a",			"n/a",	"PSE_TRACEDATA_0",	"ISI_TRACEDATA_0",	"n/a",	"n/a",	"PSE_TGPIO48",
	"GPP_F09",	"BOOT_PWR_EN",		"n/a",			"n/a",	"n/a",			"n/a",			"n/a",	"n/a",	"n/a",
	"GPP_F10",	"PSE_I2S1_SFRM",	"n/a",			"n/a",	"n/a",			"AVS_I2S4_SFRM",	"n/a",	"n/a",	"PSE_TGPIO15",
	"GPP_F11",	"n/a",			"n/a",			"RSVD",	"PSE_TRACECLK",		"ISI_TRACECLK",		"n/a",	"n/a",	"PSE_TGPIO49",
	"GPP_F12",	"RSVD",			"AVS_I2S4_TXD",		"RSVD",	"PSE_TRACESWO",		"ISI_TRACESWO",		"n/a",	"n/a",	"n/a",
	"GPP_F13",	"RSVD",			"AVS_I2S4_SFRM",	"RSVD",	"PSE_SWDIO",		"ISI_SWDIO",		"n/a",	"n/a",	"n/a",
	"GPP_F14",	"RSVD",			"AVS_I2S4_RXD",		"RSVD",	"PSE_TRACEDATA_1",	"ISI_TRACEDATA_1",	"n/a",	"n/a",	"n/a",
	"GPP_F15",	"RSVD",			"n/a",			"RSVD",	"PSE_TRACEDATA_2",	"ISI_TRACEDATA_2",	"n/a",	"n/a",	"n/a",
	"GPP_F16",	"RSVD",			"AVS_I2S4_SCLK",	"RSVD",	"PSE_SWCLK",		"ISI_SWCLK",		"n/a",	"n/a",	"n/a",
	"GPP_F17",	"n/a",			"n/a",			"RSVD",	"PSE_TRACEDATA_3",	"ISI_TRACEDATA_3",	"n/a",	"n/a",	"PSE_TGPIO50",
	"GPP_F18",	"PSE_I2S1_TXD",		"n/a",			"n/a",	"n/a",			"AVS_I2S4_TXD",		"n/a",	"n/a",	"PSE_TGPIO16",
	"GPP_F19",	"PSE_I2S1_RXD",		"n/a",			"n/a",	"n/a",			"AVS_I2S4_RXD",		"n/a",	"n/a",	"PSE_TGPIO17",
	"GPP_F20",	"RSVD",			"n/a",			"n/a",	"n/a",			"n/a",			"n/a",	"n/a",	"n/a",
	"GPP_F21",	"RSVD",			"n/a",			"n/a",	"n/a",			"n/a",			"n/a",	"n/a",	"n/a",
	"GPP_F22",	"PMC_VNN_CTRL",		"n/a",			"n/a",	"n/a",			"n/a",			"n/a",	"n/a",	"n/a",
	"GPP_F23",	"PMC_V1P05_CTRL",	"n/a",			"n/a",	"n/a",			"n/a",			"n/a",	"n/a",	"n/a",
	"GPIO_RSVD_30",	"n/a",			"n/a",			"n/a",	"n/a",			"n/a",			"n/a",	"n/a",	"n/a",
	"GPIO_RSVD_31",	"n/a",			"n/a",			"n/a",	"n/a",			"n/a",			"n/a",	"n/a",	"n/a",
	"GPIO_RSVD_32",	"n/a",			"n/a",			"n/a",	"n/a",			"n/a",			"n/a",	"n/a",	"n/a",
	"GPIO_RSVD_33",	"n/a",			"n/a",			"n/a",	"n/a",			"n/a",			"n/a",	"n/a",	"n/a",
	"GPIO_RSVD_34",	"n/a",			"n/a",			"n/a",	"n/a",			"n/a",			"n/a",	"n/a",	"n/a",
	"GPIO_RSVD_35",	"n/a",			"n/a",			"n/a",	"n/a",			"n/a",			"n/a",	"n/a",	"n/a",
	"GPIO_RSVD_36",	"n/a",			"n/a",			"n/a",	"n/a",			"n/a",			"n/a",	"n/a",	"n/a",
};

const struct gpio_group elkhartlake_pch_group_f = {
	.display	= "------- GPIO Group GPP_F -------",
	.pad_count	= ARRAY_SIZE(elkhartlake_pch_group_f_names) / 9,
	.func_count	= 9,
	.pad_names	= elkhartlake_pch_group_f_names,
};

const char *const elkhartlake_pch_group_g_names[] = {
	"GPP_G00",	"SD_SDIO_CMD",		"n/a",		"n/a",		"n/a",		"n/a",	"n/a",	"n/a",	"n/a",
	"GPP_G01",	"SD_SDIO_D0",		"n/a",		"n/a",		"n/a",		"n/a",	"n/a",	"n/a",	"n/a",
	"GPP_G02",	"SD_SDIO_D1",		"n/a",		"n/a",		"n/a",		"n/a",	"n/a",	"n/a",	"n/a",
	"GPP_G03",	"SD_SDIO_D2",		"n/a",		"n/a",		"n/a",		"n/a",	"n/a",	"n/a",	"n/a",
	"GPP_G04",	"SD_SDIO_D3",		"n/a",		"n/a",		"n/a",		"n/a",	"n/a",	"n/a",	"n/a",
	"GPP_G05",	"SD_SDIO_CD_N",		"n/a",		"n/a",		"n/a",		"n/a",	"n/a",	"n/a",	"n/a",
	"GPP_G06",	"SD_SDIO_CLK",		"n/a",		"n/a",		"n/a",		"n/a",	"n/a",	"n/a",	"n/a",
	"GPP_G07",	"AVS_I2S2_SCLK",	"n/a",		"n/a",		"DMIC_CLK_A0",	"n/a",	"n/a",	"n/a",	"n/a",
	"GPP_G08",	"AVS_I2S2_SFRM",	"RSVD",		"n/a",		"DMIC_DATA0",	"n/a",	"n/a",	"n/a",	"n/a",
	"GPP_G09",	"AVS_I2S2_TXD",		"RSVD",		"RSVD",		"DMIC_CLK_A1",	"n/a",	"n/a",	"n/a",	"n/a",
	"GPP_G10",	"AVS_I2S2_RXD",		"n/a",		"n/a",		"DMIC_DATA1",	"n/a",	"n/a",	"n/a",	"n/a",
	"GPP_G11",	"AVS_I2S3_SCLK",	"n/a",		"n/a",		"DMIC_DATA0",	"n/a",	"n/a",	"n/a",	"PSE_TGPIO07",
	"GPP_G12",	"AVS_I2S3_SFRM",	"SATA_1_GP",	"SATAXPCIE_1",	"DMIC_DATA1",	"n/a",	"n/a",	"n/a",	"PSE_TGPIO31",
	"GPP_G13",	"AVS_I2S3_TXD",		"n/a",		"n/a",		"DMIC_CLK_B0",	"n/a",	"n/a",	"n/a",	"PSE_TGPIO08",
	"GPP_G14",	"AVS_I2S3_RXD",		"n/a",		"n/a",		"DMIC_CLK_B1",	"n/a",	"n/a",	"n/a",	"PSE_TGPIO09",
	"GPP_G15",	"ESPI_IO0",		"n/a",		"n/a",		"n/a",		"n/a",	"n/a",	"n/a",	"n/a",
	"GPP_G16",	"ESPI_IO1",		"n/a",		"n/a",		"n/a",		"n/a",	"n/a",	"n/a",	"n/a",
	"GPP_G17",	"ESPI_IO2",		"Reserved",	"n/a",		"n/a",		"n/a",	"n/a",	"n/a",	"n/a",
	"GPP_G18",	"ESPI_IO3",		"Reserved",	"n/a",		"n/a",		"n/a",	"n/a",	"n/a",	"n/a",
	"GPP_G19",	"AVS_I2S1_SCLK",	"n/a",		"n/a",		"n/a",		"n/a",	"n/a",	"n/a",	"n/a",
	"GPP_G20",	"ESPI_CS0_N",		"n/a",		"n/a",		"n/a",		"n/a",	"n/a",	"n/a",	"n/a",
	"GPP_G21",	"ESPI_CLK",		"n/a",		"n/a",		"n/a",		"n/a",	"n/a",	"n/a",	"n/a",
	"GPP_G22",	"ESPI_RST0_N",		"n/a",		"n/a",		"n/a",		"n/a",	"n/a",	"n/a",	"n/a",
	"GPP_G23",	"SD_SDIO_WP",		"n/a",		"n/a",		"n/a",		"n/a",	"n/a",	"n/a",	"n/a",
	"GPIO_RSVD_2",	"n/a",			"n/a",		"n/a",		"n/a",		"n/a",	"n/a",	"n/a",	"n/a",
};

const struct gpio_group elkhartlake_pch_group_g = {
	.display	= "------- GPIO Group GPP_G -------",
	.pad_count	= ARRAY_SIZE(elkhartlake_pch_group_g_names) / 9,
	.func_count	= 9,
	.pad_names	= elkhartlake_pch_group_g_names,
};

const char *const elkhartlake_pch_group_h_names[] = {
	"GPP_H00",	"PSE_GBE1_INT",		"n/a",			"n/a",	"PSE_UART5_RXD",	"n/a",	"n/a",	"n/a",	"n/a",
	"GPP_H01",	"PSE_GBE1_RST_N",	"n/a",			"n/a",	"PSE_UART5_TXD",	"n/a",	"n/a",	"n/a",	"n/a",
	"GPP_H02",	"PSE_GBE1_AUXTS",	"n/a",			"n/a",	"PSE_UART5_RTS_N",	"n/a",	"n/a",	"n/a",	"n/a",
	"GPP_H03",	"PSE_GBE1_PPS",		"n/a",			"n/a",	"PSE_UART5_CTS_N",	"n/a",	"n/a",	"n/a",	"PSE_TGPIO21",
	"GPP_H04",	"SIO_I2C2_SDA",		"n/a",			"n/a",	"PSE_PWM08",		"n/a",	"n/a",	"n/a",	"PSE_TGPIO10",
	"GPP_H05",	"SIO_I2C2_SCL",		"n/a",			"n/a",	"PSE_PWM09",		"n/a",	"n/a",	"n/a",	"PSE_TGPIO11",
	"GPP_H06",	"SIO_I2C3_SDA",		"PSE_I2C5_SDA",		"n/a",	"PSE_PWM10",		"n/a",	"n/a",	"n/a",	"n/a",
	"GPP_H07",	"SIO_I2C3_SCL",		"PSE_I2C5_SCL",		"n/a",	"PSE_PWM11",		"n/a",	"n/a",	"n/a",	"n/a",
	"GPP_H08",	"SIO_I2C4_SDA",		"RSVD",			"n/a",	"PSE_PWM12",		"n/a",	"n/a",	"n/a",	"n/a",
	"GPP_H09",	"SIO_I2C4_SCL",		"RSVD",			"n/a",	"PSE_PWM13",		"n/a",	"n/a",	"n/a",	"n/a",
	"GPP_H10",	"PCIE_CLKREQ4_N",	"n/a",			"n/a",	"PSE_PWM14",		"n/a",	"n/a",	"n/a",	"n/a",
	"GPP_H11",	"PCIE_CLKREQ5_N",	"n/a",			"n/a",	"PSE_PWM15",		"n/a",	"n/a",	"n/a",	"n/a",
	"GPP_H12",	"PSE_UART1_RXD",	"n/a",			"n/a",	"M2_SKT2_CFG0",		"n/a",	"n/a",	"n/a",	"PSE_TGPIO51",
	"GPP_H13",	"PSE_UART1_TXD",	"n/a",			"n/a",	"M2_SKT2_CFG1",		"n/a",	"n/a",	"n/a",	"PSE_TGPIO52",
	"GPP_H14",	"n/a",			"n/a",			"n/a",	"M2_SKT2_CFG2",		"n/a",	"n/a",	"n/a",	"PSE_TGPIO53",
	"GPP_H15",	"PSE_UART1_CTS_N",	"n/a",			"n/a",	"M2_SKT2_CFG3",		"n/a",	"n/a",	"n/a",	"PSE_TGPIO54",
	"GPP_H16",	"PCIE_LNK_DOWN",	"DDI2_DDC_SCL",		"n/a",	"n/a",			"n/a",	"n/a",	"n/a",	"n/a",
	"GPP_H17",	"SD_SDIO_PWR_EN_N",	"n/a",			"n/a",	"n/a",			"n/a",	"n/a",	"n/a",	"n/a",
	"GPP_H18",	"PMC_CPU_C10_GATE_N",	"n/a",			"n/a",	"n/a",			"n/a",	"n/a",	"n/a",	"n/a",
	"GPP_H19",	"n/a",			"DDI2_DDC_SDA",		"n/a",	"PMC_TGPIO0",		"n/a",	"n/a",	"n/a",	"PSE_TGPIO20",
	"GPP_H20",	"PSE_PWM07",		"DDI2_HPD",		"n/a",	"RSVD",			"n/a",	"n/a",	"n/a",	"PSE_TGPIO55",
	"GPP_H21",	"PSE_HSUART1_DE",	"PSE_UART1_RTS_N",	"n/a",	"RSVD",			"n/a",	"n/a",	"n/a",	"PSE_TGPIO56",
	"GPP_H22",	"PSE_HSUART1_RE",	"n/a",			"n/a",	"RSVD",			"n/a",	"n/a",	"n/a",	"PSE_TGPIO57",
	"GPP_H23",	"PSE_HSUART1_EN",	"n/a",			"n/a",	"RSVD",			"n/a",	"n/a",	"n/a",	"PSE_TGPIO58",
};

const struct gpio_group elkhartlake_pch_group_h = {
	.display	= "------- GPIO Group GPP_H -------",
	.pad_count	= ARRAY_SIZE(elkhartlake_pch_group_h_names) / 9,
	.func_count	= 9,
	.pad_names	= elkhartlake_pch_group_h_names,
};

const char *const elkhartlake_pch_group_r_names[] = {
	"GPP_R00",	"HDA_BCLK",	"AVS_I2S0_SCLK",	"PSE_I2S0_SCLK",	"RSVD",	"n/a",
	"GPP_R01",	"HDA_SYNC",	"AVS_I2S0_SFRM",	"PSE_I2S0_SFRM",	"n/a",	"n/a",
	"GPP_R02",	"HDA_SDO",	"AVS_I2S0_TXD",		"PSE_I2S0_TXD",		"RSVD",	"DMIC_CLK_B0",
	"GPP_R03",	"HDA_SDI0",	"AVS_I2S0_RXD",		"PSE_I2S0_RXD",		"RSVD",	"DMIC_CLK_B1",
	"GPP_R04",	"HDA_RST_N",	"n/a",			"n/a",			"n/a",	"DMIC_CLK_A1",
	"GPP_R05",	"HDA_SDI1",	"AVS_I2S1_RXD",		"n/a",			"n/a",	"DMIC_DATA1",
	"GPP_R06",	"n/a",		"AVS_I2S1_TXD",		"n/a",			"n/a",	"DMIC_CLK_A0",
	"GPP_R07",	"n/a",		"AVS_I2S1_SFRM",	"n/a",			"n/a",	"DMIC_DATA0",
};

const struct gpio_group elkhartlake_pch_group_r = {
	.display	= "------- GPIO Group GPP_R -------",
	.pad_count	= ARRAY_SIZE(elkhartlake_pch_group_r_names) / 6,
	.func_count	= 6,
	.pad_names	= elkhartlake_pch_group_r_names,
};

const char *const elkhartlake_pch_group_s_names[] = {
	"GPIO_RSVD_13",	"n/a",
	"GPIO_RSVD_14",	"n/a",
	"GPIO_RSVD_15",	"n/a",
	"GPIO_RSVD_16",	"n/a",
	"GPIO_RSVD_17",	"n/a",
	"GPIO_RSVD_18",	"n/a",
	"GPIO_RSVD_19",	"n/a",
	"GPIO_RSVD_20",	"n/a",
	"GPIO_RSVD_21",	"n/a",
	"GPIO_RSVD_22",	"n/a",
	"GPIO_RSVD_23",	"n/a",
	"GPIO_RSVD_24",	"n/a",
	"GPIO_RSVD_25",	"n/a",
	"GPIO_RSVD_26",	"n/a",
	"GPIO_RSVD_27",	"n/a",
	"GPIO_RSVD_28",	"n/a",
	"GPIO_RSVD_29",	"n/a",
	"GPP_S00",	"n/a",
	"GPP_S01",	"n/a",
};

const struct gpio_group elkhartlake_pch_group_s = {
	.display	= "------- GPIO Group GPP_S -------",
	.pad_count	= ARRAY_SIZE(elkhartlake_pch_group_s_names) / 2,
	.func_count	= 2,
	.pad_names	= elkhartlake_pch_group_s_names,
};

const char *const elkhartlake_pch_group_t_names[] = {
	"GPP_T00",	"PSE_QEPA2",		"n/a",			"SIO_I2C6_SDA",		"n/a",	"n/a",	"n/a",	"n/a",	"PSE_TGPIO08",
	"GPP_T01",	"PSE_QEPB2",		"n/a",			"SIO_I2C6_SCL",		"n/a",	"n/a",	"n/a",	"n/a",	"PSE_TGPIO09",
	"GPP_T02",	"PSE_QEPI2",		"n/a",			"SIO_I2C7_SDA",		"n/a",	"n/a",	"n/a",	"n/a",	"PSE_TGPIO07",
	"GPP_T03",	"n/a",			"n/a",			"SIO_I2C7_SCL",		"n/a",	"n/a",	"n/a",	"n/a",	"PSE_TGPIO06",
	"GPP_T04",	"PSE_GBE0_INT",		"n/a",			"n/a",			"RSVD",	"n/a",	"n/a",	"n/a",	"n/a",
	"GPP_T05",	"PSE_GBE0_RST_N",	"n/a",			"n/a",			"RSVD",	"n/a",	"n/a",	"n/a",	"n/a",
	"GPP_T06",	"PSE_GBE0_AUXTS",	"USB2_OC1_N",		"n/a",			"RSVD",	"n/a",	"n/a",	"n/a",	"n/a",
	"GPP_T07",	"PSE_GBE0_PPS",		"n/a",			"n/a",			"RSVD",	"n/a",	"n/a",	"n/a",	"PSE_TGPIO59",
	"GPP_T08",	"USB2_OC2_N",		"RSVD",			"n/a",			"n/a",	"n/a",	"n/a",	"n/a",	"PSE_TGPIO22",
	"GPP_T09",	"PSE_HSUART2_EN",	"RSVD",			"n/a",			"n/a",	"n/a",	"n/a",	"n/a",	"n/a",
	"GPP_T10",	"PSE_HSUART2_RE",	"RSVD",			"n/a",			"n/a",	"n/a",	"n/a",	"n/a",	"n/a",
	"GPP_T11",	"USB2_OC3_N",		"RSVD",			"n/a",			"n/a",	"n/a",	"n/a",	"n/a",	"PSE_TGPIO06",
	"GPP_T12",	"PSE_UART2_RXD",	"SIO_UART0_RXD",	"n/a",			"n/a",	"n/a",	"n/a",	"n/a",	"n/a",
	"GPP_T13",	"PSE_UART2_TXD",	"SIO_UART0_TXD",	"n/a",			"n/a",	"n/a",	"n/a",	"n/a",	"n/a",
	"GPP_T14",	"PSE_UART2_RTS_N",	"SIO_UART0_RTS_N",	"PSE_HSUART2_DE",	"n/a",	"n/a",	"n/a",	"n/a",	"n/a",
	"GPP_T15",	"PSE_UART2_CTS_N",	"SIO_UART0_CTS_N",	"n/a",			"n/a",	"n/a",	"n/a",	"n/a",	"n/a",
};

const struct gpio_group elkhartlake_pch_group_t = {
	.display	= "------- GPIO Group GPP_T -------",
	.pad_count	= ARRAY_SIZE(elkhartlake_pch_group_t_names) / 9,
	.func_count	= 9,
	.pad_names	= elkhartlake_pch_group_t_names,
};

const char *const elkhartlake_pch_group_u_names[] = {
	"GPP_U00",	"GBE_INT",		"PSE_I2C6_SCL",	"RSVD",	"n/a",			"n/a",	"n/a",	"n/a",	"n/a",
	"GPP_U01",	"GBE_RST_N",		"PSE_I2C6_SDA",	"RSVD",	"n/a",			"n/a",	"n/a",	"n/a",	"n/a",
	"GPP_U02",	"GBE_PPS",		"PSE_I2C7_SCL",	"RSVD",	"n/a",			"n/a",	"n/a",	"n/a",	"n/a",
	"GPP_U03",	"GBE_AUXTS",		"PSE_I2C7_SDA",	"RSVD",	"n/a",			"n/a",	"n/a",	"n/a",	"n/a",
	"GPP_U04",	"ISI_SPIM_CS",		"n/a",		"RSVD",	"PSE_SPI1_CS0_N",	"n/a",	"n/a",	"n/a",	"n/a",
	"GPP_U05",	"ISI_SPIM_SCLK",	"n/a",		"RSVD",	"PSE_SPI1_CLK",		"n/a",	"n/a",	"n/a",	"n/a",
	"GPP_U06",	"ISI_SPIM_MISO",	"n/a",		"RSVD",	"PSE_SPI1_MISO",	"n/a",	"n/a",	"n/a",	"n/a",
	"GPP_U07",	"PSE_QEPA3",		"n/a",		"RSVD",	"PSE_SPI1_MOSI",	"n/a",	"n/a",	"n/a",	"PSE_TGPIO10",
	"GPP_U08",	"ISI_SPIS_CS",		"n/a",		"RSVD",	"n/a",			"n/a",	"n/a",	"n/a",	"PSE_TGPIO10",
	"GPP_U09",	"ISI_SPIS_SCLK",	"ISI_I2CS_SCL",	"RSVD",	"n/a",			"n/a",	"n/a",	"n/a",	"PSE_TGPIO11",
	"GPP_U10",	"ISI_SPIS_MISO",	"ISI_I2CS_SDA",	"RSVD",	"n/a",			"n/a",	"n/a",	"n/a",	"PSE_TGPIO12",
	"GPP_U11",	"PSE_QEPB3",		"n/a",		"RSVD",	"n/a",			"n/a",	"n/a",	"n/a",	"PSE_TGPIO11",
	"GPP_U12",	"ISI_CHX_OKNOK_0",	"n/a",		"RSVD",	"n/a",			"n/a",	"n/a",	"n/a",	"n/a",
	"GPP_U13",	"ISI_CHX_OKNOK_1",	"n/a",		"RSVD",	"n/a",			"n/a",	"n/a",	"n/a",	"n/a",
	"GPP_U14",	"ISI_CHX_RLY_SWTCH",	"n/a",		"RSVD",	"n/a",			"n/a",	"n/a",	"n/a",	"n/a",
	"GPP_U15",	"ISI_CHX_PMIC_EN",	"n/a",		"RSVD",	"n/a",			"n/a",	"n/a",	"n/a",	"PSE_TGPIO13",
	"GPP_U16",	"ISI_OKNOK_0",		"n/a",		"RSVD",	"n/a",			"n/a",	"n/a",	"n/a",	"n/a",
	"GPP_U17",	"ISI_OKNOK_1",		"n/a",		"RSVD",	"n/a",			"n/a",	"n/a",	"n/a",	"n/a",
	"GPP_U18",	"ISI_ALERT_N",		"n/a",		"RSVD",	"n/a",			"n/a",	"n/a",	"n/a",	"n/a",
	"GPP_U19",	"PSE_QEPI3",		"n/a",		"RSVD",	"n/a",			"n/a",	"n/a",	"n/a",	"PSE_TGPIO12",
	"GPIO_RSVD_4",	"n/a",			"n/a",		"n/a",	"n/a",			"n/a",	"n/a",	"n/a",	"n/a",
	"GPIO_RSVD_5",	"n/a",			"n/a",		"n/a",	"n/a",			"n/a",	"n/a",	"n/a",	"n/a",
	"GPIO_RSVD_6",	"n/a",			"n/a",		"n/a",	"n/a",			"n/a",	"n/a",	"n/a",	"n/a",
	"GPIO_RSVD_7",	"n/a",			"n/a",		"n/a",	"n/a",			"n/a",	"n/a",	"n/a",	"n/a",
};

const struct gpio_group elkhartlake_pch_group_u = {
	.display	= "------- GPIO Group GPP_U -------",
	.pad_count	= ARRAY_SIZE(elkhartlake_pch_group_u_names) / 9,
	.func_count	= 9,
	.pad_names	= elkhartlake_pch_group_u_names,
};

const char *const elkhartlake_pch_group_v_names[] = {
	"GPP_V00",	"EMMC_CMD",
	"GPP_V01",	"EMMC_DATA0",
	"GPP_V02",	"EMMC_DATA1",
	"GPP_V03",	"EMMC_DATA2",
	"GPP_V04",	"EMMC_DATA3",
	"GPP_V05",	"EMMC_DATA4",
	"GPP_V06",	"EMMC_DATA5",
	"GPP_V07",	"EMMC_DATA6",
	"GPP_V08",	"EMMC_DATA7",
	"GPP_V09",	"EMMC_RCLK",
	"GPP_V10",	"EMMC_CLK",
	"GPP_V11",	"EMMC_RST_N",
	"GPP_V12",	"PSE_TGPIO00",
	"GPP_V13",	"PSE_TGPIO01",
	"GPP_V14",	"PSE_TGPIO02",
	"GPP_V15",	"PSE_TGPIO03",
};

const struct gpio_group elkhartlake_pch_group_v = {
	.display	= "------- GPIO Group GPP_V -------",
	.pad_count	= ARRAY_SIZE(elkhartlake_pch_group_v_names) / 2,
	.func_count	= 2,
	.pad_names	= elkhartlake_pch_group_v_names,
};

const char *const elkhartlake_pch_group_vgpio_names[] = {
	"VGPIO_0",	"VGPIO_0",
	"VGPIO_4",	"VGPIO_4",
	"VGPIO_5",	"VGPIO_5",
	"VGPIO_6",	"VGPIO_6",
	"VGPIO_7",	"VGPIO_7",
	"VGPIO_8",	"VGPIO_8",
	"VGPIO_9",	"VGPIO_9",
	"VGPIO_10",	"VGPIO_10",
	"VGPIO_11",	"VGPIO_11",
	"VGPIO_12",	"VGPIO_12",
	"VGPIO_13",	"VGPIO_13",
	"VGPIO_18",	"VGPIO_18",
	"VGPIO_19",	"VGPIO_19",
	"VGPIO_20",	"VGPIO_20",
	"VGPIO_21",	"VGPIO_21",
	"VGPIO_22",	"VGPIO_22",
	"VGPIO_23",	"VGPIO_23",
	"VGPIO_24",	"VGPIO_24",
	"VGPIO_25",	"VGPIO_25",
	"VGPIO_30",	"VGPIO_30",
	"VGPIO_31",	"VGPIO_31",
	"VGPIO_32",	"VGPIO_32",
	"VGPIO_33",	"VGPIO_33",
	"VGPIO_34",	"VGPIO_34",
	"VGPIO_35",	"VGPIO_35",
	"VGPIO_36",	"VGPIO_36",
	"VGPIO_37",	"VGPIO_37",
	"VGPIO_39",	"VGPIO_39",
};

const struct gpio_group elkhartlake_pch_group_vgpio = {
	.display	= "------- GPIO Group GPP_VGPIO -------",
	.pad_count	= ARRAY_SIZE(elkhartlake_pch_group_vgpio_names) / 2,
	.func_count	= 2,
	.pad_names	= elkhartlake_pch_group_vgpio_names,
};


const char *const elkhartlake_pch_group_vgpio_usb_names[] = {
	"VGPIO_USB_0",	"VGPIO_USB_0",
	"VGPIO_USB_1",	"VGPIO_USB_1",
	"VGPIO_USB_2",	"VGPIO_USB_2",
	"VGPIO_USB_3",	"VGPIO_USB_3",
};

const struct gpio_group elkhartlake_pch_group_vgpio_usb = {
	.display	= "------- GPIO Group GPP_VGPIO_USB -------",
	.pad_count	= ARRAY_SIZE(elkhartlake_pch_group_vgpio_usb_names) / 2,
	.func_count	= 2,
	.pad_names	= elkhartlake_pch_group_vgpio_usb_names,
};

const struct gpio_group *const elkhartlake_community_0_groups[] = {
	&elkhartlake_pch_group_b,
	&elkhartlake_pch_group_t,
	&elkhartlake_pch_group_g,
};

const struct gpio_community elkhartlake_community_0 = {
	.name		= "------- GPIO Community 0 -------",
	.pcr_port_id	= 0x6e,
	.group_count	= ARRAY_SIZE(elkhartlake_community_0_groups),
	.groups		= elkhartlake_community_0_groups,
};

const struct gpio_group *const elkhartlake_community_1_groups[] = {
	&elkhartlake_pch_group_v,
	&elkhartlake_pch_group_h,
	&elkhartlake_pch_group_d,
	&elkhartlake_pch_group_u,
	&elkhartlake_pch_group_vgpio,
};

const struct gpio_community elkhartlake_community_1 = {
	.name		= "------- GPIO Community 1 -------",
	.pcr_port_id	= 0x6d,
	.group_count	= ARRAY_SIZE(elkhartlake_community_1_groups),
	.groups		= elkhartlake_community_1_groups,
};

const struct gpio_group *const elkhartlake_community_2_groups[] = {
	&elkhartlake_pch_group_gpd,
};

const struct gpio_community elkhartlake_community_2 = {
	.name		= "------- GPIO Community 2 -------",
	.pcr_port_id	= 0x6c,
	.group_count	= ARRAY_SIZE(elkhartlake_community_2_groups),
	.groups		= elkhartlake_community_2_groups,
};

const struct gpio_group *const elkhartlake_community_3_groups[] = {
	&elkhartlake_pch_group_s,
	&elkhartlake_pch_group_a,
	&elkhartlake_pch_group_vgpio_usb,
};

const struct gpio_community elkhartlake_community_3 = {
	.name		= "------- GPIO Community 3 -------",
	.pcr_port_id	= 0x6b,
	.group_count	= ARRAY_SIZE(elkhartlake_community_3_groups),
	.groups		= elkhartlake_community_3_groups,
};

const struct gpio_group *const elkhartlake_community_4_groups[] = {
	&elkhartlake_pch_group_c,
	&elkhartlake_pch_group_f,
	&elkhartlake_pch_group_e,
};

const struct gpio_community elkhartlake_community_4 = {
	.name		= "------- GPIO Community 4 -------",
	.pcr_port_id	= 0x6a,
	.group_count	= ARRAY_SIZE(elkhartlake_community_4_groups),
	.groups		= elkhartlake_community_4_groups,
};

const struct gpio_group *const elkhartlake_community_5_groups[] = {
	&elkhartlake_pch_group_r,
};

const struct gpio_community elkhartlake_community_5 = {
	.name		= "------- GPIO Community 5 -------",
	.pcr_port_id	= 0x69,
	.group_count	= ARRAY_SIZE(elkhartlake_community_5_groups),
	.groups		= elkhartlake_community_5_groups,
};

const struct gpio_community *const elkhartlake_pch_communities[] = {
	&elkhartlake_community_0,
	&elkhartlake_community_1,
	&elkhartlake_community_2,
	&elkhartlake_community_3,
	&elkhartlake_community_4,
	&elkhartlake_community_5,
};

#endif
