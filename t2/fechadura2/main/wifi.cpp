#include "wifi.h"
#include <string.h>
#include "esp_mac.h"

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        printf("Cliente se conectou: " MACSTR "\n", MAC2STR(event->mac));
       
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        printf("Cliente se desconectou: " MACSTR "\n", MAC2STR(event->mac));

    }
}

void WIFI::accessPoint(const char *AP_SSID, uint8_t AP_CHANNEL, int MAX_CONN){

       esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL);

    wifi_config_t ap_cfg;

    memcpy(ap_cfg.ap.ssid,AP_SSID,strlen(AP_SSID));
    ap_cfg.ap.channel = AP_CHANNEL;
    ap_cfg.ap.max_connection = 10;
    ap_cfg.ap.authmode = WIFI_AUTH_OPEN;
    ap_cfg.ap.beacon_interval = 100;
    ap_cfg.ap.ssid_len = (uint8_t) strlen(AP_SSID);

    esp_wifi_set_mode(WIFI_MODE_AP);
    esp_wifi_set_config(WIFI_IF_AP, &ap_cfg);

    esp_err_t tmp = esp_wifi_start();
    printf("%-15s  [%s]\n","AccessPoint",(tmp==0)?"Sucesso":"Falha");

}



