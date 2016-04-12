#pragma once
#include <stdint.h>
#include "m.h"

#ifdef __cplusplus
extern "C" {
#endif

#define APP_MESSAGE_OUTBOX_SIZE 720

typedef enum {
    msg_dead               = 0xdead0000,
    msg_ad                 = 0xad000000,
    msg_accepted           = 0x01000000,
    msg_timed_out          = 0x02000000,
    msg_rejected           = 0x03000000,
    msg_training_completed = 0x04000000,
    msg_exercise_completed = 0x05000000
} msgkey_t;

/**
 * 5 B in header
 */
struct __attribute__((__packed__)) header {
    uint8_t preamble1;              // 1
    uint8_t preamble2;              // 2
    uint8_t types_count;            // 3
    uint8_t samples_per_second;     // 4
    double timestamp;               // 12
    uint32_t count;                 // 16
    // Types
    uint32_t type;                  // 20
};

///
/// Sets up App Messages BLE communication, returning a ``message_callback_t`` value,
/// which prepends the ``struct header`` above ahead of the samples passed.
/// The ``type``, ``samples_per_second``, and ``sample_size`` will be set in the header.
/// - parameter type the type ???
/// - parameter samples_per_second the actual number of samples per second
/// - parameter sample_size the size in B of one sample
///
message_callback_t am_start(uint32_t type, uint8_t samples_per_second, uint8_t sample_size);

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
