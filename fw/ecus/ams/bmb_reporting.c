#include "bmbs.h"

#include "opencan_tx.h"

#define CANTX_CELLVOLTAGES_1_4(BMB_NUM) \
    void CANTX_populate_AMS_Bmb##BMB_NUM##_CellVoltages1_4( \
    struct CAN_Message_AMS_Bmb##BMB_NUM##_CellVoltages1_4 * const m) {  \
        m->AMS_bmb##BMB_NUM##_cell1 = bmbs[BMB_##BMB_NUM].status.cell_voltages[0]; \
        m->AMS_bmb##BMB_NUM##_cell2 = bmbs[BMB_##BMB_NUM].status.cell_voltages[1]; \
        m->AMS_bmb##BMB_NUM##_cell3 = bmbs[BMB_##BMB_NUM].status.cell_voltages[2]; \
        m->AMS_bmb##BMB_NUM##_cell4 = bmbs[BMB_##BMB_NUM].status.cell_voltages[3]; \
    }

#define CANTX_CELLVOLTAGES_5_8(BMB_NUM) \
    void CANTX_populate_AMS_Bmb##BMB_NUM##_CellVoltages5_8( \
    struct CAN_Message_AMS_Bmb##BMB_NUM##_CellVoltages5_8 * const m) {  \
        m->AMS_bmb##BMB_NUM##_cell5 = bmbs[BMB_##BMB_NUM].status.cell_voltages[4]; \
        m->AMS_bmb##BMB_NUM##_cell6 = bmbs[BMB_##BMB_NUM].status.cell_voltages[5]; \
        m->AMS_bmb##BMB_NUM##_cell7 = bmbs[BMB_##BMB_NUM].status.cell_voltages[6]; \
        m->AMS_bmb##BMB_NUM##_cell8 = bmbs[BMB_##BMB_NUM].status.cell_voltages[7]; \
    }

#define CANTX_CELLVOLTAGES_9_12(BMB_NUM) \
    void CANTX_populate_AMS_Bmb##BMB_NUM##_CellVoltages9_12( \
    struct CAN_Message_AMS_Bmb##BMB_NUM##_CellVoltages9_12 * const m) {  \
        m->AMS_bmb##BMB_NUM##_cell9 = bmbs[BMB_##BMB_NUM].status.cell_voltages[8]; \
        m->AMS_bmb##BMB_NUM##_cell10 = bmbs[BMB_##BMB_NUM].status.cell_voltages[9]; \
        m->AMS_bmb##BMB_NUM##_cell11 = bmbs[BMB_##BMB_NUM].status.cell_voltages[10]; \
        m->AMS_bmb##BMB_NUM##_cell12 = bmbs[BMB_##BMB_NUM].status.cell_voltages[11]; \
    }

#define CANTX_CELLVOLTAGES_13_16(BMB_NUM) \
    void CANTX_populate_AMS_Bmb##BMB_NUM##_CellVoltages13_16( \
    struct CAN_Message_AMS_Bmb##BMB_NUM##_CellVoltages13_16 * const m) {  \
        m->AMS_bmb##BMB_NUM##_cell13 = bmbs[BMB_##BMB_NUM].status.cell_voltages[12]; \
        m->AMS_bmb##BMB_NUM##_cell14 = bmbs[BMB_##BMB_NUM].status.cell_voltages[13]; \
        m->AMS_bmb##BMB_NUM##_cell15 = bmbs[BMB_##BMB_NUM].status.cell_voltages[14]; \
        m->AMS_bmb##BMB_NUM##_cell16 = bmbs[BMB_##BMB_NUM].status.cell_voltages[15]; \
    }

#define FOREACH_BMB(action) \
    action(0) \
    action(1) \
    action(2) \
    action(3) \
    action(4)

FOREACH_BMB(CANTX_CELLVOLTAGES_1_4)
FOREACH_BMB(CANTX_CELLVOLTAGES_5_8)
FOREACH_BMB(CANTX_CELLVOLTAGES_9_12)
FOREACH_BMB(CANTX_CELLVOLTAGES_13_16)

// do what's right | made with <3 at Cooper Union
