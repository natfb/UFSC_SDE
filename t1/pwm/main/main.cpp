#include <stdio.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdlib.h>     /* llabs */
#include "esp_timer.h"

#define PINO  GPIO_NUM_23 // motor
#define HALL  GPIO_NUM_22 // transistor efeito hall

volatile int valor = 0;
volatile int64_t tempoAnterior = 0;

extern "C" void app_main();

void INTERRUPCAO (void *) {
    valor++;
}

void app_main(void)
{
    gpio_set_direction(HALL, GPIO_MODE_INPUT);
    gpio_set_pull_mode(HALL, GPIO_PULLUP_ONLY);

    gpio_install_isr_servise();
    gpio_set_intr_type(HALL, GPIO_INTR_POSEDGE);
    gpio_isr_handler_add(HALL, INTERRUPCAO, NULL);

    int x;

    while (1) {
        x = valor;
        valor = 0;
        printf("frequência= %d\n", x*10);
        vTaskDelay(6000 / portTICK_PERIOD_MS);
    }
}
