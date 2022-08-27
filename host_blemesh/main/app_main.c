#include <string.h>
#include <esp_log.h>
#include "nvs_flash.h"
#include "system.h"
#include "app.h"
#include "app_uart.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "app_uart.h"
static const char* TAG = "app_main";

void process_thread(void *arg)
{
    sl_system_init();
    app_init();

    while (1)
    {
        // Do not remove this call: Silicon Labs components process action routine
        // must be called from the super loop.
        sl_system_process_action();

        // Application process.
        app_process_action();
        
        vTaskDelay(50/portTICK_PERIOD_MS);
    }
    
}

void app_main(void)
{
    /* Initialize NVS. */
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
   
    xTaskCreate(process_thread,"process", 4096,NULL,3,NULL);

}