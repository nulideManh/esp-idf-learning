/*  WiFi softAP Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "app_http_server.h"
#include "app_mqtt.h"
#include "json_parser.h"
#include "json_generator.h"
#include "esp_output.h"
#include "app_flash.h"
#include "esp_smartconfig.h"

/* The examples use WiFi configuration that you can set via project configuration menu.

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/
#define EXAMPLE_ESP_WIFI_SSID      CONFIG_ESP_WIFI_SSID
#define EXAMPLE_ESP_WIFI_PASS      CONFIG_ESP_WIFI_PASSWORD
#define EXAMPLE_ESP_WIFI_CHANNEL   CONFIG_ESP_WIFI_CHANNEL
#define EXAMPLE_MAX_STA_CONN       CONFIG_ESP_MAX_STA_CONN

static const char *TAG = "wifi softAP";
EventGroupHandle_t s_wifi_event_group;
#define BIT_WIFI_CONNECTED	BIT0
#define BIT_WIFI_CONNECT_FAIL   BIT1
#define BIT_WIFI_RECV_INFO	BIT2
#define ESPTOUCH_DONE_BIT  BIT3

typedef enum{
    MODE_AP = 0,
    MODE_SC = 1,
    MODE_BLE = 2,
}   wifi_config_mode_t;

wifi_config_mode_t wifi_config_mode = MODE_AP;

wifi_config_t wifi_config = {
    .sta = {
        .ssid = "Admin",
        .password = "",
        /* Setting a password implies station will connect to all security modes including WEP/WPA.
            * However these modes are deprecated and not advisable to be used. Incase your Access point
            * doesn't support WPA2, these mode can be enabled by commenting below line */
        .threshold.authmode = WIFI_AUTH_WPA2_PSK,

        .pmf_cfg = {
            .capable = true,
            .required = false
        },
    },
};

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data)
{
    static int s_retry_num = 0;
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" join, AID=%d",
                 MAC2STR(event->mac), event->aid);
    } 
    else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" leave, AID=%d",
                 MAC2STR(event->mac), event->aid);
    }

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < 5) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry %d to connect to the AP", s_retry_num);
        } else {
            xEventGroupSetBits(s_wifi_event_group, BIT_WIFI_CONNECT_FAIL);
        }
        ESP_LOGI(TAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, BIT_WIFI_CONNECTED);
    } 

    if (event_base == SC_EVENT && event_id == SC_EVENT_SCAN_DONE) {
        ESP_LOGI(TAG, "Scan done");
    } else if (event_base == SC_EVENT && event_id == SC_EVENT_FOUND_CHANNEL) {
        ESP_LOGI(TAG, "Found channel");
    } else if (event_base == SC_EVENT && event_id == SC_EVENT_GOT_SSID_PSWD) {
        ESP_LOGI(TAG, "Got SSID and password");

        smartconfig_event_got_ssid_pswd_t *evt = (smartconfig_event_got_ssid_pswd_t *)event_data;
        wifi_config_t wifi_config;

        bzero(&wifi_config, sizeof(wifi_config_t));
        memcpy(wifi_config.sta.ssid, evt->ssid, sizeof(wifi_config.sta.ssid));
        memcpy(wifi_config.sta.password, evt->password, sizeof(wifi_config.sta.password));
        wifi_config.sta.bssid_set = evt->bssid_set;
        if (wifi_config.sta.bssid_set == true) {
            memcpy(wifi_config.sta.bssid, evt->bssid, sizeof(wifi_config.sta.bssid));
        }

        ESP_ERROR_CHECK( esp_wifi_disconnect() );
        ESP_ERROR_CHECK( esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
        esp_wifi_connect();
    } else if (event_base == SC_EVENT && event_id == SC_EVENT_SEND_ACK_DONE) {
        xEventGroupSetBits(s_wifi_event_group, ESPTOUCH_DONE_BIT);
    }       

}

void wifi_init_softap(void)
{
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = "Deviot.vn",
            .ssid_len = strlen("Deviot.vn"),
            .channel = EXAMPLE_ESP_WIFI_CHANNEL,
            .password = "",
            .max_connection = EXAMPLE_MAX_STA_CONN,
            .authmode = WIFI_AUTH_OPEN
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
             EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS, EXAMPLE_ESP_WIFI_CHANNEL);
}

void wifi_cb(char *data, int len)
{
    data[len] = '\0';
    // "Deviot/Deviot21"
    char *pt = strtok(data, "/");
    printf("->ssid: %s\n", pt);
    strcpy((char*)wifi_config.sta.ssid, pt);

    pt = strtok(NULL, "/");
    printf("->pass: %s\n", pt);
    strcpy((char*)wifi_config.sta.password, pt);

    xEventGroupSetBits(s_wifi_event_group, BIT_WIFI_RECV_INFO);
}

void wifi_init_sta(void)
{
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_init_sta finished.");
}

void wifi_sc_init(void)
{
    ESP_ERROR_CHECK( esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL) );
    ESP_ERROR_CHECK( esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL) );
    ESP_ERROR_CHECK( esp_event_handler_register(SC_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL) );

    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK( esp_wifi_start() );  

    ESP_ERROR_CHECK( esp_smartconfig_set_type(SC_TYPE_ESPTOUCH) );
    smartconfig_start_config_t sc_cfg = SMARTCONFIG_START_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_smartconfig_start(&sc_cfg) );     
}

typedef struct {
    char buf[256];
    size_t offset;
} json_gen_test_result_t;

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

json_gen_test_result_t rs;
static int json_gen_ack(json_gen_test_result_t *result, char *type, int gpio, bool state)
{
	char buf[20];
    memset(result, 0, sizeof(json_gen_test_result_t));
	json_gen_str_t jstr;
	json_gen_str_start(&jstr, buf, sizeof(buf), flush_str, result);
	json_gen_start_object(&jstr);//{
        json_gen_obj_set_string(&jstr, "type", type);
        json_gen_obj_set_int(&jstr, "gpio", gpio);
        json_gen_obj_set_bool(&jstr, "state", state);
	json_gen_end_object(&jstr); // }
	json_gen_str_end(&jstr);
    return 0;
    // result->buf
}

void mqtt_data_callback (char *data, uint16_t len)
{
    data[len] = 0;
    printf("DATA=%s\r\n", data); // deviot.vn\0@@####$$$$
    // parser JSON
    jparse_ctx_t jctx;
	int ret = json_parse_start(&jctx, data, len);
	if (ret != OS_SUCCESS) {
		printf("Parser failed\n");
		return;
	}   

    char type[10];
    if (json_obj_get_string(&jctx, "type", type, 10) == OS_SUCCESS)
    printf("get type: %s\n", type); 

    int gpio_value;
    if (json_obj_get_int(&jctx, "gpio", &gpio_value) == OS_SUCCESS)
    printf("get gpio: %d\n", gpio_value); 

    bool gpio_state;
	if (json_obj_get_bool(&jctx, "state", &gpio_state) == OS_SUCCESS)
		printf("get state %s\n", gpio_state ? "true" : "false");
    
    json_parse_end(&jctx);

    if(strcmp(type, "cmd") == 0){
        esp_output_create(gpio_value);
        esp_output_set_level(gpio_value, gpio_state);
    }

    json_gen_ack(&rs, "ack", gpio_value, gpio_state);
    app_mqtt_publish("control/led", rs.buf, strlen(rs.buf));
}

int restart_cnt = 0;
char clientID[30] = "";
int clientIDlength = 0;

void app_main(void)
{
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    esp_err_t err = app_flash_get_int("reset_cnt", &restart_cnt);
    if(err != ESP_OK){
        restart_cnt = 0;
        app_flash_set_int("reset_cnt", restart_cnt);
    }
    else{
        restart_cnt++;
        printf("reset_cnt = %d\n", restart_cnt);
        app_flash_set_int("reset_cnt", restart_cnt);
    }

    err = app_flash_get_str("home_id", clientID, &clientIDlength);
    if(err != ESP_OK){
        printf("->nhay vao day\n");
        strcpy(clientID, "anhgiangdeptrai");
        app_flash_set_str("home_id", clientID);
    }
    else{
        printf("homeID: %.*s\n", clientIDlength, clientID);
        app_flash_set_str("home_id", clientID);
    }    

    s_wifi_event_group = xEventGroupCreate();
    app_mqtt_set_cb(mqtt_data_callback);
    app_mqtt_init();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );    
    esp_netif_create_default_wifi_sta();
    esp_netif_create_default_wifi_ap();
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );

    wifi_config_t conf;
    esp_wifi_get_config(ESP_IF_WIFI_STA, &conf);
    if(conf.sta.ssid[0] != 0){  // tim thay wifi
        printf("Found wifi info\n");
        ESP_ERROR_CHECK( esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL) );
        ESP_ERROR_CHECK( esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL) );        
        esp_wifi_start();
    }
    else{   // khong tim thay wifi
        if(wifi_config_mode == MODE_AP){    // khoi dong access point mode
            ESP_LOGI(TAG, "ESP_WIFI_MODE_AP");
            wifiInfo_set_cb(wifi_cb);
            wifi_init_softap();
            start_webserver();
            xEventGroupWaitBits(s_wifi_event_group,BIT_WIFI_RECV_INFO, true, false, portMAX_DELAY);
            wifi_init_sta();
        }
        else if(wifi_config_mode == MODE_SC){   // khoi dong smartconfig mode
            wifi_sc_init();
            xEventGroupWaitBits(s_wifi_event_group,ESPTOUCH_DONE_BIT, true, false, portMAX_DELAY);
            esp_smartconfig_stop();
            ESP_LOGI(TAG, "smartconfig over");
        }
    }

    EventBits_t uxBits = xEventGroupWaitBits(s_wifi_event_group,BIT_WIFI_CONNECTED | BIT_WIFI_CONNECT_FAIL, true, false, portMAX_DELAY);
    if(uxBits & BIT_WIFI_CONNECTED){
        printf("WIFI CONNECTED\n");
        app_mqtt_start();
    }
    if(uxBits & BIT_WIFI_CONNECT_FAIL){
        printf("WIFI CONNECT FAIL\n");
    }
}
