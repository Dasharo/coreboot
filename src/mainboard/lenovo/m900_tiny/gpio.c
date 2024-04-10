/* SPDX-License-Identifier: GPL-2.0-only */

#include <mainboard/gpio.h>
#include <soc/gpio.h>

/* Pad configuration was generated automatically using intelp2m utility */
static const struct pad_config gpio_table[] = {
	/* ------- GPIO Community 0 ------- */

	/* ------- GPIO Group GPP_A ------- */

	PAD_CFG_NF(GPP_A0, NONE, PLTRST, NF1),
	PAD_CFG_NF(GPP_A1, UP_20K, PLTRST, NF1),
	PAD_CFG_NF(GPP_A2, UP_20K, PLTRST, NF1),
	PAD_CFG_NF(GPP_A3, UP_20K, PLTRST, NF1),
	PAD_CFG_NF(GPP_A4, UP_20K, PLTRST, NF1),
	PAD_CFG_NF(GPP_A5, NONE, PLTRST, NF1),
	PAD_CFG_NF(GPP_A6, NONE, PLTRST, NF1),
	PAD_CFG_NF(GPP_A7, NONE, PLTRST, NF1),
	PAD_CFG_NF(GPP_A8, NONE, PLTRST, NF1),
	PAD_CFG_NF(GPP_A9, DN_20K, PLTRST, NF1),
	PAD_CFG_NF(GPP_A10, DN_20K, PLTRST, NF1),
	PAD_CFG_NF(GPP_A11, NONE, PLTRST, NF1),
	PAD_NC(GPP_A12, NONE),
	PAD_CFG_NF(GPP_A13, NONE, DEEP, NF1),
	PAD_CFG_NF(GPP_A14, NONE, DEEP, NF1),
	PAD_CFG_NF(GPP_A15, UP_20K, DEEP, NF1),
	PAD_NC(GPP_A16, NONE),
	PAD_CFG_GPO(GPP_A17, 1, PLTRST),
	PAD_NC(GPP_A18, NONE),
	PAD_CFG_GPI_TRIG_OWN(GPP_A19, NONE, PLTRST, OFF, ACPI),
	PAD_NC(GPP_A20, NONE),
	PAD_NC(GPP_A21, NONE),
	PAD_CFG_GPI_TRIG_OWN(GPP_A22, NONE, PLTRST, OFF, ACPI),
	PAD_CFG_GPO(GPP_A23, 1, PLTRST),

	/* ------- GPIO Group GPP_B ------- */

	PAD_CFG_GPO(GPP_B0, 0, DEEP),
	PAD_CFG_GPO(GPP_B1, 0, DEEP),
	PAD_CFG_NF(GPP_B2, NONE, PLTRST, NF1),
	PAD_CFG_GPI_TRIG_OWN(GPP_B3, NONE, PLTRST, OFF, ACPI),
	PAD_CFG_GPI_TRIG_OWN(GPP_B4, NONE, PLTRST, OFF, ACPI),
	PAD_NC(GPP_B5, NONE),
	PAD_CFG_NF(GPP_B6, NONE, DEEP, NF1),
	PAD_NC(GPP_B7, NONE),
	PAD_NC(GPP_B8, NONE),
	PAD_NC(GPP_B9, NONE),
	PAD_CFG_GPI_TRIG_OWN(GPP_B10, NONE, PLTRST, OFF, ACPI),
	PAD_CFG_GPO(GPP_B11, 0, DEEP),
	PAD_CFG_NF(GPP_B12, NONE, DEEP, NF1),
	PAD_CFG_NF(GPP_B13, NONE, DEEP, NF1),
	PAD_CFG_NF(GPP_B14, DN_20K, PLTRST, NF1),
	PAD_CFG_GPI_TRIG_OWN(GPP_B15, NONE, PLTRST, OFF, ACPI),
	PAD_CFG_GPI_TRIG_OWN(GPP_B16, NONE, PLTRST, OFF, ACPI),
	PAD_CFG_GPI_TRIG_OWN(GPP_B17, NONE, PLTRST, OFF, ACPI),
	PAD_CFG_GPO(GPP_B18, 0, PLTRST),
	PAD_CFG_NF(GPP_B19, NONE, PLTRST, NF1),
	PAD_CFG_GPI_TRIG_OWN(GPP_B20, NONE, PLTRST, OFF, ACPI),
	PAD_CFG_GPI_TRIG_OWN(GPP_B21, NONE, PLTRST, OFF, ACPI),
	PAD_CFG_GPO(GPP_B22, 0, PLTRST),
	PAD_CFG_NF(GPP_B23, DN_20K, PLTRST, NF1),

	/* ------- GPIO Community 1 ------- */

	/* ------- GPIO Group GPP_C ------- */

	PAD_CFG_NF(GPP_C0, NONE, DEEP, NF1),
	PAD_CFG_NF(GPP_C1, NONE, DEEP, NF1),
	PAD_CFG_GPI_TRIG_OWN(GPP_C2, NONE, DEEP, OFF, ACPI),
	PAD_CFG_NF(GPP_C3, NONE, DEEP, NF1),
	PAD_CFG_NF(GPP_C4, NONE, DEEP, NF1),
	PAD_CFG_NF(GPP_C5, NONE, PLTRST, NF1),
	/* GPP_C6 - RESERVED */
	/* GPP_C7 - RESERVED */
	PAD_CFG_GPI_TRIG_OWN(GPP_C8, NONE, PLTRST, OFF, ACPI),
	PAD_NC(GPP_C9, NONE),
	PAD_NC(GPP_C10, NONE),
	PAD_NC(GPP_C11, NONE),
	PAD_NC(GPP_C12, NONE),
	PAD_CFG_GPI_TRIG_OWN(GPP_C13, NONE, PLTRST, OFF, ACPI),
	PAD_NC(GPP_C14, NONE),
	PAD_CFG_GPO(GPP_C15, 1, PLTRST),
	PAD_CFG_NF(GPP_C16, NONE, DEEP, NF1),
	PAD_CFG_NF(GPP_C17, NONE, DEEP, NF1),
	PAD_CFG_GPI_TRIG_OWN(GPP_C18, NONE, PLTRST, OFF, ACPI),
	PAD_CFG_GPI_TRIG_OWN(GPP_C19, NONE, PLTRST, OFF, ACPI),
	PAD_NC(GPP_C22, NONE),
	PAD_CFG_GPO(GPP_C23, 0, PLTRST),

	/* ------- GPIO Group GPP_D ------- */

	PAD_NC(GPP_D0, NONE),
	PAD_NC(GPP_D1, NONE),
	PAD_NC(GPP_D2, NONE),
	PAD_NC(GPP_D3, NONE),
	PAD_NC(GPP_D4, NONE),
	PAD_NC(GPP_D5, NONE),
	PAD_NC(GPP_D6, NONE),
	PAD_NC(GPP_D7, NONE),
	PAD_NC(GPP_D8, NONE),
	PAD_CFG_GPO(GPP_D9, 1, PLTRST),
	PAD_CFG_GPO(GPP_D10, 1, PLTRST),
	PAD_NC(GPP_D11, NONE),
	PAD_NC(GPP_D12, NONE),
	PAD_CFG_GPI_TRIG_OWN(GPP_D13, NONE, PLTRST, OFF, ACPI),
	PAD_CFG_GPI_TRIG_OWN(GPP_D14, NONE, PLTRST, OFF, ACPI),
	PAD_CFG_GPI_TRIG_OWN(GPP_D15, NONE, PLTRST, OFF, ACPI),
	PAD_CFG_GPI_TRIG_OWN(GPP_D16, NONE, PLTRST, OFF, ACPI),
	PAD_CFG_GPI_TRIG_OWN(GPP_D17, NONE, PLTRST, OFF, ACPI),
	PAD_CFG_GPI_TRIG_OWN(GPP_D18, NONE, PLTRST, OFF, ACPI),
	PAD_NC(GPP_D19, NONE),
	PAD_NC(GPP_D20, NONE),
	PAD_NC(GPP_D21, NONE),
	PAD_NC(GPP_D22, NONE),
	PAD_CFG_GPI_TRIG_OWN(GPP_D23, NONE, PLTRST, OFF, ACPI),

	/* ------- GPIO Group GPP_E ------- */

	PAD_CFG_GPI_TRIG_OWN(GPP_E0, NONE, PLTRST, OFF, ACPI),
	PAD_NC(GPP_E1, NONE),
	PAD_CFG_GPI_TRIG_OWN(GPP_E2, NONE, PLTRST, OFF, ACPI),
	PAD_CFG_GPI_TRIG_OWN(GPP_E3, NONE, PLTRST, OFF, ACPI),
	PAD_NC(GPP_E4, NONE),
	PAD_NC(GPP_E5, NONE),
	PAD_NC(GPP_E6, NONE),
	PAD_CFG_GPI_TRIG_OWN(GPP_E7, NONE, PLTRST, OFF, ACPI),
	PAD_CFG_NF(GPP_E8, NONE, PLTRST, NF1),
	PAD_CFG_NF(GPP_E9, NONE, DEEP, NF1),
	PAD_CFG_NF(GPP_E10, NONE, DEEP, NF1),
	PAD_CFG_NF(GPP_E11, NONE, DEEP, NF1),
	PAD_CFG_NF(GPP_E12, NONE, DEEP, NF1),

	/* ------- GPIO Group GPP_F ------- */

	PAD_NC(GPP_F0, NONE),
	PAD_CFG_NF(GPP_F1, NONE, DEEP, NF1),
	PAD_CFG_NF(GPP_F2, NONE, DEEP, NF1),
	PAD_NC(GPP_F3, NONE),
	PAD_NC(GPP_F4, NONE),
	PAD_CFG_GPI_SCI(GPP_F5, NONE, DEEP, EDGE_SINGLE, NONE),
	PAD_CFG_GPI_TRIG_OWN(GPP_F6, NONE, RSMRST, OFF, ACPI),
	PAD_NC(GPP_F7, NONE),
	PAD_NC(GPP_F8, NONE),
	PAD_NC(GPP_F9, NONE),
	PAD_CFG_GPI_APIC_HIGH(GPP_F10, NONE, PLTRST),
	PAD_CFG_GPI_TRIG_OWN(GPP_F11, NONE, PLTRST, OFF, ACPI),
	PAD_CFG_GPI_APIC_HIGH(GPP_F12, NONE, PLTRST),
	PAD_CFG_GPI_APIC_HIGH(GPP_F13, NONE, PLTRST),
	PAD_CFG_GPI_APIC_HIGH(GPP_F14, NONE, DEEP),
	PAD_CFG_NF(GPP_F15, NONE, DEEP, NF1),
	PAD_NC(GPP_F16, NONE),
	PAD_NC(GPP_F17, NONE),
	PAD_NC(GPP_F18, NONE),
	PAD_CFG_GPI_TRIG_OWN(GPP_F19, NONE, PLTRST, OFF, ACPI),
	PAD_CFG_GPI_TRIG_OWN(GPP_F20, NONE, PLTRST, OFF, ACPI),
	PAD_CFG_GPI_TRIG_OWN(GPP_F21, NONE, PLTRST, OFF, ACPI),
	PAD_NC(GPP_F22, NONE),
	PAD_NC(GPP_F23, NONE),

	/* ------- GPIO Group GPP_G ------- */

	PAD_NC(GPP_G0, NONE),
	PAD_NC(GPP_G1, NONE),
	PAD_NC(GPP_G2, NONE),
	PAD_NC(GPP_G3, NONE),
	PAD_NC(GPP_G4, NONE),
	PAD_NC(GPP_G5, NONE),
	PAD_NC(GPP_G6, NONE),
	PAD_NC(GPP_G7, NONE),
	PAD_CFG_NF(GPP_G8, NONE, DEEP, NF1),
	PAD_CFG_NF(GPP_G9, NONE, DEEP, NF1),
	PAD_CFG_NF(GPP_G10, NONE, DEEP, NF1),
	PAD_CFG_NF(GPP_G11, NONE, DEEP, NF1),
	PAD_CFG_GPI_TRIG_OWN(GPP_G12, NONE, PLTRST, OFF, ACPI),
	PAD_CFG_GPI_TRIG_OWN(GPP_G13, NONE, PLTRST, OFF, ACPI),
	PAD_CFG_GPI_TRIG_OWN(GPP_G14, NONE, PLTRST, OFF, ACPI),
	PAD_CFG_GPI_TRIG_OWN(GPP_G15, NONE, PLTRST, OFF, ACPI),
	PAD_CFG_GPI_TRIG_OWN(GPP_G16, NONE, PLTRST, OFF, ACPI),
	PAD_CFG_GPI_TRIG_OWN(GPP_G17, NONE, PLTRST, OFF, ACPI),
	PAD_CFG_GPO(GPP_G18, 0, PLTRST),
	PAD_CFG_GPI_SCI(GPP_G19, NONE, PLTRST, LEVEL, INVERT),
	PAD_CFG_GPI_TRIG_OWN(GPP_G20, NONE, PLTRST, OFF, ACPI),
	PAD_CFG_GPI_TRIG_OWN(GPP_G21, NONE, PLTRST, OFF, ACPI),
	PAD_NC(GPP_G22, NONE),
	PAD_NC(GPP_G23, NONE),

	/* ------- GPIO Group GPP_H ------- */

	PAD_NC(GPP_H0, NONE),
	PAD_NC(GPP_H1, NONE),
	PAD_NC(GPP_H2, NONE),
	PAD_NC(GPP_H3, NONE),
	PAD_CFG_GPI_TRIG_OWN(GPP_H4, NONE, PLTRST, OFF, ACPI),
	PAD_CFG_NF(GPP_H5, NONE, DEEP, NF1),
	PAD_CFG_GPI_TRIG_OWN(GPP_H6, NONE, PLTRST, OFF, ACPI),
	PAD_NC(GPP_H7, NONE),
	PAD_NC(GPP_H8, NONE),
	PAD_CFG_GPI_TRIG_OWN(GPP_H9, NONE, PLTRST, OFF, ACPI),
	PAD_NC(GPP_H10, NONE),
	PAD_CFG_GPO(GPP_H11, 1, PLTRST),
	PAD_CFG_GPO(GPP_H12, 0, PLTRST),
	PAD_NC(GPP_H13, NONE),
	PAD_NC(GPP_H14, NONE),
	PAD_NC(GPP_H15, NONE),
	PAD_NC(GPP_H16, NONE),
	PAD_NC(GPP_H17, NONE),
	PAD_NC(GPP_H18, NONE),
	PAD_CFG_GPI_TRIG_OWN(GPP_H19, NONE, PLTRST, OFF, ACPI),
	PAD_CFG_GPI_TRIG_OWN(GPP_H20, NONE, PLTRST, OFF, ACPI),
	PAD_CFG_GPO(GPP_H21, 1, PLTRST),
	PAD_CFG_GPI_TRIG_OWN(GPP_H22, NONE, PLTRST, OFF, ACPI),
	PAD_CFG_GPI_TRIG_OWN(GPP_H23, NONE, PLTRST, OFF, ACPI),

	/* ------- GPIO Community 2 ------- */

	/* -------- GPIO Group GPD -------- */

	PAD_CFG_GPI_TRIG_OWN(GPD0, NONE, PLTRST, OFF, ACPI),
	PAD_CFG_GPI_TRIG_OWN(GPD1, NONE, PWROK, OFF, ACPI),
	PAD_CFG_NF(GPD2, DN_20K, PWROK, NF1),
	PAD_CFG_NF(GPD3, UP_20K, PWROK, NF1),
	PAD_CFG_NF(GPD4, NONE, PWROK, NF1),
	PAD_CFG_NF(GPD5, NONE, PWROK, NF1),
	PAD_CFG_NF(GPD6, NONE, PWROK, NF1),
	PAD_CFG_GPO(GPD7, 1, PWROK),
	PAD_CFG_NF(GPD8, NONE, PWROK, NF1),
	PAD_CFG_NF(GPD9, NONE, PWROK, NF1),
	PAD_CFG_NF(GPD10, NONE, PWROK, NF1),
	PAD_CFG_NF(GPD11, NONE, PWROK, NF1),

	/* ------- GPIO Community 3 ------- */

	/* ------- GPIO Group GPP_I ------- */

	PAD_CFG_NF(GPP_I0, NONE, PLTRST, NF1),
	PAD_CFG_NF(GPP_I1, NONE, PLTRST, NF1),
	PAD_CFG_NF(GPP_I2, NONE, PLTRST, NF1),
	PAD_CFG_GPI_SMI(GPP_I3, NONE, PLTRST, EDGE_SINGLE, NONE),
	PAD_CFG_GPI_TRIG_OWN(GPP_I4, NONE, PLTRST, OFF, ACPI),
	PAD_CFG_NF(GPP_I5, NONE, PLTRST, NF1),
	PAD_CFG_NF(GPP_I6, DN_20K, PLTRST, NF1),
	PAD_CFG_NF(GPP_I7, NONE, PLTRST, NF1),
	PAD_CFG_NF(GPP_I8, DN_20K, PLTRST, NF1),
	PAD_CFG_NF(GPP_I9, NONE, PLTRST, NF1),
	PAD_CFG_NF(GPP_I10, DN_20K, PLTRST, NF1),
};

void mainboard_configure_gpios(void)
{
	gpio_configure_pads(gpio_table, ARRAY_SIZE(gpio_table));
}