#include "app_http_server.h"
#include <esp_http_server.h>
#include <esp_log.h>
extern const uint8_t html_pem_start[] asm("_binary_web_html_start");
extern const uint8_t html_pem_end[] asm("_binary_web_html_end");

static const char *TAG = "http server";

static post_handle_t switch_data_handle = NULL;
static post_handle_t wifi_data_handle = NULL;
static post_handle_t slider_data_handle = NULL;
static post_handle_t rgb_data_handle = NULL;

static httpd_handle_t user_server = NULL;
/* An HTTP GET handler */
static esp_err_t dht11_get_data_handler(httpd_req_t *req)
{
    char buf[100];
    size_t buf_len;

    /* Send response with custom headers and body set as the
     * string passed in user context*/
    // httpd_resp_set_type(req, "text/html");
    static float tem = 0;
    static float hum = 0;
    sprintf(buf, "{\"temperate\":\"%.2f\",\"humidity\":\"%.2f\"}", tem, hum);
    if(tem++ > 100)
        tem = 0;
    if(hum++ > 100)
        hum = 0;    
    httpd_resp_send(req, buf, strlen(buf));

    // /* After sending the HTTP response the old HTTP request
    //  * headers are lost. Check if HTTP request headers can be read now. */
    // if (httpd_req_get_hdr_value_len(req, "Host") == 0) {
    //     ESP_LOGI(TAG, "Request headers lost");
    // }
    return ESP_OK;
}

static const httpd_uri_t dht11 = {
    .uri       = "/getdht11",      
    .method    = HTTP_GET,
    .handler   = dht11_get_data_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx  = "dht11"
};

/* An HTTP GET handler */
static esp_err_t hello_get_handler(httpd_req_t *req)
{
    char*  buf;
    size_t buf_len;

    /* Send response with custom headers and body set as the
     * string passed in user context*/
    // httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, (const char *)html_pem_start, html_pem_end-html_pem_start);

    // /* After sending the HTTP response the old HTTP request
    //  * headers are lost. Check if HTTP request headers can be read now. */
    // if (httpd_req_get_hdr_value_len(req, "Host") == 0) {
    //     ESP_LOGI(TAG, "Request headers lost");
    // }
    return ESP_OK;
}

static const httpd_uri_t hello = {
    .uri       = "/helloworld",      
    .method    = HTTP_GET,
    .handler   = hello_get_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx  = "Hello World!"
};

static esp_err_t color_get_handler(httpd_req_t *req)
{
    char*  buf;
    size_t buf_len;

    /* Send response with custom headers and body set as the
     * string passed in user context*/
    buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG, "Found URL query => %s", buf);
            char param[32];
            /* Get value of expected key from query string */
            if (httpd_query_key_value(buf, "rgb", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG, "Found rgb=%s", param);
                rgb_data_handle(param, strlen(param));
            }
        }
        free(buf);
    }
    // /* After sending the HTTP response the old HTTP request
    //  * headers are lost. Check if HTTP request headers can be read now. */
    // if (httpd_req_get_hdr_value_len(req, "Host") == 0) {
    //     ESP_LOGI(TAG, "Request headers lost");
    // }
    return ESP_OK;
}

static const httpd_uri_t getcolor = {
    .uri       = "/getcolor",      
    .method    = HTTP_GET,
    .handler   = color_get_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx  = NULL
};

/* An HTTP POST handler */
static esp_err_t switch_post_handler(httpd_req_t *req)
{
    char buf[100];
    int read_len = httpd_req_recv(req, buf, 100);
    switch_data_handle(buf, read_len);
    // End response
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

static const httpd_uri_t switch1 = {
    .uri       = "/switch",
    .method    = HTTP_POST,
    .handler   = switch_post_handler,
    .user_ctx  = NULL
};

/* An HTTP POST handler */
static esp_err_t wifiInfo_post_handler(httpd_req_t *req)
{
    char buf[100];
    int read_len = httpd_req_recv(req, buf, 100);
    wifi_data_handle(buf, read_len);
    // End response
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

static const httpd_uri_t wifiInfo = {
    .uri       = "/wifiInfo",
    .method    = HTTP_POST,
    .handler   = wifiInfo_post_handler,
    .user_ctx  = NULL
};

/* An HTTP POST handler */
static esp_err_t slider_post_handler(httpd_req_t *req)
{
    char buf[100];
    int read_len = httpd_req_recv(req, buf, 100);
    // printf("Value Slider: %.*s\n", read_len, buf);
    slider_data_handle(buf, read_len);
    // End response
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

static const httpd_uri_t slider = {
    .uri       = "/slider",
    .method    = HTTP_POST,
    .handler   = slider_post_handler,
    .user_ctx  = NULL
};

/* This handler allows the custom error handling functionality to be
 * tested from client side. For that, when a PUT request 0 is sent to
 * URI /ctrl, the /hello and /echo URIs are unregistered and following
 * custom error handler http_404_error_handler() is registered.
 * Afterwards, when /hello or /echo is requested, this custom error
 * handler is invoked which, after sending an error message to client,
 * either closes the underlying socket (when requested URI is /echo)
 * or keeps it open (when requested URI is /hello). This allows the
 * client to infer if the custom error handler is functioning as expected
 * by observing the socket state.
 */
esp_err_t http_404_error_handler(httpd_req_t *req, httpd_err_code_t err)
{
    if (strcmp("/hello", req->uri) == 0) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "/hello URI is not available");
        /* Return ESP_OK to keep underlying socket open */
        return ESP_OK;
    } else if (strcmp("/echo", req->uri) == 0) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "/echo URI is not available");
        /* Return ESP_FAIL to close underlying socket */
        return ESP_FAIL;
    }
    /* For any other URI send 404 and close socket */
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Some 404 error message");
    return ESP_FAIL;
}

void stop_webserver(void)
{
    // Stop the httpd server
    httpd_stop(user_server);
}

void start_webserver(void)
{
    
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&user_server, &config) == ESP_OK) {
        // Set URI handlers
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(user_server, &hello);     // lay toan bo trang web
        httpd_register_uri_handler(user_server, &switch1);   // dk led 
        httpd_register_uri_handler(user_server, &slider);   // dk led 
        httpd_register_uri_handler(user_server, &dht11);
        httpd_register_uri_handler(user_server, &getcolor);
        httpd_register_uri_handler(user_server, &wifiInfo);
    }
    else{
        ESP_LOGI(TAG, "Error starting server!");
    }
}

void switch_set_cb (void *cb)
{
    if(cb){
        switch_data_handle = cb;
    }
}

void slider_set_cb (void *cb)
{
    if(cb){
        slider_data_handle = cb;
    }
}

void rgb_set_cb (void *cb)
{
    if(cb){
        rgb_data_handle = cb;
    }
}

void wifiInfo_set_cb (void *cb)
{
    if(cb){
        wifi_data_handle = cb;
    }
}