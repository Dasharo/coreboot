/* SPDX-License-Identifier: GPL-2.0-only */

#include <gpio.h>

#include <variant/gpio.h>

#ifndef PAD_CFG_GPIO_BIDIRECT
#define PAD_CFG_GPIO_BIDIRECT(pad, val, pull, rst, trig, own)		\
	_PAD_CFG_STRUCT(pad,						\
		PAD_FUNC(GPIO) | PAD_RESET(rst) | PAD_TRIG(trig) |	\
		PAD_BUF(NO_DISABLE) | val,				\
		PAD_PULL(pull) | PAD_CFG_OWN_GPIO(own))
#endif

/* PAD configuration was generated automatically using intelp2m utility */
static const struct pad_config gpio_table[] = {
	/* ------- GPIO Community 0 ------- */

	/* ------- GPIO Group GPP_B ------- */

	/* GPP_B0 - CORE_VID0 */
	PAD_CFG_NF(GPP_B0, NONE, DEEP, NF1),
	/* GPP_B1 - CORE_VID1 */
	PAD_CFG_NF(GPP_B1, NONE, DEEP, NF1),
	/* GPP_B2 - VRALERT# */
	PAD_CFG_NF(GPP_B2, NONE, DEEP, NF1),
	PAD_NC(GPP_B3, NONE),
	PAD_NC(GPP_B4, NONE),
	PAD_NC(GPP_B5, NONE),
	PAD_NC(GPP_B6, NONE),
	PAD_NC(GPP_B7, NONE),
	PAD_NC(GPP_B8, NONE),
	PAD_NC(GPP_B9, NONE),
	PAD_NC(GPP_B10, NONE),
	/* GPP_B11 - PMCALERT# */
	PAD_CFG_NF(GPP_B11, NONE, RSMRST, NF1),
	/* GPP_B12 - SLP_S0# */
	PAD_CFG_NF(GPP_B12, NONE, DEEP, NF1),
	/* GPP_B13 - PLTRST# */
	PAD_CFG_NF(GPP_B13, NONE, DEEP, NF1),
	/* GPP_B14 - SPKR */
	PAD_CFG_NF(GPP_B14, NONE, PLTRST, NF1),
	PAD_NC(GPP_B15, NONE),
	PAD_NC(GPP_B16, NONE),
	PAD_NC(GPP_B17, NONE),
	/* GPP_B18 - GPIO */
	PAD_CFG_GPO(GPP_B18, 0, DEEP),
	PAD_NC(GPP_B19, NONE),
	PAD_NC(GPP_B20, NONE),
	PAD_NC(GPP_B21, NONE),
	PAD_NC(GPP_B22, NONE),
	/* GPP_B23 - GPIO */
	PAD_CFG_GPO(GPP_B23, 0, DEEP),
	/* GPP_B24 - GSPI0_CLK_LOOPBK */
	PAD_CFG_NF(GPP_B24, NONE, DEEP, NF1),
	/* GPP_B25 - GSPI1_CLK_LOOPBK */
	PAD_CFG_NF(GPP_B25, NONE, DEEP, NF1),

	/* ------- GPIO Group GPP_A ------- */

	/* GPP_A0 - ESPI_IO0 */
	PAD_CFG_NF(GPP_A0, UP_20K, DEEP, NF1),
	/* GPP_A1 - ESPI_IO1 */
	PAD_CFG_NF(GPP_A1, UP_20K, DEEP, NF1),
	/* GPP_A2 - ESPI_IO2 */
	PAD_CFG_NF(GPP_A2, UP_20K, DEEP, NF1),
	/* GPP_A3 - ESPI_IO3 */
	PAD_CFG_NF(GPP_A3, UP_20K, DEEP, NF1),
	/* GPP_A4 - ESPI_CS0# */
	PAD_CFG_NF(GPP_A4, UP_20K, DEEP, NF1),
	/* GPP_A5 - ESPI_ALERT0# */
	PAD_CFG_NF(GPP_A5, UP_20K, DEEP, NF1),
	PAD_NC(GPP_A6, NONE),
	PAD_NC(GPP_A7, NONE),
	PAD_NC(GPP_A8, NONE),
	/* GPP_A9 - ESPI_CLK */
	PAD_CFG_NF(GPP_A9, DN_20K, DEEP, NF1),
	/* GPP_A10 - ESPI_RESET# */
	PAD_CFG_NF(GPP_A10, NONE, DEEP, NF1),
	PAD_NC(GPP_A11, NONE),
	PAD_NC(GPP_A12, NONE),
	PAD_NC(GPP_A13, NONE),
	/* GPP_A14 - USB_C_GPP_A14 */
	PAD_CFG_NF(GPP_A14, NONE, DEEP, NF6),
	/* GPP_A15 - USB_C_GPP_A15 */
	PAD_CFG_NF(GPP_A15, NONE, DEEP, NF6),
	/* GPP_A16 - USB_OC3# */
	PAD_CFG_NF(GPP_A16, NONE, DEEP, NF1),
	/* GPP_A17 - GPIO */
	PAD_CFG_GPO(GPP_A17, 1, PLTRST),
	/* GPP_A18 - DDSP_HPDB */
	PAD_CFG_NF(GPP_A18, NONE, DEEP, NF1),
	/* GPP_A19 - DDSP_HPD1 */
	PAD_CFG_NF(GPP_A19, NONE, DEEP, NF1),
	/* GPP_A20 - DDSP_HPD2 */
	PAD_CFG_NF(GPP_A20, NONE, DEEP, NF1),
	/* GPP_A21 - USB_C_GPP_A21 */
	PAD_CFG_NF(GPP_A21, NONE, DEEP, NF6),
	/* GPP_A22 - USB_C_GPP_A22 */
	PAD_CFG_NF(GPP_A22, NONE, DEEP, NF6),
	/* GPP_A23 - ESPI_CS1# */
	PAD_CFG_NF(GPP_A23, UP_20K, DEEP, NF1),
	/* GPP_ESPI_CLK_LOOPBK - GPP_ESPI_CLK_LOOPBK */
	PAD_CFG_NF(GPP_ESPI_CLK_LOOPBK, NONE, DEEP, NF1),

	/* ------- GPIO Community 1 ------- */

	/* ------- GPIO Group GPP_S ------- */

	PAD_NC(GPP_S0, NONE),
	PAD_NC(GPP_S1, NONE),
	PAD_NC(GPP_S2, NONE),
	PAD_NC(GPP_S3, NONE),
	PAD_NC(GPP_S4, NONE),
	PAD_NC(GPP_S5, NONE),
	PAD_NC(GPP_S6, NONE),
	PAD_NC(GPP_S7, NONE),

	/* ------- GPIO Group GPP_I ------- */

	PAD_NC(GPP_I0, NONE),
	PAD_NC(GPP_I1, NONE),
	PAD_NC(GPP_I2, NONE),
	PAD_NC(GPP_I3, NONE),
	PAD_NC(GPP_I4, NONE),
	PAD_NC(GPP_I5, NONE),
	PAD_NC(GPP_I6, NONE),
	/* GPP_I7 - EMMC_CMD */
	PAD_CFG_NF(GPP_I7, NONE, DEEP, NF1),
	/* GPP_I8 - EMMC_DATA0 */
	PAD_CFG_NF(GPP_I8, NONE, DEEP, NF1),
	/* GPP_I9 - EMMC_DATA1 */
	PAD_CFG_NF(GPP_I9, NONE, DEEP, NF1),
	/* GPP_I10 - EMMC_DATA2 */
	PAD_CFG_NF(GPP_I10, NONE, DEEP, NF1),
	/* GPP_I11 - EMMC_DATA3 */
	PAD_CFG_NF(GPP_I11, NONE, DEEP, NF1),
	/* GPP_I12 - EMMC_DATA4 */
	PAD_CFG_NF(GPP_I12, NONE, DEEP, NF1),
	/* GPP_I13 - EMMC_DATA5 */
	PAD_CFG_NF(GPP_I13, NONE, DEEP, NF1),
	/* GPP_I14 - EMMC_DATA6 */
	PAD_CFG_NF(GPP_I14, NONE, DEEP, NF1),
	/* GPP_I15 - EMMC_DATA7 */
	PAD_CFG_NF(GPP_I15, NONE, DEEP, NF1),
	/* GPP_I16 - EMMC_RCLK */
	PAD_CFG_NF(GPP_I16, NONE, DEEP, NF1),
	/* GPP_I17 - EMMC_CLK */
	PAD_CFG_NF(GPP_I17, NONE, DEEP, NF1),
	/* GPP_I18 - EMMC_RESET# */
	PAD_CFG_NF(GPP_I18, NONE, DEEP, NF1),
	PAD_NC(GPP_I19, NONE),

	/* ------- GPIO Group GPP_H ------- */

	/* GPP_H0 - GPIO */
	PAD_CFG_GPO(GPP_H0, 0, DEEP),
	/* GPP_H1 - GPIO */
	PAD_CFG_GPO(GPP_H1, 0, DEEP),
	/* GPP_H2 - GPIO */
	PAD_CFG_GPO(GPP_H2, 0, DEEP),
	PAD_NC(GPP_H3, NONE),
	/* GPP_H4 - I2C0_SDA */
	PAD_CFG_NF(GPP_H4, NONE, DEEP, NF1),
	/* GPP_H5 - I2C0_SCL */
	PAD_CFG_NF(GPP_H5, NONE, DEEP, NF1),
	PAD_NC(GPP_H6, NONE),
	PAD_NC(GPP_H7, NONE),
	/* GPP_H8 - I2C4_SDA */
	PAD_CFG_NF(GPP_H8, NONE, DEEP, NF1),
	/* GPP_H9 - I2C4_SCL */
	PAD_CFG_NF(GPP_H9, NONE, DEEP, NF1),
	PAD_NC(GPP_H8, NONE),
	PAD_NC(GPP_H9, NONE),
	PAD_NC(GPP_H10, NONE),
	PAD_NC(GPP_H11, NONE),
	PAD_NC(GPP_H12, NONE),
	PAD_NC(GPP_H13, NONE),
	PAD_NC(GPP_H14, NONE),
	/* GPP_H15 - DDPB_CTRLCLK */
	PAD_CFG_NF(GPP_H15, NONE, DEEP, NF1),
	PAD_NC(GPP_H16, NONE),
	/* GPP_H17 - DDPB_CTRLDATA */
	PAD_CFG_NF(GPP_H17, NONE, DEEP, NF1),
	/* GPP_H18 - PROC_C10_GATE# */
	PAD_CFG_NF(GPP_H18, NONE, DEEP, NF1),
	/* GPP_H19 - SRCCLKREQ4# */
	PAD_CFG_NF(GPP_H19, NONE, DEEP, NF1),
	PAD_NC(GPP_H20, NONE),
	PAD_NC(GPP_H21, NONE),
	PAD_NC(GPP_H22, NONE),
	PAD_NC(GPP_H23, NONE),
	/* ------- GPIO Group GPP_D ------- */

	PAD_NC(GPP_D0, NONE),
	PAD_NC(GPP_D1, NONE),
	PAD_NC(GPP_D2, NONE),
	PAD_NC(GPP_D3, NONE),
	PAD_NC(GPP_D4, NONE),
	/* GPP_D5 - SRCCLKREQ0# */
	PAD_CFG_NF(GPP_D5, NONE, DEEP, NF1),
	/* GPP_D6 - SRCCLKREQ1# */
	PAD_CFG_NF(GPP_D6, NONE, DEEP, NF1),
	/* GPP_D7 - SRCCLKREQ2# */
	PAD_CFG_NF(GPP_D7, NONE, DEEP, NF1),
	/* GPP_D8 - SRCCLKREQ3# */
	PAD_CFG_NF(GPP_D8, NONE, DEEP, NF1),
	PAD_NC(GPP_D9, NONE),
	PAD_NC(GPP_D10, NONE),
	PAD_NC(GPP_D11, NONE),
	PAD_NC(GPP_D12, NONE),
	PAD_NC(GPP_D13, NONE),
	PAD_NC(GPP_D14, NONE),
	PAD_NC(GPP_D15, NONE),
	PAD_NC(GPP_D16, NONE),
	PAD_NC(GPP_D17, NONE),
	PAD_NC(GPP_D18, NONE),
	PAD_NC(GPP_D19, NONE),
	/* GPP_GSPI2_CLK_LOOPBK - GPP_GSPI2_CLK_LOOPBK */
	PAD_CFG_NF(GPP_GSPI2_CLK_LOOPBK, NONE, DEEP, NF1),
	/* ------- GPIO Group vGPIO ------- */

	/* GPP_VGPIO_0 - GPIO */
	PAD_CFG_GPO(GPP_VGPIO_0, 0, DEEP),
	/* GPP_VGPIO_4 - GPIO */
	PAD_CFG_GPI_TRIG_OWN(GPP_VGPIO_4, NONE, DEEP, OFF, ACPI),
	/* GPP_VGPIO_5 - GPIO */
	PAD_CFG_GPIO_BIDIRECT(GPP_VGPIO_5, 1, NONE, DEEP, LEVEL, ACPI),
	/* GPP_VGPIO_6 - GPP_VGPIO_6 */
	PAD_CFG_NF(GPP_VGPIO_6, NONE, DEEP, NF1),
	/* GPP_VGPIO_7 - GPP_VGPIO_7 */
	PAD_CFG_NF(GPP_VGPIO_7, NONE, DEEP, NF1),
	/* GPP_VGPIO_8 - GPP_VGPIO_8 */
	PAD_CFG_NF(GPP_VGPIO_8, NONE, DEEP, NF1),
	/* GPP_VGPIO_9 - GPP_VGPIO_9 */
	PAD_CFG_NF(GPP_VGPIO_9, NONE, DEEP, NF1),
	/* GPP_VGPIO_10 - GPP_VGPIO_10 */
	PAD_CFG_NF(GPP_VGPIO_10, NONE, DEEP, NF1),
	/* GPP_VGPIO_11 - GPP_VGPIO_11 */
	PAD_CFG_NF(GPP_VGPIO_11, NONE, DEEP, NF1),
	/* GPP_VGPIO_12 - GPP_VGPIO_12 */
	PAD_CFG_NF(GPP_VGPIO_12, NONE, DEEP, NF1),
	/* GPP_VGPIO_13 - GPP_VGPIO_13 */
	PAD_CFG_NF(GPP_VGPIO_13, NONE, DEEP, NF1),
	/* GPP_VGPIO_18 - GPP_VGPIO_18 */
	PAD_CFG_NF(GPP_VGPIO_18, NONE, DEEP, NF1),
	/* GPP_VGPIO_19 - GPP_VGPIO_19 */
	PAD_CFG_NF(GPP_VGPIO_19, NONE, DEEP, NF1),
	/* GPP_VGPIO_20 - GPP_VGPIO_20 */
	PAD_CFG_NF(GPP_VGPIO_20, NONE, DEEP, NF1),
	/* GPP_VGPIO_21 - GPP_VGPIO_21 */
	PAD_CFG_NF(GPP_VGPIO_21, NONE, DEEP, NF1),
	/* GPP_VGPIO_22 - GPP_VGPIO_22 */
	PAD_CFG_NF(GPP_VGPIO_22, NONE, DEEP, NF1),
	/* GPP_VGPIO_23 - GPP_VGPIO_23 */
	PAD_CFG_NF(GPP_VGPIO_23, NONE, DEEP, NF1),
	/* GPP_VGPIO_24 - GPP_VGPIO_24 */
	PAD_CFG_NF(GPP_VGPIO_24, NONE, DEEP, NF1),
	/* GPP_VGPIO_25 - GPP_VGPIO_25 */
	PAD_CFG_NF(GPP_VGPIO_25, NONE, DEEP, NF1),
	/* GPP_VGPIO_30 - GPP_VGPIO_30 */
	PAD_CFG_NF(GPP_VGPIO_30, NONE, DEEP, NF1),
	/* GPP_VGPIO_31 - GPP_VGPIO_31 */
	PAD_CFG_NF(GPP_VGPIO_31, NONE, DEEP, NF1),
	/* GPP_VGPIO_32 - GPP_VGPIO_32 */
	PAD_CFG_NF(GPP_VGPIO_32, NONE, DEEP, NF1),
	/* GPP_VGPIO_33 - GPP_VGPIO_33 */
	PAD_CFG_NF(GPP_VGPIO_33, NONE, DEEP, NF1),
	/* GPP_VGPIO_34 - GPP_VGPIO_34 */
	PAD_CFG_NF(GPP_VGPIO_34, NONE, DEEP, NF1),
	/* GPP_VGPIO_35 - GPP_VGPIO_35 */
	PAD_CFG_NF(GPP_VGPIO_35, NONE, DEEP, NF1),
	/* GPP_VGPIO_36 - GPP_VGPIO_36 */
	PAD_CFG_NF(GPP_VGPIO_36, NONE, DEEP, NF1),
	/* GPP_VGPIO_37 - GPP_VGPIO_37 */
	PAD_CFG_NF(GPP_VGPIO_37, NONE, DEEP, NF1),

	/* ------- GPIO Community 2 ------- */

	/* ------- GPIO Group GPP_GPD ------- */

	/* GPD0 - BATLOW# */
	PAD_CFG_NF(GPD0, UP_20K, PWROK, NF1),
	/* GPD1 - ACPRESENT */
	PAD_CFG_NF(GPD1, NATIVE, PWROK, NF1),
	/* GPD2 - LAN_WAKE# */
	PAD_CFG_NF(GPD2, NATIVE, PWROK, NF1),
	/* GPD3 - PWRBTN# */
	PAD_CFG_NF(GPD3, UP_20K, PWROK, NF1),
	/* GPD4 - SLP_S3# */
	PAD_CFG_NF(GPD4, NONE, PWROK, NF1),
	/* GPD5 - SLP_S4# */
	PAD_CFG_NF(GPD5, NONE, PWROK, NF1),
	/* GPD6 - SLP_A# */
	PAD_CFG_NF(GPD6, NONE, PWROK, NF1),
	/* GPD7 - GPIO */
	PAD_CFG_GPO(GPD7, 0, PWROK),
	/* GPD8 - SUSCLK */
	PAD_CFG_NF(GPD8, NONE, PWROK, NF1),
	/* GPD9 - GPIO */
	PAD_CFG_GPO(GPD9, 0, PWROK),
	/* GPD10 - SLP_S5# */
	PAD_CFG_NF(GPD10, NONE, PWROK, NF1),
	/* GPD11 - GPIO */
	PAD_CFG_GPO(GPD11, 0, PWROK),
	/* GPD_INPUT3VSEL - GPD_INPUT3VSEL */
	PAD_CFG_NF(GPD_INPUT3VSEL, NONE, PWROK, NF1),
	/* GPD_SLP_LANB - GPD_SLP_LANB */
	PAD_CFG_NF(GPD_SLP_LANB, NONE, PWROK, NF1),
	/* GPD_SLP_SUSB - GPD_SLP_SUSB */
	PAD_CFG_NF(GPD_SLP_SUSB, NONE, PWROK, NF1),
	/* GPD_WAKEB - GPD_WAKEB */
	PAD_CFG_NF(GPD_WAKEB, NONE, PWROK, NF1),
	/* GPD_DRAM_RESETB - GPD_DRAM_RESETB */
	PAD_CFG_NF(GPD_DRAM_RESETB, NONE, PWROK, NF1),

	/* ------- GPIO Community 4 ------- */

	/* ------- GPIO Group GPP_C ------- */

	/* GPP_C0 - SMBCLK */
	PAD_CFG_NF(GPP_C0, NONE, DEEP, NF1),
	/* GPP_C1 - SMBDATA */
	PAD_CFG_NF(GPP_C1, NONE, DEEP, NF1),
	/* GPP_C2 - GPIO */
	PAD_CFG_GPO(GPP_C2, 0, DEEP),
	/* GPP_C3 - SML0CLK */
	PAD_CFG_NF(GPP_C3, NONE, DEEP, NF1),
	/* GPP_C4 - SML0DATA */
	PAD_CFG_NF(GPP_C4, NONE, DEEP, NF1),
	/* GPP_C5 - GPIO */
	PAD_CFG_GPO(GPP_C5, 0, DEEP),
	/* GPP_C6 - SML1CLK */
	PAD_CFG_NF(GPP_C6, NONE, RSMRST, NF1),
	/* GPP_C7 - SML1DATA */
	PAD_CFG_NF(GPP_C7, NONE, RSMRST, NF1),
	PAD_NC(GPP_C8, NONE),
	PAD_NC(GPP_C9, NONE),
	PAD_NC(GPP_C10, NONE),
	PAD_NC(GPP_C11, NONE),
	PAD_NC(GPP_C12, NONE),
	PAD_NC(GPP_C13, NONE),
	PAD_NC(GPP_C14, NONE),
	PAD_NC(GPP_C15, NONE),
	PAD_NC(GPP_C16, NONE),
	PAD_NC(GPP_C17, NONE),
	PAD_NC(GPP_C18, NONE),
	PAD_NC(GPP_C19, NONE),
	PAD_NC(GPP_C20, NONE),
	PAD_NC(GPP_C21, NONE),
	PAD_NC(GPP_C22, NONE),
	PAD_NC(GPP_C23, NONE),

	/* ------- GPIO Group GPP_F ------- */

	PAD_NC(GPP_F0, NONE),
	PAD_NC(GPP_F1, NONE),
	PAD_NC(GPP_F2, NONE),
	PAD_NC(GPP_F3, NONE),
	PAD_NC(GPP_F4, NONE),
	PAD_NC(GPP_F5, NONE),
	PAD_NC(GPP_F6, NONE),
	PAD_CFG_GPO(GPP_F7, 0, DEEP),
	PAD_NC(GPP_F8, NONE),
	PAD_NC(GPP_F9, NONE),
	/* GPP_F10 - GPIO */
	PAD_CFG_GPO(GPP_F10, 0, DEEP),
	PAD_NC(GPP_F11, NONE),
	PAD_NC(GPP_F12, NONE),
	PAD_NC(GPP_F13, NONE),
	PAD_NC(GPP_F14, NONE),
	PAD_NC(GPP_F15, NONE),
	/* GPP_F15 - GPIO (5G Power_On_Off)*/
	PAD_CFG_GPO(GPP_F15, 0, DEEP),
	PAD_NC(GPP_F16, NONE),
	PAD_NC(GPP_F17, NONE),
	PAD_NC(GPP_F18, NONE),
	PAD_NC(GPP_F19, NONE),
	/* GPP_F20 - Reserved */
	PAD_CFG_NF(GPP_F20, NONE, DEEP, NF1),
	/* GPP_F21 - Reserved */
	PAD_CFG_NF(GPP_F21, NONE, DEEP, NF1),
	PAD_NC(GPP_F22, NONE),
	PAD_NC(GPP_F23, NONE),
	/* GPP_F_CLK_LOOPBK - GPIO */
	PAD_NC(GPP_F_CLK_LOOPBK, NONE),

	/* ------- GPIO Group GPP_HVCMOS ------- */

	/* GPP_L_BKLTEN - n/a */
	PAD_CFG_NF(GPP_L_BKLTEN, NONE, DEEP, NF1),
	/* GPP_L_BKLTCTL - n/a */
	PAD_CFG_NF(GPP_L_BKLTCTL, NONE, DEEP, NF1),
	/* GPP_L_VDDEN - n/a */
	PAD_CFG_NF(GPP_L_VDDEN, NONE, DEEP, NF1),
	/* GPP_SYS_PWROK - n/a */
	PAD_CFG_NF(GPP_SYS_PWROK, NONE, DEEP, NF1),
	/* GPP_SYS_RESETB - n/a */
	PAD_CFG_NF(GPP_SYS_RESETB, NONE, DEEP, NF1),
	/* GPP_MLK_RSTB - n/a */
	PAD_CFG_NF(GPP_MLK_RSTB, NONE, DEEP, NF1),

	/* ------- GPIO Group GPP_E ------- */

	PAD_NC(GPP_E0, NONE),
	PAD_NC(GPP_E1, NONE),
	PAD_NC(GPP_E2, NONE),
	PAD_NC(GPP_E3, NONE),
	PAD_NC(GPP_E4, NONE),
	PAD_NC(GPP_E5, NONE),
	/* GPP_E6 - GPIO */
	PAD_CFG_GPO(GPP_E6, 0, DEEP),
	PAD_NC(GPP_E7, NONE),
	PAD_NC(GPP_E8, NONE),
	PAD_NC(GPP_E9, NONE),
	PAD_NC(GPP_E10, NONE),
	PAD_NC(GPP_E11, NONE),
	PAD_NC(GPP_E12, NONE),
	/* GPP_E13 - GPIO */
	PAD_CFG_GPI_APIC_LOW(GPP_E13, NONE, PLTRST),
	/* GPP_E14 - DDSP_HPDA */
	PAD_CFG_NF(GPP_E14, NONE, DEEP, NF1),
	PAD_NC(GPP_E15, NONE),
	PAD_NC(GPP_E16, NONE),
	PAD_NC(GPP_E17, NONE),
	PAD_NC(GPP_E18, NATIVE),
	PAD_NC(GPP_E19, NATIVE),
	PAD_NC(GPP_E20, NATIVE),
	PAD_NC(GPP_E21, NATIVE),
	/* GPP_E22 - DDPA_CTRLCLK */
	PAD_CFG_NF(GPP_E22, DN_20K, DEEP, NF1),
	/* GPP_E23 - DDPA_CTRLDATA */
	PAD_CFG_NF(GPP_E23, NONE, DEEP, NF1),
	/* GPP_E_CLK_LOOPBK - GPIO */
	PAD_NC(GPP_E_CLK_LOOPBK, NONE),
	/* ------- GPIO Community 5 ------- */

	/* ------- GPIO Group GPP_R ------- */

	PAD_NC(GPP_R0, NONE),
	PAD_NC(GPP_R1, NATIVE),
	PAD_NC(GPP_R2, NATIVE),
	PAD_NC(GPP_R3, NATIVE),
	PAD_NC(GPP_R4, NONE),
	PAD_NC(GPP_R5, NONE),
	PAD_NC(GPP_R6, NONE),
	PAD_NC(GPP_R7, NONE),
};

const struct pad_config *board_gpio_table(size_t *num)
{
	*num = ARRAY_SIZE(gpio_table);
	return gpio_table;
}
