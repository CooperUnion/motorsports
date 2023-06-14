#include <stdint.h>

// temp to make intellisense work
#define CONFIG_IDF_TARGET_ESP32S3 1
#define SOC_ADC_DMA_SUPPORTED 1
#define ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED 1

#include <esp_adc/adc_continuous.h>
#include <esp_adc/adc_cali.h>
#include <esp_adc/adc_cali_scheme.h>
#include <esp_attr.h>
#include <esp_macros.h>

#include <opencan_tx.h>

#include "dashboard_pins.h"
#include "pedal_map.h"

// ######   DEFINES & TYPES     ###### //

#define atomic _Atomic
#define CLAMP(x, low, high) ((x) > (high) ? (high) : ((x) < (low) ? (low) : (x)))

// ######      PROTOTYPES       ###### //

static void configure_pedal_adc(void);

// ######     PRIVATE DATA      ###### //

static struct {
    adc_continuous_handle_t adc1_handle;
    adc_cali_handle_t adc1_cali_handle;

    atomic volatile uint32_t last_enc1_adc_val_raw;
    atomic volatile uint32_t last_enc2_adc_val_raw;
    atomic uint32_t last_enc1_adc_val_calibrated;
    atomic uint32_t last_enc2_adc_val_calibrated;

    atomic bool pedal_irrational;
    atomic float pedal_percentage;
    atomic float pedal_torque_request;
} glo = {
    .adc1_handle = NULL,
    .adc1_cali_handle = NULL,

    .last_enc1_adc_val_raw = 0,
    .last_enc2_adc_val_raw = 0,
    .last_enc1_adc_val_calibrated = 0,
    .last_enc2_adc_val_calibrated = 0,

    .pedal_irrational = true,
    .pedal_percentage = 0.0f,
    .pedal_torque_request = 0.0f,
};

// ######          CAN          ###### //

void CANTX_populate_VCU_Pedal(struct CAN_Message_VCU_Pedal * const m) {
    *m = (struct CAN_Message_VCU_Pedal){
        .VCU_pedalEnc1Mv = glo.last_enc1_adc_val_calibrated,
        .VCU_pedalEnc2Mv = glo.last_enc2_adc_val_calibrated,
        .VCU_pedalIrrational = glo.pedal_irrational,
        .VCU_pedalPercentage = glo.pedal_percentage,
        .VCU_pedalTorqueRequest = glo.pedal_torque_request,
    };
}

// ######    RATE FUNCTIONS     ###### //

void pedal_init(void) {
    configure_pedal_adc();
}

void pedal_1kHz(void) {
    int calibrated_enc1_mv = 0;
    int calibrated_enc2_mv = 0;
    ESP_ERROR_CHECK(adc_cali_raw_to_voltage(glo.adc1_cali_handle, glo.last_enc1_adc_val_raw, &calibrated_enc1_mv));
    ESP_ERROR_CHECK(adc_cali_raw_to_voltage(glo.adc1_cali_handle, glo.last_enc2_adc_val_raw, &calibrated_enc2_mv));

    glo.last_enc1_adc_val_calibrated = calibrated_enc1_mv;
    glo.last_enc2_adc_val_calibrated = calibrated_enc2_mv;

    // are the pedals rational?
    // todo: fill in values
    const float PEDAL_RATIONALITY_MAX_PERCENT_DIFF = 0.05f;

    const float ENC1_MIN_MV = 0.0f;
    const float ENC1_MAX_MV = 5000.0f;

    const float ENC2_MIN_MV = 0.0f;
    const float ENC2_MAX_MV = 5000.0f;

    const float enc1_percentage = (calibrated_enc1_mv - ENC1_MIN_MV) / (ENC1_MAX_MV - ENC1_MIN_MV);
    const float enc2_percentage = (calibrated_enc2_mv - ENC2_MIN_MV) / (ENC2_MAX_MV - ENC2_MIN_MV);

    const float percent_diff = enc1_percentage - enc2_percentage;

    if (percent_diff >= PEDAL_RATIONALITY_MAX_PERCENT_DIFF) {
        glo.pedal_irrational = true;
        glo.pedal_percentage = 0.0f;
        glo.pedal_torque_request = 0.0f;
    } else {
        glo.pedal_irrational = false;
        // average two percentages
        const float raw_percent = (enc1_percentage + enc2_percentage) / 2.0f;

        const float PEDAL_DEADBAND_THRESHOLD = 0.02f;
        if (raw_percent > PEDAL_DEADBAND_THRESHOLD) {
            glo.pedal_percentage = raw_percent;
            glo.pedal_torque_request = pedal_to_torque(raw_percent);
        } else {
            glo.pedal_percentage = 0.0f;
            glo.pedal_torque_request = 0.0f;
        }
    }
}

// ######   PRIVATE FUNCTIONS   ###### //

static IRAM_ATTR bool adc_callback(
    const adc_continuous_handle_t handle,
    const adc_continuous_evt_data_t * const edata,
    void * const user_data)
{
    (void)handle;
    (void)user_data;

    const adc_digi_output_data_t * const data =
        (const adc_digi_output_data_t*)&(edata->conv_frame_buffer[0]);

    switch (data->type2.channel) {
        case DASH_PIN_ENC1_ADC_CHAN:
            glo.last_enc1_adc_val_raw = data->type2.data;
            break;
        case DASH_PIN_ENC2_ADC_CHAN:
            glo.last_enc2_adc_val_raw = data->type2.data;
            break;
        default:
            break;
    }

    return false;
}

static void configure_pedal_adc(void) {
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

    adc_digi_pattern_config_t patterns[] = {
        {
            .atten = ADC_ATTEN_DB_11,
            .channel = DASH_PIN_ENC1_ADC_CHAN,
            .unit = ADC_UNIT_1,
            .bit_width = 12,
        },
        {
            .atten = ADC_ATTEN_DB_11,
            .channel = DASH_PIN_ENC2_ADC_CHAN,
            .unit = ADC_UNIT_1,
            .bit_width = 12,
        },
    };

    ESP_ERROR_CHECK(adc_continuous_config(glo.adc1_handle, &(adc_continuous_config_t){
        .pattern_num = 2,  // two channels
        .adc_pattern = patterns,
        .sample_freq_hz = 80000,
        .conv_mode = ADC_CONV_SINGLE_UNIT_1,
        .format = ADC_DIGI_OUTPUT_FORMAT_TYPE2,
    }));

    ESP_ERROR_CHECK(adc_continuous_register_event_callbacks(glo.adc1_handle, &(adc_continuous_evt_cbs_t){
        .on_conv_done = adc_callback,
    }, NULL));

    ESP_ERROR_CHECK(adc_continuous_start(glo.adc1_handle));
}

// ######   PUBLIC FUNCTIONS    ###### //

bool pedal_irrational(void) {
    return glo.pedal_irrational;
}

float pedal_percentage(void) {
    return glo.pedal_percentage;
}

// do what's right | made with <3 at Cooper Union
