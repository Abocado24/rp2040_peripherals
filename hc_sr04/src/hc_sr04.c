#include "hc_sr04.h"

const float HCSR04_SPEED_OF_SOUND_CM_US = 0.0343;
const float HCSR04_DIST_NONE = -1.0;
const uint8_t HCSR04_RESET_TRIGGER_TIME_US = 2;
const uint8_t HCSR04_TRIGGER_PULSE_WIDTH_US = 10;

/**
 * @brief   Helper function that sets the interfacing mechanism with HC-SR04 sensor to a default state.
 * @param   sensor          The HC-SR04 sensor struct, is assumed to be valid.
 */
static inline void reset_sensor(hcsr04_t * sensor) {
    // Set initial values for other fields in hcsr04 struct
    sensor->current_distance = HCSR04_DIST_NONE;
    sensor->echo_received = false;
    sensor->state = HCSR04_IDLE;

    // Set up trigger pin as output and echo pin as input
    gpio_init(sensor->trigger_pin);
    gpio_set_dir(sensor->trigger_pin, GPIO_OUT);
    gpio_init(sensor->echo_pin);
    gpio_set_dir(sensor->echo_pin, GPIO_IN);
}

/**
 * @brief   Helper function that sets initial state on HC-SR04 before taking a measurement.
 * @param   sensor          The HC-SR04 sensor struct, is assumed to be valid.
 */
static inline void prepare_for_measurement(hcsr04_t * sensor) {
    // Default initial value for distance indicates an error if it occurs 
    sensor->current_distance = HCSR04_DIST_NONE;
    
    // Clear echo received flag
    sensor->echo_received = false;

    // Make sure trigger pin is low
    gpio_put(sensor->trigger_pin, 0);
    sleep_us(HCSR04_RESET_TRIGGER_TIME_US);
}

/**
 * @brief   Helper function that sends a trigger pulse to the HC-SR04 sensor. 
 * @details The HC-SR04 should send a pulse back through the echo pin, which is used to measure distance.
 * @param   sensor          The HC-SR04 sensor struct, is assumed to be valid.
 */
static inline void send_trigger_pulse(hcsr04_t * sensor) {
    gpio_put(sensor->trigger_pin, 1);
    sleep_us(HCSR04_TRIGGER_PULSE_WIDTH_US);
    gpio_put(sensor->trigger_pin, 0);
}

/**
 * @brief   Helper function that calculates the distance from the width of the echo pulse
 * @param   sensor          The HC-SR04 sensor struct, is assumed to be valid.
 */
static inline void calculate_distance(hcsr04_t * sensor) {
    uint32_t duration = absolute_time_diff_us(sensor->start_time, sensor->end_time);
    sensor->current_distance = ((float)duration * HCSR04_SPEED_OF_SOUND_CM_US) / 2.0f;
}

/**
 * @brief   Initialize HC-SR04 sensor to a default state.
 * @details The HC-SR04 sensor itself does not need to be configured, just our means of interfacing with it.
 * @param   sensor          HC-SR04 sensor struct.
 * @param   trigger_pin     HC-SR04 trigger pin.
 * @param   echo_pin        HC-SR04 trigger pin.
 * @return  hcsr04_rc_t     Return code indicating operation success/failure.
 *                          - HCSR04_RC_OK:               Operation successful.
 *                          - HCSR04_RC_BAD_ARG:          An invalid argument was provided.
 */
hcsr04_rc_t hcsr04_init(hcsr04_t * sensor, uint8_t trigger_pin, uint8_t echo_pin) {
    if(sensor == NULL) {
        return HCSR04_RC_BAD_ARG;
    }

    // Register data provided by args into hcsr04 struct
    sensor->trigger_pin = trigger_pin;
    sensor->echo_pin = echo_pin;

    // Set sensor interfacing values to default state
    reset_sensor(sensor);

    return HCSR04_RC_OK;
}

/**
 * @brief   Reset HC-SR04 sensor to a default state.
 * @details Call this function to recover from an error.
 * @param   sensor          HC-SR04 sensor struct.
 * @return  hcsr04_rc_t     Return code indicating operation success/failure.
 *                          - HCSR04_RC_OK:               Operation successful.
 *                          - HCSR04_RC_BAD_ARG:          An invalid argument was provided.
 */
hcsr04_rc_t hcsr04_reset(hcsr04_t * sensor) {
    if(sensor == NULL) {
        return HCSR04_RC_BAD_ARG;
    }

    // Disable any active interrupts on the echo pin
    gpio_set_irq_enabled(sensor->echo_pin, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, false);

    // Reset sensor interfacing values to default state
    reset_sensor(sensor);

    return HCSR04_RC_OK;
}

/**
 * @brief   Start measuring distance using HC-SR04 by sending trigger pulse through trigger pin.
 * @details Receives echo pulse through echo pin, width of echo pulse is used to determine distance.
 *          Transaction is finished by calling hcsr04_end_measurement after echo pulse is received. 
 * @param   sensor          HC-SR04 sensor struct.
 * @return  hcsr04_rc_t     Return code indicating operation success/failure.
 *                          - HCSR04_RC_OK:             Operation successful.
 *                          - HCSR04_RC_BAD_ARG:        An invalid argument was provided.
 *                          - HCSR04_RC_BUSY:           The sensor is in the middle of another measurement.  
 */
hcsr04_rc_t hcsr04_start_measurement(hcsr04_t * sensor) {
    if(sensor == NULL) {
        return HCSR04_RC_BAD_ARG;
    }

    if (sensor->state != HCSR04_IDLE) {
        return HCSR04_RC_BUSY;
    }

    // Set sensor state to busy to avoid concurrency issues
    sensor->state = HCSR04_BUSY;

    // Set sensor values and pins to an initial state before starting a measurement
    prepare_for_measurement(sensor);

    // Send pulse to trigger pin to start measurement
    send_trigger_pulse(sensor);

    return HCSR04_RC_OK;
}

/**
 * @brief   Handle a rising edge on the HC-SR04 echo pin.
 * @details Logs the time of the rising edge for future echo pulse width calculation.
 * @param   sensor          HC-SR04 sensor struct.
 * @return  hcsr04_rc_t     Return code indicating operation success/failure.
 *                          - HCSR04_RC_OK:               Operation successful.
 *                          - HCSR04_RC_BAD_ARG:          An invalid argument was provided.
 */
hcsr04_rc_t hcsr04_on_echo_pin_rise(hcsr04_t * sensor) {
    if(sensor == NULL) {
        return HCSR04_RC_BAD_ARG;
    }

    // Log start time of echo pulse
    sensor->start_time = get_absolute_time();

    return HCSR04_RC_OK;
}

/**
 * @brief   Handle a falling edge on the HC-SR04 echo pin.
 * @details Logs the time of the falling edge for future echo pulse width calculation and indicates that the echo pulse has been received.
 * @param   sensor          HC-SR04 sensor struct.
 * @return  hcsr04_rc_t     Return code indicating operation success/failure.
 *                          - HCSR04_RC_OK:               Operation successful.
 *                          - HCSR04_RC_BAD_ARG:          An invalid argument was provided.
 */
hcsr04_rc_t hcsr04_on_echo_pin_fall(hcsr04_t * sensor) {
    if(sensor == NULL) {
        return HCSR04_RC_BAD_ARG;
    }

    // Log end time of echo pulse
    sensor->end_time = get_absolute_time();

    // Indicate that echo pulse width measurement is complete
    sensor->echo_received = true;

    return HCSR04_RC_OK;
}

/**
 * @brief   Finish HC-SR04 distance measurement transaction by calculating distance from received echo pulse width.
 * @details Echo pulse must be received using on_echo_pin_rise and on_echo_pin_fall functions, ideally within a GPIO IRQ.
 *          The raw value of the pulse width is a non-atomic datatype (uint64_t), so it's easier to just calculate and use the distance.
 * @param   sensor          HC-SR04 sensor struct.
 * @return  hcsr04_rc_t     Return code indicating operation success/failure.
 *                          - HCSR04_RC_OK:             Operation successful.
 *                          - HCSR04_RC_BAD_ARG:        An invalid argument was provided.
 *                          - HCSR04_RC_NO_ECHO:        The echo pulse has not been received yet.
 */
hcsr04_rc_t hcsr04_end_measurement(hcsr04_t * sensor) {
    if(sensor == NULL) {
        return HCSR04_RC_BAD_ARG;
    }

    if(!sensor->echo_received) {
        return HCSR04_RC_NO_ECHO;
    }

    // Calculate distance from duration and store in sensor struct
    calculate_distance(sensor);

    // Set sensor state to idle now that measurement is complete
    sensor->state = HCSR04_IDLE;

    return HCSR04_RC_OK;
}
