#include "hc_06.h"

// Global information related to hc06 interrupt handler
hc06_t * hc06 = NULL;
bool tx_irq_enabled = false;
bool rx_irq_enabled = false;

/**
 * @brief   Helper function that sets the status of UART irqs
 * @details We want to be able to independently enable/disable the tx and rx interrupts for their critical selections.
 *          This is done in conjunction with setting tx_irq_enabled and rx_irq_enabled for simplicity since we're assuming only one HC-06 device.
 * @param   uart_id         The RP2040 UART peripheral to use.
 */
static inline void set_uart_irqs(uart_inst_t * uart_id) {
    uart_set_irq_enables(uart_id, rx_irq_enabled, tx_irq_enabled);
}

/**
 * @brief   Helper function to send data in tx buffer for interrupt handler
 */
static void handle_tx(void) {
    // Check if there's data to send and the UART is ready to transmit
    while (!circular_buffer_is_empty(&hc06->tx_buffer) && uart_is_writable(hc06->uart_id)) {
        // Get the next character from the tx buffer
        uint8_t tx_data;
        circular_buffer_pop(&hc06->tx_buffer, &tx_data, sizeof(uint8_t));

        // Send the character
        uart_putc(hc06->uart_id, tx_data);
    }

    // If the tx buffer is empty, mark transaction as completed by setting status flag and disable TX interrupt
    if (circular_buffer_is_empty(&hc06->tx_buffer)) {
        hc06->message_sent = true;
        uart_set_irq_enables(hc06->uart_id, true, false);
    }
}

/**
 * @brief   Helper function to store received hc06 messages for rx function
 * @details Will replace oldest message data on overrun
 */
static void handle_rx(void) {
    // Read from UART until there's no data left from the current message
    while (uart_is_readable(hc06->uart_id)) {
        uint8_t rx_data = uart_getc(hc06->uart_id);

        circular_buffer_push(&hc06->rx_buffer, &rx_data, sizeof(uint8_t));

        // End of message is denoted by newline, set flag so user knows to call rx_msg
        if(rx_data == '\n') {
            hc06->message_received = true;
        }
    }
}

/**
 * @brief   Helper function to handle TX/RX UART interrupts
 */
static void handle_uart_irq(void) {
    if (hc06 != NULL) {
        // Check if a TX UART interrupt was raised
        if (uart_is_writable(hc06->uart_id)) {
            handle_tx();
        }

        // Check if a RX UART interrupt was raised
        if (uart_is_readable(hc06->uart_id)) {
            handle_rx();
        }
    }
}

/**
 * @brief   Initialize the ability to interface with the HC-06 device.
 * @details Assumes the HC-06 device has already been configured to desired settings.
 *          Assumes the UART pins have already been set to the UART function.
 *          Assumes that the system will only use one HC-06 device.
 * @param   device          The HC-06 device struct.
 * @param   uart_id         The RP2040 UART peripheral to use with this device.
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
hc06_rc_t hc06_init(hc06_t * device, uart_inst_t * uart_id, uint32_t uart_baudrate, uint8_t uart_irq, uint8_t * tx_buffer, uint16_t tx_buffer_size, uint8_t * rx_buffer, uint16_t rx_buffer_size) {
    if(device == NULL || uart_id == NULL || tx_buffer == NULL || rx_buffer == NULL) {
        return HC06_RC_BAD_ARG;
    }
    
    // Register data provided by args into hc06 struct
    device->uart_id = uart_id;
    device->uart_baudrate = uart_baudrate;
    device->uart_irq = uart_irq;

    // Set global pointer to hc06 instance
    hc06 = device;

    // Initialize tx circular buffer
    circular_buffer_rc_t tx_buffer_init_rc = circular_buffer_init(&device->tx_buffer, tx_buffer, tx_buffer_size, sizeof(uint8_t));
    if(tx_buffer_init_rc != CIRCULAR_BUFFER_RC_OK) {
        return HC06_RC_ERROR_TX_BUFFER;
    }

    // Initialize rx circular buffer
    circular_buffer_rc_t rx_buffer_init_rc = circular_buffer_init(&device->rx_buffer, rx_buffer, rx_buffer_size, sizeof(uint8_t));
    if(rx_buffer_init_rc != CIRCULAR_BUFFER_RC_OK) {
        return HC06_RC_ERROR_RX_BUFFER;
    }
    
    // Clear tx and rx buffer data before use
    memset(tx_buffer, 0, tx_buffer_size);
    memset(rx_buffer, 0, rx_buffer_size);

    // Set up UART with specified baudrate
    uart_init(device->uart_id, device->uart_baudrate);

    // Set tx and rx status flags
    device->message_sent = false;
    device->message_received = false;

    // Set up and enable interrupt handler for UART
    irq_set_exclusive_handler(device->uart_irq, handle_uart_irq);
    irq_set_enabled(device->uart_irq, true);
    tx_irq_enabled = false;
    rx_irq_enabled = true;
    set_uart_irqs(uart_id);

    return HC06_RC_OK;
}

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
hc06_rc_t hc06_tx_msg(hc06_t* device, char* tx_buf, uint16_t len, uint16_t * chars_sent) {    
    if(device == NULL || tx_buf == NULL || chars_sent == NULL) {
        return HC06_RC_BAD_ARG;
    }

    // rc is subject to change depending on whether the circular buffer fills during pushing
    hc06_rc_t rc = HC06_RC_OK;

    // Mark beginning of tx transaction by clearing status flag, is set in tx interrupt
    device->message_sent = false;

    // Entering critical selection; disable tx irq when modifying tx buffer to prevent race conditions
    tx_irq_enabled = false;
    set_uart_irqs(device->uart_id);
    {
        // Push characters from message buffer to tx buffer, stop early if tx buffer fills during pushing
        for (*chars_sent = 0; *chars_sent < len; ++*chars_sent) {
            if(circular_buffer_is_full(&device->tx_buffer)) {
                rc = HC06_RC_ERROR_TX_BUFFER;
                break;
            }

            circular_buffer_push(&device->tx_buffer, &tx_buf[*chars_sent], sizeof(char));
        }
    }
    // Exiting critical selection; enable tx irq to start/resume sending tx data
    tx_irq_enabled = true;
    set_uart_irqs(device->uart_id);

    return rc;
}

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
hc06_rc_t hc06_rx_msg(hc06_t* device, char * rx_buf, uint16_t len, uint16_t * chars_received) {
    if(device == NULL || rx_buf == NULL || len < device->rx_buffer.buffer_capacity || chars_received == NULL) {
        return HC06_RC_BAD_ARG;
    }

    // Entering critical selection; disable rx irq when modifying rx buffer to prevent race conditions
    rx_irq_enabled = false;
    set_uart_irqs(device->uart_id);
    {
        // Pop characters from rx buffer and store in message buffer
        *chars_received = 0;
        while(!circular_buffer_is_empty(&device->rx_buffer)) {
            char rx_char;
            circular_buffer_pop(&device->rx_buffer, &rx_char, sizeof(char));
            
            rx_buf[*chars_received] = rx_char;
            (*chars_received)++;

            // Exit early on newline, as it should denote the end of a message. This covers the circular buffer containing multiple messages.
            if(rx_char == '\n') {
                break;
            }
        }

        // Null terminate the received string if it's smaller than the buffer
        if(*chars_received < len) {
            rx_buf[*chars_received] = '\0';
        }
    }
    // Exiting critical selection; enable rx irq to resume receiving rx data
    rx_irq_enabled = true;
    set_uart_irqs(device->uart_id);

    // Mark completion of rx transaction by clearing status flag
    device->message_received = false;

    return HC06_RC_OK;
}