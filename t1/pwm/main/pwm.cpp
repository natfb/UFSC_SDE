#include "pwm.h"
#include "driver/gpio.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdlib.h>     /* llabs */
#include "esp_timer.h"

esp_timer_handle_t periodic_timer;
esp_timer_handle_t oneshot_timer;
gpio_num_t pino;

// testar o motor pra ver o 100% de rpm
//  

static void timer_periodico (void* arg)
{
    int64_t time_since_boot = esp_timer_get_time();
    digitalWrite(pino, HIGH);
    printf("Periodic timer called, time since boot: %lld us\n", time_since_boot);
}

static void oneshot_timer_callback(void* arg)
{
    digitalWrite(pino, LOW);
    esp_timer_stop(periodic_timer);
}

void PWM::init(gpio_num_t PINO)
{
    pino = PINO;
    pinMode(pino, OUPUT);
}

void PWM::cicloTrabalho(uint64_t tempo_em_us)
{
    const esp_timer_create_args_t parametros = {.callback = &timer_periodico};
    const esp_timer_create_args_t oneshot_timer_args = {.callback = &oneshot_timer_callback};

    esp_timer_create(&parametros, &periodic_timer);
    esp_timer_create(&oneshot_timer_args, &oneshot_timer);


    esp_timer_start_periodic(periodic_timer, tempo_em_us);
    esp_timer_start_once(oneshot_timer, 5000000);
}

