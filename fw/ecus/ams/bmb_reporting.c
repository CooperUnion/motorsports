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
    struct CAN_Message_AMS_Bmb##BMB_NUM##_CellVoltages1_4 * restrict const m) {     \
        const BmsStatus status = bmbs[BMB_##BMB_NUM].status;                        \
        *m = (struct CAN_Message_AMS_Bmb##BMB_NUM##_CellVoltages1_4) {              \
            .AMS_bmb##BMB_NUM##_cell1 = status.cell_voltages[0], \
            .AMS_bmb##BMB_NUM##_cell2 = status.cell_voltages[1], \
            .AMS_bmb##BMB_NUM##_cell3 = status.cell_voltages[2], \
            .AMS_bmb##BMB_NUM##_cell4 = status.cell_voltages[3], \
        }; \
    }

#define CANTX_CELLVOLTAGES_5_8(BMB_NUM) \
    void CANTX_populate_AMS_Bmb##BMB_NUM##_CellVoltages5_8( \
    struct CAN_Message_AMS_Bmb##BMB_NUM##_CellVoltages5_8 * restrict const m) {     \
        const BmsStatus status = bmbs[BMB_##BMB_NUM].status;                        \
        *m = (struct CAN_Message_AMS_Bmb##BMB_NUM##_CellVoltages5_8) {              \
            .AMS_bmb##BMB_NUM##_cell5 = status.cell_voltages[4],    \
            .AMS_bmb##BMB_NUM##_cell6 = status.cell_voltages[5],    \
            .AMS_bmb##BMB_NUM##_cell7 = status.cell_voltages[6],    \
            .AMS_bmb##BMB_NUM##_cell8 = status.cell_voltages[7],    \
        };                                                                              \
    }

#define CANTX_CELLVOLTAGES_9_12(BMB_NUM) \
    void CANTX_populate_AMS_Bmb##BMB_NUM##_CellVoltages9_12( \
    struct CAN_Message_AMS_Bmb##BMB_NUM##_CellVoltages9_12 * restrict const m) {  \
        const BmsStatus status = bmbs[BMB_##BMB_NUM].status;                      \
        *m = (struct CAN_Message_AMS_Bmb##BMB_NUM##_CellVoltages9_12) {           \
            .AMS_bmb##BMB_NUM##_cell9  = status.cell_voltages[8],   \
            .AMS_bmb##BMB_NUM##_cell10 = status.cell_voltages[9],   \
            .AMS_bmb##BMB_NUM##_cell11 = status.cell_voltages[10],  \
            .AMS_bmb##BMB_NUM##_cell12 = status.cell_voltages[11],  \
        }; \
    }

#define CANTX_CELLVOLTAGES_13_16(BMB_NUM) \
    void CANTX_populate_AMS_Bmb##BMB_NUM##_CellVoltages13_16( \
    struct CAN_Message_AMS_Bmb##BMB_NUM##_CellVoltages13_16 * restrict const m) {       \
        BmsStatus status = bmbs[BMB_##BMB_NUM].status;                                  \
        *m = (struct CAN_Message_AMS_Bmb##BMB_NUM##_CellVoltages13_16) {                \
            .AMS_bmb##BMB_NUM##_cell13 = status.cell_voltages[12],  \
            .AMS_bmb##BMB_NUM##_cell14 = status.cell_voltages[13],  \
            .AMS_bmb##BMB_NUM##_cell15 = status.cell_voltages[14],  \
            .AMS_bmb##BMB_NUM##_cell16 = status.cell_voltages[15],  \
        }; \
    }

FOREACH_BMB(CANTX_CELLVOLTAGES_1_4)
FOREACH_BMB(CANTX_CELLVOLTAGES_5_8)
FOREACH_BMB(CANTX_CELLVOLTAGES_9_12)
FOREACH_BMB(CANTX_CELLVOLTAGES_13_16)

#define CANTX_BMBSTATUS(BMB_NUM) \
    void CANTX_populate_AMS_Bmb##BMB_NUM##_Status( \
    struct CAN_Message_AMS_Bmb##BMB_NUM##_Status * restrict const m) {  \
        m->AMS_bmb##BMB_NUM##_balancingStatus = bmbs[BMB_##BMB_NUM].status.balancing_status; \
    }

FOREACH_BMB(CANTX_BMBSTATUS)

#define CANTX_BMBFAULTS(BMB_NUM) \
    void CANTX_populate_AMS_Bmb##BMB_NUM##_Faults( \
    struct CAN_Message_AMS_Bmb##BMB_NUM##_Faults * restrict const m) {  \
        const SAFETY_STATUS_A_Type safety_A = bmbs[BMB_##BMB_NUM].status.safety_status_A;   \
        const SAFETY_STATUS_B_Type safety_B = bmbs[BMB_##BMB_NUM].status.safety_status_B;   \
        const SAFETY_STATUS_C_Type safety_C = bmbs[BMB_##BMB_NUM].status.safety_status_C;   \
        *m = (struct CAN_Message_AMS_Bmb##BMB_NUM##_Faults) {                           \
            .AMS_bmb##BMB_NUM##_safetyA_cellUndervoltage              = safety_A.CUV,   \
            .AMS_bmb##BMB_NUM##_safetyA_cellOvervoltage               = safety_A.COV,   \
            .AMS_bmb##BMB_NUM##_safetyA_cellChargeOvercurrent         = safety_A.OCC,   \
            .AMS_bmb##BMB_NUM##_safetyA_cellDischarge1Overcurrent     = safety_A.OCD1,  \
            .AMS_bmb##BMB_NUM##_safetyA_cellDischarge2Overcurrent     = safety_A.OCD2,  \
            .AMS_bmb##BMB_NUM##_safetyA_cellDischargeShortCircuit     = safety_A.SCD,   \
                                                                                        \
            .AMS_bmb##BMB_NUM##_safetyB_cellChargeUndertemperature    = safety_B.UTC,   \
            .AMS_bmb##BMB_NUM##_safetyB_cellDischargeUndertemperature = safety_B.UTD,   \
            .AMS_bmb##BMB_NUM##_safetyB_internalDieUndertemperature   = safety_B.UTINT, \
            .AMS_bmb##BMB_NUM##_safetyB_chargeOvertemperature         = safety_B.OTC,   \
            .AMS_bmb##BMB_NUM##_safetyB_dischargeOvertemperature      = safety_B.OTD,   \
            .AMS_bmb##BMB_NUM##_safetyB_internalDieOvertemperature    = safety_B.OTINT, \
            .AMS_bmb##BMB_NUM##_safetyB_fetOvertemperature            = safety_B.OTF,   \
                                                                                        \
            .AMS_bmb##BMB_NUM##_safetyC_hostWatchdogSafetyFault       = safety_C.HWDF,  \
            .AMS_bmb##BMB_NUM##_safetyC_prechargeTimeout              = safety_C.PTO,   \
            .AMS_bmb##BMB_NUM##_safetyC_latchedCellOvervoltage        = safety_C.COVL,  \
            .AMS_bmb##BMB_NUM##_safetyC_latchedDischargeOvercurrent   = safety_C.OCDL,  \
            .AMS_bmb##BMB_NUM##_safetyC_latchedDischargeShortCircuit  = safety_C.SCDL,  \
            .AMS_bmb##BMB_NUM##_safetyC_overcurrentInDischarge3       = safety_C.OCD3,  \
        }; \
    }

FOREACH_BMB(CANTX_BMBFAULTS)

#define CANTX_BATTERYSTATUS(BMB_NUM) \
    void CANTX_populate_AMS_Bmb##BMB_NUM##_BatteryStatus( \
    struct CAN_Message_AMS_Bmb##BMB_NUM##_BatteryStatus * restrict const m) {  \
        const BATTERY_STATUS_Type battery_status = bmbs[BMB_##BMB_NUM].status.battery_status; \
        *m = (struct CAN_Message_AMS_Bmb##BMB_NUM##_BatteryStatus) {\
            .AMS_bmb##BMB_NUM##_batteryStatus_configUpdateMode    = battery_status.CFGUPDATE, \
            .AMS_bmb##BMB_NUM##_batteryStatus_prechargeMode       = battery_status.PCHG_MODE, \
            .AMS_bmb##BMB_NUM##_batteryStatus_sleepEn             = battery_status.SLEEP_EN,  \
            .AMS_bmb##BMB_NUM##_batteryStatus_powerOnResetHappened= battery_status.POR,       \
            .AMS_bmb##BMB_NUM##_batteryStatus_previousResetWasWatchdog = battery_status.WD,   \
            .AMS_bmb##BMB_NUM##_batteryStatus_cellOpenWireChecksRunning = battery_status.COW_CHK, \
            .AMS_bmb##BMB_NUM##_batteryStatus_otpPending = battery_status.OTPW,  \
            .AMS_bmb##BMB_NUM##_batteryStatus_otpConditionsValid = battery_status.OTPB,   \
            .AMS_bmb##BMB_NUM##_batteryStatus_sec0 = battery_status.SEC0,  \
            .AMS_bmb##BMB_NUM##_batteryStatus_sec1 = battery_status.SEC1,  \
            .AMS_bmb##BMB_NUM##_batteryStatus_fusePinState = battery_status.FUSE,  \
            .AMS_bmb##BMB_NUM##_batteryStatus_safetyFaultTriggered = battery_status.SS, \
            .AMS_bmb##BMB_NUM##_batteryStatus_permanentFaultTriggered = battery_status.PF, \
            .AMS_bmb##BMB_NUM##_batteryStatus_shutdownPending = battery_status.SD_CMD, \
            .AMS_bmb##BMB_NUM##_batteryStatus_inSleepMode = battery_status.SLEEP, \
        }; \
    }

FOREACH_BMB(CANTX_BATTERYSTATUS)

// do what's right | made with <3 at Cooper Union
