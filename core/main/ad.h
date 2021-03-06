#pragma once
#include <stdint.h>
#include "m.h"

// buffer size in B
#define AD_BUFFER_SIZE (uint16_t) sizeof(struct threed_data) * 50 // 500 = 100 samples per call

#define E_AD_ALREADY_RUNNING -1
#define E_AD_MEM -2

// power-of-two samples at a time
#define AD_NUM_SAMPLES 10

/**
 * Packed 6 B of the accelerometer values
 */
struct __attribute__((__packed__)) threed_data {
    int16_t x_val : 13;
    int16_t y_val : 13;
    int16_t z_val : 13;
    int16_t _ : 1; // spare bit to fit on 5 bytes
};

#ifdef __cplusplus
extern "C" {
#endif

///
/// Starts the accelerometer recording, submits the data captured at the given
/// ``frequency`` to the ``callback``. The ``callback`` is expected to perform
/// some kind of I/O to transmit the data to some client.
///
/// Returns 0 for success, negative values for failures
///
int ad_start(const message_callback_t callback, const uint8_t frequency, const uint16_t maximum_time);

///
/// Stops the accelerometer recording. After this call, no more calls to
/// the ``callback`` function passed to ``ad_start(...)`` are expected.
///
int ad_stop();

#ifdef __cplusplus
}
#endif
