#include "i2c.h"
#include <stdio.h>
#include "driver/i2c_master.h"
#include <inttypes.h> 
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <unistd.h>
#include <unistd.h>
#include <cstring>
// #include "driver/gpio.h"

i2c_master_dev_handle_t dev_handle;
i2c_master_bus_handle_t bus_handle;
i2c_master_bus_config_t i2c_bus_config = {};
uint16_t end = 0x50;
i2c_device_config_t dev_cfg = {};

void I2C::listaTodos() {
    // uint16_t qntdUsers = qntdUsuarios();
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
    // Lê número de usuários

   
    uint8_t qtd;
    // uint8_t address_bytes[] = {0x00, 0x00}; // Declare a named array
    // i2c_master_transmit_receive(dev_handle, address_bytes, sizeof(address_bytes), &qtd, 1, -1);
    // if (qtd >= MAX_USUARIOS) {
    //     printf("Erro: banco cheio!\n");
    //     return;
    // }

    // ACHO que é mais certo assim porem possivelmente nao, entao deixei o coidog antigo coentado
    qtd = cabec.qtd;

    RegistroUsuario novo;
    memset(&novo, 0, sizeof(novo));
    strncpy(novo.id, id, 15);
        novo.id[15] = '\0';  // força terminador

        strncpy(novo.senha, senha, 15);
        novo.senha[15] = '\0';

    uint16_t addr = EEPROM_ADDR_BASE + qtd * EEPROM_TAM_REG;
    uint8_t pacote[2 + sizeof(novo)];
    pacote[0] = (addr >> 8) & 0xFF;
    pacote[1] = addr & 0xFF;
    memcpy(&pacote[2], &novo, sizeof(novo));

    i2c_master_transmit(dev_handle, pacote, sizeof(pacote), -1);
    vTaskDelay(pdMS_TO_TICKS(5));

    // Atualiza cabeçalho
    cabec.qtd++;
    
    // att na i2c
    uint8_t cabec_pacote[2 + sizeof(cabec)];
    cabec_pacote[0] = 0x00; 
    cabec_pacote[1] = 0x00;
    memcpy(&cabec_pacote[2], &cabec, sizeof(cabec));
    
    i2c_master_transmit(dev_handle, cabec_pacote, sizeof(cabec_pacote), -1);

}

uint16_t I2C::qntdUsuarios() {
    return cabec.qtd;
}

void I2C::removerPorID(const char* id) {
    uint8_t buffer[32];
    char idBuffer[17];
    idBuffer[16] = '\0';  

    for (int i = 0; i < 256; i++) {
        uint16_t addr = i * 32;
        uint8_t v[2] = { (uint8_t)(addr >> 8), (uint8_t)(addr & 0xFF) };
        i2c_master_transmit_receive(dev_handle, v, 2, buffer, 32, -1);
        strncpy(idBuffer, (char*)buffer, 16);
        if (strcmp(idBuffer, id) == 0) {
            memset(buffer, 0xFF, 32);
            uint8_t w[34];
            w[0] = v[0];
            w[1] = v[1];
            memcpy(&w[2], buffer, 32);
            i2c_master_transmit(dev_handle, w, 34, -1);
            printf("ID '%s' removido com sucesso.\n", id);
            return;
        }
    }
    printf("ID '%s' não encontrado.\n", id);
}

void I2C::init(gpio_num_t pino_scl, gpio_num_t pino_sda) {
    i2c_bus_config.clk_source = I2C_CLK_SRC_DEFAULT;
	i2c_bus_config.flags.enable_internal_pullup = true;
	i2c_bus_config.i2c_port = 0;
	i2c_bus_config.scl_io_num = pino_scl;
	i2c_bus_config.sda_io_num = pino_sda;
	i2c_bus_config.glitch_ignore_cnt=7;

	ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_config, &bus_handle));
	dev_cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
	dev_cfg.device_address = 0x50;
	dev_cfg.scl_speed_hz = 100000;
	dev_cfg.scl_wait_us = 0;
	dev_cfg.flags.disable_ack_check=1;

	ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle));

    // Lê o cabeçalho da EEPROM para a memória local
    uint8_t addr_cabec[] = {0x00, 0x00};
    esp_err_t ret = i2c_master_transmit_receive(dev_handle, addr_cabec, sizeof(addr_cabec), 
                                            (uint8_t*)&cabec, sizeof(cabec), -1);
}

void I2C::removerPorID2(const char* id) {
    if (dev_handle == nullptr) {
        printf("Erro: I2C não inicializado.\n");
        return;
    }

    uint8_t buffer[32];
    char idBuffer[17];  // 16 + null terminator
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
            // Apagar o registro (preenche com 0xFF)
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

            // Atualiza o cabeçalho (decrementa qtd)
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
        printf("ERRO FATAL: dev_handle está NULL! Falha na criação do dispositivo I2C.\n");
        return;
    }

    // Leitura do cabeçalho da EEPROM
    uint8_t addr_cabec[] = {0x00, 0x00};
    ret = i2c_master_transmit_receive(dev_handle, addr_cabec, sizeof(addr_cabec), 
                                      (uint8_t*)&cabec, sizeof(cabec), -1);
    if (ret != ESP_OK) {
        printf("Erro ao ler cabeçalho da EEPROM: %s\n", esp_err_to_name(ret));
        // Inicializa manualmente
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
        // Verifica se os valores são válidos
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

    // Resetar cabeçalho
    cabec.qtd = 0;
    cabec.max = MAX_USUARIOS;
    uint8_t cabec_pacote[2 + sizeof(cabec)];
    cabec_pacote[0] = 0x00;
    cabec_pacote[1] = 0x00;
    memcpy(&cabec_pacote[2], &cabec, sizeof(cabec));
    i2c_master_transmit(dev_handle, cabec_pacote, sizeof(cabec_pacote), -1);

    printf("EEPROM limpa com sucesso.\n");
}

void I2C::registroUsuario2(const char* id, const char* senha) {
    if (cabec.qtd >= MAX_USUARIOS) {
        printf("Erro: limite máximo de usuários atingido (%d).\n", MAX_USUARIOS);
        return;
    }

    RegistroUsuario novo;
    memset(&novo, 0, sizeof(novo));
    
    // Copia com segurança e força terminador nulo
    strncpy(novo.id, id, sizeof(novo.id) - 1);
    novo.id[sizeof(novo.id) - 1] = '\0';
    
    strncpy(novo.senha, senha, sizeof(novo.senha) - 1);
    novo.senha[sizeof(novo.senha) - 1] = '\0';

    // Endereço onde será escrito o novo registro
    uint16_t addr = EEPROM_ADDR_BASE + cabec.qtd * EEPROM_TAM_REG;

    // Monta o pacote a ser enviado (2 bytes de endereço + struct RegistroUsuario)
    uint8_t pacote[2 + sizeof(novo)];
    pacote[0] = (addr >> 8) & 0xFF;
    pacote[1] = addr & 0xFF;
    memcpy(&pacote[2], &novo, sizeof(novo));

    printf("Gravando novo usuário no endereço 0x%04X: ID='%s', Senha='%s'\n", addr, novo.id, novo.senha);

    // Tenta gravar na EEPROM
    esp_err_t ret = i2c_master_transmit(dev_handle, pacote, sizeof(pacote), -1);
    vTaskDelay(pdMS_TO_TICKS(5));

    if (ret != ESP_OK) {
        printf("Erro ao gravar registro: %s\n", esp_err_to_name(ret));
        return;
    }

    // Atualiza cabeçalho apenas se a gravação foi bem-sucedida
    cabec.qtd++;

    uint8_t cabec_pacote[2 + sizeof(cabec)];
    cabec_pacote[0] = 0x00;
    cabec_pacote[1] = 0x00;
    memcpy(&cabec_pacote[2], &cabec, sizeof(cabec));

    ret = i2c_master_transmit(dev_handle, cabec_pacote, sizeof(cabec_pacote), -1);
    if (ret != ESP_OK) {
        printf("Erro ao atualizar cabeçalho: %s\n", esp_err_to_name(ret));
    } else {
        printf("Usuário registrado com sucesso. Total de usuários: %d\n", cabec.qtd);
    }
}

void I2C::listaTodos2() {
    if (dev_handle == nullptr) {
        printf("Erro: I2C não foi inicializado.\n");
        return;
    }

    printf("Cabeçalho lido: qtd = %u, max = %u\n", cabec.qtd, cabec.max);

    if (cabec.qtd == 0) {
        printf("Nenhum usuário cadastrado.\n");
        return;
    }

    for (uint16_t i = 0; i < cabec.qtd; i++) {
        uint16_t addr_registro = EEPROM_ADDR_BASE + (i * EEPROM_TAM_REG);

        uint8_t addr_bytes[2];
        addr_bytes[0] = (addr_registro >> 8) & 0xFF;
        addr_bytes[1] = addr_registro & 0xFF;

        RegistroUsuario usuario_lido;
        memset(&usuario_lido, 0, sizeof(usuario_lido));

        esp_err_t ret = i2c_master_transmit_receive(dev_handle, addr_bytes, sizeof(addr_bytes),
                                                    (uint8_t*)&usuario_lido, sizeof(usuario_lido), -1);

        if (ret != ESP_OK) {
            printf("Erro ao ler usuário %u: %s\n", i, esp_err_to_name(ret));
            continue;
        }

        // Garante \0 explícito no final das strings
        usuario_lido.id[sizeof(usuario_lido.id) - 1] = '\0';
        usuario_lido.senha[sizeof(usuario_lido.senha) - 1] = '\0';

        // Ignora registros claramente apagados (0xFF) ou vazios
        if (usuario_lido.id[0] == '\0' || usuario_lido.id[0] == (char)0xFF) {
            printf("%u - Registro vazio\n", i);
            continue;
        }

        printf("%u - id: %s, Senha: %s\n", i, usuario_lido.id, usuario_lido.senha);
    }
}



I2C i2c;



