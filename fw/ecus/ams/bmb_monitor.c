#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "bmbs.h"
#include "bqdriver/interface.h"
#include "bqdriver/bms.h"

#define I2C_ADDR(a) (a >> 1)

#define BMB_CONF {                          \
            .bal_cell_voltage_diff = 0.01,  \
            .bal_cell_voltage_min = 3.2,    \
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

    for (;;) {
        // for (Bmb_E bmb = BMB_0; bmb < BMB_NUM_BMBS; bmb++) {
            Bmb_E bmb = BMB_0;
            set_current_bmb(bmb);
            bms_read_voltages(&bmbs[bmb]);
            bms_update_error_flags(&bmbs[bmb]);
        // }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
