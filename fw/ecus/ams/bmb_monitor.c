#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "bmbs.h"
#include "bqdriver/interface.h"
#include "bqdriver/bms.h"

#define I2C_ADDR(a) (a >> 1)

#define BMB_CONF {                          \
            .bal_cell_voltage_diff = 0.01,  \
            .bal_cell_voltage_min = 3.2,    \
        }

typedef struct
{
    uint16_t ntc_config_addr;
    uint8_t ntc_read_cmd;
} ntc_command_S;

const ntc_command_S ntcs[NTC_NUM_NTCS] = {
    [NTC_1] = {
        .ntc_config_addr = BQ769X2_SET_CONF_TS1,
        .ntc_read_cmd = BQ769X2_CMD_TEMP_TS1,
    },
    [NTC_2] = {
        .ntc_config_addr = BQ769X2_SET_CONF_TS2,
        .ntc_read_cmd = BQ769X2_CMD_TEMP_TS2,
    },
    [NTC_3] = {
        .ntc_config_addr = BQ769X2_SET_CONF_TS3,
        .ntc_read_cmd = BQ769X2_CMD_TEMP_TS3,
    },
    [NTC_4] = {
        .ntc_config_addr = BQ769X2_SET_CONF_HDQ,
        .ntc_read_cmd = BQ769X2_CMD_TEMP_HDQ,
    },
    [NTC_5] = {
        .ntc_config_addr = BQ769X2_SET_CONF_CFETOFF,
        .ntc_read_cmd = BQ769X2_CMD_TEMP_CFETOFF,
    },
    [NTC_6] = {
        .ntc_config_addr = BQ769X2_SET_CONF_DFETOFF,
        .ntc_read_cmd = BQ769X2_CMD_TEMP_DFETOFF,
    },
    [NTC_7] = {
        .ntc_config_addr = BQ769X2_SET_CONF_DCHG,
        .ntc_read_cmd = BQ769X2_CMD_TEMP_DCHG,
    },
    [NTC_8] = {
        .ntc_config_addr = BQ769X2_SET_CONF_DDSG,
        .ntc_read_cmd = BQ769X2_CMD_TEMP_DDSG,
    },
};

Bms bmbs[BMB_NUM_BMBS] = {
    [BMB_0] = {
        .conf = BMB_CONF,
        .i2c_addr = I2C_ADDR(0x10),
        .ntc_temp = { 0 },
    },
    [BMB_1] = {
        .conf = BMB_CONF,
        .i2c_addr = I2C_ADDR(0x20),
        .ntc_temp = { 0 },
    },
    [BMB_2] = {
        .conf = BMB_CONF,
        .i2c_addr = I2C_ADDR(0x30),
        .ntc_temp = { 0 },
    },
    [BMB_3] = {
        .conf = BMB_CONF,
        .i2c_addr = I2C_ADDR(0x40),
        .ntc_temp = { 0 },
    },
    [BMB_4] = {
        .conf = BMB_CONF,
        .i2c_addr = I2C_ADDR(0x50),
        .ntc_temp = { 0 },
    },
};

static Bms *current_bmb = &bmbs[BMB_0];

static void set_current_bmb(Bmb_E bmb)
{
    current_bmb = &bmbs[bmb];
    set_bmb_address(current_bmb->i2c_addr);
}

static void bmb_config_thermistors()
{
    // set 18kohm model
    uint8_t pin_config = 0;

    pin_config |= 0b00000011; // Configure Pin as a thermistor
    pin_config |= 0b00000100; // 18kOhm temperature model and 18K pull up resistor

    for (Bmb_ntc_E i = NTC_1; i < NTC_NUM_NTCS; i++)
    {
        bq769x2_datamem_write_u1(ntcs[i].ntc_config_addr, pin_config);
    }
}

static void bmb_init(Bmb_E bmb)
{
    set_current_bmb(bmb);

    bmb_config_thermistors();
}

static void bmb_read_temperature(Bmb_E bmb)
{
    set_current_bmb(bmb);

    // bmbs[bmb].ntc_temp;
    for (Bmb_ntc_E i = NTC_1; i < NTC_NUM_NTCS; i++)
    {
        int16_t temperature_reading; // unit 0.1K
        bq769x2_direct_read_i2(ntcs[i].ntc_read_cmd, &temperature_reading);
        bmbs[bmb].ntc_temp[i] = temperature_reading;

        printf("NTC_%d: %f K\n", i, ((temperature_reading * 0.1) +  273.15));
    }
}

void bmb_monitor_task(void * unused) {
    (void)unused;

    // Init temperature sensors
    for (Bmb_E bmb = BMB_0; bmb < BMB_NUM_BMBS; bmb++) {
        bmb_init(bmb);
    }


    int64_t last_config_update_time = 0;

    for (;;) {
        // config update?
        const int64_t UPDATE_INTERVAL_US = 10 * 1000 * 1000;
        const bool config_update = (esp_timer_get_time() - last_config_update_time) > UPDATE_INTERVAL_US;

        for (Bmb_E bmb = BMB_0; bmb < BMB_NUM_BMBS; bmb++) {
            set_current_bmb(bmb);

            // apply config
            if (config_update) {
                const int conf_err = bq769x2_config_update_mode(true);

                if (!conf_err) {
                    bms_apply_balancing_conf(current_bmb);
                }

                bq769x2_config_update_mode(false);

                vTaskDelay(pdMS_TO_TICKS(3));
            }

            // read data from bms
            bms_read_voltages(current_bmb);
            bms_update_error_flags(current_bmb);
            bms_update_balancing(current_bmb);

            bmb_read_temperature(bmb);
        }

        // update last config update time
        if (config_update) {
            last_config_update_time = esp_timer_get_time();
        }

        // delay for a bit until next run
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

// do what's right | made with <3 at Cooper Union
