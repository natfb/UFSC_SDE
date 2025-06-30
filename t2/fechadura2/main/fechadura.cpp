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
//  mas alguns ESPs e cabos USB podem tolerar taxas maiores, tente 921600. 
//  se não conseguir fazer upload nessas velocidades, seu cabo USB pode ser de baixa qualidade, tente trocar o cabo
//  ou reduzir a velocidade de upload para 115200 .



#include <string.h>
#include "mdns.h"
#include "cJSON.h"
#include "web.h"
#include "wifi.h"
#include "FS.h"
#include "mdns2.h"

#include "i2c.h"

#define AP_SSID      "PORTA_124"
#define AP_CHANNEL   1
#define MAX_CONN     4

#define NOME_MDNS "porta"   // ping porta.local

WEBSERVER webServer;
WIFI wifi;
FS sist_arquivos;
MDNS mdns;



static const char* get_content_type(const char* path) {
    if (strstr(path, ".html")) return "text/html";
    if (strstr(path, ".css"))  return "text/css";
    if (strstr(path, ".js"))   return "application/javascript";
    if (strstr(path, ".png"))  return "image/png";
    if (strstr(path, ".jpg"))  return "image/jpeg";
    return "text/plain";
}
static esp_err_t serve_estaticos(httpd_req_t *req) {
    char nome[20];

    sist_arquivos.nomeParticao(nome);
      char filepath[ESP_VFS_PATH_MAX + 128];
      strcpy(filepath,nome);
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

esp_err_t rota_senha(httpd_req_t *req) {
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
        printf("Recebeu ID=%-10s e senha=%-10s    Status:Sucesso\n",ID->valuestring, senha->valuestring);

        // envia msg de sucesso e abre a porta
        cJSON_AddStringToObject(resposta, "Status", "Sucesso");

    }
    else
    {
        printf("Recebeu ID=%-10s e senha=%-10s    Status:Falha\n",ID->valuestring, senha->valuestring);
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


extern "C" void app_main() ;

void app_main(void) {
    sist_arquivos.start("/storage", 10);                  // Usa SA na Flash

    wifi.accessPoint(AP_SSID,AP_CHANNEL,MAX_CONN);        // Cria um AccessPoint
    mdns.start(NOME_MDNS);                                // Cria um mDNS para acesso usando nome ex: ping porta.local

    webServer.addHandler("/*",     HTTP_GET, serve_estaticos);  // Serve arquivos estáticos
    webServer.addHandler("/senha", HTTP_POST,rota_senha);       // Trata Rota 

    webServer.start();                                    // Inicia servidor WEB

    uint8_t vet[200];
    // menu com as opcoes
    printf("[1] Lista todas as IDs e senhas\n[2] Adiciona uma nova entrada (ID e Senha)\n[3] Mostra qtd de pessoas cadastradas\n[4] Remove uma entrada de ID/senha\n[5] Inicializa BD\n");
    
    while(1) {
        // 
        // como q pega input??
        int input = 0;
        scanf("%d", &input);
        getchar(); // Limpa o buffer do teclado
        
        // acho q isso aqqui ta no luagr errado
        switch(input) {
            case '1':
                // lee bytes
                // tem que trocar esse x<100 por algo
                for (int x=0;x<100;x++)
                {
                    vet[x]=i2c.listaTodos(x);
                }

                // Mostra bytes
                for (int x=0;x<100;x++)
                {
                    printf("%d - Usuario e senha: %d\n", x, vet[x]);
                }
                break;
            case '2':
                // adicionar nova entrada
                break;
            case '3':
                // mostrar qtd de pessoas cadastradas
                // tem que pegar do cabeçalho -> coloquei no readme
                break;
            case '4':
                // remover uma entrada de ID/senha
                break;
            case '5':
                // inicializar BD
                break;
            default:
                printf("Opção inválida!\n");
        }
    };

}


