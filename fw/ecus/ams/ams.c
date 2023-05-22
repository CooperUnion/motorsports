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

unsigned char CRC8(unsigned char *ptr, unsigned char len)
//Calculates CRC8 for passed bytes. Used in i2c read and write functions
{
	unsigned char i;
	unsigned char crc=0;
	while(len--!=0)
	{
		for(i=0x80; i!=0; i/=2)
		{
			if((crc & 0x80) != 0)
			{
				crc *= 2;
				crc ^= 0x107;
			}
			else
				crc *= 2;

			if((*ptr & i)!=0)
				crc ^= 0x107;
		}
		ptr++;
	}
	return(crc);
}

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

    // printf("starting bmb monitor task...\n");
    // static TaskHandle_t bmb_monitor_handle;
    // xTaskCreatePinnedToCore(bmb_monitor_task, "BMB_MONITOR", 8192, 0, 3, &bmb_monitor_handle, 0);





    // Get to FULLACCESS MODE by first transitioning
    // from SEALED to UNSEALED MODE then to FULLACCESS MODE
    //
    // perform the transition by reading the security keys


    // bms_print_register(BQ769X2_CMD_BATTERY_STATUS);

    vTaskDelay(pdMS_TO_TICKS(500));


    bq769x2_config_update_mode(false);

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
    // static uint16_t count = 0;
    // if (count % 2)
    // {
    //     bq769x2_config_update_mode(true);
    // }
    // else
    // {
    //     bq769x2_config_update_mode(false);
    // }
    // count++;


    // bq769x2_config_update_mode(false);

    printf("Battery Status: \n");
    bms_print_register(BQ769X2_CMD_BATTERY_STATUS);
    vTaskDelay(pdMS_TO_TICKS(10));
    bms_print_register(BQ769X2_CMD_BATTERY_STATUS + 1);
    vTaskDelay(pdMS_TO_TICKS(10));

    uint8_t value;
    uint8_t battery_security_status = 0;
    bq769x2_read_bytes(BQ769X2_CMD_BATTERY_STATUS, &value, 1);
    if (value & 0b10000000)
    {
        battery_security_status += 0x01;
    }

    vTaskDelay(pdMS_TO_TICKS(10));

    bq769x2_read_bytes(BQ769X2_CMD_BATTERY_STATUS + 1, &value, 1);
    if (value & 0b00000001)
    {
        battery_security_status += 0x02;
    }

    // READ security keys:
    bq769x2_subcmd_cmd_only(BQ769X2_SUBCMD_SECURITY_KEYS);

    uint8_t rx_data[8] = { 0 };
    bq769x2_read_bytes(BQ769X2_SUBCMD_DATA_START, rx_data, 8);

    printf("Security Keys: ");
    for (size_t i = 0; i < 8; i++)
    {
        printf("%x ", rx_data[i]);
    }
    printf("\n");

// // 0x925B
//     uint32_t u32;
//     int err = bq769x2_subcmd_read(0x9257, &u32, 1);
//     if (err)
//     {
//         printf("error reading datamem\n");
//     }
//     else
//     {
//         printf("u32: %lx\n", u32);
//     }




    switch (battery_security_status)
    {
        case 0x01:
            printf("Security status: FULLACCESS\n");
            break;

        case 0x02:
            // while (1)
            {
                printf("Security status: UNSEALED\n");
                vTaskDelay(pdMS_TO_TICKS(500));
                printf("Security status: UNSEALED\n");
                vTaskDelay(pdMS_TO_TICKS(500));
                printf("Security status: UNSEALED\n");
                vTaskDelay(pdMS_TO_TICKS(500));
                printf("Security status: UNSEALED\n");
                vTaskDelay(pdMS_TO_TICKS(500));
            }
            break;

        case 0x03:
            printf("Security status: SEALED\n");

            // bq769x2_config_update_mode(true);

            uint8_t keys[8] = { 0 };
            keys[0] = rx_data[0];
            keys[1] = rx_data[1];

            bq769x2_write_bytes(BQ769X2_CMD_SUBCMD_LOWER, keys, 2);
            vTaskDelay(pdMS_TO_TICKS(50));
            // bq769x2_write_bytes(BQ769X2_CMD_SUBCMD_LOWER + 1, &keys[1], 1);
            // vTaskDelay(pdMS_TO_TICKS(50));
            printf("keys (0 | 1) %x | %x\n", keys[0], keys[1]);

            keys[0] = rx_data[2];
            keys[1] = rx_data[3];
            bq769x2_write_bytes(BQ769X2_CMD_SUBCMD_LOWER, keys, 2);
            vTaskDelay(pdMS_TO_TICKS(50));
            // bq769x2_write_bytes(BQ769X2_CMD_SUBCMD_LOWER + 1, &keys[1], 1);
            // vTaskDelay(pdMS_TO_TICKS(50));
            printf("keys (0 | 1) %x | %x\n", keys[0], keys[1]);

            // bq769x2_config_update_mode(true);

            // bq769x2_subcmd_cmd_only(BQ769X2_SUBCMD_SECURITY_KEYS);
            // uint8_t keys[8] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE };

            // bq769x2_write_bytes(BQ769X2_SUBCMD_DATA_START, keys, 8);

            // uint8_t checksum_data[10] = { 0x35, 0x00, 0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE };

            // uint8_t checksum = CRC8(checksum_data, 10);
            // bq769x2_write_bytes(BQ769X2_SUBCMD_DATA_CHECKSUM, &checksum, 1);

            // uint8_t len = 0x0C;
            // bq769x2_write_bytes(BQ769X2_SUBCMD_DATA_LENGTH, &len, 1);


            // Test out writing the keys
            // tx_data[0] = BQ769X2_SUBCMD_SECURITY_KEYS & 0xFF;
            // bq769x2_write_bytes(BQ769X2_CMD_SUBCMD_LOWER, tx_data, 1);

            // vTaskDelay(pdMS_TO_TICKS(10));

            // tx_data[0] = (BQ769X2_SUBCMD_SECURITY_KEYS >> 8) & 0xFF;
            // bq769x2_write_bytes(BQ769X2_CMD_SUBCMD_UPPER, tx_data, 1);



            // tx_data[0] = BQ769X2_SUBCMD_SECURITY_KEYS & 0xFF;
            // bq769x2_write_bytes(BQ769X2_CMD_SUBCMD_LOWER, tx_data, 1);

            // vTaskDelay(pdMS_TO_TICKS(10));

            // tx_data[0] = (BQ769X2_SUBCMD_SECURITY_KEYS >> 8) & 0xFF;
            // bq769x2_write_bytes(BQ769X2_CMD_SUBCMD_UPPER, tx_data, 1);

            // uint8_t keys[8] = { 0 };
            // keys[0] = rx_data[1];
            // keys[1] = rx_data[0];
            // keys[2] = rx_data[3];
            // keys[3] = rx_data[2];

            // keys[4] = rx_data[5];
            // keys[5] = rx_data[4];
            // keys[6] = rx_data[7];
            // keys[7] = rx_data[6];

            // printf("Sending Keys: ");
            // for (size_t i = 0; i < 8; i++)
            // {
            //     printf("%x ", keys[i]);
            // }
            // printf("\n");

            // bq769x2_write_bytes(0x40, keys, 8);

            // vTaskDelay(pdMS_TO_TICKS(10));
            break;

        case 0x00:
            // invalid
            printf("Security status: INVALID\n");
            break;
        default:
            break;
    }

    vTaskDelay(pdMS_TO_TICKS(10));

    // bms_read_voltages(&bmbs[0]);
    // bms_print_registers();
    // bms_update_error_flags(&bmbs[0]);
}

// do what's right | made with <3 at Cooper Union
