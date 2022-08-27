#include "driver/gpio.h"

// cau hinh 1 chan la output
void esp_output_create(int pin)
{
    gpio_pad_select_gpio(pin);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(pin, GPIO_MODE_OUTPUT);    
}

void esp_output_set_level(int pin, int level)
{
    gpio_set_level(pin, level);
}