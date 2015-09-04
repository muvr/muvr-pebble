#pragma once
#include <stdint.h>
#include "m.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    msg_dead               = 0xdead0000,
    msg_ad                 = 0xad000000,
    msg_accepted           = 0x01000000,
    msg_timed_out          = 0x02000000,
    msg_rejected           = 0x03000000,
    msg_training_completed = 0x04000000
} msgkey_t;

/**
 * 5 B in header
 */
struct __attribute__((__packed__)) header {
    uint8_t type;                   // 1
    uint8_t count;                  // 2
    uint8_t samples_per_second;     // 3
    uint8_t sample_size;            // 4
    uint8_t device_id;              // 5
    /// the sequence counter (being uint8_t, it means that we fail to spot errors that are multiples of 256 errors apart)
    uint8_t  sequence_number;       // 6
    uint64_t timestamp;             // 13
    uint16_t duration;              // 15
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

///
/// Send a simple message with the key & value
///
void am_send_simple(const msgkey_t key, const uint8_t value);

#ifdef __cplusplus
}
#endif
