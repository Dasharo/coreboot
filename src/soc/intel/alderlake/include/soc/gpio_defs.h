/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef _SOC_ALDERLAKE_GPIO_DEFS_H_
#define _SOC_ALDERLAKE_GPIO_DEFS_H_

#ifndef __ACPI__
#include <stddef.h>
#endif
#include <soc/gpio_soc_defs.h>

#define GPIO_NUM_PAD_CFG_REGS   4 /* DW0, DW1, DW2, DW3 */

#define NUM_GPIO_COMx_GPI_REGS(n)	\
		(ALIGN_UP((n), GPIO_MAX_NUM_PER_GROUP) / GPIO_MAX_NUM_PER_GROUP)

#define NUM_GPIO_COM0_GPI_REGS NUM_GPIO_COMx_GPI_REGS(NUM_GPIO_COM0_PADS)
#define NUM_GPIO_COM1_GPI_REGS NUM_GPIO_COMx_GPI_REGS(NUM_GPIO_COM1_PADS)
#define NUM_GPIO_COM2_GPI_REGS NUM_GPIO_COMx_GPI_REGS(NUM_GPIO_COM2_PADS)
#define NUM_GPIO_COM4_GPI_REGS NUM_GPIO_COMx_GPI_REGS(NUM_GPIO_COM4_PADS)
#define NUM_GPIO_COM5_GPI_REGS NUM_GPIO_COMx_GPI_REGS(NUM_GPIO_COM5_PADS)

#define NUM_GPI_STATUS_REGS	\
		((NUM_GPIO_COM0_GPI_REGS) +\
		(NUM_GPIO_COM1_GPI_REGS) +\
		(NUM_GPIO_COM2_GPI_REGS) +\
		(NUM_GPIO_COM4_GPI_REGS) +\
		(NUM_GPIO_COM5_GPI_REGS))
/*
 * IOxAPIC IRQs for the GPIOs
 */

/* Group T */
#define GPP_T0_IRQ				0x30
#define GPP_T1_IRQ				0x31
#define GPP_T2_IRQ				0x32
#define GPP_T3_IRQ				0x33
#define GPP_T4_IRQ				0x34
#define GPP_T5_IRQ				0x35
#define GPP_T6_IRQ				0x36
#define GPP_T7_IRQ				0x37
#define GPP_T8_IRQ				0x38
#define GPP_T9_IRQ				0x39
#define GPP_T10_IRQ				0x3A
#define GPP_T11_IRQ				0x3B
#define GPP_T12_IRQ				0x3C
#define GPP_T13_IRQ				0x3D
#define GPP_T14_IRQ				0x3E
#define GPP_T15_IRQ				0x3F

/* Group A */
#define GPP_A0_IRQ				0x40
#define GPP_A1_IRQ				0x41
#define GPP_A2_IRQ				0x42
#define GPP_A3_IRQ				0x43
#define GPP_A4_IRQ				0x44
#define GPP_A5_IRQ				0x45
#define GPP_A6_IRQ				0x46
#define GPP_A7_IRQ				0x47
#define GPP_A8_IRQ				0x48
#define GPP_A9_IRQ				0x49
#define GPP_A10_IRQ				0x4A
#define GPP_A11_IRQ				0x4B
#define GPP_A12_IRQ				0x4C
#define GPP_A13_IRQ				0x4D
#define GPP_A14_IRQ				0x4E
#define GPP_A15_IRQ				0x4F
#define GPP_A16_IRQ				0x50
#define GPP_A17_IRQ				0x51
#define GPP_A18_IRQ				0x52
#define GPP_A19_IRQ				0x53
#define GPP_A20_IRQ				0x54
#define GPP_A21_IRQ				0x55
#define GPP_A22_IRQ				0x56
#define GPP_A23_IRQ				0x57

/* Group B */
#define GPP_B0_IRQ				0x18
#define GPP_B1_IRQ				0x19
#define GPP_B2_IRQ				0x1A
#define GPP_B3_IRQ				0x1B
#define GPP_B4_IRQ				0x1C
#define GPP_B5_IRQ				0x1D
#define GPP_B6_IRQ				0x1E
#define GPP_B7_IRQ				0x1F
#define GPP_B8_IRQ				0x20
#define GPP_B9_IRQ				0x21
#define GPP_B10_IRQ				0x22
#define GPP_B11_IRQ				0x23
#define GPP_B12_IRQ				0x24
#define GPP_B13_IRQ				0x25
#define GPP_B14_IRQ				0x26
#define GPP_B15_IRQ				0x27
#define GPP_B16_IRQ				0x28
#define GPP_B17_IRQ				0x29
#define GPP_B18_IRQ				0x2A
#define GPP_B19_IRQ				0x2B
#define GPP_B20_IRQ				0x2C
#define GPP_B21_IRQ				0x2D
#define GPP_B22_IRQ				0x2E
#define GPP_B23_IRQ				0x2F

/* Group C */
#define GPP_C0_iIRQ				0x6E
#define GPP_C1_IRQ				0x6F
#define GPP_C2_IRQ				0x70
#define GPP_C3_IRQ				0x71
#define GPP_C4_IRQ				0x72
#define GPP_C5_IRQ				0x73
#define GPP_C6_IRQ				0x74
#define GPP_C7_IRQ				0x75
#define GPP_C8_IRQ				0x76
#define GPP_C9_IRQ				0x77
#define GPP_C10_IRQ				0x18
#define GPP_C11_IRQ				0x19
#define GPP_C12_IRQ				0x1A
#define GPP_C13_IRQ				0x1B
#define GPP_C14_IRQ				0x1C
#define GPP_C15_IRQ				0x1D
#define GPP_C16_IRQ				0x1E
#define GPP_C17_IRQ				0x1F
#define GPP_C18_IRQ				0x20
#define GPP_C19_IRQ				0x21
#define GPP_C20_IRQ				0x22
#define GPP_C21_IRQ				0x23
#define GPP_C22_IRQ				0x24
#define GPP_C23_IRQ				0x25

/* Group D */
#define GPP_D0_IRQ				0x2C
#define GPP_D1_IRQ				0x2D
#define GPP_D2_IRQ				0x2E
#define GPP_D3_IRQ				0x2F
#define GPP_D4_IRQ				0x30
#define GPP_D5_IRQ				0x31
#define GPP_D6_IRQ				0x32
#define GPP_D7_IRQ				0x33
#define GPP_D8_IRQ				0x34
#define GPP_D9_IRQ				0x35
#define GPP_D10_IRQ				0x36
#define GPP_D11_IRQ				0x37
#define GPP_D12_IRQ				0x38
#define GPP_D13_IRQ				0x39
#define GPP_D14_IRQ				0x3A
#define GPP_D15_IRQ				0x3B
#define GPP_D16_IRQ				0x3C
#define GPP_D17_IRQ				0x3D
#define GPP_D18_IRQ				0x3E
#define GPP_D19_IRQ				0x3F

/* Group E */
#define GPP_E0_IRQ				0x26
#define GPP_E1_IRQ				0x27
#define GPP_E2_IRQ				0x28
#define GPP_E3_IRQ				0x29
#define GPP_E4_IRQ				0x30
#define GPP_E5_IRQ				0x31
#define GPP_E6_IRQ				0x32
#define GPP_E7_IRQ				0x33
#define GPP_E8_IRQ				0x34
#define GPP_E9_IRQ				0x35
#define GPP_E10_IRQ				0x36
#define GPP_E11_IRQ				0x37
#define GPP_E12_IRQ				0x38
#define GPP_E13_IRQ				0x39
#define GPP_E14_IRQ				0x3A
#define GPP_E15_IRQ				0x3B
#define GPP_E16_IRQ				0x3C
#define GPP_E17_IRQ				0x3D
#define GPP_E18_IRQ				0x3E
#define GPP_E19_IRQ				0x3F
#define GPP_E20_IRQ				0x40
#define GPP_E21_IRQ				0x41
#define GPP_E22_IRQ				0x42
#define GPP_E23_IRQ				0x43

/* Group F */
#define GPP_F0_IRQ				0x56
#define GPP_F1_IRQ				0x57
#define GPP_F2_IRQ				0x58
#define GPP_F3_IRQ				0x59
#define GPP_F4_IRQ				0x5A
#define GPP_F5_IRQ				0x5B
#define GPP_F6_IRQ				0x5C
#define GPP_F7_IRQ				0x5D
#define GPP_F8_IRQ				0x5E
#define GPP_F9_IRQ				0x5F
#define GPP_F10_IRQ				0x60
#define GPP_F11_IRQ				0x61
#define GPP_F12_IRQ				0x62
#define GPP_F13_IRQ				0x63
#define GPP_F14_IRQ				0x64
#define GPP_F15_IRQ				0x65
#define GPP_F16_IRQ				0x66
#define GPP_F17_IRQ				0x67
#define GPP_F18_IRQ				0x68
#define GPP_F19_IRQ				0x69
#define GPP_F20_IRQ				0x6A
#define GPP_F21_IRQ				0x6B
#define GPP_F22_IRQ				0x6C
#define GPP_F23_IRQ				0x6D

/* Group H */
#define GPP_H0_IRQ				0x74
#define GPP_H1_IRQ				0x75
#define GPP_H2_IRQ				0x76
#define GPP_H3_IRQ				0x77
#define GPP_H4_IRQ				0x18
#define GPP_H5_IRQ				0x19
#define GPP_H6_IRQ				0x1A
#define GPP_H7_IRQ				0x1B
#define GPP_H8_IRQ				0x1C
#define GPP_H9_IRQ				0x1D
#define GPP_H10_IRQ				0x1E
#define GPP_H11_IRQ				0x1F
#define GPP_H12_IRQ				0x20
#define GPP_H13_IRQ				0x21
#define GPP_H14_IRQ				0x22
#define GPP_H15_IRQ				0x23
#define GPP_H16_IRQ				0x24
#define GPP_H17_IRQ				0x25
#define GPP_H18_IRQ				0x26
#define GPP_H19_IRQ				0x27
#define GPP_H20_IRQ				0x28
#define GPP_H21_IRQ				0x29
#define GPP_H22_IRQ				0x2A
#define GPP_H23_IRQ				0x2B

/* Group R */
#define GPP_R0_IRQ				0x58
#define GPP_R1_IRQ				0x59
#define GPP_R2_IRQ				0x5A
#define GPP_R3_IRQ				0x5B
#define GPP_R4_IRQ				0x5C
#define GPP_R5_IRQ				0x5D
#define GPP_R6_IRQ				0x5E
#define GPP_R7_IRQ				0x5F

/* Group S */
#define GPP_S0_IRQ				0x6C
#define GPP_S1_IRQ				0x6D
#define GPP_S2_IRQ				0x6E
#define GPP_S3_IRQ				0x6F
#define GPP_S4_IRQ				0x70
#define GPP_S5_IRQ				0x71
#define GPP_S6_IRQ				0x72
#define GPP_S7_IRQ				0x73

/* Group GPD */
#define GPD0_IRQ				0x60
#define GPD1_IRQ				0x61
#define GPD2_IRQ				0x62
#define GPD3_IRQ				0x63
#define GPD4_IRQ				0x64
#define GPD5_IRQ				0x65
#define GPD6_IRQ				0x66
#define GPD7_IRQ				0x67
#define GPD8_IRQ				0x68
#define GPD9_IRQ				0x69
#define GPD10_IRQ				0x6A
#define GPD11_IRQ				0x6B

/* Register defines. */
#define GPIO_MISCCFG				0x10
#define  GPE_DW_SHIFT				8
#define  GPE_DW_MASK				0xfff00
#define HOSTSW_OWN_REG_0			0xb0
#define GPI_INT_STS_0				0x100
#define GPI_INT_EN_0				0x110
#define GPI_SMI_STS_0				0x180
#define GPI_SMI_EN_0				0x1A0
#define PAD_CFG_BASE				0x700

#endif
