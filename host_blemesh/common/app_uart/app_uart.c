#include "app_uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"


#define APP_UART_RTS  (UART_PIN_NO_CHANGE)
#define APP_UART_CTS  (UART_PIN_NO_CHANGE)
#define BUF_SIZE (1024)
uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
};
void uart1_init(int tx_pin,int rx_pin,int baurate)
{
    uart_config.baud_rate = baurate;
    uart_driver_install(UART_NUM_1, BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(UART_NUM_1, &uart_config);
    uart_set_pin(UART_NUM_1, tx_pin, rx_pin, APP_UART_RTS, APP_UART_CTS);
}
int uart1_write_bytes(const char* data, uint32_t len)
{
    return uart_write_bytes(UART_NUM_1, data, len);
}
int uart1_read_bytes(uint8_t* data, uint32_t len)
{
    return uart_read_bytes(UART_NUM_1, data, len, 20 / portTICK_RATE_MS);
}