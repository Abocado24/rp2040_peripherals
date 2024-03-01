#ifndef I2C_GENERAL_H
#define I2C_GENERAL_H

#include "hardware/i2c.h"

void i2c_write_reg(i2c_inst_t *i2c_inst, uint8_t addr, const uint8_t reg, const uint8_t src);
void i2c_read_regs(i2c_inst_t *i2c_inst, uint8_t addr, const uint8_t start, uint8_t *dst, size_t len);

#endif // I2C_GENERAL_H