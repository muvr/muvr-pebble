#pragma once
#include <stdint.h>
#include "m.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 5 B in header
 */
struct __attribute__((__packed__)) header {
    uint8_t type;                   // 1
    uint8_t count;                  // 2
    uint8_t samples_per_second;     // 3
    uint8_t sample_size;            // 4
    uint8_t queue_size;             // 5
    uint64_t timestamp;             // 13
};

///
/// Sets up App Messages BLE communication, returning a ``message_callback_t`` value,
/// which prepends the ``struct header`` above ahead of the samples passed.
/// The ``type``, ``samples_per_second``, and ``sample_size`` will be set in the header.
///
message_callback_t am_start(uint8_t type, uint8_t samples_per_second, uint8_t sample_size);

///
/// Stops the App Messages communication
///
void am_stop();

///
/// Returns the status to the given ``text``, having space for ``max_size`` bytes.
///
void am_get_status(char *text, uint16_t max_size);

#ifdef __cplusplus
}
#endif
