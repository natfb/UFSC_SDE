#ifndef __I2C__
#define __I2C__

#include <stdint.h>
#include "driver/gpio.h"
#include <iostream>

using namespace std;

#define MAX_USUARIOS       100
#define EEPROM_ADDR_BASE   sizeof(tipo_cabecalho)
#define EEPROM_TAM_REG     32

typedef struct {
    uint16_t qtd;
    uint16_t max;
} tipo_cabecalho;

typedef struct {
    char id[16];
    char senha[16];
} RegistroUsuario;

class I2C {
    private:
        tipo_cabecalho cabec;
    public:
        void listaTodos();
        void listaTodos2();
        void registroUsuario(const char* id, const char* senha);
        void registroUsuario2(const char* id, const char* senha);
        uint16_t qntdUsuarios();
        void limparEEPROM();
        void removerPorID(const char* id);
        void removerPorID2(const char* id);
        void init(gpio_num_t pino_scl, gpio_num_t pino_sda);
        void init2(gpio_num_t pino_scl, gpio_num_t pino_sda);
};

extern I2C i2c;

#endif