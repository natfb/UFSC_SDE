#ifndef __NOVO_WIFI__
#define __NOVO_WIFI__
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"

class WIFI{
    private:

    public:
        void accessPoint(const char *AP_SSID, uint8_t AP_CHANNEL, int MAX_CONN);

};
#endif