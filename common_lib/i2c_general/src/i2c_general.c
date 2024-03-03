#include "i2c_general.h"

// Write a byte of data to a register via I2C
i2c_general_rc_t i2c_write_reg(i2c_inst_t * i2c_inst, uint8_t addr, const uint8_t reg, const uint8_t src) {
    // Catch invalid argument
    if(!i2c_inst) {
        return I2C_GENERAL_RC_INVALID_ARG;
    }

    // Write data to register
    uint8_t data[2] = {reg, src};
    int write_result = i2c_write_blocking(i2c_inst, addr, data, 2, false);
    // Indicate I2C write failure if it occurs
    if(write_result != 1) {
        return I2C_GENERAL_RC_WRITE_FAILURE;
    }

    return I2C_GENERAL_RC_OK;
}

// Read a sequence of registers via I2C
i2c_general_rc_t i2c_read_regs(i2c_inst_t * i2c_inst, uint8_t addr, const uint8_t start, uint8_t* dst, size_t len) {
    // Catch invalid arguments
    if(!i2c_inst || !dst || len == 0) {
        return I2C_GENERAL_RC_INVALID_ARG;
    }

    // Write requested register address to start reading from
    int write_result = i2c_write_blocking(i2c_inst, addr, &start, 1, true);
    // Indicate I2C write failure if it occurs
    if(write_result != 1) {
        return I2C_GENERAL_RC_WRITE_FAILURE;
    }

    // Read the contiguous block of data
    int read_result = i2c_read_blocking(i2c_inst, addr, dst, len, false);
    // Indicate I2C read failure if it occurs
    if(read_result != 1) {
        return I2C_GENERAL_RC_READ_FAILURE;
    }

    return I2C_GENERAL_RC_OK;
}

// Initiate a non-blocking read for a sequence of registers via I2C, using DMA
// An unused DMA channel is claimed for the transfer, channel number is returned by reference (dma_channel)
i2c_general_rc_t i2c_read_regs_dma_start(i2c_inst_t* i2c_inst, uint8_t addr, const uint8_t start, volatile uint8_t* dst, size_t len, int * dma_channel) {
    // Catch invalid arguments
    if(!i2c_inst || !dst || len == 0 || !dma_channel) {
        return I2C_GENERAL_RC_INVALID_ARG;
    }

    // Write requested register address to start reading from
    // Is blocking, but typically fast since it's a single byte
    int write_result = i2c_write_blocking(i2c_inst, addr, &start, 1, true);
    // Indicate I2C write failure if it occurs
    if(write_result != 1) {
        return I2C_GENERAL_RC_WRITE_FAILURE;
    }

    // Claim an unused dma channel to use for the transfer
    *dma_channel = dma_claim_unused_channel(true);
    // Indicate DMA failure if it occurs
    if(*dma_channel < 0) {
        return I2C_GENERAL_RC_DMA_FAILURE;
    }

    // Configure DMA channel for the I2C read opperation from the default configuration state
    dma_channel_config cfg = dma_channel_get_default_config(*dma_channel);
    channel_config_set_transfer_data_size(&cfg, DMA_SIZE_8);
    channel_config_set_read_increment(&cfg, false);
    channel_config_set_write_increment(&cfg, true);
    channel_config_set_dreq(&cfg, DREQ_I2C0_RX + i2c_hw_index(i2c_inst));

    // Set up the DMA transfer and start it immediately
    // Will copy received data from I2C data register to recv buffer
    volatile uint32_t * data_register = &i2c_inst->hw->data_cmd;
    dma_channel_configure(*dma_channel, &cfg, dst, data_register, len, true);

    return I2C_GENERAL_RC_OK;
}

// Check if an I2C function using DMA asynchronously has finished
// Completion status is passed by reference through is_finished
i2c_general_rc_t i2c_dma_finish(i2c_inst_t* i2c_inst, int dma_channel, bool * is_finished) {
    // Catch invalid arguments
    if(!i2c_inst || dma_channel < 0 || !is_finished) {
        return I2C_GENERAL_RC_INVALID_ARG;
    }

    // Check if DMA transfer has finished
    if(!dma_channel_is_busy(dma_channel)) {
        // Perform cleanup

        // Free DMA channel for other operations
        dma_channel_unclaim(dma_channel);

        // Reset I2C state if needed
        // Add later if I find that I need it

        *is_finished = true;
    }
    else {
        *is_finished = false;
    }

    return I2C_GENERAL_RC_OK;
}