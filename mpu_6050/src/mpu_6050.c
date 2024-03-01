#include "mpu_6050.h"

// Conversion factors corresponding to the configured gyro and accelerometer ranges
// Use accel_range and gyro_range enums with these arrays to get the correct factor
const double MPU_6050_ACCEL_CONVERSION_FACTORS[] = {16384.0, 8192.0, 4096.0, 2048.0};
const double MPU_6050_GYRO_CONVERSION_FACTORS[] = {131.0, 65.5, 32.8, 16.4};

// Initialize required peripherals for the MPU-6050
// Assumes that I2C is already initialized; sensor id read will fail 
mpu_6050_rc_t mpu_6050_init(mpu_6050_t* mpu_6050, i2c_inst_t * i2c_inst) {
    // Check if the pointer is valid
    if(!mpu_6050) {
        return MPU_6050_RC_ERROR_NULL_INST;
    }

    // Register I2C inst (should be initialized already)
    mpu_6050->i2c_inst = i2c_inst;

    // Get sensor ID using configured I2C
    uint8_t sensor_id;
    i2c_read_regs(mpu_6050->i2c_inst, MPU_6050_ADDR, MPU_6050_WHO_AM_I, &sensor_id, 1);
    mpu_6050->sensor_id = sensor_id;

    if(mpu_6050->sensor_id != MPU_6050_EXPECTED_ID) {
        return MPU_6050_RC_ERROR_BAD_ID;
    }

    return MPU_6050_RC_OK;
}

// Set the MPU 6050 clock source
mpu_6050_rc_t mpu_6050_set_clock_source(mpu_6050_t *mpu_6050, mpu_6050_clock_source_t clock_source) {
    // Check if the pointer is valid
    if(!mpu_6050) {
        return MPU_6050_RC_ERROR_NULL_INST;
    }
    // Check for valid sensor ID
    if(mpu_6050->sensor_id != MPU_6050_EXPECTED_ID) {
        return MPU_6050_RC_ERROR_BAD_ID;
    }

    // Read in current power_management_1 register; change bits in place and write back
    uint8_t power_management_1;
    i2c_read_regs(mpu_6050->i2c_inst, MPU_6050_ADDR, MPU_6050_PWR_MGMT_1, &power_management_1, 1);

    // clock source is bottom 3 bits in power_management_1, mask and write bits accordingly
    power_management_1 &= MPU_6050_CONFIG_CLOCK_SOURCE_MASK;
    power_management_1 |= clock_source;

    // Write power_management_1 back with new clock source
    i2c_write_reg(mpu_6050->i2c_inst, MPU_6050_ADDR, MPU_6050_PWR_MGMT_1, power_management_1);

    // Update clock source in mpu_6050 struct after making change
    mpu_6050->clock_source = clock_source;

    return MPU_6050_RC_OK;
}

// Set the MPU 6050 accelerometer range
mpu_6050_rc_t mpu_6050_set_accel_range(mpu_6050_t *mpu_6050, mpu_6050_accel_range_t range) {
    // Check if the pointer is valid
    if(!mpu_6050) {
        return MPU_6050_RC_ERROR_NULL_INST;
    }
    // Check for valid sensor ID
    if(mpu_6050->sensor_id != MPU_6050_EXPECTED_ID) {
        return MPU_6050_RC_ERROR_BAD_ID;
    }

    // Read in current accel_config register; change bits in place and write back
    uint8_t accel_config;
    i2c_read_regs(mpu_6050->i2c_inst, MPU_6050_ADDR, MPU_6050_ACCEL_CONFIG, &accel_config, 1);

    // accel config is bits 3-4 in accel_config, mask and write bits accordingly
    accel_config &= MPU_6050_CONFIG_ACCEL_MASK;
    accel_config |= (range << 3);

    // Write accel_config back with new range
    i2c_write_reg(mpu_6050->i2c_inst, MPU_6050_ADDR, MPU_6050_ACCEL_CONFIG, accel_config);

    // Update accel range in mpu_6050 struct after making change
    mpu_6050->accel_range = range;

    return MPU_6050_RC_OK;
}

// Set the MPU 6050 gyroscope range
mpu_6050_rc_t mpu_6050_set_gyro_range(mpu_6050_t *mpu_6050, mpu_6050_gyro_range_t range) {
    // Check if the pointer is valid
    if(!mpu_6050) {
        return MPU_6050_RC_ERROR_NULL_INST;
    }
    // Check for valid sensor ID
    if(mpu_6050->sensor_id != MPU_6050_EXPECTED_ID) {
        return MPU_6050_RC_ERROR_BAD_ID;
    }

    // Read in current gyro_config register; change bits in place and write back
    uint8_t gyro_config;
    i2c_read_regs(mpu_6050->i2c_inst, MPU_6050_ADDR, MPU_6050_GYRO_CONFIG, &gyro_config, 1);

    // Gyro config is bits 3-4 in gyro_config, mask and write bits accordingly
    gyro_config &= MPU_6050_CONFIG_GYRO_MASK;
    gyro_config |= (range << 3);

    // Write gyro_config back with new range
    i2c_write_reg(mpu_6050->i2c_inst, MPU_6050_ADDR, MPU_6050_GYRO_CONFIG, gyro_config);

    // Update gyro range in mpu_6050 struct after making change
    mpu_6050->gyro_range = range;

    return MPU_6050_RC_OK;
}

// Set the MPU 6050 sleep mode flag
mpu_6050_rc_t mpu_6050_set_sleep_mode(mpu_6050_t *mpu_6050, mpu_6050_sleep_state_t state) {
    // Check if the pointer is valid
    if(!mpu_6050) {
        return MPU_6050_RC_ERROR_NULL_INST;
    }
    // Check for valid sensor ID
    if(mpu_6050->sensor_id != MPU_6050_EXPECTED_ID) {
        return MPU_6050_RC_ERROR_BAD_ID;
    }

    // Read in current power_management_1 register
    uint8_t power_management_1;
    i2c_read_regs(mpu_6050->i2c_inst, MPU_6050_ADDR, MPU_6050_PWR_MGMT_1, &power_management_1, 1);

    // Sleep mode bit is bit 6 in power_management_1, mask and write bit accordingly
    power_management_1 &= MPU_6050_CONFIG_SLEEP_MODE_MASK;
    power_management_1 |= (state << 6);

    // Write power_management_1 back with new sleep mode flag
    i2c_write_reg(mpu_6050->i2c_inst, MPU_6050_ADDR, MPU_6050_PWR_MGMT_1, power_management_1);

    // Update clock source in mpu_6050 struct after making change
    mpu_6050->sleep_state = state;

    return MPU_6050_RC_OK;
}

// Reset the MPU-6050 to a default state
mpu_6050_rc_t mpu_6050_reset(mpu_6050_t* mpu_6050) {
    // Check if the pointer is valid
    if(!mpu_6050) {
        return MPU_6050_RC_ERROR_NULL_INST;
    }
    // Check for valid sensor ID
    if(mpu_6050->sensor_id != MPU_6050_EXPECTED_ID) {
        return MPU_6050_RC_ERROR_BAD_ID;
    }

    // Reset accel and gyro readings data
    clear_int16_vector(&mpu_6050->accel_raw);
    clear_int16_vector(&mpu_6050->gyro_raw);
    mpu_6050->dt = 0;
    clear_double_vector(&mpu_6050->accel_data);
    clear_double_vector(&mpu_6050->gyro_data);
    clear_double_vector(&mpu_6050->offsets.accel_offsets);
    clear_double_vector(&mpu_6050->offsets.gyro_offsets);

    // Reset device
    i2c_write_reg(mpu_6050->i2c_inst, MPU_6050_ADDR, MPU_6050_PWR_MGMT_1, 0x80);
    sleep_ms(200);

    // set clock source to a default value (internal)
    mpu_6050_set_clock_source(mpu_6050, MPU_6050_CLOCK_INTERNAL);
    sleep_ms(200);

    // set accel range to a default value (2g)
    mpu_6050_set_accel_range(mpu_6050, MPU_6050_ACCEL_2G);
    sleep_ms(200);

    // set gyro range to a default value (250 deg/s)
    mpu_6050_set_gyro_range(mpu_6050, MPU_6050_GYRO_250DPS);
    sleep_ms(200);

    // free fall, motion, zero interrupt flags are left false for now
    // motion and zero motion detection thresholds and durations are 2,5 and 4,2 for now

    // set sleep mode to a default state (off)
    mpu_6050_set_sleep_mode(mpu_6050, MPU_6050_SLEEP_DISABLED);
    sleep_ms(200);

    // wait for the sensors to stabilize
    sleep_ms(5000);

    return MPU_6050_RC_OK;
}

// Calibrate the MPU-6050 by calculating offsets for roll, pitch, and gyro z
// MPU-6050 must be in a fixed position during calibration!
mpu_6050_rc_t mpu_6050_calibrate(mpu_6050_t* mpu_6050, uint32_t samples) {
    // Check if the pointer is valid
    if(!mpu_6050) {
        return MPU_6050_RC_ERROR_NULL_INST;
    }
    // Check for valid sensor ID
    if(mpu_6050->sensor_id != MPU_6050_EXPECTED_ID) {
        return MPU_6050_RC_ERROR_BAD_ID;
    }

    // Clear offsets to properly calculate offsets for calibration
    clear_double_vector(&mpu_6050->offsets.accel_offsets);
    clear_double_vector(&mpu_6050->offsets.gyro_offsets);

    // Store offsets in SEPERATE vectors
    // Double vectors are used to avoid accumulated errors from rounding to int16_t
    vec_double_t accel_offsets = {0.0, 0.0, 0.0};
    vec_double_t gyro_offsets = {0.0, 0.0, 0.0};

    // Calculate offsets by taking the average of repeated samples
    for(uint32_t i=0; i<samples; ++i) {
        mpu_6050_read_raw(mpu_6050);
        mpu_6050_convert_read(mpu_6050);

        accel_offsets.x += mpu_6050->accel_data.x / (double)samples;
        accel_offsets.y += mpu_6050->accel_data.y / (double)samples;
        accel_offsets.z += mpu_6050->accel_data.z / (double)samples;

        gyro_offsets.x += mpu_6050->gyro_data.x / (double)samples;
        gyro_offsets.y += mpu_6050->gyro_data.y / (double)samples;
        gyro_offsets.z += mpu_6050->gyro_data.z / (double)samples;

        sleep_ms(1);
    }

    // Update calculated offsets
    // Assumes an orientation where the positive z axis is pointing directly upwards
    set_double_vector(&mpu_6050->offsets.accel_offsets, accel_offsets.x, accel_offsets.y, (1.0-accel_offsets.z));
    set_double_vector(&mpu_6050->offsets.gyro_offsets, gyro_offsets.x, gyro_offsets.y, gyro_offsets.z);

    return MPU_6050_RC_OK;
}

// Read raw accelerometer and gyro data from the MPU-6050
mpu_6050_rc_t mpu_6050_read_raw(mpu_6050_t* mpu_6050) {
    // Check if the pointer is valid
    if(!mpu_6050) {
        return MPU_6050_RC_ERROR_NULL_INST;
    }
    // Check for valid sensor ID
    if(mpu_6050->sensor_id != MPU_6050_EXPECTED_ID) {
        return MPU_6050_RC_ERROR_BAD_ID;
    }
    
    // int16_t, so 2 bytes per axis
    uint8_t accel_regs[6];
    uint8_t gyro_regs[6];

    // Accel and gyro data is contiguous from XOUT_H to ZOUT_L
    i2c_read_regs(mpu_6050->i2c_inst, MPU_6050_ADDR, MPU_6050_ACCEL_XOUT_H, accel_regs, 6);
    i2c_read_regs(mpu_6050->i2c_inst, MPU_6050_ADDR, MPU_6050_GYRO_XOUT_H, gyro_regs, 6);

    // Keep track of the time of last read using a static variable
    static double t_prev = -1.0;
    if(t_prev == -1.0) {
        // Loophole to initialize as time_us_64() on first read
        t_prev = time_us_64();
    }

    // Get and store time passed since last read for user calculations
    double t_curr = (double)time_us_64();
    mpu_6050->dt = (t_curr - t_prev) / 1.0e6;
    t_prev = t_curr;

    // Combine high and low bytes into int16_t values for each axis
    mpu_6050->accel_raw.x = (int16_t)((accel_regs[0] << 8) | accel_regs[1]);
    mpu_6050->accel_raw.y = (int16_t)((accel_regs[2] << 8) | accel_regs[3]);
    mpu_6050->accel_raw.z = (int16_t)((accel_regs[4] << 8) | accel_regs[5]);

    mpu_6050->gyro_raw.x = (int16_t)((gyro_regs[0] << 8) | gyro_regs[1]);
    mpu_6050->gyro_raw.y = (int16_t)((gyro_regs[2] << 8) | gyro_regs[3]);
    mpu_6050->gyro_raw.z = (int16_t)((gyro_regs[4] << 8) | gyro_regs[5]);

    return MPU_6050_RC_OK;
}

// Convert raw accelerometer and gyro data to meaningful units (dependent on configured accelerometer and gyro ranges)
mpu_6050_rc_t mpu_6050_convert_read(mpu_6050_t* mpu_6050) {
    // Check if the pointer is valid
    if(!mpu_6050) {
        return MPU_6050_RC_ERROR_NULL_INST;
    }
    // Check for valid sensor ID
    if(mpu_6050->sensor_id != MPU_6050_EXPECTED_ID) {
        return MPU_6050_RC_ERROR_BAD_ID;
    }

    // Get the conversion factors for the configured accelerometer and gyro ranges using the enum values
    double accel_conversion_factor = MPU_6050_ACCEL_CONVERSION_FACTORS[mpu_6050->accel_range];
    double gyro_conversion_factor = MPU_6050_GYRO_CONVERSION_FACTORS[mpu_6050->gyro_range];

    // Convert raw accelerometer data to meters per squared seconds using the conversion factor
    mpu_6050->accel_data.x = mpu_6050->accel_raw.x / accel_conversion_factor - mpu_6050->offsets.accel_offsets.x;
    mpu_6050->accel_data.y = mpu_6050->accel_raw.y / accel_conversion_factor - mpu_6050->offsets.accel_offsets.y;
    mpu_6050->accel_data.z = mpu_6050->accel_raw.z / accel_conversion_factor - mpu_6050->offsets.accel_offsets.z;

    // Convert raw gyro data to degrees per second using the conversion factor
    mpu_6050->gyro_data.x = mpu_6050->gyro_raw.x / gyro_conversion_factor - mpu_6050->offsets.gyro_offsets.x;
    mpu_6050->gyro_data.y = mpu_6050->gyro_raw.y / gyro_conversion_factor - mpu_6050->offsets.gyro_offsets.y;
    mpu_6050->gyro_data.z = mpu_6050->gyro_raw.z / gyro_conversion_factor - mpu_6050->offsets.gyro_offsets.z;

    return MPU_6050_RC_OK;
}