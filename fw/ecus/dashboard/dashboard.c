#include <stddef.h>

#include <ember_bl_servicing.h>
#include <ember_taskglue.h>

#include "pedal.h"

// ######   DEFINES & TYPES     ###### //

// ######      PROTOTYPES       ###### //

static void dash_init(void);
static void dash_1kHz(void);

// ######     PRIVATE DATA      ###### //
// ######          CAN          ###### //
// ######   PRIVATE FUNCTIONS   ###### //

ember_rate_funcs_S module_rf = {
    .call_init = dash_init,
    .call_1kHz = pedal_1kHz,
};

static void dash_init(void) {
    pedal_init();
}

static void dash_1kHz(void) {
    pedal_1kHz();
}

// ######   PUBLIC FUNCTIONS    ###### //

bool ember_bl_servicing_cb_are_we_ready_to_reboot(void) {
    return true; // todo
}
