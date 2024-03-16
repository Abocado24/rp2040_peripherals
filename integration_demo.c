#include <stdio.h>
#include <pico/stdlib.h>
#include "mpu_6050.h"
#include "edf.h"

#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>

const double GYRO_WEIGHT = 0.8;
const uint32_t SAMPLES_CALIBRATION = 10000;

typedef struct {
    vec_double_t accel_data;
    vec_double_t gyro_data;
    double dt;
} mpu_6050_data_t;

TaskHandle_t mpu_6050_read_task_handle = NULL;
TaskHandle_t mpu_6050_process_task_handle = NULL;
TaskHandle_t mpu_6050_print_task_handle = NULL;

const TickType_t mpu_6050_read_task_period = pdMS_TO_TICKS(1);
const TickType_t mpu_6050_process_task_period = pdMS_TO_TICKS(10);
const TickType_t mpu_6050_print_task_period = pdMS_TO_TICKS(100);

QueueHandle_t mpu_6050_data_queue;
QueueHandle_t angles_queue;

void mpu_6050_read_task(void *pvParameters)
{
    mpu_6050_t* mpu_6050 = (mpu_6050_t*) pvParameters;
    mpu_6050_data_t data;

    while (1) {
        // Read raw data, retry task if it fails
        if(mpu_6050_read_raw(mpu_6050) != MPU_6050_RC_OK) {
            continue;
        }
        // Convert read data, retry task if it fails
        if(mpu_6050_convert_read(mpu_6050) != MPU_6050_RC_OK) {
            continue;
        }

        // Put relevant data for processing into mpu_6050_data struct
        copy_double_vector(&mpu_6050->accel_data, &data.accel_data);
        copy_double_vector(&mpu_6050->gyro_data, &data.gyro_data);
        data.dt = mpu_6050->dt;

        // Send mpu_6050_data struct to process task
        xQueueSend(mpu_6050_data_queue, &data, portMAX_DELAY);

        // Delay to control read frequency
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

void mpu_6050_process_task(void *pvParameters)
{
    mpu_6050_data_t data;
    vec_double_t angles = {0, 0, 0};

    while (1) {
        // Receive latest data from mpu_6050_read_task
        if( xQueueReceive(mpu_6050_data_queue, &data, portMAX_DELAY) == pdPASS) {
            // get references to pieces of data (mostly for clarity)
            vec_double_t * accel_data = &data.accel_data;
            vec_double_t * gyro_data = &data.gyro_data;
            double dt = data.dt;
            
            // estimate angles based on accel data
            double accel_angle_x = (atan(accel_data->y / sqrt(pow(accel_data->x, 2) + pow(accel_data->z, 2))) * 180 / M_PI);
            double accel_angle_y = (atan(-1 * accel_data->x / sqrt(pow(accel_data->y, 2) + pow(accel_data->z, 2))) * 180 / M_PI);

            // estimate angles based on gyro data
            double gyro_angle_x = angles.x + gyro_data->x * dt;
            double gyro_angle_y = angles.y + gyro_data->y * dt;
            double gyro_angle_z = angles.z + gyro_data->z * dt;

            // fuse estimates with complementary filter to determine roll and pitch
            angles.x = GYRO_WEIGHT*gyro_angle_x + (1-GYRO_WEIGHT)*accel_angle_x;
            angles.y = GYRO_WEIGHT*gyro_angle_y + (1-GYRO_WEIGHT)*accel_angle_y;
            
            // yaw calculation is its own thing since there's nothing from accel to fuse with
            angles.z = gyro_angle_z;

            // Send angles to print task
            xQueueSend(angles_queue, &angles, portMAX_DELAY);

            // Delay to control frequency
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
}

void mpu_6050_print_task(void *pvParameters)
{
    vec_double_t angles;

    while (1) {
        // Initially assume the queue is empty
        BaseType_t xReceived = pdFALSE;

        // Go through the queue until the latest angle passed from mpu_6050_process_task is received
        while(xQueueReceive(angles_queue, &angles, 0) == pdPASS) {
            xReceived = pdTRUE;
        }

        // If at least one item was received, print the angles
        if(xReceived == pdTRUE) {
            printf("%f/%f/%f\n", angles.x, angles.y, angles.z);
        }

        // Delay to control frequency
        vTaskDelay(pdMS_TO_TICKS(100));
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

    // Initialize and calibrate MPU-6050
    mpu_6050_t mpu_6050;
    mpu_6050_rc_t rc = mpu_6050_init(&mpu_6050, i2c_default);
    while(rc != MPU_6050_RC_OK)
    {
        // Repeatedly try to initialize until successful
        printf("rc = %d, Sensor ID = %x\n", rc, mpu_6050.sensor_id);
        sleep_ms(1000);
        rc = mpu_6050_init(&mpu_6050, i2c_default);
    }
    mpu_6050_reset(&mpu_6050);
    mpu_6050_calibrate(&mpu_6050, SAMPLES_CALIBRATION);

    // Create queues
    mpu_6050_data_queue = xQueueCreate(20, sizeof(mpu_6050_data_t));
    angles_queue = xQueueCreate(20, sizeof(vec_double_t));

    // Create tasks, passing the address of mpu_6050 as the parameter
    xTaskCreate(mpu_6050_read_task, "MPU-6050 Read Task", configMINIMAL_STACK_SIZE, (void*)&mpu_6050, EDF_UNSELECTED_PRIORITY, &mpu_6050_read_task_handle);
    xTaskCreate(mpu_6050_process_task, "MPU-6050 Process Task", configMINIMAL_STACK_SIZE, NULL, EDF_UNSELECTED_PRIORITY, &mpu_6050_process_task_handle);
    xTaskCreate(mpu_6050_print_task, "MPU-6050 Print Task", configMINIMAL_STACK_SIZE, NULL, EDF_UNSELECTED_PRIORITY, &mpu_6050_print_task_handle);
 
    // Define initial tasklist for EDF scheduler
    edf_task_t tasklist[3] = 
    {
        {.task_handle=mpu_6050_read_task_handle, .task_deadline=mpu_6050_read_task_period, .task_period=mpu_6050_read_task_period, .task_state=EDF_TASK_READY},
        {.task_handle=mpu_6050_process_task_handle, .task_deadline=mpu_6050_process_task_period, .task_period=mpu_6050_process_task_period, .task_state=EDF_TASK_READY},
        {.task_handle=mpu_6050_print_task_handle, .task_deadline=mpu_6050_print_task_period, .task_period=mpu_6050_print_task_period, .task_state=EDF_TASK_READY},
    };

    // Start the EDF scheduler
    edf_start(tasklist, 3);

    while(1);
}