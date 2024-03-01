#include "i2c_general.h"

// Write a byte of data to a register via I2C
void i2c_write_reg(i2c_inst_t * i2c_inst, uint8_t addr, const uint8_t reg, const uint8_t src)
{
    uint8_t data[2] = {reg, src};
    i2c_write_blocking(i2c_inst, addr, data, 2, false);
}

// Read a sequence of registers via I2C
void i2c_read_regs(i2c_inst_t * i2c_inst, uint8_t addr, const uint8_t start, uint8_t* dst, size_t len)
{
    i2c_write_blocking(i2c_inst, addr, &start, 1, true);
    i2c_read_blocking(i2c_inst, addr, dst, len, false);
}