#include "app_flash.h"
#include <stdio.h>
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"

#define NAME_SPACE  "user_data"

esp_err_t  app_flash_set_int(char * key, int value)
{
    // Open
    printf("\n");
    printf("Open namespace %s ", NAME_SPACE);
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open(NAME_SPACE, NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        printf("Error\n");
    } else {
        printf("Done\n");    
    }
    // Write
    printf("Update %s ", key);
    err = nvs_set_i32(my_handle, key, value);
    printf((err != ESP_OK) ? "Failed!\n" : "Done\n");

    err = nvs_commit(my_handle);
    // Close
    nvs_close(my_handle);    
    return err;
}

esp_err_t  app_flash_set_str(char * key, char *str)
{
    // Open
    printf("\n");
    printf("Open namespace %s ", NAME_SPACE);
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open(NAME_SPACE, NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        printf("Error\n");
    } else {
        printf("Done\n");    
    }
    // Write
    printf("Update  %s ", key);
    err = nvs_set_str(my_handle, key, str);
    printf((err != ESP_OK) ? "Failed!\n" : "Done\n");
    
    err = nvs_commit(my_handle);
    // Close
    nvs_close(my_handle);  
    return err;  
}

esp_err_t  app_flash_get_int(char * key, int *value)
{
    // Open
    printf("\n");
    printf("Open namespace %s ", NAME_SPACE);
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open(NAME_SPACE, NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        printf("Error\n");
    } else {
        printf("Done\n");    
    }
    // Write
    printf("Get %s ", key);
    err = nvs_get_i32(my_handle, key, value);
    printf((err != ESP_OK) ? "Failed!\n" : "Done\n");
    // Close
    nvs_close(my_handle); 
    return err;   
}

esp_err_t  app_flash_get_str(char * key, char *str, int *len)
{
    // Open
    printf("\n");
    printf("Open namespace %s ", NAME_SPACE);
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open(NAME_SPACE, NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        printf("Error\n");
    } else {
        printf("Done\n");  
    }  
    
    // Write
    printf("Get %s ", key);
    size_t length = 0;
    err = nvs_get_str(my_handle, key, NULL,&length);    // tra ve length thuc te cua key
    *len = length;
    err = nvs_get_str(my_handle, key, str, &length);
    printf((err != ESP_OK) ? "Failed!\n" : "Done\n");
    // Close
    nvs_close(my_handle);  
    return err;  
}