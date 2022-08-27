#ifndef __RGB_LED_H
#define __RGB_LED_H

// GPIO
#define RGB_LED_RED_GPIO 5
#define RGB_LED_GREEN_GPIO 17
#define RGB_LED_BLUE_GPIO 16

// color mix channels
#define RGB_LED_CHANNEL_NUM 3

// bool g_pwm_init_handle = false;

// RGB_LED cofiguration
typedef struct {
    int channel;
    int gpio;
    int mode;
    int timer_index;
} ledc_info_t;
ledc_info_t ledc_ch[RGB_LED_CHANNEL_NUM];

// void rgb_led_pwm_init(void);

// void rgb_led_set_color(uint8_t red, uint8_t green, uint8_t blue);

void rgb_led_test(uint8_t r, uint8_t g, uint8_t b);

void rgb_led_started(uint8_t r, uint8_t g, uint8_t b);

void rgb_led_wifi_app_started(void);



#endif