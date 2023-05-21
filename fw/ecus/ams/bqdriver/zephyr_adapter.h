#pragma once

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

uint64_t k_uptime_get(void);
void k_usleep(uint32_t duration_us);

enum zeph_errors {
    EINVAL = -22,
    ENODEV = -19,
    EIO = -5,
};

#define __ASSERT(a, ...) (assert(a), fprintf(stderr, __VA_ARGS__))
#define CLAMP(x, low, high) ((x) > (high) ? (high) : ((x) < (low) ? (low) : (x)))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

int i2c_init(void);

int i2c_write_read(uint16_t addr,
	const void *write_buf, size_t num_write,
	void *read_buf, size_t num_read);

int i2c_write(const uint8_t *buf,
	uint32_t num_bytes, uint16_t addr);
