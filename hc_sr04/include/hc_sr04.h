/**
 * @file    hc_sr04.h
 * @brief   Defines a struct for interfacing with the HC-SR04 Ultrasonic Distance Sensor.
 */

#ifndef HCSR04_H
#define HCSR04_H

// Includes
#include "pico/stdlib.h"
#include "hardware/gpio.h"

typedef enum {
    HCSR04_IDLE,
    HCSR04_BUSY
} hcsr04_state_t;

typedef enum {
    HCSR04_RC_OK            = 0,
    HCSR04_RC_BAD_ARG       = 1,
    HCSR04_RC_BUSY          = 2,
    HCSR04_RC_NO_ECHO       = 3,
} hcsr04_rc_t;

typedef struct {
    // GPIO pins
    uint8_t trigger_pin;
    uint8_t echo_pin;
   
    // Sensor data
    float current_distance;
    volatile bool echo_received;
    volatile absolute_time_t start_time;
    volatile absolute_time_t end_time;
    hcsr04_state_t state;
} hcsr04_t;

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
hcsr04_rc_t hcsr04_init(hcsr04_t * sensor, uint8_t trigger_pin, uint8_t echo_pin);

/**
 * @brief   Reset HC-SR04 sensor to a default state.
 * @details Call this function to recover from an error.
 * @param   sensor          HC-SR04 sensor struct.
 * @return  hcsr04_rc_t     Return code indicating operation success/failure.
 *                          - HCSR04_RC_OK:               Operation successful.
 *                          - HCSR04_RC_BAD_ARG:          An invalid argument was provided.
 */
hcsr04_rc_t hcsr04_reset(hcsr04_t * sensor);

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
hcsr04_rc_t hcsr04_start_measurement(hcsr04_t * sensor);

/**
 * @brief   Handle a rising edge on the HC-SR04 echo pin.
 * @details Logs the time of the rising edge for future echo pulse width calculation.
 * @param   sensor          HC-SR04 sensor struct.
 * @return  hcsr04_rc_t     Return code indicating operation success/failure.
 *                          - HCSR04_RC_OK:               Operation successful.
 *                          - HCSR04_RC_BAD_ARG:          An invalid argument was provided.
 */
hcsr04_rc_t hcsr04_on_echo_pin_rise(hcsr04_t * sensor);

/**
 * @brief   Handle a falling edge on the HC-SR04 echo pin.
 * @details Logs the time of the falling edge for future echo pulse width calculation and indicates that the echo pulse has been received.
 * @param   sensor          HC-SR04 sensor struct.
 * @return  hcsr04_rc_t     Return code indicating operation success/failure.
 *                          - HCSR04_RC_OK:               Operation successful.
 *                          - HCSR04_RC_BAD_ARG:          An invalid argument was provided.
 */
hcsr04_rc_t hcsr04_on_echo_pin_fall(hcsr04_t * sensor);

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
hcsr04_rc_t hcsr04_end_measurement(hcsr04_t * sensor);

#endif // HCSR04_H
