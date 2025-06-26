#include "mdns2.h"
#include "mdns.h"

void MDNS::start(const char *st)
{
    ESP_ERROR_CHECK(mdns_init());
    ESP_ERROR_CHECK(mdns_hostname_set(st));
    ESP_ERROR_CHECK(mdns_instance_name_set("ESP32 com mDNS"));

    // Registrar um serviço HTTP
    
    esp_err_t tmp = mdns_service_add("ESPHTTP", "_http", "_tcp", 80, NULL, 0);
    printf("%-15s  [%s]\n","mDNS",(tmp==0)?"Sucesso":"Falha");

}
