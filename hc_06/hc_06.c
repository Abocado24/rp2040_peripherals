#include "hc_06.h"

// Global pointer to hc06 instance for interrupt handler
// This means that only one hc06 instance can be used at a time
hc06_t * p_hc06 = NULL;

void hc06_init(hc06_t * hc06, uart_inst_t * uart_id, uint32_t baud_rate, uint8_t uart_tx_pin, uint8_t uart_rx_pin, uint8_t uart_irq)
{
    // Register data provided by args into hc06 struct
    hc06->uart_id = uart_id;
    hc06->baud_rate = baud_rate;
    hc06->uart_tx_pin = uart_tx_pin;
    hc06->uart_rx_pin = uart_rx_pin;
    hc06->uart_irq = uart_irq;

    // Clear rx buffer
    for(uint8_t i = 0; i < BUF_SIZE; i++) {
        hc06->rx_buffer.buffer[i] = 0;
    }

    // Set global pointer to hc06 instance
    p_hc06 = hc06;

    // Reset head and tail positions of rx buffer
    hc06->rx_buffer.head = 0;
    hc06->rx_buffer.tail = 0;

    // Set up our UART with the required speed.
    uart_init(hc06->uart_id, hc06->baud_rate);

    // Set the TX and RX pins by using the function select on the GPIO
    gpio_set_function(hc06->uart_tx_pin, GPIO_FUNC_UART);
    gpio_set_function(hc06->uart_rx_pin, GPIO_FUNC_UART);

    // Set up and enable interrupt handler for UART
    irq_set_exclusive_handler(hc06->uart_irq, hc06_uart_rx);
    irq_set_enabled(hc06->uart_irq, true);
    uart_set_irq_enables(hc06->uart_id, true, false);
}

// Check if rx buffer is empty
bool hc06_rx_buffer_empty(hc06_t* hc06)
{
    return hc06->rx_buffer.tail == hc06->rx_buffer.head;
}

// Interrupt handler for receiving UART data
void hc06_uart_rx(void) {
    if(p_hc06 != NULL) {
        // Read from UART FIFO until it's empty
        while(uart_is_readable(p_hc06->uart_id)) {
            // Read a byte from the FIFO
            uint8_t rx_data = uart_getc(p_hc06->uart_id);

            // Write the byte to the circular buffer
            p_hc06->rx_buffer.buffer[p_hc06->rx_buffer.head] = rx_data;

            // Increment head position
            p_hc06->rx_buffer.head = (p_hc06->rx_buffer.head + 1) % BUF_SIZE; 
        }
    }
}

// Transfer data from rx buffer to a buffer provided by the user
// Returns number of bytes transferred
uint8_t hc06_transfer_rx_buffer(hc06_t* hc06, char buf[], uint8_t buf_len)
{
    // Handle case where buffer is too small
    if(buf_len < BUF_SIZE) {
        return 0;
    }

    // Number of bytes transferred (is also an index)
    uint8_t i=0;

    // Disable interrupts
    irq_set_enabled(hc06->uart_irq, false);
    {
        // Read from circular buffer until it's empty
        while(!hc06_rx_buffer_empty(hc06)) {
            // Read a byte from the circular buffer
            uint8_t rx_data = hc06->rx_buffer.buffer[hc06->rx_buffer.tail];

            // Write the byte to the buffer using index
            buf[i] = rx_data;

            // Increment tail position
            hc06->rx_buffer.tail = (hc06->rx_buffer.tail + 1) % BUF_SIZE;

            // Increment index
            i++;
        }
    }
    // Enable interrupts
    irq_set_enabled(hc06->uart_irq, true);

    // Return number of bytes transferred
    return i;
}

