/**
 * @file    hc_06_demo.c
 * @brief   Demo for HC-06 driver functionality.
 * 
 * @section Dependencies
 * - 'hc_06.h':   The HC-06 driver being demonstrated in this file.
 */


#include <pico/stdlib.h>
#include "hc_06.h"

#define UART_ID uart0
#define UART_BAUDRATE 9600
#define UART_TX_PIN 0
#define UART_RX_PIN 1
#define UART_IRQ UART0_IRQ

// Demo HC-06 library by echoing back received data
int main()
{
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    uint8_t rx_buffer[HC06_DEFAULT_BUFFER_SIZE];
    uint8_t tx_buffer[HC06_DEFAULT_BUFFER_SIZE];

    hc06_t hc06;
    hc06_init(&hc06, UART_ID, UART_BAUDRATE, UART_IRQ, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

    char init_msg[] = "UART for HC-06 is set up\n";
    uart_puts(uart0, init_msg);

    while(1) {
        // Wait for a new message to echo
        if(hc06.message_received) {
            // Receive it
            char rx_buf[HC06_DEFAULT_MSG_SIZE];
            memset(rx_buf, 0, HC06_DEFAULT_MSG_SIZE);
            uint16_t chars_received = 0;
            hc06_rx_msg(&hc06, rx_buf, HC06_DEFAULT_MSG_SIZE, &chars_received);

            // Then send it back
            uint16_t chars_sent;
            hc06_tx_msg(&hc06, rx_buf, chars_received, &chars_sent);
        }

        sleep_ms(1000);
    }
}