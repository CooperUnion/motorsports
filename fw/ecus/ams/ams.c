// Cooper Motorsports Precharge Controller

#include <opencan_tx.h>

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "bmb_monitor.h"
#include "bms_ic.h"
#include "bqdriver/interface.h"
#include "ember_taskglue.h"
#include "node_pins.h"

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

// TEMP
#include "bqdriver/bms.h"
#include "bmbs.h"
#include "bqdriver/registers.h"

static void ams_init()
{
    // SETUP AND PROGRAM THE BMS
    gpio_config(&(gpio_config_t){
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = BIT64(NODE_BOARD_PIN_LED1),
    });

    printf("initializing i2c...\n");
    bq769x2_init();

    set_bmb_address((0x10 >> 1));
    // set_bmb_address((0x30 >> 1));

    // printf("starting bmb monitor task...\n");
    // static TaskHandle_t bmb_monitor_handle;
    // xTaskCreatePinnedToCore(bmb_monitor_task, "BMB_MONITOR", 8192, 0, 3, &bmb_monitor_handle, 0);

    printf("Battery Status: \n");
    bms_print_register(BQ769X2_CMD_BATTERY_STATUS);
    vTaskDelay(pdMS_TO_TICKS(10));
    bms_print_register(BQ769X2_CMD_BATTERY_STATUS + 1);
    vTaskDelay(pdMS_TO_TICKS(10));

    // Read the Security Keys:
    bq769x2_subcmd_cmd_only(BQ769X2_SUBCMD_SECURITY_KEYS);

    uint8_t rx_data[8] = { 0 };
    bq769x2_read_bytes(BQ769X2_SUBCMD_DATA_START, rx_data, 8);

    printf("Security Keys: ");
    for (size_t i = 0; i < 8; i++)
    {
        printf("%x ", rx_data[i]);
    }
    printf("\n");

    // Read the security Mode
    uint8_t value;
    uint8_t battery_security_status = 0;
    bq769x2_read_bytes(BQ769X2_CMD_BATTERY_STATUS + 1, &value, 1);
    battery_security_status = value & 0b00000011;


    const bool in_full_access_mode = (battery_security_status == 0x01);

    vTaskDelay(pdMS_TO_TICKS(2000));


    if (in_full_access_mode)
    {
        uint8_t value;
        printf("Security status: FULLACCESS\n");

        bq769x2_config_update_mode(true);

        // Configure the data registers

        // REG0
        uint8_t reg0_config = 0x01;
        bq769x2_subcmd_write_u1(BQ769X2_SET_CONF_REG0, reg0_config);

        // REG12
        uint8_t reg12_config = 0b00111111; // Reg1: 5V, Reg2: 1.8V
        bq769x2_subcmd_write_u1(BQ769X2_SET_CONF_REG12, reg12_config);

        vTaskDelay(pdMS_TO_TICKS(50));

        vTaskDelay(pdMS_TO_TICKS(50));

        // // I2C Address
        #define I2C_ADDR(a) (a >> 1)
        uint8_t i2c_addr_config = 0x20;
        bq769x2_subcmd_write_u1(BQ769X2_SET_CONF_I2C_ADDR, i2c_addr_config);

        vTaskDelay(pdMS_TO_TICKS(50));
        bq769x2_subcmd_cmd_only(BQ769X2_SUBCMD_SWAP_COMM_MODE);

        set_bmb_address(i2c_addr_config >> 1);

        uint8_t read_i2c_addr = 0;
        bq769x2_subcmd_read_u1(BQ769X2_SET_CONF_I2C_ADDR, &read_i2c_addr);
        printf("Reading I2C address: %x\n", read_i2c_addr);

        bq769x2_config_update_mode(false);

        // Doubled Check the Data Memory registers

        // REG0
        uint8_t read_reg0_config = 0;
        bq769x2_subcmd_read_u1(BQ769X2_SET_CONF_REG0, &read_reg0_config);

        // REG12
        uint8_t read_reg12_config = 0;
        bq769x2_subcmd_read_u1(BQ769X2_SET_CONF_REG12, &read_reg12_config);

        if ((reg0_config == read_reg0_config) &&
            (reg12_config == read_reg12_config))
        {
            printf("\tReg0 and Reg12 are correct!\n");
        }

        read_i2c_addr = 0;
        bq769x2_subcmd_read_u1(BQ769X2_SET_CONF_I2C_ADDR, &read_i2c_addr);
        printf("Reading I2C address: %x\n", read_i2c_addr);

        if (read_i2c_addr == i2c_addr_config)
        {
            printf("\tI2C address properly set!\n");
        }

        bq769x2_config_update_mode(true);

        // Check the OTPB bit is not set in battery status

        bq769x2_read_bytes(BQ769X2_CMD_BATTERY_STATUS + 1, &value, 1);
        uint8_t otpb_bit_set = value & 0b10000000; // otp block bit
        if (!otpb_bit_set)
        {
            printf("OTPB BIT Clear! (%x)\n", value);
        }

        // Read OTP_WR_CHECK(), and if 0x80 is read then it's ready to OTP
        uint8_t otp_wr_check = 0;
        bq769x2_subcmd_read_u1(BQ769X2_SUBCMD_OTP_WR_CHECK, &otp_wr_check);
        printf("OPT_WR_CHECK [%x]\n", otp_wr_check);
        if (otp_wr_check == 0x80) // (0x80) ready for OTP write
        {
            printf("\tREADY FOR OTP WR!!!\n");
        }

        // :skull: :skull: :skull:
        // // send OTP_WRITE()
        // bq769x2_subcmd_cmd_only(BQ769X2_SUBCMD_OTP_WRITE);

        // vTaskDelay(pdMS_TO_TICKS(100));
        // // Read from 0x40, and is 0x80 is read then the OTP was successfull
        // uint8_t otp_write_result = 0;
        // bq769x2_subcmd_read_u1(BQ769X2_SUBCMD_DATA_START, &otp_write_result);
        // if (otp_write_result == 0x80)
        // {
        //     printf("OTP Written correctly !!\n");
        // }

        bq769x2_config_update_mode(false);
    }
}

#define UNSEAL_KEY     0xDEAD
#define FULLACCESS_KEY 0xBEEF

static void ams_10Hz()
{
    static bool led1 = false;
    gpio_set_level(NODE_BOARD_PIN_LED1, led1);
    led1 = !led1;
}

static void ams_1Hz()
{
    printf("Battery Status: \n");
    bms_print_register(BQ769X2_CMD_BATTERY_STATUS);
    vTaskDelay(pdMS_TO_TICKS(10));
    bms_print_register(BQ769X2_CMD_BATTERY_STATUS + 1);
    vTaskDelay(pdMS_TO_TICKS(10));

    // Read the Security Keys:
    bq769x2_subcmd_cmd_only(BQ769X2_SUBCMD_SECURITY_KEYS);

    uint8_t rx_data[8] = { 0 };
    bq769x2_read_bytes(BQ769X2_SUBCMD_DATA_START, rx_data, 8);

    printf("Security Keys: ");
    for (size_t i = 0; i < 8; i++)
    {
        printf("%x ", rx_data[i]);
    }
    printf("\n");

    // Read the security Mode
    uint8_t value;
    uint8_t battery_security_status = 0;
    bq769x2_read_bytes(BQ769X2_CMD_BATTERY_STATUS + 1, &value, 1);
    battery_security_status = value & 0b00000011;

    switch (battery_security_status)
    {
        case 0x01:
            printf("Security status: FULLACCESS\n");

            // bq769x2_config_update_mode(true);

            // // Configure the data registers

            // // REG0
            // uint8_t reg0_config = 0x01;
            // bq769x2_subcmd_write_u1(BQ769X2_SET_CONF_REG0, reg0_config);

            // // REG12
            // uint8_t reg12_config = 0b00111111; // Reg1: 5V, Reg2: 1.8V
            // bq769x2_subcmd_write_u1(BQ769X2_SET_CONF_REG12, reg12_config);

            // vTaskDelay(pdMS_TO_TICKS(50));

            // vTaskDelay(pdMS_TO_TICKS(50));

            // // // I2C Address
            // #define I2C_ADDR(a) (a >> 1)
            // uint8_t i2c_addr_config = 0x10;
            // bq769x2_subcmd_write_u1(BQ769X2_SET_CONF_I2C_ADDR, i2c_addr_config);

            // vTaskDelay(pdMS_TO_TICKS(50));
            // bq769x2_subcmd_cmd_only(BQ769X2_SUBCMD_SWAP_COMM_MODE);

            // set_bmb_address(i2c_addr_config >> 1);

            // uint8_t read_i2c_addr = 0;
            // bq769x2_subcmd_read_u1(BQ769X2_SET_CONF_I2C_ADDR, &read_i2c_addr);
            // printf("Reading I2C address: %x\n", read_i2c_addr);

            // bq769x2_config_update_mode(false);

            // // Doubled Check the Data Memory registers

            // // REG0
            // uint8_t read_reg0_config = 0;
            // bq769x2_subcmd_read_u1(BQ769X2_SET_CONF_REG0, &read_reg0_config);

            // // REG12
            // uint8_t read_reg12_config = 0;
            // bq769x2_subcmd_read_u1(BQ769X2_SET_CONF_REG12, &read_reg12_config);

            // if ((reg0_config == read_reg0_config) &&
            //     (reg12_config == read_reg12_config))
            // {
            //     printf("\tReg0 and Reg12 are correct!\n");
            // }

            // read_i2c_addr = 0;
            // bq769x2_subcmd_read_u1(BQ769X2_SET_CONF_I2C_ADDR, &read_i2c_addr);
            // printf("Reading I2C address: %x\n", read_i2c_addr);

            // if (read_i2c_addr == i2c_addr_config)
            // {
            //     printf("\tI2C address properly set!\n");
            // }

            // bq769x2_config_update_mode(true);

            // // Check the OTPB bit is not set in battery status

            // bq769x2_read_bytes(BQ769X2_CMD_BATTERY_STATUS + 1, &value, 1);
            // uint8_t otpb_bit_set = value & 0b10000000; // otp block bit
            // if (!otpb_bit_set)
            // {
            //     printf("OTPB BIT Clear! (%x)\n", value);
            // }

            // // Read OTP_WR_CHECK(), and if 0x80 is read then it's ready to OTP
            // uint8_t otp_wr_check = 0;
            // bq769x2_subcmd_read_u1(BQ769X2_SUBCMD_OTP_WR_CHECK, &otp_wr_check);
            // printf("OPT_WR_CHECK [%x]\n", otp_wr_check);
            // if (otp_wr_check == 0x80) // (0x80) ready for OTP write
            // {
            //     printf("\tREADY FOR OTP WR!!!\n");
            // }

            // // send OTP_WRITE()
            // bq769x2_subcmd_cmd_only(BQ769X2_SUBCMD_OTP_WRITE);

            // vTaskDelay(pdMS_TO_TICKS(100));
            // // Read from 0x40, and is 0x80 is read then the OTP was successfull
            // uint8_t otp_write_result = 0;
            // bq769x2_subcmd_read_u1(BQ769X2_SUBCMD_DATA_START, &otp_write_result);
            // if (otp_write_result == 0x80)
            // {
            //     printf("OTP Written correctly !!\n");
            // }

            // bq769x2_config_update_mode(false);


            break;

        case 0x02:
            // Currently not handeling UNSEALED mode
            printf("Security status: UNSEALED\n");
            break;

        case 0x03:
            // Currently not handeling SEALED mode
            printf("Security status: SEALED\n");

            // // bq769x2_config_update_mode(true);

            // uint8_t keys[8] = { 0 };
            // keys[0] = rx_data[1];
            // keys[1] = rx_data[0];

            // bq769x2_write_bytes(BQ769X2_CMD_SUBCMD_LOWER, keys, 2);
            // vTaskDelay(pdMS_TO_TICKS(50));
            // // bq769x2_write_bytes(BQ769X2_CMD_SUBCMD_LOWER + 1, &keys[1], 1);
            // // vTaskDelay(pdMS_TO_TICKS(50));
            // printf("keys (0 | 1) %x | %x\n", keys[0], keys[1]);

            // keys[0] = rx_data[3];
            // keys[1] = rx_data[2];
            // bq769x2_write_bytes(BQ769X2_CMD_SUBCMD_LOWER, keys, 2);
            // vTaskDelay(pdMS_TO_TICKS(50));
            // // bq769x2_write_bytes(BQ769X2_CMD_SUBCMD_LOWER + 1, &keys[1], 1);
            // // vTaskDelay(pdMS_TO_TICKS(50));
            // printf("keys (0 | 1) %x | %x\n", keys[0], keys[1]);
            break;

        case 0x00:
            // invalid
            printf("Security status: INVALID\n");
            break;
        default:
            break;
    }

    vTaskDelay(pdMS_TO_TICKS(10));
    printf("\n");

    // bms_read_voltages(&bmbs[0]);
    // bms_print_registers();
    // bms_update_error_flags(&bmbs[0]);
}

// do what's right | made with <3 at Cooper Union
