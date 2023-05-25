#pragma once

#include "bqdriver/bms.h"

typedef enum {
    BMB_0,
    BMB_1,
    BMB_2,
    BMB_3,
    BMB_4,

    BMB_NUM_BMBS
} Bmb_E;

typedef enum {
    NTC_1,
    NTC_2,
    NTC_3,
    NTC_4,
    NTC_5,
    NTC_6,
    NTC_7,
    NTC_8,

    NTC_NUM_NTCS
} Bmb_ntc_E;

extern Bms bmbs[BMB_NUM_BMBS];

// do what's right | made with <3 at Cooper Union
