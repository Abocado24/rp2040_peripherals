#include "hc_sr04.h"

// Array to hold sensor instances registered by echo pin, used to identify sensor in ISR
hcsr04_t* sensor_instances[HCSR04_MAX_GPIO_PINS] = {0};

// Helper function that sets HC-SR04 sensor values and pins to an initial state before starting a measurement 
static void prepare_for_measurement(hcsr04_t *sensor) {
    // Set current_distance to HCSR04_DIST_INIT (-1f); this indicates an incomplete measurement in the event of an error
    sensor->current_distance = HCSR04_DIST_INIT;
    
    // Clear echo received flag
    sensor->echo_received = false;

    // Make sure trigger pin is low
    gpio_put(sensor->trigger_pin, 0);
    sleep_us(2);
}

// Helper function that sends a trigger pulse to the HC-SR04 sensor
static void send_trigger_pulse(hcsr04_t *sensor) {
    gpio_put(sensor->trigger_pin, 1);
    sleep_us(10);
    gpio_put(sensor->trigger_pin, 0);
}

// Helper function to measuring HC-SR04 echo pulse width as an interrupt handler
static void handle_echo_irq(uint gpio, uint32_t events) {
    // Retrieve the sensor instance associated with the GPIO
    hcsr04_t* sensor = sensor_instances[gpio];

    if(sensor != NULL) {
        if (events & GPIO_IRQ_EDGE_RISE) {
            // Get an accurate start time for the echo pulse
            sensor->start_time = get_absolute_time();
        } else if (events & GPIO_IRQ_EDGE_FALL) {
            // Get an accurate end time for for the echo pulse
            sensor->end_time = get_absolute_time();
            // Set echo received flag to true now that measurement is complete
            sensor->echo_received = true;
            gpio_set_irq_enabled(sensor->echo_pin, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, false);
        }
    }
}

// Helper function that calculates the distance from the duration of the echo pulse
static void calculate_distance(hcsr04_t *sensor) {
    uint32_t duration = absolute_time_diff_us(sensor->start_time, sensor->end_time);
    sensor->current_distance = ((float)duration * HCSR04_SPEED_OF_SOUND) / 2.0f;
}

// Initialize HC-SR04 sensor
void hcsr04_init(hcsr04_t *sensor, uint trigger_pin, uint echo_pin) {
    // Register data provided by args into hcsr04 struct
    sensor->trigger_pin = trigger_pin;
    sensor->echo_pin = echo_pin;

    // Set initial values for other fields in hcsr04 struct
    sensor->current_distance = 0.0;
    sensor->echo_received = false;
    sensor->state = HCSR04_IDLE;

    // Set up trigger pin as output and echo pin as input
    gpio_init(trigger_pin);
    gpio_set_dir(trigger_pin, GPIO_OUT);
    gpio_init(echo_pin);
    gpio_set_dir(echo_pin, GPIO_IN);

    // Register sensor instance into global array using echo pin
    sensor_instances[echo_pin] = sensor;
}

// Get distance from HC-SR04 sensor
// This is a blocking implementation that polls until the measurement is complete with a timeout
hcsr04_rc_t hcsr04_get_distance(hcsr04_t *sensor, uint32_t timeout_us) {
    // Do nothing if sensor is busy
    if (sensor->state != HCSR04_IDLE) {
        return HCSR04_RC_IN_USE;
    }

    // Set sensor state to busy while measuring to avoid concurrency issues
    sensor->state = HCSR04_BUSY;

    // Set sensor values and pins to an initial state before starting a measurement
    prepare_for_measurement(sensor);

    // Send pulse to trigger pin to start measurement
    send_trigger_pulse(sensor);

    // Wait until echo is received and measure duration
    // Set distance to -1.0f and exit early if timeout is reached
    absolute_time_t timeout = make_timeout_time_us(timeout_us);

    // Get an accurate start time for when trigger pulse is sent
    while (!gpio_get(sensor->echo_pin)) {
        if (time_reached(timeout)) {
            return HCSR04_RC_START_TIMEOUT;
        }
        sensor->start_time = get_absolute_time();
    }

    // Get an accurate end time for when echo is received
    while (gpio_get(sensor->echo_pin)) {
        if (time_reached(timeout)) {
            return HCSR04_RC_END_TIMEOUT;
        }
    }
    sensor->end_time = get_absolute_time();

    // Set echo received flag to true now that measurement is complete
    sensor->echo_received = true;

    // Calculate distance from duration and store in sensor struct
    calculate_distance(sensor);

    // Set sensor state to idle now that measurement is complete
    sensor->state = HCSR04_IDLE;

    return HCSR04_RC_OK;
}

// Start measuring distance from HC-SR04 sensor
// This is a non-blocking implementation that uses interrupts
// Use hcsr04_end_measurement() to get the distance when you need it
hcsr04_rc_t hcsr04_start_measurement(hcsr04_t *sensor) {
    // Do nothing if sensor is busy
    if (sensor->state != HCSR04_IDLE) {
        return HCSR04_RC_IN_USE;
    }

    // Set sensor state to busy to avoid concurrency issues
    sensor->state = HCSR04_BUSY;

    // Set sensor values and pins to an initial state before starting a measurement
    prepare_for_measurement(sensor);

    // Set up interrupt handler for echo pin
    gpio_set_irq_enabled_with_callback(sensor->echo_pin, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &handle_echo_irq);

    // Send pulse to trigger pin to start measurement
    send_trigger_pulse(sensor);

    return HCSR04_RC_OK;
}

// Finish distance measurement of HC-SR04 sensor once echo pulse width is measured
// This is a non-blocking implementation that uses interrupts
// Make sure to check the rc value to see if the measurement has actually been completed
hcsr04_rc_t hcsr04_end_measurement(hcsr04_t *sensor) {
    if (sensor->echo_received) {
        // Calculate distance from duration and store in sensor struct
        calculate_distance(sensor);

        // Set sensor state to idle now that measurement is complete
        sensor->state = HCSR04_IDLE;

        return HCSR04_RC_OK;
    }
    else {
        return HCSR04_RC_IN_USE;
    }
}

// Resets the HC-SR04 sensor to a default state
// Should always call if HCSR04_RC_IN_USE is returned (or to otherwise recover from an error)
void hcsr04_reset(hcsr04_t *sensor) {
    // Disable any active interrupts on the echo pin
    gpio_set_irq_enabled(sensor->echo_pin, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, false);

    // Reinitialize the GPIO pins
    gpio_init(sensor->trigger_pin);
    gpio_set_dir(sensor->trigger_pin, GPIO_OUT);
    gpio_init(sensor->echo_pin);
    gpio_set_dir(sensor->echo_pin, GPIO_IN);

    // Reset sensor values to their initial state
    sensor->current_distance = HCSR04_DIST_INIT;
    sensor->echo_received = false;
    sensor->state = HCSR04_IDLE;

    // Clear any stored instances for this sensor
    sensor_instances[sensor->echo_pin] = NULL;

    // Re-register the sensor instance
    sensor_instances[sensor->echo_pin] = sensor;
}
