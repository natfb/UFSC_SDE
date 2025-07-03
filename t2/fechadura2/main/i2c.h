#ifndef __I2C__
#define __I2C__

#include <stdint.h>
#include "driver/gpio.h"
#include <iostream>

using namespace std;

class I2C {
    public:
        // Usuario listaTodos(uint16_t posicao);
        void registroUsuario(string usuario);
        void qntdUsuarios();
        void removerPorID(const char* id);
        void init(gpio_num_t pino_scl, gpio_num_t pino_sda);
};

extern I2C i2c;

#endif