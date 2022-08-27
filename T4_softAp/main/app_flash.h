#ifndef __APP_FLASH_H
#define __APP_FLASH_H
#include "esp_err.h"
esp_err_t  app_flash_set_int(char * key, int value);
esp_err_t  app_flash_set_str(char * key, char *str);
esp_err_t  app_flash_get_int(char * key, int *value);
esp_err_t  app_flash_get_str(char * key, char *str, int *len);
#endif