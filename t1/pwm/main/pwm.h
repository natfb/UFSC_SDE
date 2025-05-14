#ifndef __PWM__
#define __PWM__
#include "driver/gpio.h"

class PWM {
    public:
        void init(gpio_num_t PINO);
        void cicloTrabalho(uint64_t tempo_em_us);
};

extern PWM pwm;

#endif