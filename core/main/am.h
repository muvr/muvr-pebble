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
    uint8_t  type;                   // 1
    uint8_t  count;                  // 2
    uint8_t  samples_per_second;     // 3
    uint8_t  sample_size;            // 4
    uint8_t time_offset;             // 5
};

message_callback_t am_start(uint8_t ty);
void am_stop();
int am_count();
uint32_t am_tag();
int am_last_error();
char* am_last_error_text();
int am_last_error_distance();
int am_error_count();
size_t am_queue_length();

#ifdef __cplusplus
}
#endif
