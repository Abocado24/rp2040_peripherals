#include "hc_06.h"

// Global pointer to hc06 instance for interrupt handler
// This means that only one hc06 instance can be used at a time
hc06_t * p_hc06 = NULL;

// Helper function to check if circular buffer is empty
static bool buffer_is_empty(circular_buffer_t * buffer) {
    return buffer->head == buffer->tail;
}

// Helper function to check if circular buffer is full
static bool buffer_is_full(circular_buffer_t * buffer) {
    return (buffer->head + 1) % HC06_MSG_SIZE == buffer->tail;
}

// Helper function to send data in tx buffer for interrupt handler
static void handle_tx(void) {
    // Check if there's data to send and the UART is ready to transmit
    while (!buffer_is_empty(&p_hc06->tx_buffer) && uart_is_writable(p_hc06->uart_id)) {
        // Get the next character from the tx buffer
        uint8_t tx_data = p_hc06->tx_buffer.buffer[p_hc06->tx_buffer.tail];
        p_hc06->tx_buffer.tail = (p_hc06->tx_buffer.tail + 1) % HC06_MSG_SIZE;

        // Send the character
        uart_putc(p_hc06->uart_id, tx_data);
    }

    // If the tx buffer is empty, set message_sent flag and disable TX interrupt
    if (buffer_is_empty(&p_hc06->tx_buffer)) {
        p_hc06->message_sent = true;
        uart_set_irq_enables(p_hc06->uart_id, true, false); // Disable TX interrupt
    }
}


// Helper function to store received hc06 message for interrupt handler
static void handle_rx(void) {
    // Read from UART until there's no data left from the current message
    while (uart_is_readable(p_hc06->uart_id)) {
        uint8_t rx_data = uart_getc(p_hc06->uart_id);

        // If buffer is not full, store the received data
        if (!buffer_is_full(&p_hc06->rx_buffer)) {
            // Store received data in circular buffer
            p_hc06->rx_buffer.buffer[p_hc06->rx_buffer.head] = rx_data;
            p_hc06->rx_buffer.head = (p_hc06->rx_buffer.head + 1) % HC06_MSG_SIZE;

            // Set message_received flag if newline character is received
            if (rx_data == '\n') {
                p_hc06->message_received = true;
                // Exit loop as a complete message is received
                break;
            }
        } 
        else {
            // Buffer is full, place a newline at the end of the buffer
            p_hc06->rx_buffer.buffer[(p_hc06->rx_buffer.head - 1 + HC06_MSG_SIZE) % HC06_MSG_SIZE] = '\n';
            p_hc06->message_received = true; // Mark message as received

            // Discard the rest of the message if buffer is full
            // Continue reading until the end of the message
            if (rx_data == '\n') {
                // Exit loop as the end of the current message is reached
                break;
            }
        }
    }
}

// Helper function to handle UART interrupts
// Determines whether to handle TX or RX interrupt and calls the appropriate handler
static void handle_uart_irq(void) {
    if (p_hc06 != NULL) {
        // Check if TX interrupt is enabled and the UART TX FIFO is empty
        if (uart_is_writable(p_hc06->uart_id)) {
            handle_tx();
        }

        // Check if RX interrupt is enabled and the UART RX FIFO is not empty
        if (uart_is_readable(p_hc06->uart_id)) {
            handle_rx();
        }
    }
}

// Initialize HC-06 module
void hc06_init(hc06_t * hc06, uart_inst_t * uart_id, uint32_t baud_rate, uint8_t uart_tx_pin, uint8_t uart_rx_pin, uint8_t uart_irq)
{
    // Register data provided by args into hc06 struct
    hc06->uart_id = uart_id;
    hc06->baud_rate = baud_rate;
    hc06->uart_tx_pin = uart_tx_pin;
    hc06->uart_rx_pin = uart_rx_pin;
    hc06->uart_irq = uart_irq;

    // Set global pointer to hc06 instance
    p_hc06 = hc06;

    // Clear tx and rx buffers
    for(uint8_t i = 0; i < HC06_MSG_SIZE; i++) {
        hc06->tx_buffer.buffer[i] = 0;
        hc06->rx_buffer.buffer[i] = 0;
    }

    // Reset head and tail positions of tx and rx buffers
    hc06->tx_buffer.head = 0;
    hc06->tx_buffer.tail = 0;
    hc06->rx_buffer.head = 0;
    hc06->rx_buffer.tail = 0;

    // Set initial values for message flags
    hc06->message_sent = false;
    hc06->message_received = false;

    // Set up our UART with the required speed.
    uart_init(hc06->uart_id, hc06->baud_rate);

    // Set the TX and RX pins by using the function select on the GPIO
    gpio_set_function(hc06->uart_tx_pin, GPIO_FUNC_UART);
    gpio_set_function(hc06->uart_rx_pin, GPIO_FUNC_UART);

    // Set up and enable interrupt handler for UART
    irq_set_exclusive_handler(hc06->uart_irq, handle_uart_irq);
    irq_set_enabled(hc06->uart_irq, true);
    uart_set_irq_enables(hc06->uart_id, true, false);
}

bool hc06_send_message_blocking(hc06_t* hc06, const char* message, uint32_t timeout_ms) {
    // Exit if invalid input
    if (!hc06 || !message) {
        return false;
    }

    // Start the timer
    absolute_time_t start_time = get_absolute_time();
    
    // Send each character of the message
    while (*message) {
        // Check for timeout
        if (absolute_time_diff_us(start_time, get_absolute_time()) >= timeout_ms * 1000) {
            return false; // Timeout occurred
        }

        // Wait for the UART to become ready to transmit
        while (!uart_is_writable(hc06->uart_id)) {
            tight_loop_contents(); // Can be replaced with sleep_ms(1) for less CPU usage
        }

        // Send the character
        uart_putc(hc06->uart_id, *message++);
    }

    // Wait for the last byte to be completely sent
    while (!uart_is_writable(hc06->uart_id)) {
        if (absolute_time_diff_us(start_time, get_absolute_time()) >= timeout_ms * 1000) {
            return false; // Timeout occurred
        }
        tight_loop_contents(); // Can be replaced with sleep_ms(1) for less CPU usage
    }

    return true; // Message sent successfully
}

uint8_t hc06_send_message_non_blocking(hc06_t* hc06, const char* msg, uint8_t len) {
    // Exit if another message is currently being sent
    if(!hc06_message_sent(hc06)) {
        return 0;
    }
    
    if (hc06 == NULL || msg == NULL) {
        return 0;
    }

    uint8_t bytes_copied = 0;
    for (uint8_t i = 0; i < len; ++i) {
        // Check if tx buffer is full
        if (buffer_is_full(&hc06->tx_buffer)) {
            break; // No more space in the tx buffer
        }

        // Store the character in the tx buffer
        hc06->tx_buffer.buffer[hc06->tx_buffer.head] = msg[i];
        hc06->tx_buffer.head = (hc06->tx_buffer.head + 1) % HC06_MSG_SIZE;
        bytes_copied++;
    }

    hc06->message_sent = false; // Reset message_sent flag

    // Enable TX interrupt to start sending
    uart_set_irq_enables(hc06->uart_id, true, true); // Enable RX and TX interrupts

    return bytes_copied;
}

// Receive a message from HC-06 in a blocking manner with timeout
// Returns number of bytes received or 0 if timed out
uint8_t hc06_receive_message_blocking(hc06_t* hc06, char buf[], uint8_t len, uint32_t timeout_ms) {
    // Exit if invalid input
    if (!hc06 || len != HC06_MSG_SIZE) {
        return 0;
    }

    // Number of bytes transferred (is also an index)
    uint8_t i = 0;
    // Start time for timeout calculation
    absolute_time_t start_time = get_absolute_time();

    // Read from UART until a newline character is received or the buffer is full
    while (i < len) {
        // Check if data is available within the timeout period
        if (uart_is_readable_within_us(hc06->uart_id, timeout_ms * 1000)) {
            uint8_t rx_data = uart_getc(hc06->uart_id);
            buf[i++] = rx_data;

            // Break the loop if newline character is received
            if (rx_data == '\n') {
                break;
            }
        } else {
            // Check if the timeout has been reached
            if (absolute_time_diff_us(start_time, get_absolute_time()) > timeout_ms * 1000) {
                // If no data received, return 0
                if (i == 0) {
                    return 0;
                }
                break;  // Exit loop on timeout
            }
        }
    }

    // Add a newline character to the end of the buffer if it doesn't fit and discard the rest of the message
    if (i == len && buf[i - 1] != '\n') {
        buf[i - 1] = '\n';
        while (uart_is_readable(hc06->uart_id) && uart_getc(hc06->uart_id) != '\n');
    }
    // Null terminate the string if it's smaller than the buffer
    else if (i < len) {
        buf[i] = '\0';
    }

    // Return the number of bytes received
    return i;
}

// Transfer data from rx buffer to a buffer provided by the user
// Returns number of bytes transferred
uint8_t hc06_receive_message_non_blocking(hc06_t* hc06, char buf[], uint8_t len)
{
    // Exit if another message is currently being received
    if(!hc06_message_received(hc06)) {
        return 0;
    }

    // Exit on invalid input
    if(!hc06 || !hc06->message_received || len != HC06_MSG_SIZE) {
        return 0;
    }

    // Number of bytes transferred (is also an index)
    uint8_t i=0;

    // Disable interrupts
    irq_set_enabled(hc06->uart_irq, false);
    {
        // Read from rx buffer until it's empty
        while(!buffer_is_empty(&hc06->rx_buffer)) {
            // Read a byte from the circular buffer
            uint8_t rx_data = hc06->rx_buffer.buffer[hc06->rx_buffer.tail];

            // Write the byte to the buffer using index
            buf[i] = rx_data;

            // Increment tail position
            hc06->rx_buffer.tail = (hc06->rx_buffer.tail + 1) % HC06_MSG_SIZE;

            // Increment index
            i++;
        }

        // Null terminate the string if it's smaller than the buffer
        if(i < len) {
            buf[i] = '\0';
        }
    }
    // Clear the message_received flag after reading the message
    hc06->message_received = false;

    // Enable interrupts
    irq_set_enabled(hc06->uart_irq, true);

    // Return number of bytes transferred
    return i;
}

bool hc06_message_sent(hc06_t* hc06)
{
    return hc06->message_sent;
}

// Check if a complete message has been received
bool hc06_message_received(hc06_t* hc06)
{
    return hc06->message_received;
}

