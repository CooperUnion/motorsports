#include "zephyr_adapter.h"

#include <driver/i2c.h>
#include <freertos/FreeRTOS.h>

#include "ams_pins.h"

#define BMS_IC_I2C_HOST I2C_NUM_0

uint64_t k_uptime_get(void) {
    // todo
    return 0;
}

void k_usleep(uint32_t duration) {
    // todo
}

int i2c_init(void) {
    // Configure I2C

    i2c_config_t conf = {
        .mode             = I2C_MODE_MASTER,
        .sda_io_num       = AMS_PIN_SDA,
        .scl_io_num       = AMS_PIN_SCL,
        .sda_pullup_en    = GPIO_PULLUP_DISABLE,
        .scl_pullup_en    = GPIO_PULLUP_DISABLE,
        .master.clk_speed = 50000, // 50kHz
    };

    ESP_ERROR_CHECK(i2c_param_config(BMS_IC_I2C_HOST, &conf));

    uint32_t interrupt_flags = 0;

    ESP_ERROR_CHECK(i2c_driver_install(
        BMS_IC_I2C_HOST,
        I2C_MODE_MASTER,
        0, // slv_rx/tx buffer length is ignored in master mode
        0, // slv_rx/tx buffer length is ignored in master mode
        interrupt_flags
    ));

    // ESP_ERROR_CHECK(i2c_set_timeout(BMS_IC_I2C_HOST, 50000));

    return 0;
}

int i2c_write_read(uint16_t addr,
	const void *write_buf, size_t num_write,
	void *read_buf, size_t num_read)
{
    return i2c_master_write_read_device(BMS_IC_I2C_HOST, addr, write_buf, num_write, read_buf, num_read, pdMS_TO_TICKS(5));
}

int i2c_write(const uint8_t *buf,
	uint32_t num_bytes, uint16_t addr)
{
    return i2c_master_write_to_device(BMS_IC_I2C_HOST, addr, buf, num_bytes, pdMS_TO_TICKS(5));
}

// do what's right | made with <3 at Cooper Union
