idf_component_register(SRCS "fechadura.cpp" "i2c.cpp" "web.cpp" "wifi.cpp" "FS.cpp" "mdns2.cpp"
                    INCLUDE_DIRS "."
                    REQUIRES driver esp_wifi  spiffs esp_event esp_netif nvs_flash esp_http_server json)

spiffs_create_partition_image(storage ../partition FLASH_IN_PROJECT)