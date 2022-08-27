#ifndef _APP_UART_H_
#define _APP_UART_H_
#include <stdint.h>
#define APP_UART_TXD      (GPIO_NUM_4)
#define APP_UART_RXD      (GPIO_NUM_5)
#define APP_UART_BAURATE  115200
void uart1_init(int tx_pin,int rx_pin,int baurate);
int uart1_write_bytes(const char* data, uint32_t len);
int uart1_read_bytes(uint8_t* data, uint32_t len);
#endif