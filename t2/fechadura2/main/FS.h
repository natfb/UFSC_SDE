#ifndef __MEU_FS__
#define __MEU_FS__
#include "esp_spiffs.h"
#include "nvs_flash.h"

class FS {
    private:
        const char *particao;
    public:
        void nomeParticao ( char *b);
        esp_err_t start(const char *base, size_t files);
};

#endif