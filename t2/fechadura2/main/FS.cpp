#include "FS.h"
#include <string.h>

void FS::nomeParticao (char *b)
{
   strcpy(b, particao);
}

esp_err_t FS::start(const char *base, size_t files)
{   
    nvs_flash_init();
    particao=base;
     esp_vfs_spiffs_conf_t config = {
        .base_path=base,
        .partition_label = NULL,
        .max_files = files,
        .format_if_mount_failed = true
    };



    esp_err_t tmp = esp_vfs_spiffs_register(&config);
    printf("%-15s  [%s]\n","Inicializa SA",(tmp==0)?"Sucesso":"Falha");
    return tmp;
}