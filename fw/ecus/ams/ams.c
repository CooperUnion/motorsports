// Cooper Motorsports Precharge Controller

#include <opencan_tx.h>

#include "driver/gpio.h"
#include "esp_log.h"

#include "ember_taskglue.h"
#include "bms_ic.h"

#define GPIO_BLINK_LED 46

static void ams_init();
static void ams_1Hz();
static void ams_10Hz();

ember_rate_funcs_S module_rf = {
    .call_init = ams_init,
    .call_1Hz = ams_1Hz,
    .call_10Hz = ams_10Hz,
    .call_100Hz = NULL,
    .call_1kHz = NULL,
};

static uint16_t device_number = 0;
static void ams_init()
{
    // SETUP AND PROGRAM THE BMS
    esp_log_write(ESP_LOG_INFO, "AMS: ", "initialized!!\n");

    gpio_set_direction(GPIO_BLINK_LED, GPIO_MODE_INPUT_OUTPUT);

    // gpio_set_level(GPIO_BLINK_LED, 1);

    bms_ic_init();

}

static void ams_1Hz()
{
    uint8_t blink_led = gpio_get_level(GPIO_BLINK_LED);
    blink_led = (blink_led) ? 0 : 1;

    gpio_set_level(GPIO_BLINK_LED, blink_led);

    bms_ic_config_reg0();
    bms_ic_config_reg12();
    device_number = bms_ic_device_number();

    printf("Device Number: %x\n", device_number);
}


static void ams_10Hz()
{
    // esp_log_write(ESP_LOG_INFO, "AMS: ", "\trunning at 10Hz!\n");

    // uint8_t blink_led = gpio_get_level(GPIO_BLINK_LED);
    // blink_led = (blink_led) ? 0 : 1;

    // gpio_set_level(GPIO_BLINK_LED, blink_led);
}

// do what's right | made with <3 at Cooper Union
