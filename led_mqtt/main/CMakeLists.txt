idf_component_register(SRCS "app_main.c" "app_http_server.c" "app_nvs.c" "app_config.c" "app_mqtt.c"
                    INCLUDE_DIRS ".")

set(req json_parser json_generator)