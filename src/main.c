#include "pebble.h"
#include "../core/main/ad.h"
#include "../core/main/am.h"
#include "resistance_exercise_layer.h"

static struct {
    Window *rex_window;
    message_callback_t message_callback;
} main_ctx;

static void accepted(const uint8_t index) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "accepted(%d)", index);
    ad_start(main_ctx.message_callback, AD_SAMPLING_50HZ, 2050);
    rex_not_moving();
}

static void rejected(void) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "rejected()");
    ad_start(main_ctx.message_callback, AD_SAMPLING_50HZ, 2050);
    rex_not_moving();
}

static void timed_out(const uint8_t index) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "timed_out(%d)", index);
    ad_start(main_ctx.message_callback, AD_SAMPLING_50HZ, 2050);
    rex_not_moving();
}

static void init(void) {
    main_ctx.rex_window = rex_init();
    window_stack_push(main_ctx.rex_window, true);

    main_ctx.message_callback = am_start(0xad, 50, 5);

    //ad_start(main_ctx.message_callback, AD_SAMPLING_50HZ, 2050);
#if 1
    resistance_exercise_t x[] = {
       {.name = "Bicep curl",   .repetitions = 10, .weight = 20},
       {.name = "Tricep press", .repetitions = 10, .weight = 25}
    };
    rex_classification_completed(x, 2, accepted, timed_out, rejected);
#endif
}

static void deinit(void) {
    ad_stop();
    am_stop();

    window_destroy(main_ctx.rex_window);
}

int main(void) {
    init();
    app_event_loop();
    deinit();
}
