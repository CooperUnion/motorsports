#include <stdint.h>

#include <opencan_tx.h>

void CANTX_populate_M192_Command_Message(struct CAN_Message_M192_Command_Message * const m) {
    static uint8_t counter = 0;
    *m = (struct CAN_Message_M192_Command_Message) {
        .Torque_Command         = 0.0f,
        .Speed_Command          = 0.0f,
        .Direction_Command      = CAN_DIRECTION_COMMAND_CW,
        .Inverter_Enable        = CAN_INVERTER_ENABLE_TURN_THE_INVERTER_OFF,
        .Inverter_Discharge     = CAN_INVERTER_DISCHARGE_DISCHARGE_DISABLE,
        .Speed_Mode_Enable      = true,
        .RollingCounter         = counter++,
        .Torque_Limit_Command   = 0.0f,
    };
}

// do what's right | made with <3 at Cooper Union
