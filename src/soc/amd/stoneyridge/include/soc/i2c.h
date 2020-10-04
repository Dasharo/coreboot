/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __STONEYRIDGE_I2C_H__
#define __STONEYRIDGE_I2C_H__

#include <types.h>
#include <soc/gpio.h>

struct soc_amd_i2c_save {
	uint32_t control_value;
	uint8_t mux_value;
};

#define GPIO_I2C0_SCL			BIT(0)
#define GPIO_I2C1_SCL			BIT(1)
#define GPIO_I2C2_SCL			BIT(2)
#define GPIO_I2C3_SCL			BIT(3)
#define GPIO_I2C_MASK			(BIT(0) | BIT(1) | BIT(2) | BIT(3))

#define I2C0_SCL_PIN			GPIO_145
#define I2C1_SCL_PIN			GPIO_147
#define I2C2_SCL_PIN			GPIO_113
#define I2C3_SCL_PIN			GPIO_19

#define I2C0_SCL_PIN_IOMUX_GPIOxx	GPIO_145_IOMUX_GPIOxx
#define I2C1_SCL_PIN_IOMUX_GPIOxx	GPIO_147_IOMUX_GPIOxx
#define I2C2_SCL_PIN_IOMUX_GPIOxx	GPIO_113_IOMUX_GPIOxx
#define I2C3_SCL_PIN_IOMUX_GPIOxx	GPIO_19_IOMUX_GPIOxx

void sb_reset_i2c_slaves(void);

#endif /* __STONEYRIDGE_I2C_H__ */
