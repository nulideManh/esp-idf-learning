#ifndef __APP_MQTT_H
#define __APP_MQTT_H
#include "stdbool.h"

ESP_EVENT_DECLARE_BASE(MQTT_DEV_EVENT);

typedef enum {
    MQTT_DEV_EVENT_CONNECTED,
    MQTT_DEV_EVENT_DISCONNECT,
    MQTT_DEV_EVENT_DATA,
    MQTT_DEV_EVENT_SUBCRIBED,
    MQTT_DEV_EVENT_UNSUBCRIBED,
    MQTT_DEV_EVENT_PUBLISHED
} mqtt_event_id_t;

typedef struct {
    char buf[256];
    size_t offset;
} json_gen_test_result_t;


void json_gen_test(json_gen_test_result_t *result, char* key1, bool value1, char* key2, int value2, char* key3, char* value3);

typedef void (*mqtt_handle_t) (char *data, int len);
void mqtt_app_init(void);
void mqtt_app_start(void);
void mqtt_set_callback(void *cb);
void mqtt_pub(char *topic, char *data, int len);
void mqtt_sub(char *topic);
#endif