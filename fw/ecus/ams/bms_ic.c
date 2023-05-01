#include <stddef.h>
#include <stdio.h>
#include <string.h>

// #include "esp_log.h"
#include "driver/spi_master.h"

#include "bms_ic.h"
#include "bq769x2.h"
#include "freertos/task.h"

#define AMS_PIN_MISO 37
#define AMS_PIN_MOSI 35
#define AMS_PIN_SCLK 36
#define AMS_PIN_CS   33

// Need:
// subcommand read/write1/write2
// direct command
// read register
// write register

static uint8_t CRC8(uint8_t *data, uint8_t len)
//Calculates CRC8 for passed bytes. Used in i2c read and write functions
{
	uint8_t crc = 0;

	while(len != 0)
	{
		for(uint8_t i=0x80; i != 0; i /= 2)
		{
			if((crc & 0x80) != 0)
			{
				crc *= 2;
				crc ^= 0x107;
			}
			else
				crc *= 2;

			if((*data & i)!=0)
				crc ^= 0x107;
		}
		data++;
        len--;
	}

	return crc;
}

// Calculates the checksum when writing to a RAM register. The checksum is the inverse of the sum of the bytes.
// needed by Reg12_Control()
static uint8_t Checksum(uint8_t *ptr, uint8_t len)
{
	uint8_t checksum = 0;

	for(uint8_t i = 0; i < len; i++)
    {
		checksum += ptr[i];
    }

	checksum = 0xff & ~checksum;

	return checksum;
}

// Function called before the SPI transaction
static void spi_precallback()
{
    // printf("SPI PRECALLBACK\n");
}

spi_device_handle_t spi;
uint8_t rx_buffer[4] = { 0 };
uint8_t tx_buffer[10] = { 0 };

static void bms_ic_print_msg(spi_transaction_t msg, uint8_t *TX_Buffer, esp_err_t err)
{
    if (err == ESP_OK)
    {
        printf("Successul transaction\n");
        printf("\ttx: ");
        for (int i = 0; i < (msg.length / 8); i++)
        {
            printf("%x ", TX_Buffer[i]);
        }
        putchar('\n');

        printf("\trx: ");
        for (int i = 0; i < (msg.length / 8); i++)
        {
            printf("%x ", msg.rx_data[i]);
        }
        putchar('\n');

    }
    else
    {
        printf("err: something went wrong %d\n", err);
    }
}

void bms_ic_init()
{
    printf("initializing bms_ic\n   ");

    // set up SPI
    spi_bus_config_t spi_bus = {
        .miso_io_num = AMS_PIN_MISO,
        .mosi_io_num = AMS_PIN_MOSI,
        .sclk_io_num = AMS_PIN_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4096,

        // .flags = SPICOMMON_BUSFLAG_MASTER,

        // plus a lot of other data members
        // Setup interrrupts later
        // .isr_cpu_id = ,
        // .intr_flags = ,
    };

    spi_bus_initialize(SPI2_HOST, &spi_bus, SPI_DMA_DISABLED);

    spi_device_interface_config_t spi_device = {
        .mode = 0,
        .clock_speed_hz = 100000, // 100kHz
        .spics_io_num = AMS_PIN_CS,
        .queue_size = 6, // number of transactions to queue
        .pre_cb = spi_precallback,
        .flags = 0,
    };

    spi_bus_add_device(SPI2_HOST, &spi_device, &spi);

    char str[4] = "Hey";
    memcpy(rx_buffer, str, sizeof(str));

}

// Reg1: 5V (for LED)
// the preregulator (referred to as reg0) needs to be configured as well as reg1/2_EN
void bms_ic_config_reg12()
{
    // reg0 enable
	uint8_t TX_Buffer[4] = { 0 };

    spi_transaction_t msg = {
        .length = 24, // 16
        .tx_buffer = TX_Buffer,
        .flags = (/* SPI_TRANS_USE_TXDATA | */ SPI_TRANS_USE_RXDATA),
    };
    esp_err_t err;

    // SET REGISTER

    // First two write set the register address to write to:
    // -------------------------------------------------------
    TX_Buffer[0] = (0x3E | 0x80);
    TX_Buffer[1] = (REG0Config & 0x00FF);
    TX_Buffer[2] = CRC8(TX_Buffer, 2);

    memset(msg.rx_data, 0, 4);
    err = spi_device_polling_transmit(spi, &msg);
    bms_ic_print_msg(msg, TX_Buffer, err);
    vTaskDelay(2);

    // Repeat command to get the reflected message
    memset(msg.rx_data, 0, 4);
    err = spi_device_polling_transmit(spi, &msg);
    bms_ic_print_msg(msg, TX_Buffer, err);
    vTaskDelay(2);

    // -------------------------------------------------------

    TX_Buffer[0] = (0x3F | 0x80);
    TX_Buffer[1] = (REG0Config & 0xFF00) >> 8; // second byte
    TX_Buffer[2] = CRC8(TX_Buffer, 2);

    memset(msg.rx_data, 0, 4);
    err = spi_device_polling_transmit(spi, &msg);
    bms_ic_print_msg(msg, TX_Buffer, err);
    vTaskDelay(2);

    // Repeat command to get the reflected message
    memset(msg.rx_data, 0, 4);
    err = spi_device_polling_transmit(spi, &msg);
    bms_ic_print_msg(msg, TX_Buffer, err);
    vTaskDelay(2);

    // -------------------------------------------------------
    // write the data
    uint8_t reg0_config_reg = 0x01;

    TX_Buffer[0] = (0x40 | 0x80);
    TX_Buffer[1] = reg0_config_reg;
    TX_Buffer[2] = CRC8(TX_Buffer, 2);

    memset(msg.rx_data, 0, 4);
    err = spi_device_polling_transmit(spi, &msg);
    bms_ic_print_msg(msg, TX_Buffer, err);
    vTaskDelay(2);

    // Repeat command to get the reflected message
    memset(msg.rx_data, 0, 4);
    err = spi_device_polling_transmit(spi, &msg);
    bms_ic_print_msg(msg, TX_Buffer, err);
    vTaskDelay(2);

    // -------------------------------------------------------
    // checksum includes: register address in little endian, and the buffer data (also in little endian form)
    uint8_t checksum_buff[3] = { 0 };
    checksum_buff[0] = REG0Config & 0x00FF;
    checksum_buff[1] = (REG0Config & 0xFF00) >> 8;
    checksum_buff[2] = reg0_config_reg; // the data

    // write the checksum to 0x60
    TX_Buffer[0] = (0x60 | 0x80);
    TX_Buffer[1] = Checksum(checksum_buff, 3);
    TX_Buffer[2] = CRC8(TX_Buffer, 2);

    memset(msg.rx_data, 0, 4);
    err = spi_device_polling_transmit(spi, &msg);
    bms_ic_print_msg(msg, TX_Buffer, err);
    vTaskDelay(2);

    // Repeat command to get the reflected message
    memset(msg.rx_data, 0, 4);
    err = spi_device_polling_transmit(spi, &msg);
    bms_ic_print_msg(msg, TX_Buffer, err);
    vTaskDelay(2);

    // -------------------------------------------------------
    // write the combined legnth of everything to 0x61
    //  length includes data in 0x3E 0x3F, data written, and 0x60 0x61
    TX_Buffer[0] = (0x61 | 0x80);
    TX_Buffer[1] = 0x05;
    TX_Buffer[2] = CRC8(TX_Buffer, 2);

    memset(msg.rx_data, 0, 4);
    err = spi_device_polling_transmit(spi, &msg);
    bms_ic_print_msg(msg, TX_Buffer, err);
    vTaskDelay(2);

    // Repeat command to get the reflected message
    memset(msg.rx_data, 0, 4);
    err = spi_device_polling_transmit(spi, &msg);
    bms_ic_print_msg(msg, TX_Buffer, err);
    vTaskDelay(2);

    // ------------------------------------------------------------------------------
    // Subcommand to set Reg1 to 5V (only need to write)

    // REG12_CONTROL
    // // 0x7 for 5V

    TX_Buffer[0] = (0x3E | 0x80);
    TX_Buffer[1] = (REG12_CONTROL & 0x00FF);
    TX_Buffer[2] = CRC8(TX_Buffer, 2);

    memset(msg.rx_data, 0, 4);
    err = spi_device_polling_transmit(spi, &msg);
    bms_ic_print_msg(msg, TX_Buffer, err);
    vTaskDelay(2);

    // Repeat command to get the reflected message
    memset(msg.rx_data, 0, 4);
    err = spi_device_polling_transmit(spi, &msg);
    bms_ic_print_msg(msg, TX_Buffer, err);
    vTaskDelay(2);

    // -------------------------------------------------------

    TX_Buffer[0] = (0x3F | 0x80);
    TX_Buffer[1] = (REG12_CONTROL & 0xFF00) >> 8; // second byte
    TX_Buffer[2] = CRC8(TX_Buffer, 2);

    memset(msg.rx_data, 0, 4);
    err = spi_device_polling_transmit(spi, &msg);
    bms_ic_print_msg(msg, TX_Buffer, err);
    vTaskDelay(2);

    // Repeat command to get the reflected message
    memset(msg.rx_data, 0, 4);
    err = spi_device_polling_transmit(spi, &msg);
    bms_ic_print_msg(msg, TX_Buffer, err);
    vTaskDelay(2);

    // -------------------------------------------------------
    // -------------------------------------------------------

    // write the data
    uint8_t reg12_config_reg = 0b00001111;

    TX_Buffer[0] = (0x40 | 0x80);
    TX_Buffer[1] = reg12_config_reg; // REVIEW THIS !!!!!!
    TX_Buffer[2] = CRC8(TX_Buffer, 2);

    memset(msg.rx_data, 0, 4);
    err = spi_device_polling_transmit(spi, &msg);
    bms_ic_print_msg(msg, TX_Buffer, err);
    vTaskDelay(2);

    // Repeat command to get the reflected message
    memset(msg.rx_data, 0, 4);
    err = spi_device_polling_transmit(spi, &msg);
    bms_ic_print_msg(msg, TX_Buffer, err);
    vTaskDelay(2);

    // -------------------------------------------------------
    // checksum includes: register address in little endian, and the buffer data (also in little endian form)
    checksum_buff[0] = REG12_CONTROL & 0x00FF;
    checksum_buff[1] = (REG12_CONTROL & 0xFF00) >> 8;
    checksum_buff[2] = reg12_config_reg; // the data

    // write the checksum to 0x60
    TX_Buffer[0] = (0x60 | 0x80);
    TX_Buffer[1] = Checksum(checksum_buff, 3);
    TX_Buffer[2] = CRC8(TX_Buffer, 2);

    memset(msg.rx_data, 0, 4);
    err = spi_device_polling_transmit(spi, &msg);
    bms_ic_print_msg(msg, TX_Buffer, err);
    vTaskDelay(2);

    // Repeat command to get the reflected message
    memset(msg.rx_data, 0, 4);
    err = spi_device_polling_transmit(spi, &msg);
    bms_ic_print_msg(msg, TX_Buffer, err);
    vTaskDelay(2);

    // -------------------------------------------------------
    // write the combined legnth of everything to 0x61
    //  length includes data in 0x3E 0x3F, data written, and 0x60 0x61
    TX_Buffer[0] = (0x61 | 0x80);
    TX_Buffer[1] = 0x05;
    TX_Buffer[2] = CRC8(TX_Buffer, 2);

    memset(msg.rx_data, 0, 4);
    err = spi_device_polling_transmit(spi, &msg);
    bms_ic_print_msg(msg, TX_Buffer, err);
    vTaskDelay(2);

    // Repeat command to get the reflected message
    memset(msg.rx_data, 0, 4);
    err = spi_device_polling_transmit(spi, &msg);
    bms_ic_print_msg(msg, TX_Buffer, err);
    vTaskDelay(2);
}

uint16_t bms_ic_device_number()
{
    // to read from a subcommand (need to write the two byte subcommand over spi and read two bytes from the subcommand)
      // ** subcommand register address (0x3E 0x3F) (for reading 0x8E 0x8F) **
        // write to address 0x3E -> 0x8E (|'rd with 0x80 for reading  7-bit is r/w)
            // repeat until miso reflects command written
        // write to address 0x3F -> 0x8F (|'rd with 0x80 for reading  7-bit is r/w)
            // repeat until miso reflects command written

      // data buffer (0x40 0x41)
        // then read from address 0x40 -> 0x40 (r/w bit is 0)
            // repeat until MOSI reflects command back (0x40)

    // the polynomial is x^8 + x^2 + x + 1 (0x07)
    // crc calculation (crc is required to send or else MISO will output 0xFFFFAA!!!)

    // Read Device Number
    // uint8_t TX_Reg[4] = {0x00, 0x00, 0x00, 0x00};
	uint8_t TX_Buffer[4] = {0x00, 0x00};

    // uint16_t command = DEVICE_NUMBER;
	//TX_Reg in little endian format
	// TX_Reg[0] = command & 0xff;
	// TX_Reg[1] = (command >> 8) & 0xff;

    //
    // SPI_WriteReg(0x3E,TX_Reg,2);
    //
    // uint8_t addr = 0x80 | 0x3E;
    // // uint8_t rxdata [2];
    // uint32_t match = 0;
    // uint32_t retries = 10;

    printf("\nNew transaction\n");

    spi_transaction_t msg = {
        .length = 24, // 16
        .tx_buffer = TX_Buffer,
        .flags = (/* SPI_TRANS_USE_TXDATA | */ SPI_TRANS_USE_RXDATA),
    };

    esp_err_t err;

    // spi_device_polling_transmit(spi, &msg);


    do
    {
        TX_Buffer[0] = 0xBE;
        TX_Buffer[1] = 0x01;
        TX_Buffer[2] = CRC8(TX_Buffer, 2); // 0x9E

        memset(msg.rx_data, 0, 4);
        esp_err_t err = spi_device_polling_transmit(spi, &msg);
        vTaskDelay(2);

        bms_ic_print_msg(msg, TX_Buffer, err);
    } while (!memcmp(TX_Buffer, msg.rx_data, msg.length / 8));

    memset(msg.rx_data, 0, 4);
    printf("ARRAYS ARE THE SAME!!\n");

    do
    {
        TX_Buffer[0] = 0xBF;
        TX_Buffer[1] = 0x00;
        TX_Buffer[2] = CRC8(TX_Buffer, 2); // 0x8C

        memset(msg.rx_data, 0, 4);
        esp_err_t err = spi_device_polling_transmit(spi, &msg);
        vTaskDelay(2);

        bms_ic_print_msg(msg, TX_Buffer, err);

    }    while (!memcmp(TX_Buffer, msg.rx_data, msg.length / 8));

    printf("ARRAYS ARE THE SAME!!\n");

    // do the second half

    TX_Buffer[0] = 0x40;
    TX_Buffer[1] = 0xFF;
    TX_Buffer[2] = CRC8(TX_Buffer, 2); // 0xA8

    memset(msg.rx_data, 0, 4);
    err = spi_device_polling_transmit(spi, &msg);
    bms_ic_print_msg(msg, TX_Buffer, err);

    vTaskDelay(2);


    uint16_t device_number = 0;

    // repeat previous commmand
    memset(msg.rx_data, 0, 4);
    err = spi_device_polling_transmit(spi, &msg);
    if (msg.rx_data[0] == TX_Buffer[0])
    {
        bms_ic_print_msg(msg, TX_Buffer, err);
        device_number = msg.rx_data[1];
    }
    else
    {
        printf("Error %x not repeated\n", TX_Buffer[0]);
    }

    vTaskDelay(2);


    printf("Device Number: %x\n", device_number);

    // ---------

    TX_Buffer[0] = 0x41;
    TX_Buffer[1] = 0xFF;
    TX_Buffer[2] = CRC8(TX_Buffer, 2); // 0xBD

    memset(msg.rx_data, 0, 4);
    err = spi_device_polling_transmit(spi, &msg);
    bms_ic_print_msg(msg, TX_Buffer, err);

    memset(msg.rx_data, 0, 4);
    err = spi_device_polling_transmit(spi, &msg);
    if (msg.rx_data[0] == TX_Buffer[0])
    {
        bms_ic_print_msg(msg, TX_Buffer, err);
        device_number += (msg.rx_data[1] << 8);
    }
    else
    {
        printf("Error %x not repeated\n", TX_Buffer[0]);
    }

    return device_number;
}
