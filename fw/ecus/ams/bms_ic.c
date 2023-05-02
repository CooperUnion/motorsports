#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

// #include "esp_log.h"
#include "driver/spi_master.h"

#include "bms_ic.h"
#include "bq769x2.h"
#include "freertos/task.h"

#define AMS_PIN_MISO 37
#define AMS_PIN_MOSI 35
#define AMS_PIN_SCLK 36
#define AMS_PIN_CS   33

#define SPI_RETRIES 4       // Number of retries for the spi transactions

#define SPI_RW_BIT            7
#define SET_BIT(num, index)   ((num) | (1U << (index)))
#define CLEAR_BIT(num, index) ((num) & ~(1U << (index)))

// Need:
// subcommand read/write1/write2
// direct command
// read register
// write register

spi_device_handle_t spi;

static uint8_t CRC8(uint8_t *data, uint8_t len);

static uint8_t Checksum(uint8_t *ptr, uint8_t len);

static void bms_ic_print_msg(spi_transaction_t msg, esp_err_t err);

static void send_checksum(spi_transaction_t msg, uint8_t *checksum_buff, uint8_t length)
{

}

// read subcommand
// Write bytes of data starting at addr
// each transaction only sends 1 byte of data
static void spi_write(uint16_t addr, uint8_t *data, uint8_t w_bytes)
{
    uint8_t tx_buffer[4] = { 0 };
    uint8_t rx_buffer[4] = { 0 };

    spi_transaction_t msg = {
        .length = 24,
        .tx_buffer = tx_buffer,
        .rx_buffer = rx_buffer,
    };

    // send data over spi
    for (size_t i = 0; i < w_bytes; i++)
    {
        tx_buffer[0] = (addr + i) | 0x80;
        tx_buffer[1] = data[i];
        tx_buffer[2] = CRC8(tx_buffer, 2);

        // send message
        esp_err_t err;
        err = spi_device_polling_transmit(spi, &msg);
        bms_ic_print_msg(msg, err);
        vTaskDelay(1);

        bool message_reflected = false;
        // Resend message until message is reflected on MISO
        for (size_t j = 0; (j < SPI_RETRIES) && !message_reflected;  j++)
        {
            if (!memcmp(tx_buffer, rx_buffer, (msg.length / 8)))
            {
                err = spi_device_polling_transmit(spi, &msg);
                // bms_ic_print_msg(msg, err);
                vTaskDelay(1);
            }
            else
            {
                message_reflected = true;
            }
        }
    }
}

static void spi_read(uint16_t addr, uint8_t *rx_data, uint8_t r_bytes)
{
    uint8_t tx_buffer[4] = { 0 };
    uint8_t rx_buffer[4] = { 0 };

    spi_transaction_t msg = {
        .length = 24,
        .tx_buffer = tx_buffer,
        .rx_buffer = rx_buffer,
    };

    // Read date from spi
    for (size_t i = 0; i < r_bytes; i++)
    {
        tx_buffer[0] = (addr + i);
        tx_buffer[1] = 0xFF;
        tx_buffer[2] = CRC8(tx_buffer, 2);

        // send message
        esp_err_t err;
        err = spi_device_polling_transmit(spi, &msg);
        bms_ic_print_msg(msg, err);
        vTaskDelay(1);

        bool message_reflected = false;
        // Resend message until message is reflected on MISO
        for (size_t j = 0; (j < SPI_RETRIES) && !message_reflected;  j++)
        {
            // Since byte is being read, the data byte doesn't matter
            if (tx_buffer[0] != rx_buffer[0])
            {
                err = spi_device_polling_transmit(spi, &msg);
                printf("spi_read\n");
                bms_ic_print_msg(msg, err);
                vTaskDelay(1);
            }
            else
            {
                message_reflected = true;
                rx_data[i] = rx_buffer[1];
            }
        }
    }
}

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

static void bms_ic_print_msg(spi_transaction_t msg, esp_err_t err)
{
    uint8_t *tx_buffer = (uint8_t *)msg.tx_buffer;
    uint8_t *rx_buffer = (uint8_t *)msg.rx_buffer;

    if (err == ESP_OK)
    {
        printf("Transaction\n");
        printf("\ttx: ");
        for (int i = 0; i < (msg.length / 8); i++)
        {
            printf("%x ", tx_buffer[i]);
        }
        putchar('\n');

        printf("\trx: ");
        for (int i = 0; i < (msg.length / 8); i++)
        {
            printf("%x ", rx_buffer[i]);
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
}

// Reg1: 5V (for LED)
// the preregulator (referred to as reg0) needs to be configured as well as reg1/2_EN
void bms_ic_config_reg0()
{
    // reg0 enable
    uint8_t tx_buffer[4] = { 0 };
    uint8_t rx_buffer[4] = { 0 };

    spi_transaction_t msg = {
        .length = 24, // 16
        .tx_buffer = tx_buffer,
        .rx_buffer = rx_buffer,
    };
    esp_err_t err;

    // SET REGISTER
    uint8_t reg0_config_reg = 0x01;

    uint8_t tx_data[4] = { 0 };
    tx_data[0] = REG0Config & 0x00FF;
    tx_data[1] = (REG0Config & 0xFF00) >> 8;
    tx_data[2] = reg0_config_reg;

    spi_write(0x3E, tx_data, 3);

    // -------------------------------------------------------
    // checksum includes: register address in little endian, and the buffer data (also in little endian form)
    uint8_t checksum_buff[3] = { 0 };
    checksum_buff[0] = REG0Config & 0x00FF;
    checksum_buff[1] = (REG0Config & 0xFF00) >> 8;
    checksum_buff[2] = reg0_config_reg; // the data

    tx_data[0] = Checksum(checksum_buff, 3);
    tx_data[1] = 0x05;
    spi_write(0x60, tx_data, 2);
}

void bms_ic_config_reg12()
{
    // REG12_CONTROL
    // // 0x7 for 5V

    uint8_t tx_buffer[4] = { 0 };
    uint8_t rx_buffer[4] = { 0 };

    spi_transaction_t msg = {
        .length = 24, // 16
        .tx_buffer = tx_buffer,
        .rx_buffer = rx_buffer,
        // .flags = (/* SPI_TRANS_USE_TXDATA | */ SPI_TRANS_USE_RXDATA),
    };
    esp_err_t err;

    // SET REGISTER
    uint8_t reg12_config_reg = 0b00001111;

    uint8_t tx_data[4] = { 0 };
    tx_data[0] = REG12_CONTROL & 0x00FF;
    tx_data[1] = (REG12_CONTROL & 0xFF00) >> 8;
    tx_data[2] = reg12_config_reg;

    spi_write(0x3E, tx_data, 3);

    uint8_t checksum_buff[3] = { 0 };
    checksum_buff[0] = REG12_CONTROL & 0x00FF;
    checksum_buff[1] = (REG12_CONTROL & 0xFF00) >> 8;
    checksum_buff[2] = reg12_config_reg; // the data

    tx_data[0] = Checksum(checksum_buff, 3);
    tx_data[1] = 0x05;
    spi_write(0x60, tx_data, 2);
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

    uint8_t tx_data[4] = { 0 };
    tx_data[0] = DEVICE_NUMBER & 0x00FF;
    tx_data[1] = (DEVICE_NUMBER & 0xFF00) >> 8;

    spi_write(0x3E, tx_data, 2);

    uint8_t rx_data[4] = { 0 };
    spi_read(0x40, rx_data, 2);

    uint16_t device_number = 0;
    device_number = (rx_data[0] + (rx_data[1] << 8));

    return device_number;
}
