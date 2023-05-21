#pragma once

void bms_ic_init();

// SPI
void bms_ic_spi_config_reg0();

void bms_ic_spi_config_reg12();

uint16_t bms_ic_spi_device_number();

void bms_ic_cell_voltages(uint32_t *cell_voltages);


// I2C
void bms_ic_swap_to_i2c();

void bms_ic_test_i2c();

uint16_t bms_ic_i2c_device_number();

