// Cooper Motorsports Precharge Controller

#include <opencan_tx.h>

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

#include "driver/gpio.h"

#include "ember_taskglue.h"
#include "bms_ic.h"
#include "bqdriver/bms.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define GPIO_BLINK_LED 46

#define GPIO_TEST_I2C 48

static void ams_init();
static void ams_1Hz();

ember_rate_funcs_S module_rf = {
    .call_init = ams_init,
    .call_1Hz = ams_1Hz,
    .call_10Hz = NULL,
    .call_100Hz = NULL,
    .call_1kHz = NULL,
};

static uint16_t device_number = 0;
static bool is_spi = true;
static uint32_t cell_voltages[16] = { 0 };

static void ams_init()
{
    // SETUP AND PROGRAM THE BMS
    gpio_set_direction(GPIO_BLINK_LED, GPIO_MODE_INPUT_OUTPUT);

    gpio_set_direction(GPIO_TEST_I2C, GPIO_MODE_OUTPUT);


    // gpio_set_level(GPIO_BLINK_LED, 1);

    // bms_ic_init();

    // // set I2C address
    // // swap_to_i2c()

    // vTaskDelay(2000); // delay to printout spi transactions on init

    // device_number = bms_ic_spi_device_number();
    // device_number = bms_ic_spi_device_number();

    // bms_ic_spi_config_reg0();
    // bms_ic_spi_config_reg12();

    bms_ic_swap_to_i2c();
}

static void ams_1Hz()
{

    // bms_ic_spi_config_reg0();
    // bms_ic_spi_config_reg12();

    uint8_t blink_led = gpio_get_level(GPIO_BLINK_LED);
    blink_led = (blink_led) ? 0 : 1;

    gpio_set_level(GPIO_BLINK_LED, blink_led);

    static uint32_t counter = 1;
    // if (counter % 2 == 0 && is_spi)
    // {
    //     // bms_ic_swap_to_i2c();
    //     is_spi = false;
    // }
    counter++;

    // printf("spi\n");
    // device_number = bms_ic_spi_device_number();
    // bms_ic_spi_config_reg0();
    // bms_ic_spi_config_reg12();

    // if (is_spi)
    // {
    // }
    // else
    // {
    //     // printf("i2c\n");
    //     // device_number = bms_ic_i2c_device_number();
    // }

    // // device_number = bms_ic_i2c_device_number();
    // device_number = bms_ic_spi_device_number();
    // printf("counter: %ld || Device Number: %x\n", counter, device_number);

    // bms_ic_cell_voltages(cell_voltages);

    // printf("is_spi: %d\n", is_spi);

    // bms_ic_test_i2c();

    // successfully swap to i2c
    fprintf(stderr, "reading dev num and voltages\n");
    const uint16_t dev_num = bms_ic_i2c_device_number();
    Bms bms;
    bms_read_voltages(&bms);
    bms_update_error_flags(&bms);
    bms_print_registers(&bms);
}

// do what's right | made with <3 at Cooper Union
