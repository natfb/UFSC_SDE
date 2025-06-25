// idf.py build
// idf.py flash     --> faz upload da parte do código e também da partição dos arquivos
//
//
// Para fazer upload apenas da partição de código (quando nada da partição de arquivos foi mudada)
// idf.py build
// esptool.py  -b 460800  write_flash 0x10000 build/fechadura.bin
//
//
// Para fazer upload apenas da partição dos arquivos  (quando nada da partição de código foi mudada)
// idf.py build
// esptool.py  -b 460800 write_flash 0x110000 build/storage.bin
//
//
//  OBS: o parâmetro -b indica a velocidade que  upload ocorre, 460800 é uma velocidade rápida
//  se não conseguir fazer upload nessa velocidade, seu cabo USB pode ser de baixa qualidade, tente trocar o cabo
// ou reduzir a velocidade de upload.



#include <string.h>
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_http_server.h"
#include "esp_spiffs.h"
#include "nvs_flash.h"
#include "mdns.h"
#include "cJSON.h"

#define AP_SSID      "PORTA_124"
#define AP_CHANNEL   1
#define MAX_CONN     4

#define GPIO_OUTPUT_PIN 2

#define MDNS "porta"
#define ESP_VFS_PATH_MAX  128
#define MIN(A,B)  ((A<B)?A:B)

static const char* get_content_type(const char* path) {
    if (strstr(path, ".html")) return "text/html";
    if (strstr(path, ".css"))  return "text/css";
    if (strstr(path, ".js"))   return "application/javascript";
    if (strstr(path, ".png"))  return "image/png";
    if (strstr(path, ".jpg"))  return "image/jpeg";
    return "text/plain";
}

static esp_err_t file_handler(httpd_req_t *req) {
      char filepath[ESP_VFS_PATH_MAX + 128];
      strcpy(filepath,"/storage");
      strcat(filepath,req->uri);
    if (req->uri[strlen(req->uri)-1] == '/') {
        strcat(filepath, "index.html");
    }

    FILE *f = fopen(filepath, "r");
    if (!f) {
       printf("404: %s not found\n", filepath);
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Not found");
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, get_content_type(filepath));

    char chunk[1024];
    size_t len;
    while ((len = fread(chunk, 1, sizeof(chunk), f)) > 0) {
        if (httpd_resp_send_chunk(req, chunk, len) != ESP_OK) {
            fclose(f);
            return ESP_FAIL;
        }
    }
    fclose(f);
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}


void mdns_setup(void) {
    ESP_ERROR_CHECK(mdns_init());
    ESP_ERROR_CHECK(mdns_hostname_set(MDNS));
    ESP_ERROR_CHECK(mdns_instance_name_set("ESP32 com mDNS"));

    // Registrar um serviço HTTP
    ESP_ERROR_CHECK(mdns_service_add("ESPHTTP", "_http", "_tcp", 80, NULL, 0));
}




esp_err_t testa_senha(httpd_req_t *req) {
    char buf[200];
    int ret = httpd_req_recv(req, buf, MIN(req->content_len, sizeof(buf) - 1));
    if (ret <= 0) return ESP_FAIL;
    buf[ret] = 0;

    cJSON *json = cJSON_Parse(buf);
    if (!json) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "JSON inválido");
        return ESP_FAIL;
    }

    cJSON *senha = cJSON_GetObjectItem(json, "senha");
    if (!cJSON_IsString(senha)) {
        cJSON_Delete(json);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Campo 'senha' ausente");
        return ESP_FAIL;
    }

    cJSON *ID = cJSON_GetObjectItem(json, "ID");
    if (!cJSON_IsString(ID)) {
        cJSON_Delete(json);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Campo 'ID' ausente");
        return ESP_FAIL;
    }
    printf("Recebeu ID=%s e senha=%s\n",ID->valuestring, senha->valuestring);

      cJSON *resposta = cJSON_CreateObject();

    //
    //
    //
    //
    // AQUI tem que fazer acesso ao BD e verificar se as credenciais batem
    //
    // Para um teste rápido, coloca-se um valor fixo
    // 
    if ((strcmp(senha->valuestring,"1234")==0) && (strcmp(ID->valuestring,"aluno")==0))
    {
        printf("Sucesso\n");
        // envia msg de sucesso e abre a porta
        cJSON_AddStringToObject(resposta, "Status", "Sucesso");

    }
    else
    {
        printf("Falha\n");
        // Envia mensagem de falha
        cJSON_AddStringToObject(resposta, "Status", "Falha");
    }
    cJSON_Delete(json);


  
    const char *res_str = cJSON_PrintUnformatted(resposta);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, res_str);

    free((void*)res_str);
    cJSON_Delete(resposta);
    return ESP_OK;
}


// ------------------------------------------------------------
void start_server() {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
        config.uri_match_fn = httpd_uri_match_wildcard;

    httpd_handle_t server = NULL;
   httpd_uri_t uri = {
            .uri = "/*",
            .method = HTTP_GET,
            .handler = file_handler
            };


    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &(httpd_uri_t){ .uri = "/senha", .method = HTTP_POST, .handler = testa_senha });
        httpd_register_uri_handler(server, &uri);



    }else printf("Erro criando rotas\n");
    
}


esp_err_t init_spiffs(){
    esp_vfs_spiffs_conf_t config = {
        .base_path="/storage",
        .partition_label = NULL,
        .max_files = 10,
        .format_if_mount_failed = true
    };
    printf(" >> Inicializa particao de dados\n");
    return esp_vfs_spiffs_register(&config);
}

void wifi_init_accessPoint(void)
{
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t ap_cfg = {
        .ap = {
            .ssid = AP_SSID,
            .ssid_len = strlen(AP_SSID),
            .channel = AP_CHANNEL,
            .max_connection = 10,
            .authmode = WIFI_AUTH_OPEN,
             .beacon_interval = 100,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_cfg));
    ESP_ERROR_CHECK(esp_wifi_start());

}

void app_main(void) {

    nvs_flash_init();
    init_spiffs();
    wifi_init_accessPoint();
    start_server();
    mdns_setup();


    printf("Access point ESP funcionando.\n");
    printf("Após conectar no access point, acesse o mesmo com o nome:%s.local\n",MDNS);


    // 
    // Insira aqui a parte que mostra  o menu do banco de dados e permite as operações
    //

}


