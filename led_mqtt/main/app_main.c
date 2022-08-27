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
#include "ledc_io.h"
#include "rgb_led.h"
#include "output_iot.h"
#include "json_parser.h"
#include "json_generator.h"

static const char *TAG = "MAIN";
#define KEY "restart_cnt"
#define KEY1 "string"

json_gen_test_result_t rs;

static void event_handler(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data)
{
    if (event_base == MQTT_DEV_EVENT) {
        if (event_id == MQTT_DEV_EVENT_CONNECTED) {
            ESP_LOGW(TAG, "MQTT_DEV_EVENT_CONNECTED");
            //mqtt_sub("ngao/led");
            
        }
        else if (event_id == MQTT_DEV_EVENT_DISCONNECT) {
            ESP_LOGW(TAG, "MQTT_DEV_EVENT_DISCONNECT");
        }
        else if (event_id == MQTT_DEV_EVENT_SUBCRIBED) {
            ESP_LOGW(TAG, "MQTT_DEV_EVENT_SUBCRIBED");
        }
    }
}

// static int json_gen_ack(json_gen_test_result_t *result, char *deviceId, bool status, int intensity, char* rgb, int mode)
// {
// 	char buf[20];
//     memset(result, 0, sizeof(json_gen_test_result_t));
// 	json_gen_str_t jstr;
// 	json_gen_str_start(&jstr, buf, sizeof(buf), flush_str, result);
// 	json_gen_start_object(&jstr);//{
//         json_gen_obj_set_string(&jstr, "deviceId", deviceId);
//         json_gen_obj_set_bool(&jstr, "status", status);
//         json_gen_obj_set_int(&jstr, "intensity", intensity);
//         json_gen_obj_set_string(&jstr, "rgb", rgb);
//         json_gen_obj_set_int(&jstr, "mode", mode);
// 	json_gen_end_object(&jstr); // }
// 	json_gen_str_end(&jstr);
//     return 0;
//     // result->buf
// }
int new_mode;

void handle_data(int r, int g, int b) {
    while(new_mode == 2) {
        rgb_led_off();
        vTaskDelay(500 / portTICK_PERIOD_MS);
        rgb_led_test(r,g,b);
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}

void mqtt_data_callback(char* data, uint16_t len) {
    data[len] = 0;
    printf("DATA=%s\r\n", data); // deviot.vn\0@@####$$$$
    // parser JSON
    jparse_ctx_t jctx;
	int ret = json_parse_start(&jctx, data, len);
	if (ret != OS_SUCCESS) {
		printf("Parser failed\n");
		return;
	}   

    char deviceId[50];
    if (json_obj_get_string(&jctx, "deviceId", deviceId, 50) == OS_SUCCESS)
        printf("Get device ID: %s\n", deviceId);

    bool status;
    if (json_obj_get_bool(&jctx, "status", &status) == OS_SUCCESS)
        printf("Get state: %s\n", status ? "true" : "false");

    int intensity;
	if (json_obj_get_int(&jctx, "intensity", &intensity) == OS_SUCCESS)
		printf("Get intensity: %d\n", intensity);

    char rgb[10];
    if (json_obj_get_string(&jctx, "rgb", rgb, 10) == OS_SUCCESS) 
        printf("Get rgb: %s\n", rgb);

    int mode;
	if (json_obj_get_int(&jctx, "mode", &mode) == OS_SUCCESS) {
        printf("Get mode: %d\n", mode);
        new_mode = mode;
        //check_mode = true;
    }
		
    
    json_parse_end(&jctx);

    if(strcmp(deviceId, "62bd097e3e5f8e5cc2c5849b") == 0){
        output_io_create(3);
        output_io_set_level(3, status);
    }
    else if(strcmp(deviceId, "62bd097e3e5f8e5cc2c5849a") == 0) {
        pwm_init(19);
        ESP_LOGI(TAG, "Duty: %d", intensity);
        pwm_set_duty(intensity);
    }
    else if (strcmp(deviceId, "62bd097e3e5f8e5cc2c5849c") == 0) {
        char val[3] = {0,0,0};
        int r,g,b;
        char* s;
        val[0] = *(rgb + 1);
        val[1] = *(rgb + 2);
        r = strtol(val, &s, 16);

        val[0] = *(rgb + 3);
        val[1] = *(rgb + 4);
        g = strtol(val, &s, 16);

        val[0] = *(rgb + 5);
        val[1] = *(rgb + 6);
        b = strtol(val, &s, 16);

        ESP_LOGI(TAG, "r = %d, g = %d, b = %d", r,g,b);
        if (mode == 1) {
            rgb_led_test(r,g,b);
        }
        else if (mode == 2) {
            handle_data(r, g, b);
        }
        else if (mode == 3) {
            rgb_led_red();
            vTaskDelay(500 / portTICK_PERIOD_MS); 
            rgb_led_orange();
            vTaskDelay(500 / portTICK_PERIOD_MS);
            rgb_led_yellow();
            vTaskDelay(500 / portTICK_PERIOD_MS);
            rgb_led_green();
            vTaskDelay(500 / portTICK_PERIOD_MS);
            rgb_led_blue();
            vTaskDelay(500 / portTICK_PERIOD_MS);
            rgb_led_indigo();
            vTaskDelay(500 / portTICK_PERIOD_MS);
            rgb_led_violet();
            vTaskDelay(500 / portTICK_PERIOD_MS);
            rgb_led_off();
            vTaskDelay(500 / portTICK_PERIOD_MS);
        }
        
    }

    json_gen_test(&rs, deviceId, "control", status, mode, 3, 5, intensity);
    mqtt_pub("/manhnv", rs.buf, strlen(rs.buf));
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
    output_io_create(3);
    pwm_init(19);

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

   
    json_gen_test_result_t result;
    json_gen_test(&result, "62bd097e3e5f8e5cc2c5849b", "control", false, "3", 3, 6, 5); 
    
    //mqtt_set_rgb_callback(mqtt_rgb_data_callback);    
    mqtt_set_data_callback(mqtt_data_callback);
    //mqtt_set_led_duty_callback(mqtt_duty_data_callback);
    
    app_config();
    mqtt_app_start();
    
    //chac chan la connected wifi

}
