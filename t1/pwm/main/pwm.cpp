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
uint64_t tempo_em_us;

volatile int t_ciclo_de_trabalho = 2500;
// tempo do ciclo de trabalho = 1 / rpm | tranf em us
// testar o motor pra ver o 100% de rpm
// low = 100% - high 

static void timer_periodico (void* arg)
{
    gpio_set_direction(pino, GPIO_MODE_OUTPUT); //config pino como saida
    gpio_set_level(pino, HIGH);
    esp_timer_start_once(oneshot_timer, tempo_em_us); //Inicia um cronômetro de disparo.
}

static void oneshot_timer_callback(void* arg)
{
    gpio_set_direction(pino, GPIO_MODE_OUTPUT);
    gpio_set_level(pino, LOW);
}

void PWM::init(gpio_num_t PINO)
{
    pino = PINO;
    pinMode(pino, OUTPUT); //pino saída para controlar o transistor que liga o motor.
}

void PWM::cicloTrabalho(uint64_t tempo_em_us)
{
    const esp_timer_create_args_t parametros = {.callback = &timer_periodico}; //essa função será chamada automaticamente quando o periódico expirar
    const esp_timer_create_args_t oneshot_timer_args = {.callback = &oneshot_timer_callback};

    esp_timer_create(&parametros, &periodic_timer);
    esp_timer_create(&oneshot_timer_args, &oneshot_timer);

    esp_timer_start_periodic(periodic_timer, t_ciclo_de_trabalho);
}



