#include "i2c.h"
#include <stdio.h>
#include "driver/i2c_master.h"
#include <inttypes.h> 
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <unistd.h>
// #include "driver/gpio.h"

i2c_master_dev_handle_t dev_handle;
i2c_master_bus_handle_t bus_handle;
i2c_master_bus_config_t i2c_bus_config = {};
uint16_t end = 0x50;
i2c_device_config_t dev_cfg = {};

uint8_t I2C::listaTodos(uint16_t posicao) {
    uint8_t resultado;
	uint8_t v[2];
	v[0] = (posicao >> 8) & 0xff;
	v[1] = (posicao & 0xff);
	i2c_master_transmit_receive(dev_handle, v, 2, &resultado, 1, -1);
	return resultado;
}

void I2C::registroUsuario() {
}

void I2C::qntdUsuarios() {

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



