#pragma once
#include <stdint.h>
#include "m.h"

#define E_AD_ALREADY_RUNNING -1
#define E_AD_MEM -2

// power-of-two samples at a time
#define AD_NUM_SAMPLES 2

/**
 * Packed 5 B of the accelerometer values
 */
struct __attribute__((__packed__)) threed_data {
    int16_t x_val : 13;
    int16_t y_val : 13;
    int16_t z_val : 13;
};

typedef enum {
//    AD_SAMPLING_10HZ = 10,
//    AD_SAMPLING_25HZ = 25,
//    AD_SAMPLING_50HZ = 50,
    AD_SAMPLING_100HZ = 100
} ad_sampling_rate_t;

#ifdef __cplusplus
extern "C" {
#endif

int ad_start(message_callback_t callback, ad_sampling_rate_t frequency);
int ad_stop();

#ifdef __cplusplus
}
#endif
