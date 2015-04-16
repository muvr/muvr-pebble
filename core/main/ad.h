#pragma once
#include <stdint.h>
#include "m.h"

// buffer size in B
#define AD_BUFFER_SIZE (uint16_t)500 // 500 = 100 samples per call

#define E_AD_ALREADY_RUNNING -1
#define E_AD_MEM -2

// power-of-two samples at a time
#define AD_NUM_SAMPLES 10

/**
 * Packed 5 B of the accelerometer values
 */
struct __attribute__((__packed__)) threed_data {
    int16_t x_val : 13;
    int16_t y_val : 13;
    int16_t z_val : 13;
};

///
/// Defines the sampling rates
///
typedef enum {
//    AD_SAMPLING_10HZ = 10,
//    AD_SAMPLING_25HZ = 25,
    AD_SAMPLING_50HZ = 50,
    AD_SAMPLING_100HZ = 100
} ad_sampling_rate_t;

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
int ad_start(message_callback_t callback, ad_sampling_rate_t frequency);

///
/// Stops the accelerometer recording. After this call, no more calls to
/// the ``callback`` function passed to ``ad_start(...)`` are expected.
///
int ad_stop();

#ifdef __cplusplus
}
#endif
