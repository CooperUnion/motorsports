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
static void ams_10Hz();
static void ams_1Hz();

ember_rate_funcs_S module_rf = {
    .call_init = ams_init,
    .call_1Hz = ams_1Hz,
    .call_10Hz = ams_10Hz,
    .call_100Hz = NULL,
    .call_1kHz = NULL,
};

static void ams_init()
{
    // SETUP AND PROGRAM THE BMS
    gpio_set_direction(GPIO_BLINK_LED, GPIO_MODE_INPUT_OUTPUT);

    bms_ic_init();
}

static void ams_10Hz()
{
    static bool led1 = false;
    gpio_set_level(GPIO_BLINK_LED, led1);
    led1 = !led1;
}

static void ams_1Hz()
{
    fprintf(stderr, "reading dev num and voltages\n");
    const uint16_t dev_num = bms_ic_i2c_device_number();
    Bms bms;
    bms_read_voltages(&bms);
    bms_update_error_flags(&bms);
    bms_print_registers(&bms);
}

// do what's right | made with <3 at Cooper Union
