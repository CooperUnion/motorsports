// Cooper Motorsports Precharge Controller

#include <stdint.h>

// temp to make intellisense work
#define CONFIG_IDF_TARGET_ESP32S3 1
#define SOC_ADC_DMA_SUPPORTED 1
#define ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED 1

#include <sdkconfig.h>

#include <soc/soc_caps.h>

#include <esp_adc/adc_continuous.h>
#include <esp_adc/adc_cali.h>
#include <esp_adc/adc_cali_scheme.h>
#include <esp_attr.h>
#include <driver/gpio.h>
#include <esp_timer.h>
#include <FreeRTOS/FreeRTOS.h>
#include <FreeRTOS/task.h>
#include <hal/adc_types.h>

#include <ember_taskglue.h>
#include <node_pins.h>
#include <opencan_tx.h>

#include "precharge_pins.h"

// ######   DEFINES & TYPES     ###### //

#define atomic _Atomic

typedef enum {
    PCH_STATE_INIT,
    PCH_STATE_IDLE,
} pch_state_E;

// ######      PROTOTYPES       ###### //

static inline uint32_t current_time(void);
static void set_state(pch_state_E new_state);

static bool adc_callback(
    adc_continuous_handle_t handle,
    const adc_continuous_evt_data_t *edata,
    void *user_data);

// ######     PRIVATE DATA      ###### //

static struct {
    atomic pch_state_E state;
    atomic uint32_t state_start_time;

    adc_continuous_handle_t adc1_handle;
    adc_cali_handle_t adc1_cali_handle;
    atomic volatile uint32_t last_adc_val_raw;

    atomic float hvdc_voltage;
} glo = {
    .state = PCH_STATE_INIT,
    .state_start_time = 0U,

    .adc1_handle = NULL,
    .adc1_cali_handle = NULL,
    .last_adc_val_raw = 0U,

    .hvdc_voltage = 0U,
};

// ######          CAN          ###### //

void CANTX_populate_PCH_Status(struct CAN_Message_PCH_Status * const m) {
    static uint8_t counter;

    m->PCH_counter = counter++;
    m->PCH_dcBusVoltage = glo.hvdc_voltage;
}

// ######    RATE FUNCTIONS     ###### //

static void pch_init(void);
static void pch_1kHz(void);
static void pch_10Hz(void);

const ember_rate_funcs_S module_rf = {
    .call_init = pch_init,
    .call_1kHz = pch_1kHz,
    .call_10Hz = pch_10Hz,
};


static void pch_init(void) {
    // configure relay/AIR control pins
    gpio_config(&(gpio_config_t){
        .pin_bit_mask =
            BIT64(PRECHARGE_PIN_AIR_NEG_CTRL) |
            BIT64(PRECHARGE_PIN_AIR_POS_CTRL) |
            BIT64(PRECHARGE_PIN_PRECH_RELAY_CTRL),
        .mode = GPIO_MODE_OUTPUT,
    });

    gpio_set_level(PRECHARGE_PIN_AIR_NEG_CTRL, 0);
    gpio_set_level(PRECHARGE_PIN_AIR_POS_CTRL, 0);
    gpio_set_level(PRECHARGE_PIN_PRECH_RELAY_CTRL, 0);


    // configure LEDs
    gpio_config(&(gpio_config_t){
        .pin_bit_mask = BIT64(PRECHARGE_PIN_LED_D7) | BIT64(PRECHARGE_PIN_LED_D8) |
                        BIT64(NODE_BOARD_PIN_LED1) | BIT64(NODE_BOARD_PIN_LED2),
        .mode = GPIO_MODE_OUTPUT,
    });

    // configure VDC isoamp input ADC
    ESP_ERROR_CHECK(adc_cali_create_scheme_curve_fitting(&(adc_cali_curve_fitting_config_t){
        .unit_id = ADC_UNIT_1,
        .atten = ADC_ATTEN_DB_11,
        .bitwidth = ADC_BITWIDTH_12,
    }, &glo.adc1_cali_handle));

    ESP_ERROR_CHECK(adc_continuous_new_handle(&(adc_continuous_handle_cfg_t){
        .max_store_buf_size = 800,
        .conv_frame_size = 4 * 40,  // 4 bytes per conversion, 40 sample-wide frame
    }, &glo.adc1_handle));

    ESP_ERROR_CHECK(adc_continuous_config(glo.adc1_handle, &(adc_continuous_config_t){
        .pattern_num = 1,  // one channel
        .adc_pattern = &(adc_digi_pattern_config_t){
            .atten = ADC_ATTEN_DB_11,
            .channel = ADC_CHANNEL_5,
            .unit = ADC_UNIT_1,
            .bit_width = 12,
        },
        .sample_freq_hz = 80000,
        .conv_mode = ADC_CONV_SINGLE_UNIT_1,
        .format = ADC_DIGI_OUTPUT_FORMAT_TYPE2,
    }));

    ESP_ERROR_CHECK(adc_continuous_register_event_callbacks(glo.adc1_handle, &(adc_continuous_evt_cbs_t){
        .on_conv_done = adc_callback,
    }, NULL));

    adc_continuous_start(glo.adc1_handle);
}

static void pch_10Hz(void) {
    static bool led_state;
    gpio_set_level(NODE_BOARD_PIN_LED1, led_state);
    led_state = !led_state;
}

static void pch_1kHz(void) {
    // convert ADC value
    int calibrated_adc_mv = 0;
    ESP_ERROR_CHECK(adc_cali_raw_to_voltage(glo.adc1_cali_handle, glo.last_adc_val_raw, &calibrated_adc_mv));

    glo.hvdc_voltage = PRECHARGE_ADC_GAIN_ISOAMP_MV_TO_V * calibrated_adc_mv;
}

// ######   PRIVATE FUNCTIONS   ###### //

static void set_state(pch_state_E new_state) {
    glo.state_start_time = current_time();
    glo.state = new_state;
}

static inline uint32_t current_time(void) {
    return esp_timer_get_time() / (int64_t)1000;
}

static uint32_t time_in_state(void) {
    return current_time() - glo.state_start_time;
}

static IRAM_ATTR bool adc_callback(
    adc_continuous_handle_t handle,
    const adc_continuous_evt_data_t *edata,
    void *user_data)
{
    (void)handle;
    (void)user_data;

    const adc_digi_output_data_t *data = (const adc_digi_output_data_t*)&(edata->conv_frame_buffer[0]);
    glo.last_adc_val_raw = data->type2.data;

    return false;
}

// ######   PUBLIC FUNCTIONS    ###### //

// do what's right | made with <3 at Cooper Union
