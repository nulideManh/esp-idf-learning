#ifndef __APP_MQTT_H
#define __APP_MQTT_H
#include <stdint.h>
#define MQTT_BROKER     "mqtt://broker.hivemq.com:1883"
typedef void (*mqtt_data_handle_t) (char *data, uint16_t len);
void app_mqtt_init(void);
void app_mqtt_start(void);
void app_mqtt_stop(void);
void app_mqtt_publish(char * topic, char *data, uint16_t len);
void app_mqtt_subscribe(char * topic);
void app_mqtt_set_cb(void *cb);
#endif