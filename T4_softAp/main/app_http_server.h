#ifndef __APP_HTTP_SERVER_H
#define __APP_HTTP_SERVER_H

typedef void (*post_handle_t) (char *data, int len);
void start_webserver(void);
void stop_webserver(void);
void switch_set_cb (void *cb);
void slider_set_cb (void *cb);
void rgb_set_cb (void *cb);
void wifiInfo_set_cb (void *cb);
#endif