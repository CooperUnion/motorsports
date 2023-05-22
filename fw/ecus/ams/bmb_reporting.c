#include "bmbs.h"

#include "opencan_tx.h"

#define FOREACH_BMB(action) \
    action(0) \
    action(1) \
    action(2) \
    action(3) \
    action(4)

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

FOREACH_BMB(CANTX_CELLVOLTAGES_1_4)
FOREACH_BMB(CANTX_CELLVOLTAGES_5_8)
FOREACH_BMB(CANTX_CELLVOLTAGES_9_12)
FOREACH_BMB(CANTX_CELLVOLTAGES_13_16)

#define CANTX_BMBSTATUS(BMB_NUM) \
    void CANTX_populate_AMS_Bmb##BMB_NUM##_Status( \
    struct CAN_Message_AMS_Bmb##BMB_NUM##_Status * const m) {  \
        m->AMS_bmb##BMB_NUM##_balancingStatus = bmbs[BMB_##BMB_NUM].status.balancing_status; \
    }

FOREACH_BMB(CANTX_BMBSTATUS)

#define CANTX_BMBFAULTS(BMB_NUM) \
    void CANTX_populate_AMS_Bmb##BMB_NUM##_Faults( \
    struct CAN_Message_AMS_Bmb##BMB_NUM##_Faults * const m) {  \
        const SAFETY_STATUS_A_Type safety_A = bmbs[BMB_##BMB_NUM].status.safety_status_A;   \
        const SAFETY_STATUS_B_Type safety_B = bmbs[BMB_##BMB_NUM].status.safety_status_B;   \
        const SAFETY_STATUS_C_Type safety_C = bmbs[BMB_##BMB_NUM].status.safety_status_C;   \
                                                                                            \
        m->AMS_bmb##BMB_NUM##_safetyA_cellUndervoltage              = safety_A.CUV;   \
        m->AMS_bmb##BMB_NUM##_safetyA_cellOvervoltage               = safety_A.COV;   \
        m->AMS_bmb##BMB_NUM##_safetyA_cellChargeOvercurrent         = safety_A.OCC;   \
        m->AMS_bmb##BMB_NUM##_safetyA_cellDischarge1Overcurrent     = safety_A.OCD1;  \
        m->AMS_bmb##BMB_NUM##_safetyA_cellDischarge2Overcurrent     = safety_A.OCD2;  \
        m->AMS_bmb##BMB_NUM##_safetyA_cellDischargeShortCircuit     = safety_A.SCD;   \
                                                                                      \
        m->AMS_bmb##BMB_NUM##_safetyB_cellChargeUndertemperature    = safety_B.UTC;   \
        m->AMS_bmb##BMB_NUM##_safetyB_cellDischargeUndertemperature = safety_B.UTD;   \
        m->AMS_bmb##BMB_NUM##_safetyB_internalDieUndertemperature   = safety_B.UTINT; \
        m->AMS_bmb##BMB_NUM##_safetyB_chargeOvertemperature         = safety_B.OTC;   \
        m->AMS_bmb##BMB_NUM##_safetyB_dischargeOvertemperature      = safety_B.OTD;   \
        m->AMS_bmb##BMB_NUM##_safetyB_internalDieOvertemperature    = safety_B.OTINT; \
        m->AMS_bmb##BMB_NUM##_safetyB_fetOvertemperature            = safety_B.OTF;   \
                                                                                      \
        m->AMS_bmb##BMB_NUM##_safetyC_hostWatchdogSafetyFault       = safety_C.HWDF;  \
        m->AMS_bmb##BMB_NUM##_safetyC_prechargeTimeout              = safety_C.PTO;   \
        m->AMS_bmb##BMB_NUM##_safetyC_latchedCellOvervoltage        = safety_C.COVL;  \
        m->AMS_bmb##BMB_NUM##_safetyC_latchedDischargeOvercurrent   = safety_C.OCDL;  \
        m->AMS_bmb##BMB_NUM##_safetyC_latchedDischargeShortCircuit  = safety_C.SCDL;  \
        m->AMS_bmb##BMB_NUM##_safetyC_overcurrentInDischarge3       = safety_C.OCD3;  \
    }

FOREACH_BMB(CANTX_BMBFAULTS)

// do what's right | made with <3 at Cooper Union
