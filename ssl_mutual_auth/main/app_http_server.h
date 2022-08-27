#ifndef _APP_HTTP_SERVER_H_
#define _APP_HTTP_SERVER_H_
#include <esp_http_server.h>
enum
{
    HTTP_GET_RECEIVE_HEADER = 1,
    HTTP_GET_RECEIVE_QUERY,
    HTTP_GET_RECEIVE_LOST
};

#define APP_HTTP_SERVER_GET_DEFAULT_URI     "/get"
#define APP_HTTP_SERVER_POST_DEFAULT_URI     "/post"
#define APP_HTTP_SERVER_PUT_DEFAULT_URI     "/put" 
#define APP_HTTP_SERVER_POST_MAX_BUFFER       100
#define HTTP_GET_RESPONSE_DEFAULT            "ABCD"

typedef void (*http_post_handler_func_t)(char*, int);
typedef void (*http_get_handler_func_t)(char* url_query,char* host);
void app_http_server_post_set_callback(void* post_handler_callback);
void app_http_server_get_set_callback(void* get_handler_callback);
void app_http_server_start(void);
void app_http_server_stop(void);
void app_http_server_send_resp(char* data, int len);

#endif