#include "i2c.h"
#include <stdio.h>
#include "driver/i2c_master.h"
#include <inttypes.h> 
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <unistd.h>
#include <unistd.h>
#include <cstring>

i2c_master_dev_handle_t dev_handle;
i2c_master_bus_handle_t bus_handle;
i2c_master_bus_config_t i2c_bus_config = {};
uint16_t end = 0x50;
i2c_device_config_t dev_cfg = {};

void I2C::listaTodos() {
    uint16_t qntdUsers = qntdUsuarios();

    if (qntdUsers == 0) {
        printf("Nenhum usuário cadastrado.\n");
        return;
    }

    for (uint16_t i = 0; i < qntdUsers; i++) {
        uint16_t addr_registro = EEPROM_ADDR_BASE + (i * EEPROM_TAM_REG);

        
        uint8_t addr_bytes[2];
        addr_bytes[0] = (addr_registro >> 8) & 0xFF; 
        addr_bytes[1] = addr_registro & 0xFF;      

        RegistroUsuario usuario_lido; 
        memset(&usuario_lido, 0, sizeof(usuario_lido));

        esp_err_t ret = i2c_master_transmit_receive(dev_handle, addr_bytes, sizeof(addr_bytes), 
                                                    (uint8_t*)&usuario_lido, sizeof(usuario_lido), -1);

        if (ret != ESP_OK) {
            printf("Erro ao ler cabeçalho da EEPROM: %s\n", esp_err_to_name(ret));
        } else {
            printf("Cabeçalho lido: qtd = %u, max = %u\n", cabec.qtd, cabec.max);
        }

        if (ret == ESP_OK) {
            usuario_lido.id[sizeof(usuario_lido.id) - 1] = '\0';
            usuario_lido.senha[sizeof(usuario_lido.senha) - 1] = '\0';
            printf("%u - id: %s, Senha: %s\n", i, usuario_lido.id, usuario_lido.senha);
        } else {
            printf("Falha ao ler usuário na posição %u\n", i);
        }
    }
}

void I2C::registroUsuario(const char* id, const char* senha) {

    uint8_t qtd;
    qtd = cabec.qtd;

    RegistroUsuario novo;
    memset(&novo, 0, sizeof(novo));
    strncpy(novo.id, id, 15);
        novo.id[15] = '\0'; 

        strncpy(novo.senha, senha, 15);
        novo.senha[15] = '\0';

    uint16_t addr = EEPROM_ADDR_BASE + qtd * EEPROM_TAM_REG;
    uint8_t pacote[2 + sizeof(novo)];
    pacote[0] = (addr >> 8) & 0xFF;
    pacote[1] = addr & 0xFF;
    memcpy(&pacote[2], &novo, sizeof(novo));

    i2c_master_transmit(dev_handle, pacote, sizeof(pacote), -1);
    vTaskDelay(pdMS_TO_TICKS(5));

    cabec.qtd++;
    
    uint8_t cabec_pacote[2 + sizeof(cabec)];
    cabec_pacote[0] = 0x00; 
    cabec_pacote[1] = 0x00;
    memcpy(&cabec_pacote[2], &cabec, sizeof(cabec));
    
    i2c_master_transmit(dev_handle, cabec_pacote, sizeof(cabec_pacote), -1);

}

uint16_t I2C::qntdUsuarios() {
    return cabec.qtd;
}

void I2C::removerPorID2(const char* id) {
    if (dev_handle == nullptr) {
        printf("Erro: I2C não inicializado.\n");
        return;
    }

    uint8_t buffer[32];
    char idBuffer[17]; 
    idBuffer[16] = '\0';

    for (int i = 0; i < cabec.qtd; i++) {
        uint16_t addr = EEPROM_ADDR_BASE + i * EEPROM_TAM_REG;
        uint8_t addr_bytes[2] = { (uint8_t)(addr >> 8), (uint8_t)(addr & 0xFF) };

        esp_err_t ret = i2c_master_transmit_receive(dev_handle, addr_bytes, 2, buffer, 32, -1);
        if (ret != ESP_OK) {
            printf("Erro ao ler posição %d: %s\n", i, esp_err_to_name(ret));
            continue;
        }

        memcpy(idBuffer, buffer, 16);
        idBuffer[16] = '\0';

        if (strcmp(idBuffer, id) == 0) {

            memset(buffer, 0xFF, 32);
            uint8_t pacote[34];
            pacote[0] = addr_bytes[0];
            pacote[1] = addr_bytes[1];
            memcpy(&pacote[2], buffer, 32);
            ret = i2c_master_transmit(dev_handle, pacote, sizeof(pacote), -1);
            if (ret != ESP_OK) {
                printf("Erro ao apagar ID '%s': %s\n", id, esp_err_to_name(ret));
                return;
            }

            cabec.qtd--;
            uint8_t cabec_pacote[2 + sizeof(cabec)];
            cabec_pacote[0] = 0x00;
            cabec_pacote[1] = 0x00;
            memcpy(&cabec_pacote[2], &cabec, sizeof(cabec));
            ret = i2c_master_transmit(dev_handle, cabec_pacote, sizeof(cabec_pacote), -1);
            if (ret != ESP_OK) {
                printf("Erro ao atualizar cabeçalho: %s\n", esp_err_to_name(ret));
            }

            printf("ID '%s' removido com sucesso.\n", id);
            return;
        }
    }

    printf("ID '%s' não encontrado.\n", id);
}


void I2C::init2(gpio_num_t pino_scl, gpio_num_t pino_sda) {
    dev_handle = nullptr;

    i2c_bus_config.clk_source = I2C_CLK_SRC_DEFAULT;
    i2c_bus_config.flags.enable_internal_pullup = true;
    i2c_bus_config.i2c_port = 0;
    i2c_bus_config.scl_io_num = pino_scl;
    i2c_bus_config.sda_io_num = pino_sda;
    i2c_bus_config.glitch_ignore_cnt = 7;

    esp_err_t ret;

    ret = i2c_new_master_bus(&i2c_bus_config, &bus_handle);
    if (ret != ESP_OK) {
        printf("Erro ao criar o barramento I2C: %s\n", esp_err_to_name(ret));
        return;
    }

    dev_cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    dev_cfg.device_address = 0x50;
    dev_cfg.scl_speed_hz = 100000;
    dev_cfg.scl_wait_us = 0;
    dev_cfg.flags.disable_ack_check = 1;

    ret = i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle);
    if (ret != ESP_OK) {
        printf("Erro ao adicionar dispositivo I2C: %s\n", esp_err_to_name(ret));
        return;
    }

    if (dev_handle == nullptr) {
        printf("dev_handle está NULL! falha do I2C.\n");
        return;
    }

    uint8_t addr_cabec[] = {0x00, 0x00};
    ret = i2c_master_transmit_receive(dev_handle, addr_cabec, sizeof(addr_cabec), 
                                      (uint8_t*)&cabec, sizeof(cabec), -1);
    if (ret != ESP_OK) {
        printf("Erro ao ler cabeçalho da EEPROM: %s\n", esp_err_to_name(ret));
        // inicializa manualmente
        cabec.qtd = 0;
        cabec.max = MAX_USUARIOS;

        uint8_t cabec_pacote[2 + sizeof(cabec)];
        cabec_pacote[0] = 0x00;
        cabec_pacote[1] = 0x00;
        memcpy(&cabec_pacote[2], &cabec, sizeof(cabec));

        ret = i2c_master_transmit(dev_handle, cabec_pacote, sizeof(cabec_pacote), -1);
        if (ret == ESP_OK) {
            printf("Cabeçalho inicializado com sucesso: qtd = %u, max = %u\n", cabec.qtd, cabec.max);
        } else {
            printf("Erro ao inicializar cabeçalho: %s\n", esp_err_to_name(ret));
        }
    } else {
        // Verifica valores são válidos
        if (cabec.qtd > MAX_USUARIOS || cabec.max > MAX_USUARIOS) {
            printf("Cabeçalho inválido detectado (qtd = %u, max = %u). Reinicializando...\n", cabec.qtd, cabec.max);
            cabec.qtd = 0;
            cabec.max = MAX_USUARIOS;

            uint8_t cabec_pacote[2 + sizeof(cabec)];
            cabec_pacote[0] = 0x00;
            cabec_pacote[1] = 0x00;
            memcpy(&cabec_pacote[2], &cabec, sizeof(cabec));

            ret = i2c_master_transmit(dev_handle, cabec_pacote, sizeof(cabec_pacote), -1);
            if (ret == ESP_OK) {
                printf("Cabeçalho reinicializado com sucesso: qtd = %u, max = %u\n", cabec.qtd, cabec.max);
            } else {
                printf("Erro ao reinicializar cabeçalho: %s\n", esp_err_to_name(ret));
            }
        } else {
            printf("Cabeçalho lido com sucesso: qtd = %u, max = %u\n", cabec.qtd, cabec.max);
        }
    }
}

void I2C::limparEEPROM() {
    if (dev_handle == nullptr) {
        printf("Erro: I2C não inicializado.\n");
        return;
    }

    printf("Limpando EEPROM...\n");
    for (int i = 0; i < MAX_USUARIOS; i++) {
        uint16_t addr = EEPROM_ADDR_BASE + i * EEPROM_TAM_REG;
        uint8_t pacote[34];
        pacote[0] = (addr >> 8) & 0xFF;
        pacote[1] = addr & 0xFF;
        memset(&pacote[2], 0xFF, 32);
        esp_err_t ret = i2c_master_transmit(dev_handle, pacote, sizeof(pacote), -1);
        if (ret != ESP_OK) {
            printf("Erro ao apagar posição %d: %s\n", i, esp_err_to_name(ret));
        }
        vTaskDelay(pdMS_TO_TICKS(2));
    }

    cabec.qtd = 0;
    cabec.max = MAX_USUARIOS;
    uint8_t cabec_pacote[2 + sizeof(cabec)];
    cabec_pacote[0] = 0x00;
    cabec_pacote[1] = 0x00;
    memcpy(&cabec_pacote[2], &cabec, sizeof(cabec));
    i2c_master_transmit(dev_handle, cabec_pacote, sizeof(cabec_pacote), -1);

    printf("EEPROM limpa com sucesso.\n");
}

bool I2C::verificarUsuario(const char* id_a_verificar, const char* senha_a_verificar) {
    uint16_t qntdUsers = this->qntdUsuarios();

    for (uint16_t i = 0; i < qntdUsers; i++) {
        uint16_t addr_registro = EEPROM_ADDR_BASE + (i * EEPROM_TAM_REG);
        uint8_t addr_bytes[2] = { (uint8_t)(addr_registro >> 8), (uint8_t)(addr_registro & 0xFF) };
        RegistroUsuario usuario_lido;
        memset(&usuario_lido, 0, sizeof(usuario_lido));

        esp_err_t ret = i2c_master_transmit_receive(dev_handle, addr_bytes, sizeof(addr_bytes),
                                                    (uint8_t*)&usuario_lido, sizeof(usuario_lido), -1);

        if (ret == ESP_OK) {
            usuario_lido.id[sizeof(usuario_lido.id) - 1] = '\0';
            usuario_lido.senha[sizeof(usuario_lido.senha) - 1] = '\0';

            printf("Debug: Comparando ID_Web['%s'] com ID_Mem['%s']\n", id_a_verificar, usuario_lido.id);
            printf("Debug: Comparando Senha_Web['%s'] com Senha_Mem['%s']\n", senha_a_verificar, usuario_lido.senha);
            if ((strcmp(id_a_verificar, usuario_lido.id) == 0) &&
                (strcmp(senha_a_verificar, usuario_lido.senha) == 0))
            {
                return true;
            }
        } else {
            printf("Falha ao ler usuário na posição %u durante a verificação.\n", i);
        }
    }
    return false;
}


I2C i2c;



