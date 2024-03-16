#include <stdio.h>
#include "pico/stdlib.h"
#include "hc_sr04.h"

#define ECHO_PIN 6
#define TRIGGER_PIN 7

int main() {
    // Initialize stdio for USB output
    stdio_init_all();

    printf("HC-SR04 Test Program\n");

    // Initialize sensor
    hcsr04_t sensor;
    hcsr04_init(&sensor, TRIGGER_PIN, ECHO_PIN);

    while (1) {
        // Test blocking measurement
        printf("Blocking measurement...\n");
        hcsr04_get_distance(&sensor, HCSR04_DEFAULT_TIMEOUT_US);
        printf("Distance (blocking): %.2f cm\n", sensor.current_distance);

        // Wait a bit
        sleep_ms(1000);

        // Test non-blocking measurement
        printf("Non-blocking measurement...\n");
        hcsr04_start_measurement(&sensor);

        // Wait for measurement to complete or timeout
        while (hcsr04_end_measurement(&sensor) != HCSR04_RC_OK) {
            sleep_ms(100); // Wait a bit before checking again
        }
        printf("Distance (non-blocking): %.2f cm\n", sensor.current_distance);

        // Wait a bit
        sleep_ms(1000);

        // Test what happens if we call anything unexpected after start_measurement
        printf("Testing repeated calls...\n");
        hcsr04_rc_t rc_1, rc_2;

        // Test case 1: Calling start_measurement twice
        rc_1 = hcsr04_start_measurement(&sensor);
        rc_2 = hcsr04_start_measurement(&sensor);
        while(!hcsr04_end_measurement(&sensor));

        printf("calling start_measurement twice returns %d and %d, final distance is %f\n", rc_1, rc_2, sensor.current_distance);

        // Reset the sensor to recover from erronous position
        printf("reset sensor\n");
        hcsr04_reset(&sensor);

        // Wait a bit
        sleep_ms(1000);

        // Test case 2: Calling start_measurement and then get_distance
        rc_1 = hcsr04_start_measurement(&sensor);
        rc_2 = hcsr04_get_distance(&sensor, HCSR04_DEFAULT_TIMEOUT_US);
        while(!hcsr04_end_measurement(&sensor));

        printf("calling start_measurement and then get_distance returns %d and %d, final distance is %f\n", rc_1, rc_2, sensor.current_distance);

        // Reset the sensor to recover from erronous position
        printf("reset sensor\n");
        hcsr04_reset(&sensor);

        // Wait a bit
        sleep_ms(1000);
    }
}
