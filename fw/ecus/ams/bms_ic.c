#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

// #include "esp_log.h"
#include "driver/spi_master.h"
#include "driver/i2c.h"
#include "driver/gpio.h"

#include "bms_ic.h"
#include "bq769x2.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define AMS_PIN_MISO 37
#define AMS_PIN_MOSI 35
#define AMS_PIN_SCLK 36
#define AMS_PIN_CS   33

#define AMS_PIN_SDA 26
#define AMS_PIN_SCL 21

#define BMS_IC_I2C_HOST I2C_NUM_0

#define SPI_RETRIES 10       // Number of retries for the spi transactions

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
        vTaskDelay(2);

        bool message_reflected = false;
        // Resend message until message is reflected on MISO
        for (size_t j = 0; (j < SPI_RETRIES) && !message_reflected;  j++)
        {
            if (!memcmp(tx_buffer, rx_buffer, (msg.length / 8)))
            {
                err = spi_device_polling_transmit(spi, &msg);
                bms_ic_print_msg(msg, err);
                vTaskDelay(2);
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
        vTaskDelay(2);

        bool message_reflected = false;
        // Resend message until message is reflected on MISO
        for (size_t j = 0; (j < SPI_RETRIES) && !message_reflected;  j++)
        {
            // Since byte is being read, the data byte doesn't matter
            if (tx_buffer[0] != rx_buffer[0])
            {
                err = spi_device_polling_transmit(spi, &msg);
                // printf("spi_read\n");
                bms_ic_print_msg(msg, err);
                vTaskDelay(2);
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

    // Configure I2C

    i2c_config_t conf = {
        .mode             = I2C_MODE_MASTER,
        .sda_io_num       = AMS_PIN_SDA,
        .scl_io_num       = AMS_PIN_SCL,
        .sda_pullup_en    = GPIO_PULLUP_DISABLE,
        .scl_pullup_en    = GPIO_PULLUP_DISABLE,
        .master.clk_speed = 1000, // 50kHz
    };


    ESP_ERROR_CHECK(i2c_param_config(BMS_IC_I2C_HOST, &conf));

    uint32_t interrupt_flags = 0;

    i2c_driver_install(
        BMS_IC_I2C_HOST,
        I2C_MODE_MASTER,
        0, // slv_rx/tx buffer length is ignored in master mode
        0, // slv_rx/tx buffer length is ignored in master mode
        interrupt_flags
    );
    // ESP_ERROR_CHECK(i2c_set_timeout(BMS_IC_I2C_HOST, 50000));
}

// the preregulator (referred to as reg0) needs to be configured before reg12
void bms_ic_spi_config_reg0()
{
    // reg0 enable

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
    tx_data[1] = 0x05;  // length of data written
    spi_write(0x60, tx_data, 2);
}

void bms_ic_spi_config_reg12()
{
    // SET REGISTER
    uint8_t reg12_config_reg;
    // reg12_config_reg = 0b00000000;
    reg12_config_reg = 0b00001111;

    // Blink LED
    // static uint32_t counter = 0;
    // if (counter % 2)
    // {
    //     reg12_config_reg = 0b00001111;
    // }
    // else
    // {
    //     reg12_config_reg = 0b00000001;
    // }
    // counter++;

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

uint16_t bms_ic_spi_device_number()
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

// void bms_ic_set_i2c_addr(uint16_t i2c_addr)
// {
//     // #define I2CAddress 0x923A      //Settings:Configuration:I2C Address


// }


// populate cell voltages
void bms_ic_cell_voltages(uint32_t *cell_voltages)
{

}


void bms_ic_swap_to_i2c()
{
    // Command only subcommand
    // #define SWAP_TO_I2C 0x29E7

    uint8_t tx_data[4] = { 0 };
    tx_data[0] = SWAP_TO_I2C & 0x00FF;
    tx_data[1] = (SWAP_TO_I2C & 0xFF00) >> 8;

    spi_write(0x3E, tx_data, 2);
}


// void bms_ic_test_i2c()
// {
//     uint8_t buf[4] = {'D', 'E', 'A', 'D'};
//     i2c_cmd_handle_t cmd_handle = i2c_cmd_link_create();
//     i2c_master_start(cmd_handle);
//     i2c_master_write_byte(cmd_handle, ((0x10 << 1) | I2C_MASTER_WRITE), true);
//     i2c_master_write(cmd_handle, buf, sizeof(buf), true);
//     i2c_master_stop(cmd_handle);

//     const uint8_t timeout_ticks = 1;
//     i2c_master_cmd_begin(BMS_IC_I2C_HOST, cmd_handle, timeout_ticks);
//     i2c_cmd_link_delete(cmd_handle);
// }

static void i2c_write(uint16_t addr, uint8_t *data, uint8_t length)
{
    // uint8_t tx_data[4] = { 0 };

    i2c_cmd_handle_t cmd_handle = i2c_cmd_link_create();
    ESP_ERROR_CHECK(i2c_master_start(cmd_handle));
    ESP_ERROR_CHECK(i2c_master_write_byte(cmd_handle, addr, false));

    ESP_ERROR_CHECK(i2c_master_write(cmd_handle, data, length, true));
    ESP_ERROR_CHECK(i2c_master_stop(cmd_handle));

    const uint32_t timeout_ticks = 10000;
    ESP_ERROR_CHECK(i2c_master_cmd_begin(BMS_IC_I2C_HOST, cmd_handle, timeout_ticks));
    i2c_cmd_link_delete(cmd_handle);

    printf("i2c tx: ");
    printf("%x ", addr);
    for (int i = 0; i < length; i++)
    {
        printf("%x ", data[i]);
    }
    putchar('\n');
}

static void i2c_read(uint16_t addr, uint8_t *rx_buff, uint8_t length)
{
    (void)addr;
    // uint8_t tx_data[4];

    i2c_cmd_handle_t cmd_handle = i2c_cmd_link_create();
    ESP_ERROR_CHECK(i2c_master_start(cmd_handle));
    ESP_ERROR_CHECK(i2c_master_write_byte(cmd_handle, (0x10), false));
    ESP_ERROR_CHECK(i2c_master_write_byte(cmd_handle, 0x40, true));
    ESP_ERROR_CHECK(i2c_master_write_byte(cmd_handle, (0x11), false));
    ESP_ERROR_CHECK(i2c_master_read(cmd_handle, rx_buff, 3, I2C_MASTER_ACK));
    ESP_ERROR_CHECK(i2c_master_stop(cmd_handle));

    // uint8_t crc_data[4] = { 0x10, 0x40, 0x11, 0x00};

    // uint8_t data[4] = { 0 };
    // data[0] = 0x40;
    // data[1] = (0x10 | I2C_MASTER_READ);
    // data[2] = CRC8(crc_data, 3);
    // i2c_master_write(cmd_handle, data, 3, true);


    // i2c_master_read(cmd_handle, rx_buff, length, I2C_MASTER_ACK);
    // i2c_master_stop(cmd_handle);


    const uint8_t timeout_ticks = 100;

    ESP_ERROR_CHECK(i2c_master_cmd_begin(BMS_IC_I2C_HOST, cmd_handle, timeout_ticks));

    // for (int i = 0; i < 2 && (rx_buff[0] == 0xFF); i++)
    // {
    //     ESP_ERROR_CHECK(i2c_master_cmd_begin(BMS_IC_I2C_HOST, cmd_handle, timeout_ticks));
    //     vTaskDelay(2);

    //     printf("i2c rx: ");
    //     for (int i = 0; i < sizeof(rx_buff); i++)
    //     {
    //         printf("%x ", rx_buff[i]);
    //     }
    //     putchar('\n');
    // }

    i2c_cmd_link_delete(cmd_handle);
}


uint16_t bms_ic_i2c_device_number()
{
    uint8_t tx_data[4] = { 0 };
    tx_data[0] = 0x3E;
    tx_data[1] = DEVICE_NUMBER & 0x00FF;
    tx_data[2] = (DEVICE_NUMBER & 0xFF00) >> 8;
    tx_data[3] = CRC8(tx_data, 3);


    i2c_write(0x10, tx_data, 3);


    vTaskDelay(100);
    // vTaskDelay(2);

    uint8_t rx_buff[4] = { 0 };

    gpio_set_level(48, 1); // GPIO_TEST_I2C
    i2c_read(0x40, rx_buff, 3);
    gpio_set_level(48, 0); // GPIO_TEST_I2C
    // printf("i2c rx: ");
    // for (int i = 0; i < sizeof(rx_buff); i++)
    // {
    //     printf("%x ", rx_buff[i]);
    // }
    // putchar('\n');

    return 0x0000;
}
