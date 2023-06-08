// Cooper Motorsports Precharge Controller

#include <stdint.h>

// temp to make intellisense work
#define CONFIG_IDF_TARGET_ESP32S3 1
#define SOC_ADC_DMA_SUPPORTED 1
#define ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED 1

#include <esp_adc/adc_continuous.h>
#include <esp_adc/adc_cali.h>
#include <esp_adc/adc_cali_scheme.h>
#include <esp_timer.h>
#include <driver/gpio.h>
#include <hal/adc_types.h>

#include <ember_bl_servicing.h>
#include <ember_taskglue.h>
#include <node_pins.h>
#include <opencan_rx.h>
#include <opencan_tx.h>

#include "precharge_pins.h"

// ######   DEFINES & TYPES     ###### //

#define PRECHARGE_CONNECT_THRESHOLD_V   (32.0f)
#define MAX_PRECHARGE_TIME_MS           (100.0f)

#define atomic _Atomic

typedef enum {
    PCH_STATE_INIT,
    PCH_STATE_IDLE,
    PCH_STATE_PRECHARGING,
    PCH_STATE_CONNECTED,
    PCH_STATE_DISCONNECTING,
    PCH_STATE_FAULT,
} pch_state_E;

typedef struct {
    pch_state_E state;
    uint32_t start_time;
} state_status_S;

// ######      PROTOTYPES       ###### //

static void set_state(pch_state_E new_state);
static uint32_t current_time(void);
static pch_state_E current_state(void);
static uint32_t time_in_state(void);

static bool adc_callback(
    adc_continuous_handle_t handle,
    const adc_continuous_evt_data_t *edata,
    void *user_data);

static void configure_hvdc_adc(void);

static bool is_any_voltage_present_on_tractive_system(void);
static bool is_high_voltage_present_on_tractive_system(void);
static void open_all_contactors_without_delay_or_checks(void);

// ######     PRIVATE DATA      ###### //

static struct {
    atomic state_status_S current_state;

    adc_continuous_handle_t adc1_handle;
    adc_cali_handle_t adc1_cali_handle;
    atomic volatile uint32_t last_adc_val_raw;
    atomic uint32_t last_adc_val_calibrated;

    atomic float hvdc_voltage;
} glo = {
    .current_state = {
        .state = PCH_STATE_INIT,
        .start_time = 0U,
    },

    .adc1_handle = NULL,
    .adc1_cali_handle = NULL,
    .last_adc_val_raw = 0U,

    .hvdc_voltage = 0U,
};

// ######          CAN          ###### //

void CANTX_populate_PCH_Status(struct CAN_Message_PCH_Status * const m) {
    static uint8_t counter;

    m->PCH_counter = counter++;

    typeof(m->PCH_state) state = PCH_STATE_FAULT;
    switch (current_state()) {
        case PCH_STATE_INIT:            state = CAN_PCH_STATE_INIT; break;
        case PCH_STATE_IDLE:            state = CAN_PCH_STATE_IDLE; break;
        case PCH_STATE_PRECHARGING:     state = CAN_PCH_STATE_PRECHARGING; break;
        case PCH_STATE_CONNECTED:       state = CAN_PCH_STATE_CONNECTED; break;
        case PCH_STATE_DISCONNECTING:   state = CAN_PCH_STATE_DISCONNECTING; break;
        case PCH_STATE_FAULT:
        default:                        state = CAN_PCH_STATE_FAULT; break;
    }
    m->PCH_state = state;

    m->PCH_calibratedAdcMv = glo.last_adc_val_calibrated;
    m->PCH_rawAdcMv = glo.last_adc_val_raw;
    m->PCH_dcBusVoltage = glo.hvdc_voltage;
    m->PCH_pcContactorState = gpio_get_level(PRECHARGE_PIN_PRECH_RELAY_CTRL);
    m->PCH_airNegContactorState = gpio_get_level(PRECHARGE_PIN_AIR_NEG_CTRL);
    m->PCH_airPosContactorState = gpio_get_level(PRECHARGE_PIN_AIR_POS_CTRL);
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
    ESP_ERROR_CHECK(gpio_config(&(gpio_config_t){
        .pin_bit_mask =
            BIT64(PRECHARGE_PIN_AIR_NEG_CTRL) |
            BIT64(PRECHARGE_PIN_AIR_POS_CTRL) |
            BIT64(PRECHARGE_PIN_PRECH_RELAY_CTRL),
        .mode = GPIO_MODE_OUTPUT,
    }));

    open_all_contactors_without_delay_or_checks();

    // configure LEDs
    ESP_ERROR_CHECK(gpio_config(&(gpio_config_t){
        .pin_bit_mask = BIT64(PRECHARGE_PIN_LED_D7) | BIT64(PRECHARGE_PIN_LED_D8) |
                        BIT64(NODE_BOARD_PIN_LED1) | BIT64(NODE_BOARD_PIN_LED2),
        .mode = GPIO_MODE_OUTPUT,
    }));

    configure_hvdc_adc();
    set_state(PCH_STATE_IDLE);
}

static void pch_10Hz(void) {
    static bool led_state;
    gpio_set_level(NODE_BOARD_PIN_LED1, led_state);
    gpio_set_level(NODE_BOARD_PIN_LED2, !led_state);
    gpio_set_level(PRECHARGE_PIN_LED_D7, !led_state);
    gpio_set_level(PRECHARGE_PIN_LED_D8, led_state);
    led_state = !led_state;
}

static void pch_1kHz(void) {
    // convert ADC value
    int calibrated_adc_mv = 0;
    ESP_ERROR_CHECK(adc_cali_raw_to_voltage(glo.adc1_cali_handle, glo.last_adc_val_raw, &calibrated_adc_mv));

    if (PRECHARGE_ADC_OFFSET_MV >= calibrated_adc_mv) {
        glo.hvdc_voltage = 0.0f;
    } else {
        glo.hvdc_voltage = PRECHARGE_ADC_GAIN_ISOAMP_MV_TO_V * (calibrated_adc_mv - PRECHARGE_ADC_OFFSET_MV);
    }

    glo.last_adc_val_calibrated = calibrated_adc_mv;

    const bool mom_present = CANRX_is_node_MOM_ok();
    enum CAN_MOM_tractiveSystemRunlevel runlevel = CANRX_get_MOM_tractiveSystemRunlevel();
    const bool mom_says_please_hv = mom_present && runlevel == CAN_MOM_TRACTIVESYSTEMRUNLEVEL_PLEASE_HV;
    const bool mom_says_no_hv = mom_present && runlevel == CAN_MOM_TRACTIVESYSTEMRUNLEVEL_ABSOLUTELY_NO_HV;

    static bool mom_says_please_hv_last = false;

    // main state machine
    switch (current_state()) {
        case PCH_STATE_INIT:
            // should be unreachable; pch_1kHz() will not run until pch_init() is done
            set_state(PCH_STATE_FAULT);
            break;

        case PCH_STATE_IDLE:
            // is HV present? We should fault.
            // if (is_any_voltage_present_on_tractive_system()) {
            //     set_state(PCH_STATE_FAULT);
            //     break;
            // }

            // wait for CAN command
            if (mom_says_please_hv && !mom_says_please_hv_last) {
                gpio_set_level(PRECHARGE_PIN_AIR_NEG_CTRL, 1);
                gpio_set_level(PRECHARGE_PIN_PRECH_RELAY_CTRL, 1);
                set_state(PCH_STATE_PRECHARGING);
                break;
            }

            open_all_contactors_without_delay_or_checks();
            break;

        case PCH_STATE_PRECHARGING:
            // we are precharging.
            // if (time_in_state() > MAX_PRECHARGE_TIME_MS) {
            //     open_all_contactors_without_delay_or_checks();
            //     set_state(PCH_STATE_FAULT);
            //     break;
            // }

            if (mom_says_no_hv || !mom_present) {
                set_state(PCH_STATE_DISCONNECTING);
                break;
            }

            if (glo.hvdc_voltage >= PRECHARGE_CONNECT_THRESHOLD_V) {
                gpio_set_level(PRECHARGE_PIN_AIR_POS_CTRL, 0);
                gpio_set_level(PRECHARGE_PIN_AIR_POS_CTRL, 1);
                set_state(PCH_STATE_CONNECTED);
                break;
            }

            // if (time_in_state() > 20000U) {
            //     gpio_set_level(PRECHARGE_PIN_AIR_POS_CTRL, 1);
            //     set_state(PCH_STATE_CONNECTED);
            //     break;
            // }

            break;

        case PCH_STATE_CONNECTED:
            // we are connected.

            // // maybe if there's a dead short or something...
            // if (!is_high_voltage_present_on_tractive_system()) {
            //     set_state(PCH_STATE_FAULT);
            // }

            if (mom_says_no_hv || !mom_present) {
                set_state(PCH_STATE_DISCONNECTING);
                break;
            }

            break;

        case PCH_STATE_DISCONNECTING:
            // we need to disconnect.
            // todo: we should check the current through the contactors!

            // Is there high voltage present on the tractive system?
            // If so, wait 150ms before opening the contactors to allow for drawdown.
            if (!is_high_voltage_present_on_tractive_system() || (time_in_state() > 150U)) {
                open_all_contactors_without_delay_or_checks();
            }

            set_state(PCH_STATE_IDLE);
            break;

        case PCH_STATE_FAULT:
            // Is there high voltage present on the tractive system?
            // If so, wait 150ms before opening the contactors to allow for drawdown.
            if (!is_high_voltage_present_on_tractive_system() || (time_in_state() > 150U)) {
                open_all_contactors_without_delay_or_checks();
            }

            break;

        default:
            // should be unreachable
            set_state(PCH_STATE_FAULT);
            break;
    }

    mom_says_please_hv_last = mom_says_please_hv;
}

// ######   PRIVATE FUNCTIONS   ###### //

static void set_state(const pch_state_E new_state) {
    glo.current_state = (const state_status_S){
        .start_time = current_time(),
        .state = new_state
    };
}

static uint32_t current_time(void) {
    return esp_timer_get_time() / (int64_t)1000;
}

static pch_state_E current_state(void) {
    const state_status_S tmp = glo.current_state;

    return tmp.state;
}

static uint32_t time_in_state(void) {
    const state_status_S tmp = glo.current_state;

    return current_time() - tmp.start_time;
}

static bool adc_callback(
    const adc_continuous_handle_t handle,
    const adc_continuous_evt_data_t * const edata,
    void * const user_data)
{
    (void)handle;
    (void)user_data;

    const adc_digi_output_data_t * const data =
        (const adc_digi_output_data_t*)&(edata->conv_frame_buffer[0]);

    glo.last_adc_val_raw = data->type2.data;

    return false;
}

static void configure_hvdc_adc(void) {
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

    ESP_ERROR_CHECK(adc_continuous_start(glo.adc1_handle));
}

static bool is_any_voltage_present_on_tractive_system(void) {
    return glo.hvdc_voltage > 10.0f;
}

static bool is_high_voltage_present_on_tractive_system(void) {
    return glo.hvdc_voltage >= 60.0f;
}

static void open_all_contactors_without_delay_or_checks(void) {
    gpio_set_level(PRECHARGE_PIN_AIR_POS_CTRL, 0);
    gpio_set_level(PRECHARGE_PIN_AIR_NEG_CTRL, 0);
    gpio_set_level(PRECHARGE_PIN_PRECH_RELAY_CTRL, 0);
}

// ######   PUBLIC FUNCTIONS    ###### //

/*
 * Allow reboot if we're in IDLE or FAULT.
*/
bool ember_bl_servicing_cb_are_we_ready_to_reboot(void) {
    switch (current_state()) {
        case PCH_STATE_IDLE:
        case PCH_STATE_FAULT:
            return true;
        default:
            return false;
    }
}

// do what's right | made with <3 at Cooper Union
