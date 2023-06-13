#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "bmbs.h"
#include "bqdriver/interface.h"
#include "bqdriver/bms.h"

static _Atomic bool bmb_monitor_busy;
static _Atomic float bmb_monitor_lowest_cell_v;
static _Atomic float bmb_monitor_highest_cell_v;

#define I2C_ADDR(a) (a >> 1)

#define BMB_CONF {                          \
            .bal_cell_voltage_diff = 0.01,  \
            .bal_cell_voltage_min = 3.2,    \
            .dis_ut_limit = -127,           \
            .dis_ot_limit = 128,            \
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
    float lowest_cell = 999.9f;
    float highest_cell = 0.0f;

    for (;;) {
        // config update?
        const int64_t UPDATE_INTERVAL_US = 10 * 1000 * 1000;
        const bool config_update = (esp_timer_get_time() - last_config_update_time) > UPDATE_INTERVAL_US;

        // for (Bmb_E bmb = BMB_0; bmb < BMB_NUM_BMBS; bmb++) {
            Bmb_E bmb = BMB_2;
            set_current_bmb(bmb);

            bmb_monitor_busy = true;
            // apply config
            if (config_update) {
                const int conf_err = bq769x2_config_update_mode(true);

                if (!conf_err) {
                    bms_apply_balancing_conf(current_bmb);
                }

                bq769x2_config_update_mode(false);

                vTaskDelay(pdMS_TO_TICKS(3));
            }

            static uint16_t counter;

            // uint16_t balance = 0b010001000010100;
            // bq769x2_subcmd_write_u2(BQ769X2_SUBCMD_CB_ACTIVE_CELLS, balance);
            uint16_t threshold = 3300;
            bq769x2_subcmd_read_u2(BQ769X2_SUBCMD_CB_SET_LVL, &threshold);

            uint16_t cbstatus1 = 0;
            // bq769x2_subcmd_read_u2(BQ769X2_SUBCMD_CBSTATUS1, &cbstatus1);
            bq769x2_subcmd_read_u2(BQ769X2_SUBCMD_CBSTATUS1, &cbstatus1);
            printf("cbstatus: %u, count: %u\n", cbstatus1, counter++);

            // read data from bms
            bms_read_voltages(current_bmb);
            bms_update_error_flags(current_bmb);
            bms_update_balancing(current_bmb);
            bms_read_temperatures(current_bmb);
            printf("temps: %f -- %f -- %f\n", current_bmb->status.bat_temp_min, current_bmb->status.mosfet_temp, current_bmb->status.ic_temp);

            // update highest/lowest cell
            for (uint8_t cell = 0; cell < 16; cell++) {
                const float cell_voltage = current_bmb->status.cell_voltages[cell];

                if (cell_voltage < lowest_cell) {
                    lowest_cell = cell_voltage;
                }

                if (cell_voltage > highest_cell) {
                    highest_cell = cell_voltage;
                }
            }

            bmb_monitor_busy = false;
        // }

        // update last config update time
        if (config_update) {
            last_config_update_time = esp_timer_get_time();
        }

        bmb_monitor_lowest_cell_v = lowest_cell;
        bmb_monitor_highest_cell_v = highest_cell;

        // delay for a bit until next run
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

/*
 * Safe to reboot?
*/
bool bmb_monitor_ready_for_reboot(void) {
    return !bmb_monitor_busy;
}

/*
 * Get lowest cell voltage
*/
float bmb_monitor_get_lowest_cell_v(void) {
    return bmb_monitor_lowest_cell_v;
}

/*
 * Get highest cell voltage
*/
float bmb_monitor_get_highest_cell_v(void) {
    return bmb_monitor_highest_cell_v;
}

// do what's right | made with <3 at Cooper Union
