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

// Usuario I2C::listaTodos(uint16_t posicao) {
//     uint8_t resultado;
// 	uint8_t v[2];
// 	v[0] = (posicao >> 8) & 0xff;
// 	v[1] = (posicao & 0xff);
// 	i2c_master_transmit_receive(dev_handle, v, 2, &resultado, 1, -1);
// 	return resultado;
// }

void I2C::registroUsuario(string usuario) {
   
}

void I2C::qntdUsuarios() {
    uint8_t buffer[32];
    int count = 0;

    for (int i = 0; i < 256; i++) {
        uint16_t addr = i * 32;
        uint8_t v[2] = { (uint8_t)(addr >> 8), (uint8_t)(addr & 0xFF) };
        if (i2c_master_transmit_receive(dev_handle, v, 2, buffer, 1, -1) == ESP_OK) {
            if (buffer[0] != 0xFF) count++;
        }
    }

    printf("Quantidade de usuários cadastrados: %d\n", count);
}

void removerPorID(const char* id) {
    uint8_t buffer[32];
    char idBuffer[17];
    idBuffer[16] = '\0';  // garantia de null terminator

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
	dev_cfg.scl_wait_us=0;
	dev_cfg.flags.disable_ack_check=1;

	ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle));
}


I2C i2c;



