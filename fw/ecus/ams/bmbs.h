#pragma once

#include "bqdriver/bms.h"

typedef enum {
    BMB_0,
    BMB_1,
    BMB_2,
    BMB_3,
    BMB_4,
    BMB_NUM_BMBS,
} Bmb_E;

extern Bms bmbs[BMB_NUM_BMBS];

// do what's right | made with <3 at Cooper Union
