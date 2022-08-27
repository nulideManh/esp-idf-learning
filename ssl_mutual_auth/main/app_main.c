#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "driver/gpio.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_log.h"
#include "app_config.h"
#include "app_mqtt.h"
#include "app_nvs.h"

static const char *TAG = "Sonoff";
#define KEY "restart_cnt"
#define KEY1 "string"

static void event_handler(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data)
{
    if (event_base == MQTT_DEV_EVENT) {
        if (event_id == MQTT_DEV_EVENT_CONNECTED) {
            ESP_LOGW(TAG, "MQTT_DEV_EVENT_CONNECTED");
            mqtt_sub("ngao/led");
        }
        else if (event_id == MQTT_DEV_EVENT_DISCONNECT) {
            ESP_LOGW(TAG, "MQTT_DEV_EVENT_DISCONNECT");
        }
        else if (event_id == MQTT_DEV_EVENT_SUBCRIBED) {
            ESP_LOGW(TAG, "MQTT_DEV_EVENT_SUBCRIBED");
        }
    }
}

void mqtt_data_callback(char* data, int len) {
    if (strstr(data, "OFF")) {
        gpio_set_level(3, 0);
    }
    else if (strstr(data, "ON")) {
        gpio_set_level(3, 1);
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(esp_event_handler_register(MQTT_DEV_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));

    // on/ off led
    gpio_pad_select_gpio(3);
    gpio_set_direction(3, GPIO_MODE_INPUT_OUTPUT);

    /* nvs
    int restart_cnt = 0;
    app_nvs_get_value(KEY, &restart_cnt);
    restart_cnt++;
    app_nvs_set_value(KEY, restart_cnt);

    char arr[50];
    char arr_set[50] = "";
    sprintf(arr_set, "Ngao Ngo %d", restart_cnt);
    app_nvs_get_string(KEY1, arr);
    app_nvs_set_string(KEY1,arr_set); 
    */

    /* json
    json_gen_test_result_t result;
    json_gen_test(&result, "onoff", true, "id", 1234, "str", "ngao ngo"); 
    */
    mqtt_set_callback(mqtt_data_callback);
    app_config();
    mqtt_app_start();
    
    //chac chan la connected wifi

}
