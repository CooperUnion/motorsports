// Cooper Motorsports Precharge Controller

#include <opencan_tx.h>

void CANTX_populate_PCH_Status(struct CAN_Message_PCH_Status * const m) {
    static uint8_t counter;

    m->PCH_counter = counter++;
}

// do what's right | made with <3 at Cooper Union
