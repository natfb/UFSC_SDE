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
#include "driver/ledc.h"
#include "i2c.h"
#include <iostream>
#include "driver/uart.h"

#define PINO1 GPIO_NUM_22 
#define PINO2 GPIO_NUM_23

using namespace std;

#define AP_SSID      "PORTA_124"
#define AP_CHANNEL   1
#define MAX_CONN     4

#define NOME_MDNS "porta"   // ping porta.local
#define UART_NUM UART_NUM_0
#define BUF_SIZE 1024


WEBSERVER webServer;
WIFI wifi;
FS sist_arquivos;
MDNS mdns;

#define SERVO_GPIO 18  // pino servo

void servo_init() {
    ledc_timer_config_t timer = {
        .speed_mode       = LEDC_HIGH_SPEED_MODE,
        .duty_resolution  = LEDC_TIMER_13_BIT,
        .timer_num        = LEDC_TIMER_0,
        .freq_hz          = 50,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ledc_timer_config(&timer);

    ledc_channel_config_t channel = {
        .gpio_num = SERVO_GPIO,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0
    };
    ledc_channel_config(&channel);
}

void servo_set_angulo(int angulo) {
    // Mapear ângulo de 0° a 180° para duty entre 163 e 490
    uint32_t duty = (angulo * (490 - 163)) / 180 + 163;
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, duty);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);
}

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
        servo_set_angulo(90);  // abrir porta
        vTaskDelay(pdMS_TO_TICKS(3000));
        servo_set_angulo(0);   // fechar porta

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

struct Usuario {             // Structure declaration
  string usuario;         // Member (int variable)
  string senha;   // Member (string variable)
} usuario;


void mostraMenu() {
    printf("[1] Lista todas as IDs e senhas\n[2] Adiciona uma nova entrada (ID e Senha)\n[3] Mostra qtd de pessoas cadastradas\n[4] Remove uma entrada de ID/senha\n[5] Inicializa BD\n");
}

void menu_console_task(void *pvParameter) {
    // Usuario vet[200];
    // menu com as opcoes
    mostraMenu();
    int input = 0;

    char usuario[30], senha[30];
    while(1) {
        printf("Digite a opção desejada: \n");
        uint16_t user_input = leNumero();
        printf("opcao digitada: %u\n", user_input);
        int num_usuarios = 2;
        int len;
        switch(user_input) {
            case 1:
                // lee bytes
                // tem que trocar esse x<100 por algo
                // int num_usuarios = i2c.numeroUsuarios();
                
                for (int x = 1; x < num_usuarios + 1; x++)
                {
                    // vet[x]=i2c.listaTodos(x);
                }

                // Mostra bytes
                for (int x = 1; x < num_usuarios + 1; x++)
                {
                    // cout << "Usuario: " << vet[x].id_usuario << endl;
                    // cout << "Senha: " << vet[x].senha << endl;
                }

                mostraMenu();
                break;
            case 2: 
                
                printf("Digite seu nome de usuário: \n");

                len = leString(usuario, sizeof(usuario));

                if (len > 0) {
                    printf("vc Digitou seu nome de usuário: %c\n", usuario[0]);
                }

                printf("Digite sua senha: \n");

                len = leString(senha, sizeof(senha));

                mostraMenu();
                break;
            case 3:
                i2c.qntdUsuarios();
                mostraMenu();
                break;
            case '4':
                char id[16];
                printf("Digite o ID a remover: ");
                scanf("%15s", id);
                i2c.removerPorID(id);
                mostraMenu();
                break;
            case '5':
                i2c.init(GPIO_NUM_22, GPIO_NUM_23);
                mostraMenu();
                break;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    };
}

void app_main(void) {
    sist_arquivos.start("/storage", 10);                  // Usa SA na Flash

    wifi.accessPoint(AP_SSID,AP_CHANNEL,MAX_CONN);        // Cria um AccessPoint
    mdns.start(NOME_MDNS);                                // Cria um mDNS para acesso usando nome ex: ping porta.local

    webServer.addHandler("/*",     HTTP_GET, serve_estaticos);  // Serve arquivos estáticos
    webServer.addHandler("/senha", HTTP_POST,rota_senha);       // Trata Rota 

    webServer.start();                                 // Inicia servidor WEB

    // Inicializa o I2C primeiro, pois registroUsuario precisa dele
    printf("Inicializando I2C...\n");
    i2c.init(PINO1, PINO2); // Use os pinos definidos no seu projeto
    printf("I2C inicializado.\n");

    // --- Início do teste do registro de usuário ---
    printf("\n--- Testando registro de usuario ---\n");

    // Defina os dados de teste para ID e senha
    const char* test_id = "teste_id_001";
    const char* test_senha = "senha_secreta";

    printf("Tentando registrar usuario com ID: '%s', Senha: '%s'\n", test_id, test_senha);
    i2c.registroUsuario(test_id, test_senha); // Chama diretamente a função de registro

    // task para mostrar o menu no console
    xTaskCreate(menu_console_task, "menu_console_task", 4096, NULL, 5, NULL);

    servo_init();
    servo_set_angulo(45);
}





