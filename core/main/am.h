#pragma once
#include "ad.h"
#include "fixed_queue.h"
#include "debug_macros.h"

#define DL_TAG 0x0fb0

#ifdef __cplusplus
extern "C" {
#endif

ad_sample_callback_t am_start();
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
