/**
 * @file    hc_06.h
 * @brief   Defines a struct for interfacing with the HC-06 Serial Wireless UART device.
 * 
 * @section Dependencies
 * - 'circular_buffer.h':   Provides circular buffers to store tx and rx message/character data for/from irqs.
 */

#ifndef HC_06_H
#define HC_06_H

// Includes
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/sync.h"
#include "hardware/irq.h"
#include "circular_buffer.h"

typedef enum {
    HC06_DEFAULT_BUFFER_SIZE    = 250,
    HC06_DEFAULT_MSG_SIZE       = 250,
} hc06_consts_t;

typedef enum {
    HC06_RC_OK                  = 0,
    HC06_RC_BAD_ARG             = 1,
    HC06_RC_ERROR_TX_BUFFER     = 2,
    HC06_RC_ERROR_RX_BUFFER     = 3,
} hc06_rc_t;

typedef struct {
    // UART
    uart_inst_t * uart_id;
    uint32_t uart_baudrate;
    uint8_t uart_irq;

    // Circular buffers used for non-blocking communication
    circular_buffer_t tx_buffer;
    circular_buffer_t rx_buffer;

    // Flags used to indicate current status of tx and rx transactions
    volatile bool message_sent;
    volatile bool message_received;
} hc06_t;

/**
 * @brief   Initialize the ability to interface with the HC-06 device.
 * @details Assumes the HC-06 device has already been configured to desired settings.
 *          Assumes the UART pins have already been set to the UART function.
 *          Assumes that the system will only use one HC-06 device.
 * @param   device          The HC-06 device struct.
 * @param   uart_id         The RP2040 UART peripheral to use with this device.
 * @param   uart_tx_pin     tx pin used for UART transmissions.
 * @param   uart_rx_pin     rx pin used for UART transmissions.
 * @param   uart_baudrate   The baudrate to use for UART communication.
 * @param   uart_irq        The handler for UART interrupts.
 * @param   tx_buffer       Buffer used to store data to transmit over UART.
 * @param   tx_buffer_size  Size of tx_buffer, is usually sizeof(tx_buffer).
 * @param   rx_buffer       Buffer used to store data to receive from UART.
 * @param   rx_buffer_size  Size of rx_buffer, is usually sizeof(rx_buffer).
 * @return  hc06_rc_t       Return code indicating operation success/failure.
 *                          - HC06_RC_OK:               Operation successful.
 *                          - HC06_RC_BAD_ARG:          An invalid argument was provided.
 *                          - HC06_RC_ERROR_TX_BUFFER:  Could not initialize tx buffer.
 *                          - HC06_RC_ERROR_RX_BUFFER:  Could not initialize rx buffer.
 */
hc06_rc_t hc06_init(hc06_t * device, uart_inst_t * uart_id, uint8_t uart_tx_pin, uint8_t uart_rx_pin, uint32_t uart_baudrate, uint8_t uart_irq, uint8_t * tx_buffer, uint16_t tx_buffer_size, uint8_t * rx_buffer, uint16_t rx_buffer_size);

/**
 * @brief   Transmit a new message through the HC-06 device.
 * @details Assumes that messages are strings terminating with the newline character.
 *          Is a non-blocking implementation; message data is obtained from rx interrupt as new data comes in.
 * @param   device          The HC-06 device struct.
 * @param   tx_buf          Message buffer to transmit.
 * @param   len             Length of message buffer.
 * @param   chars_sent      The number of characters from the message buffer that were transmitted (returned by reference)
 * @return  hc06_rc_t       Return code indicating operation success/failure.
 *                          - HC06_RC_OK:               Operation successful.
 *                          - HC06_RC_BAD_ARG:          An invalid argument was provided.
 *                          - HC06_RC_ERROR_TX_BUFFER:  The tx circular buffer has filled during pushing. Use chars_sent to recover as needed.
 */
hc06_rc_t hc06_tx_msg(hc06_t* device, char* tx_buf, uint16_t len, uint16_t * chars_sent);

/**
 * @brief   Receive a new message from the HC-06 device.
 * @details Assumes that messages are strings terminating with the newline character.
 *          Is a non-blocking implementation; message data is obtained from rx interrupt as new data comes in.
 * @param   device          The HC-06 device struct.
 * @param   rx_buf          Message buffer to receive in. Its size cannot be less than the capacity of the rx circular buffer.
 * @param   len             Length of message buffer. Cannot be less than the capacity of the rx circular buffer.
 * @param   chars_received  The number of characters from the message buffer that were received (returned by reference)
 * @return  hc06_rc_t       Return code indicating operation success/failure.
 *                          - HC06_RC_OK:               Operation successful.
 *                          - HC06_RC_BAD_ARG:          An invalid argument was provided.
 */
hc06_rc_t hc06_rx_msg(hc06_t* device, char * rx_buf, uint16_t len, uint16_t * chars_received);

#endif // HC_06_H
