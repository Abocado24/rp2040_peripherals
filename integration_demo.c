#include <stdio.h>
#include <pico/stdlib.h>
#include "mpu_6050.h"
#include "edf.h"

#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>

const double GYRO_WEIGHT = 0.8;
const uint32_t SAMPLES_CALIBRATION = 10000;

TaskHandle_t mpu_6050_task_handle = NULL;
TaskHandle_t print_angles_task_handle = NULL;

const TickType_t mpu_6050_task_period = pdMS_TO_TICKS(10);
const TickType_t print_angles_task_period = pdMS_TO_TICKS(100);

typedef struct {
    mpu_6050_t * mpu_6050;
    vec_double_t * angles;
} mpu_6050_task_data_t;

SemaphoreHandle_t angles_mutex;

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

// Gets data from 
void mpu_6050_task(void *pvParameters)
{
    mpu_6050_task_data_t * mpu_6050_task_data = (mpu_6050_task_data_t*) pvParameters;

    mpu_6050_t * mpu_6050 = mpu_6050_task_data->mpu_6050;
    vec_double_t * angles = mpu_6050_task_data->angles;

    while(1) {
        if(mpu_6050_read_raw(mpu_6050) != MPU_6050_RC_OK) {
            continue;
        }

        if(mpu_6050_convert_read(mpu_6050) != MPU_6050_RC_OK) {
            continue;
        }

        xSemaphoreTake(angles_mutex, portMAX_DELAY);
        {
            estimate_angles(mpu_6050->accel_data, mpu_6050->gyro_data, mpu_6050->dt, angles);
        }
        xSemaphoreGive(angles_mutex);

        edf_complete_task(mpu_6050_task_handle);
    }
}

void print_angles_task(void *pvParameters)
{
    vec_double_t * angles = (vec_double_t *)pvParameters;

    vec_double_t current_angles = {0.0, 0.0, 0.0};

    while (1) {
        xSemaphoreTake(angles_mutex, portMAX_DELAY);
        {
            copy_double_vector(angles, &current_angles);
        }
        xSemaphoreGive(angles_mutex);

        // Print current angles
        printf("%f/%f/%f\n", current_angles.x, current_angles.y, current_angles.z);

        edf_complete_task(print_angles_task_handle);
    }
}

int main()
{
    stdio_init_all();

    // Initialize I2C
    gpio_init(PICO_DEFAULT_I2C_SDA_PIN);
    gpio_init(PICO_DEFAULT_I2C_SCL_PIN);
    gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
    gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);
    i2c_init(i2c_default, 100 * 1000);

    // Sufficient delay to give user time to boot up a serial terminal
    sleep_ms(5000);

    // Initialize and calibrate MPU-6050
    mpu_6050_t mpu_6050;
    mpu_6050_rc_t rc = mpu_6050_init(&mpu_6050, i2c_default);

    while(rc != MPU_6050_RC_OK)
    {
        // Repeatedly try to initialize until successful
        printf("rc = %d, Sensor ID = %x\n", rc, mpu_6050.sensor_id);
        rc = mpu_6050_init(&mpu_6050, i2c_default);
    }
    mpu_6050_reset(&mpu_6050);

    printf("starting calibration\n");
    mpu_6050_calibrate(&mpu_6050, SAMPLES_CALIBRATION);
    printf("calibration done\n");

    // Create other variables used for tasks
    vec_double_t angles = {0.0, 0.0, 0.0};
    mpu_6050_task_data_t mpu_6050_task_data = 
    {
        .mpu_6050 = &mpu_6050,
        .angles = &angles,
    };

    angles_mutex = xSemaphoreCreateMutex();

    // Create tasks, passing the arguments to use by reference
    xTaskCreate(mpu_6050_task, "MPU-6050 Task", 256, (void*)&mpu_6050_task_data, EDF_UNSELECTED_PRIORITY, &mpu_6050_task_handle);
    xTaskCreate(print_angles_task, "MPU-6050 Print Task", 256, (void*)&angles, EDF_UNSELECTED_PRIORITY, &print_angles_task_handle);

    // Define initial tasklist for EDF scheduler
    edf_task_t tasklist[2] = 
    {
        {.task_handle=mpu_6050_task_handle, .task_deadline=mpu_6050_task_period, .task_period=mpu_6050_task_period, .task_state=EDF_TASK_READY},
        {.task_handle=print_angles_task_handle, .task_deadline=print_angles_task_period, .task_period=print_angles_task_period, .task_state=EDF_TASK_READY},
    };

   printf("starting scheduler\n");
   edf_start(tasklist, 2);

    while(1);
}