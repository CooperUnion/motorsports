#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "bmbs.h"
#include "bqdriver/interface.h"
#include "bqdriver/bms.h"

#define I2C_ADDR(a) (a >> 1)

#define BMB_CONF {                          \
            .bal_cell_voltage_diff = 0.01,  \
            .bal_cell_voltage_min = 5.0,    \
        }

Bms bmbs[BMB_NUM_BMBS] = {
    [BMB_0] = {
        .conf = BMB_CONF,
        .i2c_addr = I2C_ADDR(0x10),
    },
    [BMB_1] = {
        .conf = BMB_CONF,
        .i2c_addr = I2C_ADDR(0x20),
    },
    [BMB_2] = {
        .conf = BMB_CONF,
        .i2c_addr = I2C_ADDR(0x30),
    },
    [BMB_3] = {
        .conf = BMB_CONF,
        .i2c_addr = I2C_ADDR(0x40),
    },
    [BMB_4] = {
        .conf = BMB_CONF,
        .i2c_addr = I2C_ADDR(0x50),
    },
};

static Bms *current_bmb = &bmbs[BMB_0];

static void set_current_bmb(Bmb_E bmb)
{
    current_bmb = &bmbs[bmb];
    set_bmb_address(current_bmb->i2c_addr);
}

void bmb_monitor_task(void * unused) {
    (void)unused;

    int64_t last_config_update_time = 0;

    for (;;) {
        // config update?
        const int64_t UPDATE_INTERVAL_US = 10 * 1000 * 1000;
        const bool config_update = (esp_timer_get_time() - last_config_update_time) > UPDATE_INTERVAL_US;

        // for (Bmb_E bmb = BMB_0; bmb < BMB_NUM_BMBS; bmb++)
        {
            Bmb_E bmb = BMB_2;
            set_current_bmb(bmb);

            // apply config
            // if (config_update) {
            //     const int conf_err = bq769x2_config_update_mode(true);

            //     if (!conf_err) {
            //         bms_apply_balancing_conf(current_bmb);
            //     }

            //     bq769x2_config_update_mode(false);

            //     vTaskDelay(pdMS_TO_TICKS(3));
            // }

            // read data from bms
            bms_read_voltages(current_bmb);
            // bms_update_error_flags(current_bmb);
            // bms_update_balancing(current_bmb);
        }

        // update last config update time
        if (config_update) {
            last_config_update_time = esp_timer_get_time();
        }

        // delay for a bit until next run
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// do what's right | made with <3 at Cooper Union
