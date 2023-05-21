#pragma once

#include <stdint.h>

uint64_t k_uptime_get(void);
void k_usleep(uint32_t duration_us);

enum zeph_errors {
    EINVAL = -22,
    ENODEV = -19,
    EIO = -5,
};

#include <assert.h>

// todo
#define __ASSERT(a, ...) assert(a)
#define CLAMP(x, low, high) ((x) > (high) ? (high) : ((x) < (low) ? (low) : (x)))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
