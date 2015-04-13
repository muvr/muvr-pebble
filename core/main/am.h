#pragma once
#include "fixed_queue.h"
#include "debug_macros.h"
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
    uint8_t time_offset;            // 5
};

message_callback_t am_start(uint8_t type, uint8_t samples_per_second, uint8_t sample_size);
void am_stop();
void am_get_status(char **text, size_t max_size);

#ifdef __cplusplus
}
#endif
