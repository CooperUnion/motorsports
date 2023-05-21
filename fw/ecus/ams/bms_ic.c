#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "driver/i2c.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "bms_ic.h"
#include "bqdriver/bms.h"
#include "bqdriver/bq769x2.h"
#include "bqdriver/interface.h"
#include "bqdriver/registers.h"

// static Bms bms = {
//     .conf = {
//         .bal_cell_voltage_diff = 0.01,
//         .bal_cell_voltage_min = 3.2,
//     }
// };

// void bms_ic_init()
// {
//     vTaskDelay(pdMS_TO_TICKS(3));
//     int bms_apply_balancing_conf(Bms *bms);
//     printf("applying balancing config...\n");

//     bq769x2_config_update_mode(true);

//     bq769x2_datamem_write_u2(BQ769X2_SET_CONF_VCELL_MODE, 0xFFFF);
//     bms_apply_balancing_conf(&bms);

//     bq769x2_config_update_mode(false);
// }

// uint16_t bms_ic_i2c_device_number()
// {
//     uint16_t device_number;
//     int err = bq769x2_subcmd_read_u2(BQ769X2_SUBCMD_DEVICE_NUMBER, &device_number);
//     if (err) {
//         fprintf(stderr, "bms: failed to read device number: %d", err);
//         return 0;
//     }
//     else {
//         printf("detected bq device number: 0x%x", device_number);
//     }

//     return device_number;
// }

// do what's right | made with <3 at Cooper Union
