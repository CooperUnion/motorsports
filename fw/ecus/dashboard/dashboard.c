#include <stddef.h>

#include <ember_bl_servicing.h>
#include <ember_taskglue.h>

ember_rate_funcs_S module_rf = { NULL };

bool ember_bl_servicing_cb_are_we_ready_to_reboot(void) {
    return true; // todo
}
