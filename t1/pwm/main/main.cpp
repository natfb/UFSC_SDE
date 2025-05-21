#include <stdio.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdlib.h>     /* llabs */
#include "esp_timer.h"
#include "pwm.h"

#define PINO  GPIO_NUM_23 // motor
#define HALL  GPIO_NUM_22 // transistor efeito hall

volatile int valor = 0;
volatile int64_t tempoAnterior = 0;
uint64_t tempo_em_us_2 = 5000;

extern "C" void app_main();

void INTERRUPCAO (void *) {
    valor++;
}

void app_main(void)
{

    // PARA TESTAR
    // gpio_set_direction(PINO, GPIO_MODE_OUTPUT);
    // gpio_set_level(PINO, 1);
    
    // pwm
    pwm.init(PINO);
    pwm.cicloTrabalho(470);
    // tempo do pwm:
    // [0, 400]
    // > 400 explode o codigo por motivos indeterminados 
    // teoricamente iria ate 478
    //Se o ciclo tempo_em_us == período, o pino fica sempre ligado → sinal DC → frequência zero, ou medições erradas.
    //Para frequência 2090 Hz (período 478 us), use algo como 450 us para ciclo alto, 28 us para ciclo baixo.
    //Se o tempo ligado = período, o timer one-shot pode acabar sendo acionado exatamente no mesmo tempo que o período, gerando conflito.
    //O pino sendo ligado e desligado a cada 2 períodos em vez de 1 (ou seja, frequência dividida por 2).

    // hall
    gpio_set_direction(HALL, GPIO_MODE_INPUT);
    gpio_set_pull_mode(HALL, GPIO_PULLUP_ONLY);

    gpio_install_isr_service(1);
    gpio_set_intr_type(HALL, GPIO_INTR_POSEDGE);
    gpio_isr_handler_add(HALL, INTERRUPCAO, NULL);
    
    
    int x;

    while (1) {
        x = valor;
        valor = 0;
        printf("frequência= %d, %d\n", x*10, valor);
        vTaskDelay(6000 / portTICK_PERIOD_MS);
    }
}
