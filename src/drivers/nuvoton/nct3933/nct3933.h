/* SPDX-License-Identifier: BSD-3-Clause */

#ifndef NCT3933_H
#define NCT3933_H

#include <types.h>

#define NCT3933_OUT_DAC_REG(x)		(x)
#define   NCT3933_CURRENT_VAL_MASK	0x7f
#define   NCT3933_CURRENT_SOURCE	(1 << 7)

/* SMBus operationss struct */
struct nct3933_smbus_ops {
	int (*read_byte)(uint8_t address, uint8_t command, uint8_t *data);
	int (*write_byte)(uint8_t address, uint8_t command, uint8_t data);
};

int nct3933_probe(const struct nct3933_smbus_ops *ops, uint8_t addr);
int nct3933_set_current_dac(const struct nct3933_smbus_ops *ops, uint8_t addr, uint8_t out_no,
			    uint8_t value);
int nct3933_get_current_dac(const struct nct3933_smbus_ops *ops, uint8_t addr, uint8_t out_no,
			    uint8_t *value);
int nct3933_set_current_x2(const struct nct3933_smbus_ops *ops, uint8_t addr, uint8_t out_no,
			   bool enable);
int nct3933_set_power_saving(const struct nct3933_smbus_ops *ops, uint8_t addr, uint8_t out_no,
			     bool enable);

#endif /* NCT3933_H */
