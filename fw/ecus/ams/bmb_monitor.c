#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "bmbs.h"
#include "bqdriver/interface.h"
#include "bqdriver/bms.h"

#define I2C_ADDR(a) (a >> 1)

#define BMB_CONF {                          \
            .bal_cell_voltage_diff = 0.01,  \
            .bal_cell_voltage_min = 3.48,   \
            \
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

static void bmb_print_manufacturing_status()
{
    MFG_STATUS_Type mfg_status;
    int err = bq769x2_subcmd_read_u2(BQ769X2_SUBCMD_MFG_STATUS, &mfg_status.u16);
    if (err) {
        printf("fuck\n");
    }
    else
    {
        printf("MANUFACTURING STATUS: %x\n", mfg_status.u16);
    }
}

void bmb_monitor_task(void * unused) {
    (void)unused;

    int64_t last_config_update_time = 0;

    set_current_bmb(BMB_2);
    bq769x2_subcmd_cmd_only(BQ769X2_SUBCMD_SLEEP_DISABLE);

    vTaskDelay(pdMS_TO_TICKS(500));

    const int conf_err = bq769x2_config_update_mode(true);
    if (!conf_err)
    {
        bq769x2_datamem_write_u1(BQ769X2_SET_FET_OPTIONS, 0x1D);

        MFG_STATUS_Type mfg_status;
        int err = bq769x2_subcmd_read_u2(BQ769X2_SUBCMD_MFG_STATUS, &mfg_status.u16);
        if (err) {
            printf("fuck\n");
        }
        else if (mfg_status.FET_EN == 0) {
            // FET_ENABLE subcommand sets FET_EN bit in MFG_STATUS and MFG_STATUS_INIT registers
            err = bq769x2_subcmd_cmd_only(BQ769X2_SUBCMD_FET_ENABLE);
            if (err) {
                printf("fuck\n");
            }
        }
    }
    else
    {
        printf("UNABLE TO CONFIG FET_OPTION(): %d\n", conf_err);
    }
    bq769x2_config_update_mode(false);

    bq769x2_config_update_mode(true);
    bq769x2_datamem_write_u1(BQ769X2_SET_CBAL_INTERVAL, 255);
    bq769x2_config_update_mode(false);

    for (;;) {
        // config update?
        const int64_t UPDATE_INTERVAL_US = 10 * 1000 * 1000; // 10 Seconds
        const bool config_update = (esp_timer_get_time() - last_config_update_time) > UPDATE_INTERVAL_US;

        // for (Bmb_E bmb = BMB_0; bmb < BMB_NUM_BMBS; bmb++)
        {
            Bmb_E bmb = BMB_2;
            set_current_bmb(bmb);

            uint16_t balance = 0b0000000000010100;
            bq769x2_subcmd_write_u2(BQ769X2_SUBCMD_CB_ACTIVE_CELLS, balance);

            uint16_t cbstatus1 = 0;
            bq769x2_subcmd_read_u2(BQ769X2_SUBCMD_CBSTATUS1, &cbstatus1);
            printf("cbstatus: %u\n",cbstatus1);

            // // apply config
            // if (config_update) {
            //     const int conf_err = bq769x2_config_update_mode(true);

            //     if (!conf_err) {
            //         bms_apply_balancing_conf(current_bmb);
            //     }

            //     bq769x2_config_update_mode(false);
            //     vTaskDelay(pdMS_TO_TICKS(3));

            //     bq769x2_subcmd_cmd_only(BQ769X2_SUBCMD_SLEEP_DISABLE);

            //     vTaskDelay(pdMS_TO_TICKS(3));
            // }

            // read data from bms
            bms_read_voltages(current_bmb);
            bms_update_error_flags(current_bmb);
            bms_update_balancing(current_bmb);

            bms_print_registers();
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
