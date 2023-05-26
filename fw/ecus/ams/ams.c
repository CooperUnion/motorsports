// Cooper Motorsports Precharge Controller

#include <opencan_tx.h>

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "bmb_monitor.h"
#include "bqdriver/interface.h"
#include "ember_taskglue.h"
#include "node_pins.h"

static bool bmb_monitor_started = false;

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
    gpio_config(&(gpio_config_t){
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = BIT64(NODE_BOARD_PIN_LED1),
    });

    printf("initializing i2c...\n");
    bq769x2_init();
}

static void ams_10Hz()
{
    static bool led1 = false;
    gpio_set_level(NODE_BOARD_PIN_LED1, led1);
    led1 = !led1;

    if (!bmb_monitor_started) {
        printf("starting bmb monitor task...\n");
        static TaskHandle_t bmb_monitor_handle;
        xTaskCreatePinnedToCore(bmb_monitor_task, "BMB_MONITOR", 8192, 0, 3, &bmb_monitor_handle, 0);

        bmb_monitor_started = true;
    }
}

static void ams_1Hz()
{

}

// do what's right | made with <3 at Cooper Union
