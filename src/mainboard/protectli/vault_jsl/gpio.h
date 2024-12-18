/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef CFG_GPIO_H
#define CFG_GPIO_H

#include <gpio.h>

static const struct pad_config gpio_table[] = {

	/* ------- GPIO Community 0 ------- */

	/* ------- GPIO Group GPP_G ------- */

	/* GPP_G0 - GPIO */
	PAD_NC(GPP_G0, NONE),
	/* GPP_G1 - GPIO */
	PAD_NC(GPP_G1, NONE),
	/* GPP_G2 - GPIO */
	PAD_NC(GPP_G2, NONE),
	/* GPP_G3 - GPIO */
	PAD_NC(GPP_G3, NONE),
	/* GPP_G4 - GPIO */
	PAD_NC(GPP_G4, NONE),
	/* GPP_G5 - GPIO */
	PAD_NC(GPP_G5, NONE),
	/* GPP_G6 - GPIO */
	PAD_NC(GPP_G6, NONE),
	/* GPP_G7 - GPIO */
	PAD_NC(GPP_G7, NONE),


	/* ------- GPIO Community 1 ------- */

	/* ------- GPIO Group GPP_E ------- */

	/* GPP_E0 - GPIO */
	PAD_NC(GPP_E0, NONE),
	/* GPP_E1 - GPIO */
	PAD_NC(GPP_E1, NONE),
	/* GPP_E2 - GPIO */
	PAD_NC(GPP_E2, NONE),
	/* GPP_E3 - GPIO */
	PAD_NC(GPP_E3, NONE),
	/* GPP_E4 - GPIO */
	PAD_NC(GPP_E4, NONE),
	/* GPP_E5 - SATA_LED_N */
	PAD_CFG_NF(GPP_E5, NONE, DEEP, NF1),
	/* GPP_E6 - GPIO */
	PAD_NC(GPP_E6, NONE),
	/* GPP_E7 - GPIO */
	PAD_NC(GPP_E7, NONE),
	/* GPP_E8 - GPIO */
	PAD_NC(GPP_E8, NONE),
	/* GPP_E9 - GPIO */
	PAD_NC(GPP_E9, NONE),
	/* GPP_E10 - GPIO */
	PAD_NC(GPP_E10, NONE),
	/* GPP_E11 - GPIO */
	PAD_NC(GPP_E11, NONE),
	/* GPP_E12 - GPIO */
	PAD_NC(GPP_E12, NONE),
	/* GPP_E13 - GPIO */
	PAD_NC(GPP_E13, NONE),
	/* GPP_E14 - GPIO */
	PAD_NC(GPP_E14, NONE),
	/* GPP_E15 - DDI1_DDC_SCL */
	PAD_CFG_NF(GPP_E15, NONE, PLTRST, NF1),
	/* GPP_E16 - DDI1_DDC_SDA */
	PAD_CFG_NF(GPP_E16, NONE, PLTRST, NF1),
	/* GPP_E17 - DDI2_DDC_SCL */
	PAD_CFG_NF(GPP_E17, NONE, DEEP, NF1),
	/* GPP_E18 - DDI2_DDC_SDA */
	PAD_CFG_NF(GPP_E18, NONE, DEEP, NF1),
	/* GPP_E19 - GPIO */
	PAD_NC(GPP_E19, NONE),
	/* GPP_E20 - GPIO */
	PAD_NC(GPP_E20, NONE),
	/* GPP_E21 - GPIO */
	PAD_NC(GPP_E21, NONE),
	/* GPP_E22 - GPIO */
	PAD_NC(GPP_E22, NONE),
	/* GPP_E23 - GPIO */
	PAD_NC(GPP_E23, NONE),


	/* ------- GPIO Community 3 ------- */

	/* ------- GPIO Group GPD ------- */

	/* GPD0 - BATLOW# */
	PAD_CFG_NF(GPD0, NONE, RSMRST, NF1),
	/* GPD1 - PMC_ACPRESENT */
	PAD_CFG_NF(GPD1, NATIVE, RSMRST, NF1),
	/* GPD2 - LAN_WAKE# */
	PAD_CFG_NF(GPD2, NATIVE, RSMRST, NF1),
	/* GPD3 - PWRBTN# */
	PAD_CFG_NF(GPD3, UP_20K, RSMRST, NF1),
	/* GPD4 - SLP_S3# */
	PAD_CFG_NF(GPD4, NONE, RSMRST, NF1),
	/* GPD5 - SLP_S4# */
	PAD_CFG_NF(GPD5, NONE, RSMRST, NF1),
	/* GPD6 - SLP_A# */
	PAD_CFG_NF(GPD6, NONE, DEEP, NF1),
	/* GPD7 - GPIO */
	PAD_NC(GPD7, NONE),
	/* GPD8 - SUSCLK */
	PAD_CFG_NF(GPD8, NONE, RSMRST, NF1),
	/* GPD9 - SLP_WLAN# */
	PAD_CFG_NF(GPD9, NONE, DEEP, NF1),
	/* GPD10 - SLP_S5# */
	PAD_CFG_NF(GPD10, NONE, DEEP, NF1),

	/* ------- GPIO Community 4 ------- */

	/* ------- GPIO Group GPP_H ------- */

	/* GPP_H0 - GPIO */
	PAD_NC(GPP_H0, NONE),
	/* GPP_H1 - GPIO */
	PAD_NC(GPP_H1, NONE),
	/* GPP_H2 - GPIO */
	PAD_NC(GPP_H2,NONE),
	/* GPP_H3 - GPIO */
	PAD_NC(GPP_H3, NONE),
	/* GPP_H4 - GPIO */
	PAD_NC(GPP_H4, NONE),
	/* GPP_H5 - GPIO */
	PAD_NC(GPP_H5, NONE),
	/* GPP_H6 - GPIO */
	PAD_NC(GPP_H6, NONE),
	/* GPP_H7 - GPIO */
	PAD_NC(GPP_H7, NONE),
	/* GPP_H8 - GPIO */
	PAD_NC(GPP_H8, NONE),
	/* GPP_H9 - GPIO */
	PAD_NC(GPP_H9, NONE),
	/* GPP_H10 - GPIO */
	PAD_NC(GPP_H10, NONE),
	/* GPP_H11 - GPIO */
	PAD_NC(GPP_H11,NONE),
	/* GPP_H12 - GPIO */
	PAD_NC(GPP_H12,NONE),
	/* GPP_H13 - GPIO */
	PAD_NC(GPP_H13,NONE),
	/* GPP_H14 - GPIO */
	PAD_NC(GPP_H14,NONE),
	/* GPP_H15 - GPIO */
	PAD_NC(GPP_H15, NONE),
	/* GPP_H16 - GPIO */
	PAD_NC(GPP_H16,NONE),
	/* GPP_H17 - GPIO */
	PAD_NC(GPP_H17, NONE),
	/* GPP_H18 - GPIO */
	PAD_NC(GPP_H18,NONE),
	/* GPP_H19 - GPIO */
	PAD_NC(GPP_H19,NONE),
	/* GPP_H20 - GPIO */
	PAD_NC(GPP_H20, NONE),
	/* GPP_H21 - GPIO */
	PAD_NC(GPP_H21, NONE),
	/* GPP_H22 - GPIO */
	PAD_NC(GPP_H22, NONE),
	/* GPP_H23 - GPIO */
	PAD_NC(GPP_H23, NONE),

	/* ------- GPIO Group GPP_D ------- */

	/* GPP_D0 - GPIO */
	PAD_NC(GPP_D0, NONE),
	/* GPP_D1 - GPIO */
	PAD_NC(GPP_D1, NONE),
	/* GPP_D2 - GPIO */
	PAD_NC(GPP_D2, NONE),
	/* GPP_D3 - GPIO */
	PAD_NC(GPP_D3, NONE),
	/* GPP_D4 - GPIO */
	PAD_NC(GPP_D4, NONE),
	/* GPP_D5 - GPIO */
	PAD_NC(GPP_D5, NONE),
	/* GPP_D6 - GPIO */
	PAD_NC(GPP_D6, NONE),
	/* GPP_D7 - GPIO */
	PAD_NC(GPP_D7, NONE),
	/* GPP_D8 - GPIO */
	PAD_NC(GPP_D8, NONE),
	/* GPP_D9 - GPIO */
	PAD_NC(GPP_D9, NONE),
	/* GPP_D10 - GPIO */
	PAD_NC(GPP_D10, NONE),
	/* GPP_D11 - GPIO */
	PAD_NC(GPP_D11, NONE),
	/* GPP_D12 - GPIO */
	PAD_NC(GPP_D12, NONE),
	/* GPP_D13 - GPIO */
	PAD_NC(GPP_D13, NONE),
	/* GPP_D14 - GPIO */
	PAD_NC(GPP_D14, NONE),
	/* GPP_D15 - GPIO */
	PAD_NC(GPP_D15, NONE),
	/* GPP_D16 - GPIO */
	PAD_NC(GPP_D16, NONE),
	/* GPP_D17 - GPIO */
	PAD_NC(GPP_D17, NONE),
	/* GPP_D18 - GPIO */
	PAD_NC(GPP_D18, NONE),
	/* GPP_D19 - GPIO */
	PAD_NC(GPP_D19, NONE),
	/* GPP_D20 - GPIO */
	PAD_NC(GPP_D20, NONE),
	/* GPP_D21 - GPIO */
	PAD_NC(GPP_D21, NONE),
	/* GPP_D22 - GPIO */
	PAD_NC(GPP_D22, NONE),
	/* GPP_D23 - GPIO */
	PAD_NC(GPP_D23, NONE),

	/* ------- GPIO Group GPP_C ------- */

	/* GPP_C0 - GPIO */
	PAD_NC(GPP_C0, NONE),
	/* GPP_C1 - GPIO */
	PAD_NC(GPP_C1, NONE),
	/* GPP_C2 - GPIO */
	PAD_NC(GPP_C2, NONE),
	/* GPP_C3 - GPIO */
	PAD_NC(GPP_C3, NONE),
	/* GPP_C4 - GPIO */
	PAD_NC(GPP_C4, NONE),
	/* GPP_C5 - GPIO */
	PAD_NC(GPP_C5, NONE),
	/* GPP_C6 - M2_PEDET (NVME=1, SATA=0) */
	PAD_CFG_GPI(GPP_C6, UP_20K, DEEP),
	/* GPP_C7 - GPIO */
	PAD_NC(GPP_C7, NONE),
	/* GPP_C8 - GPIO */
	PAD_NC(GPP_C8, NONE),
	/* GPP_C9 - GPIO */
	PAD_NC(GPP_C9, NONE),
	/* GPP_C10 - GPIO */
	PAD_NC(GPP_C10, NONE),
	/* GPP_C11 - GPIO */
	PAD_NC(GPP_C11, NONE),
	/* GPP_C12 - GPIO */
	PAD_NC(GPP_C12, NONE),
	/* GPP_C13 - GPIO */
	PAD_NC(GPP_C13, NONE),
	/* GPP_C14 - GPIO */
	PAD_NC(GPP_C14, NONE),
	/* GPP_C15 - GPIO */
	PAD_NC(GPP_C15, NONE),
	/* GPP_C16 - GPIO */
	PAD_NC(GPP_C16, NONE),
	/* GPP_C17 - GPIO */
	PAD_NC(GPP_C17, NONE),
	/* GPP_C18 - GPIO */
	PAD_NC(GPP_C18, NONE),
	/* GPP_C19 - GPIO */
	PAD_NC(GPP_C19, NONE),
	/* GPP_C20 - GPIO */
	PAD_NC(GPP_C20, NONE),
	/* GPP_C21 - GPIO */
	PAD_NC(GPP_C21, NONE),
	/* GPP_C22 - GPIO */
	PAD_NC(GPP_C22, NONE),
	/* GPP_C23 - GPIO */
	PAD_NC(GPP_C23, NONE),

	/* ------- GPIO Community 5 ------- */

	/* ------- GPIO Group GPP_F ------- */

	/* GPP_F0 - GPIO */
	PAD_NC(GPP_F0, NONE),
	/* GPP_F1 - GPIO */
	PAD_NC(GPP_F1, NONE),
	/* GPP_F2 - GPIO */
	PAD_NC(GPP_F2, NONE),
	/* GPP_F3 - GPIO */
	PAD_NC(GPP_F3, NONE),
	/* GPP_F4 - GPIO */
	PAD_NC(GPP_F4, NONE),
	/* GPP_F5 - GPIO */
	PAD_NC(GPP_F5, NONE),
	/* GPP_F6 - GPIO */
	PAD_NC(GPP_F6, NONE),
	/* GPP_F7 - EMMC_CMD */
	PAD_CFG_NF(GPP_F7, NONE, DEEP, NF1),
	/* GPP_F8 - EMMC_DATA0 */
	PAD_CFG_NF(GPP_F8, NONE, DEEP, NF1),
	/* GPP_F9 - EMMC_DATA1 */
	PAD_CFG_NF(GPP_F9, NONE, DEEP, NF1),
	/* GPP_F10 - EMMC_DATA2 */
	PAD_CFG_NF(GPP_F10, NONE, DEEP, NF1),
	/* GPP_F11 - EMMC_DATA3 */
	PAD_CFG_NF(GPP_F11, NONE, DEEP, NF1),
	/* GPP_F12 - EMMC_DATA4 */
	PAD_CFG_NF(GPP_F12, NONE, DEEP, NF1),
	/* GPP_F13 - EMMC_DATA5 */
	PAD_CFG_NF(GPP_F13, NONE, DEEP, NF1),
	/* GPP_F14 - EMMC_DATA6 */
	PAD_CFG_NF(GPP_F14, NONE, DEEP, NF1),
	/* GPP_F15 - EMMC_DATA7 */
	PAD_CFG_NF(GPP_F15, NONE, DEEP, NF1),
	/* GPP_F16 - EMMC_RCLK */
	PAD_CFG_NF(GPP_F16, NONE, DEEP, NF1),
	/* GPP_F17 - EMMC_CLK */
	PAD_CFG_NF(GPP_F17, NONE, DEEP, NF1),
	/* GPP_F18 - EMMC_RESET_N */
	PAD_CFG_NF(GPP_F18, NONE, DEEP, NF1),
	/* GPP_F19 - GPIO */
	PAD_NC(GPP_F19, NONE),

	/* ------- GPIO Group GPP_B ------- */

	/* GPP_B0 - PMC_CORE_VID0 */
	PAD_CFG_NF(GPP_B0, NONE, DEEP, NF1),
	/* GPP_B1 - PMC_CORE_VID1 */
	PAD_CFG_NF(GPP_B1, NONE, DEEP, NF1),
	/* GPP_B2 - GPIO */
	PAD_NC(GPP_B2, NONE),
	/* GPP_B3 - GPIO */
	PAD_NC(GPP_B3, NONE),
	/* GPP_B4 - GPIO */
	PAD_NC(GPP_B4, NONE),
	/* GPP_B5 - GPIO */
	PAD_NC(GPP_B5, NONE),
	/* GPP_B6 - GPIO */
	PAD_NC(GPP_B6, NONE),
	/* GPP_B7 - GPIO */
	PAD_NC(GPP_B7, NONE),
	/* GPP_B8 - GPIO */
	PAD_NC(GPP_B8, NONE),
	/* GPP_B9 - GPIO */
	PAD_NC(GPP_B9, NONE),
	/* GPP_B10 - GPIO */
	PAD_NC(GPP_B10, NONE),
	/* GPP_B11 - GPIO */
	PAD_NC(GPP_B11, NONE),
	/* GPP_B12 - PMC_SLP_S_N */
	PAD_CFG_NF(GPP_B12, NONE, DEEP, NF1),
	/* GPP_B13 - PMC_PLTRST_N */
	PAD_CFG_NF(GPP_B13, NONE, DEEP, NF1),
	/* GPP_B14 - SPKR */
	PAD_CFG_NF(GPP_B14, NONE, DEEP, NF1),
	/* GPP_B15 - GPIO */
	PAD_NC(GPP_B15, NONE),
	/* GPP_B16 - GPIO */
	PAD_NC(GPP_B16, NONE),
	/* GPP_B17 - GPIO */
	PAD_NC(GPP_B17, NONE),
	/* GPP_B18 - GPIO */
	PAD_NC(GPP_B18, NONE),
	/* GPP_B19 - GPIO */
	PAD_NC(GPP_B19, NONE),
	/* GPP_B20 - GPIO */
	PAD_NC(GPP_B20, NONE),
	/* GPP_B21 - GPIO */
	PAD_NC(GPP_B21, NONE),
	/* GPP_B22 - GPIO */
	PAD_NC(GPP_B22, NONE),
	/* GPP_B23 - DDI2_HPD */
	PAD_CFG_NF(GPP_B23, NONE, PLTRST, NF1),

	/* ------- GPIO Group GPP_A ------- */

	/* GPP_A0 - ESPI_IO_0 */
	PAD_CFG_NF(GPP_A0, UP_20K, DEEP, NF1),
	/* GPP_A1 - ESPI_IO_1 */
	PAD_CFG_NF(GPP_A1, UP_20K, DEEP, NF1),
	/* GPP_A2 - ESPI_IO_2 */
	PAD_CFG_NF(GPP_A2, UP_20K, DEEP, NF1),
	/* GPP_A3 - ESPI_IO_3 */
	PAD_CFG_NF(GPP_A3, UP_20K, DEEP, NF1),
	/* GPP_A4 - ESPI_CS_N */
	PAD_CFG_NF(GPP_A4, UP_20K, DEEP, NF1),
	/* GPP_A5 - ESPI_CLK */
	PAD_CFG_NF(GPP_A5, DN_20K, DEEP, NF1),
	/* GPP_A6 - ESPI_RESET_N */
	PAD_CFG_NF(GPP_A6, NONE, DEEP, NF1),
	/* GPP_A7 - SMB_CLK */
	PAD_CFG_NF(GPP_A7, NONE, DEEP, NF1),
	/* GPP_A8 - SMB_DATA */
	PAD_CFG_NF(GPP_A8, NONE, DEEP, NF1),
	/* GPP_A9 - GPIO */
	PAD_NC(GPP_A9, NONE),
	/* GPP_A10 - GPIO */
	PAD_NC(GPP_A10, NONE),
	/* GPP_A11 - GPIO */
	PAD_NC(GPP_A11, NONE),
	/* GPP_A12 - GPIO */
	PAD_NC(GPP_A12, NONE),
	/* GPP_A13 - GPIO */
	PAD_NC(GPP_A13, NONE),
	/* GPP_A14 - GPIO */
	PAD_NC(GPP_A14, NONE),
	/* GPP_A15 - GPIO */
	PAD_NC(GPP_A15, NONE),
	/* GPP_A16 - DDI1_HPD */
	PAD_CFG_NF(GPP_A16, NONE, PLTRST, NF1),
	/* GPP_A17 - DDI0_HPD */
	PAD_CFG_NF(GPP_A17, NONE, DEEP, NF1),
	/* GPP_A18 - GPIO */
	PAD_NC(GPP_A18, NONE),
	/* GPP_A19 - GPIO */
	PAD_NC(GPP_A19, NONE),

	/* ------- GPIO Group GPP_S ------- */

	/* GPP_S0 - GPIO */
	PAD_NC(GPP_S0, NONE),
	/* GPP_S1 - GPIO */
	PAD_NC(GPP_S1, NONE),
	/* GPP_S2 - GPIO */
	PAD_NC(GPP_S2, NONE),
	/* GPP_S3 - GPIO */
	PAD_NC(GPP_S3, NONE),
	/* GPP_S4 - GPIO */
	PAD_NC(GPP_S4, NONE),
	/* GPP_S5 - GPIO */
	PAD_NC(GPP_S5, NONE),
	/* GPP_S6 - GPIO */
	PAD_NC(GPP_S6, NONE),
	/* GPP_S7 - GPIO */
	PAD_NC(GPP_S7, NONE),

	/* ------- GPIO Group GPP_R ------- */

	/* GPP_R0 - HDA_BCLK */
	PAD_CFG_NF(GPP_R0, NONE, DEEP, NF1),
	/* GPP_R1 - HDA_SYNC */
	PAD_CFG_NF(GPP_R1, NATIVE, DEEP, NF1),
	/* GPP_R2 - HDA_SDO */
	PAD_CFG_NF(GPP_R2, NATIVE, DEEP, NF1),
	/* GPP_R3 - HDA_SDI0 */
	PAD_CFG_NF(GPP_R3, NATIVE, DEEP, NF1),
	/* GPP_R4 - HDA_RST_N */
	PAD_CFG_NF(GPP_R4, NONE, DEEP, NF1),
	/* GPP_R5 - GPIO */
	PAD_NC(GPP_R5, NONE),
	/* GPP_R6 - GPIO */
	PAD_NC(GPP_R6, NONE),
	/* GPP_R7 - GPIO */
	PAD_NC(GPP_R7, NONE),
};

#endif /* CFG_GPIO_H */
