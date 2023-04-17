// temporary file

#include <opencan_callbacks.h>
#include <opencan_tx.h>

void CANTX_populate_PCH_Status(struct CAN_Message_PCH_Status * const m) {
    static uint8_t counter;

    m->PCH_counter = counter++;
}

void ember_can_callback_notify_lost_can(void) {
    // nothing
}
