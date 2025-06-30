#ifndef __I2C__
#define __I2C__

#include <stdint.h>
#include "driver/gpio.h"

class I2C {
    public:
        uint8_t listaTodos(uint16_t posicao);
        void registroUsuario();
        void qntdUsuarios();
        void init(gpio_num_t pino_scl, gpio_num_t pino_sda);
};

extern I2C i2c;

#endif