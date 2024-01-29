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

    while(1) {
        if(hc06_has_received_data(&hc06)) {            
            char buf[HC06_BUF_SIZE] = {0};
            uint8_t rx_bytes = hc06_receive_non_blocking(&hc06, buf, HC06_BUF_SIZE);
            
            if(rx_bytes > 0) {
                uart_puts(uart0, buf);
            }
        }

        sleep_ms(1000);
    }
}