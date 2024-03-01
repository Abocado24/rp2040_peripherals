#include <stdio.h>
#include <pico/stdlib.h>
#include "mpu_6050.h"

const double GYRO_WEIGHT = 0.8;
const uint32_t SAMPLES_CALIBRATION = 10000;
const uint32_t SAMPLES_NOISE_ESTIMATION = 1000;

// Function to calculate mean and standard deviation
void calculate_mean_std(double data[], int n, double *mean, double *stddev) {
    double sum = 0.0, variance = 0.0;
    for(int i = 0; i < n; i++) {
        sum += data[i];
    }
    *mean = sum / n;
    for(int i = 0; i < n; i++) {
        variance += pow(data[i] - *mean, 2);
    }
    *stddev = sqrt(variance / n);
}

// Estimate noise present in mpu_6050 accelerometer and gyroscope
void estimate_noise(mpu_6050_t *mpu_6050) {
    double accel_x_samples[SAMPLES_NOISE_ESTIMATION];
    double accel_y_samples[SAMPLES_NOISE_ESTIMATION];
    double accel_z_samples[SAMPLES_NOISE_ESTIMATION];
    double gyro_x_samples[SAMPLES_NOISE_ESTIMATION];
    double gyro_y_samples[SAMPLES_NOISE_ESTIMATION];
    double gyro_z_samples[SAMPLES_NOISE_ESTIMATION];
    
    for (int i = 0; i < SAMPLES_NOISE_ESTIMATION; i++) {
        mpu_6050_read_raw(mpu_6050);
        mpu_6050_convert_read(mpu_6050);
        accel_x_samples[i] = mpu_6050->accel_data.x;
        accel_y_samples[i] = mpu_6050->accel_data.y;
        accel_z_samples[i] = mpu_6050->accel_data.z;
        gyro_x_samples[i] = mpu_6050->gyro_data.x;
        gyro_y_samples[i] = mpu_6050->gyro_data.y;
        gyro_z_samples[i] = mpu_6050->gyro_data.z;
        sleep_ms(10);
    }

    double mean, stddev;
    calculate_mean_std(accel_x_samples, SAMPLES_NOISE_ESTIMATION, &mean, &stddev);
    printf("Accel X - Mean: %f, StdDev: %f\n", mean, stddev);
    calculate_mean_std(accel_y_samples, SAMPLES_NOISE_ESTIMATION, &mean, &stddev);
    printf("Accel Y - Mean: %f, StdDev: %f\n", mean, stddev);
    calculate_mean_std(accel_z_samples, SAMPLES_NOISE_ESTIMATION, &mean, &stddev);
    printf("Accel Z - Mean: %f, StdDev: %f\n", mean, stddev);
    calculate_mean_std(gyro_x_samples, SAMPLES_NOISE_ESTIMATION, &mean, &stddev);
    printf("Gyro X - Mean: %f, StdDev: %f\n", mean, stddev);
    calculate_mean_std(gyro_y_samples, SAMPLES_NOISE_ESTIMATION, &mean, &stddev);
    printf("Gyro Y - Mean: %f, StdDev: %f\n", mean, stddev);
    calculate_mean_std(gyro_z_samples, SAMPLES_NOISE_ESTIMATION, &mean, &stddev);
    printf("Gyro Z - Mean: %f, StdDev: %f\n", mean, stddev);
}

// Estimates the current roll, pitch and yaw of the MPU by fusing accel + gyro data
void estimate_angles(vec_double_t accel_data, vec_double_t gyro_data, double dt, vec_double_t * angles) {
    // estimate angles based on accel data
    double accel_angle_x = (atan(accel_data.y / sqrt(pow(accel_data.x, 2) + pow(accel_data.z, 2))) * 180 / M_PI);
    double accel_angle_y = (atan(-1 * accel_data.x / sqrt(pow(accel_data.y, 2) + pow(accel_data.z, 2))) * 180 / M_PI);

    // estimate angles based on gyro data
    double gyro_angle_x = angles->x + gyro_data.x * dt;
    double gyro_angle_y = angles->y + gyro_data.y * dt;
    double gyro_angle_z = angles->z + gyro_data.z * dt;

    // fuse estimates with complementary filter to determine roll and pitch
    angles->x = GYRO_WEIGHT*gyro_angle_x + (1-GYRO_WEIGHT)*accel_angle_x;
    angles->y = GYRO_WEIGHT*gyro_angle_y + (1-GYRO_WEIGHT)*accel_angle_y;
    
    // yaw calculation is its own thing since there's nothing from accel to fuse with
    angles->z = gyro_angle_z;
}

int main() {
    stdio_init_all();

    // Initialize I2C
    gpio_init(PICO_DEFAULT_I2C_SDA_PIN);
    gpio_init(PICO_DEFAULT_I2C_SCL_PIN);
    gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
    gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);
    i2c_init(i2c_default, 100 * 1000);

    mpu_6050_t mpu_6050;
    
    sleep_ms(5000);

    printf("init mpu\n");

    mpu_6050_rc_t rc = mpu_6050_init(&mpu_6050, i2c_default);

    // Repeatedly try to initialize until successful
    while(rc != MPU_6050_RC_OK)
    {
        printf("rc = %d, Sensor ID = %x\n", rc, mpu_6050.sensor_id);
        sleep_ms(1000);
        rc = mpu_6050_init(&mpu_6050, i2c_default);
    }

    printf("reset mpu\n");

    mpu_6050_reset(&mpu_6050);

    printf("calibrate mpu\n");

    mpu_6050_calibrate(&mpu_6050, SAMPLES_CALIBRATION);

    vec_double_t angles = {0.0, 0.0, 0.0};
    while (1) {
        // Better to just do this on its own, commenting out everything else
        //estimate_noise(&mpu_6050);
        
        mpu_6050_read_raw(&mpu_6050);
        mpu_6050_convert_read(&mpu_6050);

        // Put data readings through a filter?

        estimate_angles(mpu_6050.accel_data, mpu_6050.gyro_data, mpu_6050.dt, &angles);

        // Then put the angles themselves through a filter?

        printf("%f/%f/%f\n", angles.x, angles.y, angles.z);

        sleep_ms(10);
    }
}