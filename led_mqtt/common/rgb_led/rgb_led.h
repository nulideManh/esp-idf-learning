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

void rgb_led_test(uint8_t r, uint8_t g, uint8_t b);

void rgb_led_red(void);
void rgb_led_orange(void);
void rgb_led_yellow(void);
void rgb_led_green(void);
void rgb_led_blue(void);
void rgb_led_indigo(void);
void rgb_led_violet(void);
void rgb_led_off(void);

#endif