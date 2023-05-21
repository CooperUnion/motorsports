/*
 * Copyright (c) The Libre Solar Project Contributors
 *           (c) Cooper Motorsports Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define LOG_DBG(...) printf(__VA_ARGS__)
#define LOG_INF(...) printf(__VA_ARGS__)
#define LOG_WRN(...) printf(__VA_ARGS__)
#define LOG_ERR(...) printf(__VA_ARGS__)

/**
 * Interpolation in a look-up table. Values of a must be monotonically increasing/decreasing
 *
 * @returns interpolated value of array b at position value_a
 */
float interpolate(const float a[], const float b[], size_t size, float value_a);

/**
 * Framework-independent system uptime
 *
 * @returns seconds since the system booted
 */
uint32_t uptime();

/**
 * Convert byte to bit-string
 *
 * Attention: Uses static buffer, not thread-safe
 *
 * @returns pointer to bit-string (8 characters + null-byte)
 */
const char *byte2bitstr(uint8_t b);
