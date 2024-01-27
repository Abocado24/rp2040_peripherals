#include "hc_06/hc_06.h"

#include "pico/stdlib.h"

// Demo HC-06 library by echoing back received data
// Uses UART0 (GPIO0 and GPIO1) at a baud rate of 9600
int main()
{
    hc06_t hc06;
    hc06_init(&hc06, uart0, 9600, 0, 1, UART0_IRQ);

    char init_msg[] = "UART for HC-06 is set up\n";
    uart_puts(uart0, init_msg);

    while(1) {
        if(!hc06_rx_buffer_empty(&hc06)) {            
            char buf[BUF_SIZE] = {0};
            uint8_t rx_bytes = hc06_transfer_rx_buffer(&hc06, buf, BUF_SIZE);
            
            if(rx_bytes > 0) {
                uart_puts(uart0, buf);
            }
        }

        sleep_ms(1000);
    }
}