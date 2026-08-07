/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef SUPERIO_NUVOTON_NCT6687D_SMBUS_H
#define SUPERIO_NUVOTON_NCT6687D_SMBUS_H

#include <types.h>

void nct6687d_smbus_init(uint16_t hwmbase);
int nct6687d_smbus_read_byte(uint8_t address, uint8_t command, uint8_t *data);
int nct6687d_smbus_read_word(uint8_t address, uint8_t command, uint16_t *data);
int nct6687d_smbus_write_byte(uint8_t address, uint8_t command, uint8_t data);
int nct6687d_smbus_write_word(uint8_t address, uint8_t command, uint16_t data);
int nct6687d_smbus_block_read(uint8_t address, uint8_t command, uint8_t *data, uint8_t *length);
int nct6687d_smbus_block_write(uint8_t address, uint8_t command, uint8_t *data, uint8_t length);

#endif /* SUPERIO_NUVOTON_NCT6687D_SMBUS_H */
