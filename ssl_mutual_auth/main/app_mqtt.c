/* MQTT Mutual Authentication Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_log.h"
#include "mqtt_client.h"
#include "app_mqtt.h"
#include "json_parser.h"
#include "json_generator.h"

static const char *TAG = "MQTTS_APP";
ESP_EVENT_DEFINE_BASE(MQTT_DEV_EVENT);

extern const uint8_t client_cert_pem_start[] asm("_binary_client_crt_start");
extern const uint8_t client_cert_pem_end[] asm("_binary_client_crt_end");
extern const uint8_t client_key_pem_start[] asm("_binary_client_key_start");
extern const uint8_t client_key_pem_end[] asm("_binary_client_key_end");

static esp_mqtt_client_handle_t client;
static mqtt_handle_t mqtt_handle = NULL;

static void flush_str(char *buf, void *priv)
{
    json_gen_test_result_t *result = (json_gen_test_result_t *)priv;
    if (result) {
        if (strlen(buf) > sizeof(result->buf) - result->offset) {
            printf("Result Buffer too small\r\n");
            return;
        }
        memcpy(result->buf + result->offset, buf, strlen(buf));
        result->offset += strlen(buf);
    }
}

static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
{
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    // your_context_t *context = event->context;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            //msg_id = esp_mqtt_client_subscribe(client, "ngaongo/manh", 0);
            esp_event_post(MQTT_DEV_EVENT, MQTT_DEV_EVENT_CONNECTED, NULL, 0, portMAX_DELAY);
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            esp_event_post(MQTT_DEV_EVENT, MQTT_DEV_EVENT_DISCONNECT, NULL, 0, portMAX_DELAY);
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            //msg_id = esp_mqtt_client_publish(client, "ngaongo/dht11", "data",0,0,0);
            esp_event_post(MQTT_DEV_EVENT, MQTT_DEV_EVENT_SUBCRIBED, NULL, 0, portMAX_DELAY);
            //mqtt_pub("/topic/deviot", "data", 4);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            // printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            // printf("DATA=%.*s\r\n", event->data_len, event->data);
            //esp_event_post(MQTT_DEV_EVENT, MQTT_DEV_EVENT_DATA, NULL, 0, portMAX_DELAY);
            mqtt_handle(event->data, event->data_len);
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            break;
        default:
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            break;
    }
    return ESP_OK;
}

void mqtt_app_start(void)
{
    const esp_mqtt_client_config_t mqtt_cfg = {
        // .uri = "mqtts://test.mosquitto.org:8884",
        .uri = "mqtt://test.mosquitto.org:1883",
        .event_handle = mqtt_event_handler,
    };
    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    client = esp_mqtt_client_init(&mqtt_cfg);

    esp_mqtt_client_start(client);
}

void json_gen_test(json_gen_test_result_t *result, char* key1, bool value1, char* key2, int value2, char* key3, char* value3) 
{    
    char buf[20];
    memset(result, 0, sizeof(json_gen_test_result_t));
	json_gen_str_t jstr;
	json_gen_str_start(&jstr, buf, sizeof(buf), flush_str, result);

    json_gen_start_object(&jstr);
    json_gen_obj_set_bool(&jstr, key1, value1);
    json_gen_obj_set_int(&jstr, key2, value2);
    json_gen_obj_set_string(&jstr, key3, value3);
    json_gen_end_object(&jstr);

    json_gen_str_end(&jstr);
    printf("Result: %s\n", result->buf);
}

void mqtt_app_init(void)
{
    const esp_mqtt_client_config_t mqtt_cfg = {
        // .uri = "mqtts://test.mosquitto.org:8884",
        .uri = "mqtt://test.mosquitto.org:1883",
        .event_handle = mqtt_event_handler,
        // .client_cert_pem = (const char *)client_cert_pem_start,
        // .client_key_pem = (const char *)client_key_pem_start,
    };

    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    client = esp_mqtt_client_init(&mqtt_cfg);
}

void mqtt_set_callback(void *cb)
{
    if(cb){
        mqtt_handle = cb;
    }
}

void mqtt_pub(char *topic, char *data, int len)
{
    esp_mqtt_client_publish(client, topic, data, len, 1, 0);
}

void mqtt_sub(char *topic)
{
    esp_mqtt_client_subscribe(client, topic, 1);
}