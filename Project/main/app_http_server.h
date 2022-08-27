#ifndef __APP_HTTP_SERVER_H
#define __APP_HTTP_SERVER_H
#include <stdio.h>

typedef void (*switch_handle_t) (int onoff);
typedef void (*slider_handle_t) (int duty);
typedef void (*rgb_handler_t) (uint8_t r, uint8_t g, uint8_t b);

void start_webserver(void);
void stop_webserver(void);
void switch_set_callback(void *cb);
void slider_set_callback(void *cb);
void rgb_set_callback(void *cb);
#endif