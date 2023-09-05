#include <stddef.h>

#include <ember_bl_servicing.h>
#include <ember_taskglue.h>
#include <opencan_rx.h>
#include <opencan_tx.h>

#include "pedal.h"

// ######   DEFINES & TYPES     ###### //

#define atomic _Atomic

typedef enum {
    VCU_STATE_INIT,
    VCU_STATE_GLV_ONLY,
    VCU_STATE_PREPARE_TS,
    VCU_STATE_TS_WITH_NO_DRIVE,   // 10.4.2 Tractive System Active
    VCU_STATE_DRIVE,              // 10.4.3 Ready To Drive
    VCU_STATE_STOP_TS,
    VCU_STATE_FAULT,
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
    .vcu_state = VCU_STATE_INIT,
};

// ######          CAN          ###### //

void CANTX_populate_VCU_Status(struct CAN_Message_VCU_Status * const m) {
    typeof(m->VCU_state) state;

    switch (glo.vcu_state) {
        case VCU_STATE_INIT:            state = CAN_VCU_STATE_INIT; break;
        case VCU_STATE_GLV_ONLY:        state = CAN_VCU_STATE_GLV_ONLY; break;
        case VCU_STATE_PREPARE_TS:      state = CAN_VCU_STATE_PREPARE_TS; break;
        case VCU_STATE_DRIVE:           state = CAN_VCU_STATE_DRIVE; break;
        case VCU_STATE_STOP_TS:         state = CAN_VCU_STATE_STOP_TS; break;
        case VCU_STATE_FAULT:           state = CAN_VCU_STATE_FAULT; break;
        default:                        state = CAN_VCU_STATE_INIT; break;
    }

    *m = (struct CAN_Message_VCU_Status) {
        .VCU_state = state,
    };
}

// ######    RATE FUNCTIONS     ###### //

static void dash_init(void);
static void dash_1kHz(void);
static void dash_10Hz(void);

ember_rate_funcs_S module_rf = {
    .call_init = dash_init,
    .call_1kHz = dash_1kHz,
    .call_100Hz = dash_10Hz,
};

static void dash_init(void) {
    pedal_init();
}

static void dash_1kHz(void) {
    pedal_1kHz();

    vcu_state_E next_state = glo.vcu_state;

    switch (glo.vcu_state) {
        case VCU_STATE_INIT:
            next_state = VCU_STATE_GLV_ONLY;
            break;
        case VCU_STATE_GLV_ONLY:
            if (glo.enable_tractive_system_requested) {
                next_state = VCU_STATE_PREPARE_TS;
            }
            break;
        case VCU_STATE_PREPARE_TS:
            if (CANRX_get_PCH_state() == CAN_PCH_STATE_CONNECTED) {
                next_state = VCU_STATE_TS_WITH_NO_DRIVE;
            }
            break;
        case VCU_STATE_TS_WITH_NO_DRIVE:
            if (glo.ready_to_drive_requested) {
                next_state = VCU_STATE_DRIVE;
            }
            break;
        case VCU_STATE_DRIVE:
            if (glo.disable_tractive_system_requested) {
                next_state = VCU_STATE_STOP_TS;
            }
            break;
        case VCU_STATE_STOP_TS:
            if (CANRX_get_PCH_state() == CAN_PCH_STATE_IDLE) {
                next_state = VCU_STATE_GLV_ONLY;
            }
            break;
        case VCU_STATE_FAULT:
            // requires a GLV reset. No software reset possible.
            break;
    }

    // global fault check
    if (!is_everyone_else_healthy()) {
        next_state = VCU_STATE_FAULT;
    }

    glo.vcu_state = next_state;
}

static void dash_10Hz(void) {
    pedal_10Hz();
}

// ######   PRIVATE FUNCTIONS   ###### //

static bool is_everyone_else_healthy(void) {
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
