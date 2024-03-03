#ifndef I2C_GENERAL_H
#define I2C_GENERAL_H

#include "hardware/i2c.h"
#include "hardware/dma.h"

typedef enum {
    I2C_GENERAL_RC_OK = 0,
    I2C_GENERAL_RC_INVALID_ARG = -1,
    I2C_GENERAL_RC_WRITE_FAILURE = -2,
    I2C_GENERAL_RC_READ_FAILURE = -3,
    I2C_GENERAL_RC_DMA_FAILURE = -4,
} i2c_general_rc_t;

i2c_general_rc_t i2c_write_reg(i2c_inst_t *i2c_inst, uint8_t addr, const uint8_t reg, const uint8_t src);
i2c_general_rc_t i2c_read_regs(i2c_inst_t *i2c_inst, uint8_t addr, const uint8_t start, uint8_t *dst, size_t len);
i2c_general_rc_t i2c_read_regs_dma_start(i2c_inst_t* i2c_inst, uint8_t addr, const uint8_t start, volatile uint8_t* dst, size_t len, int * dma_channel);
i2c_general_rc_t i2c_dma_finish(i2c_inst_t* i2c_inst, int dma_channel, bool * is_finished);

#endif // I2C_GENERAL_H