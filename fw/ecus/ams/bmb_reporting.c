#include "bmbs.h"

#include "opencan_tx.h"

void CANTX_populateTemplate_BMB0_CellVoltages1_4(struct CAN_TMessage_CellVoltages1_4 * const m) {
    m->cell1 = bmbs[BMB_0].status.cell_voltages[0];
    m->cell2 = bmbs[BMB_0].status.cell_voltages[1];
    m->cell3 = bmbs[BMB_0].status.cell_voltages[2];
    m->cell4 = bmbs[BMB_0].status.cell_voltages[3];
    // printf("cell1: %f\n", m->cell1);
}

void CANTX_populateTemplate_BMB0_CellVoltages5_8(struct CAN_TMessage_CellVoltages5_8 * const m) {
    m->cell5 = bmbs[BMB_0].status.cell_voltages[4];
    m->cell6 = bmbs[BMB_0].status.cell_voltages[5];
    m->cell7 = bmbs[BMB_0].status.cell_voltages[6];
    m->cell8 = bmbs[BMB_0].status.cell_voltages[7];
}

void CANTX_populateTemplate_BMB0_CellVoltages9_12(struct CAN_TMessage_CellVoltages9_12 * const m) {
    m->cell9 = bmbs[BMB_0].status.cell_voltages[8];
    m->cell10 = bmbs[BMB_0].status.cell_voltages[9];
    m->cell11 = bmbs[BMB_0].status.cell_voltages[10];
    m->cell12 = bmbs[BMB_0].status.cell_voltages[11];
}

void CANTX_populateTemplate_BMB0_CellVoltages13_16(struct CAN_TMessage_CellVoltages13_16 * const m) {
    m->cell13 = bmbs[BMB_0].status.cell_voltages[12];
    m->cell14 = bmbs[BMB_0].status.cell_voltages[13];
    m->cell15 = bmbs[BMB_0].status.cell_voltages[14];
    m->cell16 = bmbs[BMB_0].status.cell_voltages[15];
}
