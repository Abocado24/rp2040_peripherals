#include "hc_06/hc_06.h"

#include "pico/stdlib.h"

#define UART_ID uart0
#define UART_BAUDRATE 9600
#define UART_TX_PIN 0
#define UART_RX_PIN 1
#define UART_IRQ UART0_IRQ

// Demo HC-06 library by echoing back received data
int main()
{
    hc06_t hc06;
    hc06_init(&hc06, UART_ID, UART_BAUDRATE, UART_TX_PIN, UART_RX_PIN, UART_IRQ);

    char init_msg[] = "UART for HC-06 is set up\n";
    uart_puts(uart0, init_msg);

    bool use_blocking = true;

    while(1) {
        if(use_blocking) {
            char buf[HC06_MSG_SIZE] = {0};
            uint8_t rx_bytes = hc06_receive_message_blocking(&hc06, buf, HC06_MSG_SIZE, HC06_DEFAULT_TIMEOUT_MS);

            if(rx_bytes > 0) {
                hc06_send_message_blocking(&hc06, buf, HC06_DEFAULT_TIMEOUT_MS);
            }

            // Alternate between blocking and non-blocking implementations to test both
            use_blocking = !use_blocking;
        }
        else {
            if(hc06_message_received(&hc06)) {            
                char buf[HC06_MSG_SIZE] = {0};
                uint8_t rx_bytes = hc06_receive_message_non_blocking(&hc06, buf, HC06_MSG_SIZE);
            
                if(rx_bytes > 0) {
                    hc06_send_message_non_blocking(&hc06, buf, rx_bytes);
                }

                // Alternate between blocking and non-blocking implementations to test both
                use_blocking = !use_blocking;
            }
        }

        sleep_ms(1000);
    }
}