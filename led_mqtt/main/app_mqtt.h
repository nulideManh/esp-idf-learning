#ifndef __APP_MQTT_H
#define __APP_MQTT_H
#include "stdbool.h"

ESP_EVENT_DECLARE_BASE(MQTT_DEV_EVENT);

// const char* TEMP_ID   = "62bd099e3e5f8e5cc2c584a4";
// const char* LIGHT_ID = "62bd097e3e5f8e5cc2c5849b";
// const char* HUMID_ID  = "62bd09aa3e5f8e5cc2c584af";
// const char* GAS_ID    = "62c5b71e8b3f52c4d5dabb34";
// const char* BUZZER_ID = "62c5b72b8b3f52c4d5dabb42";

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

void json_gen_test(json_gen_test_result_t *result, char* deviceID, char* control, bool status, char* mode,
                    int direction, int speed, int intensity); 

typedef void (*mqtt_handle_t) (char *data, int len);

void flush_str(char *buf, void *priv);

void mqtt_app_start(void);
void mqtt_set_data_callback(void *cb);
void mqtt_pub(char *topic, char *data, int len);
void mqtt_sub(char *topic);
#endif