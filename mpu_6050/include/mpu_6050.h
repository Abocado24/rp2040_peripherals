#ifndef MPU_6050_H
#define MPU_6050_H

#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/sync.h"
#include "hardware/uart.h"
#include "hardware/i2c.h"

#include "i2c_general.h"
#include "vector_lib.h"

#include <math.h>
#include <time.h>

// I2C registers for interfacing with MPU-6050
#define MPU_6050_ADDR           0x68
#define MPU_6050_PWR_MGMT_1     0x6B
#define MPU_6050_WHO_AM_I       0x75
#define MPU_6050_ACCEL_XOUT_H   0x3B
#define MPU_6050_GYRO_XOUT_H    0x43
#define MPU_6050_GYRO_CONFIG    0x1B
#define MPU_6050_ACCEL_CONFIG   0x1C
#define MPU_6050_MOT_THRESHOLD  0x1F
#define MPU_6050_MOT_DURATION   0x20
#define MPU_6050_ZMOT_THRESHOLD 0x21
#define MPU_6050ZMOT_DURATION   0x22

// Expected value in the who_am_i register of MPU_6050; used to validate I2C connections
#define MPU_6050_EXPECTED_ID   0x68

// Masks used to clear bits in registers corresponding to MPU-6050 settings
#define MPU_6050_CONFIG_CLOCK_SOURCE_MASK   0xF8
#define MPU_6050_CONFIG_ACCEL_MASK          0xE7
#define MPU_6050_CONFIG_GYRO_MASK           0xE7
#define MPU_6050_CONFIG_SLEEP_MODE_MASK     0xBF

typedef enum {
    MPU_6050_RC_OK = 0,
    MPU_6050_RC_ERROR_NULL_INST = -1,
    MPU_6050_RC_ERROR_BAD_ID = -2,
    MPU_6050_RC_ERROR_INVALID_ARG = -3,
} mpu_6050_rc_t;

typedef struct {
    vec_double_t accel_offsets;
    vec_double_t gyro_offsets;
} mpu_6050_offsets_t;

typedef enum {
    MPU_6050_CLOCK_INTERNAL = 0U,
    MPU_6050_CLOCK_PLL_X_GYRO = 1U,
    MPU_6050_CLOCK_PLL_Y_GYRO = 2U,
    MPU_6050_CLOCK_PLL_Z_GYRO = 3U,
    MPU_6050_CLOCK_PLL_EXT_32KHZ = 4U,
    MPU_6050_CLOCK_PLL_EXT_19_2MHZ = 5U,
    MPU_6050_CLOCK_STOP = 7U
} mpu_6050_clock_source_t;

typedef enum {
    MPU_6050_ACCEL_2G = 0U,
    MPU_6050_ACCEL_4G = 1U,
    MPU_6050_ACCEL_8G = 2U,
    MPU_6050_ACCEL_16G = 3U,
} mpu_6050_accel_range_t;

typedef enum {
    MPU_6050_GYRO_250DPS = 0U,
    MPU_6050_GYRO_500DPS = 1U,
    MPU_6050_GYRO_1000DPS = 2U,
    MPU_6050_GYRO_2000DPS = 3U,
} mpu_6050_gyro_range_t;

typedef enum {
    MPU_6050_SLEEP_DISABLED = 0U,
    MPU_6050_SLEEP_ENABLED = 1U,
} mpu_6050_sleep_state_t;

typedef struct {
    i2c_inst_t * i2c_inst;
    uint8_t sensor_id;

    mpu_6050_clock_source_t clock_source;
    mpu_6050_accel_range_t accel_range;
    mpu_6050_gyro_range_t gyro_range;
    mpu_6050_sleep_state_t sleep_state;

    vec_int16_t accel_raw;
    vec_int16_t gyro_raw;
    double dt;
    vec_double_t accel_data;
    vec_double_t gyro_data;
    mpu_6050_offsets_t offsets;
} mpu_6050_t;

mpu_6050_rc_t mpu_6050_init(mpu_6050_t *mpu_6050, i2c_inst_t * i2c_inst);

mpu_6050_rc_t mpu_6050_set_clock_source(mpu_6050_t *mpu_6050, mpu_6050_clock_source_t clock_source);
mpu_6050_rc_t mpu_6050_set_accel_range(mpu_6050_t *mpu_6050, mpu_6050_accel_range_t range);
mpu_6050_rc_t mpu_6050_set_gyro_range(mpu_6050_t *mpu_6050, mpu_6050_gyro_range_t range);
mpu_6050_rc_t mpu_6050_set_sleep_mode(mpu_6050_t *mpu_6050, mpu_6050_sleep_state_t state);

mpu_6050_rc_t mpu_6050_reset(mpu_6050_t *mpu_6050);
mpu_6050_rc_t mpu_6050_calibrate(mpu_6050_t* mpu_6050, uint32_t samples);

mpu_6050_rc_t mpu_6050_read_raw(mpu_6050_t* mpu_6050);
mpu_6050_rc_t mpu_6050_convert_read(mpu_6050_t* mpu_6050);


#endif // MPU_6050_H