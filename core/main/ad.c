#include "compat.h"
#include <pebble.h>
#include "ad.h"

#define TIME_NAN 0

/**
 * Context that holds the current callback and samples_per_second. It is used in the accelerometer
 * callback to calculate the G forces and to push the packed sample buffer to the callback.
 */
static struct {
    // the callback function
    message_callback_t callback;
    // the samples_per_second
    uint8_t samples_per_second;
    // the buffer
    uint8_t buffer[AD_BUFFER_SIZE];
    // the maximum time
    uint16_t maximum_time;
    // the position in the buffer
    uint16_t buffer_position;
    // the start time
    uint64_t start_time;
} ad_context;

#define SIGNED_12_MAX(x) (int16_t)((x) > 4095 ? 4095 : ((x) < -4095 ? -4095 : (x)))

/**
 * Handle the samples arriving.
 */
static void ad_raw_accel_data_handler(AccelRawData *data, uint32_t num_samples, uint64_t timestamp) {
    if (num_samples != AD_NUM_SAMPLES) return /* FAIL */;
    size_t len = sizeof(struct threed_data) * num_samples;
    // pack
    struct threed_data *ad = (struct threed_data *)(ad_context.buffer + ad_context.buffer_position);
    for (unsigned int i = 0; i < num_samples; ++i) {
        
#ifndef TEST_WITH_SINES
        ad[i].x_val = SIGNED_12_MAX(data[i].x);
        ad[i].y_val = SIGNED_12_MAX(data[i].y);
        ad[i].z_val = SIGNED_12_MAX(data[i].z);
#else
        ad[i].x_val = SIGNED_12_MAX( sin_lookup((timestamp * 300) % TRIG_MAX_ANGLE) >> 6 ); // SIGNED_12_MAX(data[i].x);
        ad[i].y_val = SIGNED_12_MAX( sin_lookup((timestamp * 300) % TRIG_MAX_ANGLE) >> 6 ); // SIGNED_12_MAX(data[i].y);
        ad[i].z_val = SIGNED_12_MAX( cos_lookup((timestamp * 300) % TRIG_MAX_ANGLE) >> 6 ); // SIGNED_12_MAX(data[i].z);
#endif
        
    }
    ad_context.buffer_position += len;

    bool submit = false;
    if (ad_context.start_time != TIME_NAN) {
        if (timestamp - ad_context.start_time >= ad_context.maximum_time) {
            submit = true;
        }
    } else {
        ad_context.start_time = timestamp;
    }
    if (ad_context.buffer_position == AD_BUFFER_SIZE) {
        submit = true;
    }

    if (submit) {
        uint16_t duration = (uint16_t)(timestamp - ad_context.start_time);
        if (ad_context.callback != NULL) {
            ad_context.callback(ad_context.buffer, ad_context.buffer_position, timestamp, duration);
        } else {
            APP_LOG(APP_LOG_LEVEL_DEBUG, "Not submitting %d samples, %d reported duration", ad_context.buffer_position / sizeof(struct threed_data), duration);
        }
        ad_context.buffer_position = 0;
        ad_context.start_time = timestamp;
    }
}

int ad_start(const message_callback_t callback, const ad_sampling_rate_t frequency, const uint16_t maximum_time) {
    if (ad_context.callback != NULL) return E_AD_ALREADY_RUNNING;

    ad_context.callback = callback;
    ad_context.samples_per_second = (uint8_t) frequency;
    //ad_context.buffer = malloc(AD_BUFFER_SIZE);
    ad_context.maximum_time = maximum_time;
    ad_context.start_time = TIME_NAN;

    if (ad_context.buffer == NULL) return E_AD_MEM;

    accel_service_set_sampling_rate((AccelSamplingRate)frequency);
    accel_raw_data_service_subscribe(AD_NUM_SAMPLES, ad_raw_accel_data_handler);
    accel_service_set_sampling_rate((AccelSamplingRate)frequency);

    return 1;
}

int ad_stop() {
    ad_context.callback = NULL;
    return 1;
}
