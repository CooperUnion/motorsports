#include <stddef.h>

#include <ember_bl_servicing.h>
#include <ember_taskglue.h>
#include <opencan_rx.h>

#include "pedal.h"

// ######   DEFINES & TYPES     ###### //

#define atomic _Atomic

typedef enum {
    INIT,
    GLV_ONLY,
    PREPARE_TS,
    TS_WITH_NO_DRIVE,   // 10.4.2 Tractive System Active
    DRIVE,              // 10.4.3 Ready To Drive
    STOP_TS,
    FAULT,
} vcu_state_E;

// ######      PROTOTYPES       ###### //

static bool is_everyone_else_healthy(void);
static bool is_ams_healthy(void);
static bool is_pch_healthy(void);

// ######     PRIVATE DATA      ###### //

static struct {
    atomic bool enable_tractive_system_requested;
    atomic bool ready_to_drive_requested;
    atomic bool disable_tractive_system_requested;
    atomic vcu_state_E vcu_state;
} glo = {
    .enable_tractive_system_requested = false,
    .ready_to_drive_requested = false,
    .vcu_state = INIT,
};

// ######          CAN          ###### //
// ######    RATE FUNCTIONS     ###### //

static void dash_init(void);
static void dash_1kHz(void);

ember_rate_funcs_S module_rf = {
    .call_init = dash_init,
    .call_1kHz = dash_1kHz,
};

static void dash_init(void) {
    pedal_init();
}

static void dash_1kHz(void) {
    pedal_1kHz();

    vcu_state_E next_state = glo.vcu_state;

    switch (glo.vcu_state) {
        case INIT:
            next_state = GLV_ONLY,
            break;
        case GLV_ONLY:
            if (glo.enable_tractive_system_requested) {
                next_state = PREPARE_TS;
            }
            break;
        case PREPARE_TS:
            if (CANRX_get_PCH_state() == CAN_PCH_STATE_CONNECTED) {
                next_state = TS_WITH_NO_DRIVE;
            }
            break;
        case TS_WITH_NO_DRIVE:
            if (glo.ready_to_drive_requested) {
                next_state = DRIVE;
            }
            break;
        case DRIVE:
            if (glo.disable_tractive_system_requested) {
                next_state = STOP_TS;
            }
        case STOP_TS:
            if (CANRX_get_PCH_state() == CAN_PCH_STATE_IDLE) {
                next_state = GLV_ONLY;
            }
            break;
        case FAULT:
            // requires a GLV reset. No software reset possible.
            break;
    }

    // global fault check
    if (!is_everyone_else_healthy()) {
        next_state = FAULT;
    }

    glo.vcu_state = next_state;
}

// ######   PRIVATE FUNCTIONS   ###### //

static bool is_everyone_else_ok(void) {
    return is_pch_healthy() && is_ams_healthy();
}

static bool is_pch_healthy(void) {
    return CANRX_is_node_PCH_ok() &&
           (CANRX_get_PCH_state() != CAN_PCH_STATE_FAULT);
}

static bool is_ams_healthy(void) {
    return CANRX_is_node_AMS_ok() &&
           (CANRX_get_AMS_state() != CAN_AMS_STATE_FAULT);
}

// ######   PUBLIC FUNCTIONS    ###### //

bool ember_bl_servicing_cb_are_we_ready_to_reboot(void) {
    return true; // todo
}

// do what's right | made with <3 at Cooper Union
