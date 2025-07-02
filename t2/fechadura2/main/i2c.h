#ifndef __I2C__
#define __I2C__

#include <stdint.h>
#include "driver/gpio.h"

#define MAX_USUARIOS       100
#define EEPROM_ADDR_BASE   0x0001
#define EEPROM_TAM_REG     32

typedef struct {
    char id[16];
    char senha[16];
} RegistroUsuario;

class I2C {
    public:
        uint8_t listaTodos(uint16_t posicao);
        void registroUsuario(const char* id, const char* senha);
        void qntdUsuarios();
        void removerPorID(const char* id);
        void init(gpio_num_t pino_scl, gpio_num_t pino_sda);
};

extern I2C i2c;

#endif