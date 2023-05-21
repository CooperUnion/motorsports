/*
 * Copyright (c) The Libre Solar Project Contributors
 *           (c) Cooper Motorsports Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "helper.h"
#include "zephyr_adapter.h"

#include <string.h>

uint32_t uptime()
{
    // TODO: return uptime
    return k_uptime_get() / 1000;
}

float interpolate(const float a[], const float b[], size_t size, float value_a)
{
    if (a[0] < a[size - 1]) {
        for (unsigned int i = 0; i < size; i++) {
            if (value_a <= a[i]) {
                if (i == 0) {
                    return b[0]; // value_a smaller than first element
                }
                else {
                    return b[i - 1] + (b[i] - b[i - 1]) * (value_a - a[i - 1]) / (a[i] - a[i - 1]);
                }
            }
        }
        return b[size - 1]; // value_a larger than last element
    }
    else {
        for (unsigned int i = 0; i < size; i++) {
            if (value_a >= a[i]) {
                if (i == 0) {
                    return b[0]; // value_a smaller than first element
                }
                else {
                    return b[i - 1] + (b[i] - b[i - 1]) * (value_a - a[i - 1]) / (a[i] - a[i - 1]);
                }
            }
        }
        return b[size - 1]; // value_a larger than last element
    }
}

const char *byte2bitstr(uint8_t b)
{
    static char str[9];

    str[0] = '\0';
    for (int z = 128; z > 0; z >>= 1) {
        strcat(str, ((b & z) == z) ? "1" : "0");
    }

    return str;
}
