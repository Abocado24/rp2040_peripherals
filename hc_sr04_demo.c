#include <stdio.h>
#include "pico/stdlib.h"
#include "hc_sr04.h"

const uint8_t ECHO_PIN = 6;
const uint8_t TRIGGER_PIN = 7;

hcsr04_t hcsr04;

/**
 * @brief   Handler for all GPIO pin interrupts; is currently just for a single HCSR04 sensor.
 */
void handle_gpio_irq(uint gpio, uint32_t events) {
    if(gpio == ECHO_PIN) {
        if (events & GPIO_IRQ_EDGE_RISE) {
            // Measure start of echo pulse
            hcsr04_on_echo_pin_rise(&hcsr04);
        } else if (events & GPIO_IRQ_EDGE_FALL) {
            // Measure end of echo pulse
            hcsr04_on_echo_pin_fall(&hcsr04);
        }
    }
}

/**
 * @brief   Test blocking implementation of sensor
 */
void blocking_implementation_test() {
    printf("Measuring blocking...\n");

    // Disable GPIO IRQ for blocking implementation
    gpio_set_irq_enabled(hcsr04.echo_pin, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, false);

    // Measure data blocking
    hcsr04_start_measurement(&hcsr04);

    // Poll for rising edge of echo pin and handle it
    while(!gpio_get(hcsr04.echo_pin));
    hcsr04_on_echo_pin_rise(&hcsr04);

    // Poll for falling edge of echo pin and handle it
    while(gpio_get(hcsr04.echo_pin));
    hcsr04_on_echo_pin_fall(&hcsr04);

    // Complete measurement and print result
    hcsr04_end_measurement(&hcsr04);
    printf("Distance: %.2f cm\n", hcsr04.current_distance);
}

/**
 * @brief   Test non-blocking implementation of sensor
 */
void nonblocking_implementation_test() {
    printf("Measuring non-blocking...\n");

    // Enable GPIO IRQ for non-blocking implementation
    gpio_set_irq_enabled(hcsr04.echo_pin, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);

    // Measure data via non-blocking interrupt
    hcsr04_start_measurement(&hcsr04);

    // Wait for measurement to complete and print result
    while (hcsr04_end_measurement(&hcsr04) != HCSR04_RC_OK) {
        sleep_ms(100); // Wait a bit before checking again
    }
    printf("Distance: %.2f cm\n", hcsr04.current_distance);
}

/**
 * @brief   Test the calling of start_measurement when a transaction is in progress
 */
void repeated_calls_test() {
    printf("Testing repeated calls...\n");

    // Enable GPIO IRQ for test (if we prove it for one we've proved it for both and non-blocking uses less lines of code)
    gpio_set_irq_enabled(hcsr04.echo_pin, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);

    hcsr04_rc_t rc_1 = hcsr04_start_measurement(&hcsr04);
    hcsr04_rc_t rc_2 = hcsr04_start_measurement(&hcsr04);
    while(!hcsr04_end_measurement(&hcsr04));

    printf("calling start_measurement twice returns %d and %d, final distance is %f\n", rc_1, rc_2, hcsr04.current_distance);
}

int main() {
    // Initialize stdio for USB output
    stdio_init_all();

    // Give time to load up serial monitor for debugging
    sleep_ms(3000);

    printf("HC-SR04 Test Program\n");

    // Initialize sensor
    hcsr04_init(&hcsr04, TRIGGER_PIN, ECHO_PIN);

    // Register callback function for non-blocking implementation
    gpio_set_irq_enabled_with_callback(hcsr04.echo_pin, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &handle_gpio_irq);

    while (1) {
        // Test blocking implementation of sensor
        blocking_implementation_test();
        sleep_ms(1000);

        // Test non-blocking implementation of sensor
        nonblocking_implementation_test();
        sleep_ms(1000);

        // Test the calling of start_measurement when a transaction is in progress
        repeated_calls_test();
        sleep_ms(1000);
        
        // Reset the sensor to guarantee initial state for next loop iteration
        hcsr04_reset(&hcsr04);
        sleep_ms(1000);
    }
}
