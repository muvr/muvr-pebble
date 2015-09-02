#include <pebble.h>
#include "../core/main/ad.h"
#include "../core/main/am.h"
#include "resistance_exercise_layer.h"

static struct {
    Window *rex_window;
    bool exercising;
    bool training;
    message_callback_t message_callback;
} main_ctx;

static void start(void *data) {
    ad_start(main_ctx.message_callback, AD_SAMPLING_50HZ, 2050);
    rex_not_moving();

    main_ctx.exercising = true;
    if (main_ctx.training) rex_exercising();
}

static void accepted(const uint8_t index) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "accepted(%d)", index);
    am_send_simple(0xb0000003, index);
    app_timer_register(1500, start, NULL);

    main_ctx.exercising = true;
    if (main_ctx.training) rex_exercising();
}

static void timed_out(const uint8_t index) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "timed_out(%d)", index);
    am_send_simple(0xb1000003, index);
    app_timer_register(1500, start, NULL);

    main_ctx.exercising = true;
    if (main_ctx.training) rex_exercising();
}

static void rejected(void) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "rejected()");
    am_send_simple(0xb2000003, 0);
    app_timer_register(1500, start, NULL);

    main_ctx.exercising = true;
    if (main_ctx.training) rex_exercising();
}

static void app_message_received(DictionaryIterator *iterator, void *context) {
    Tuple *t = dict_read_first(iterator);
    while (t != NULL) {
        switch (t->key) {
            case 0xa0000000: // not moving
                rex_not_moving();
                return;
            case 0xa0000001: // moving
                rex_moving();
                return;
            case 0xa0000002: // exercising
                rex_exercising();
                return;
            case 0xa0000003: // classified
                ad_stop();
                vibes_double_pulse();
                resistance_exercise_t *res = (resistance_exercise_t*)t->value->data;
                uint8_t count = t->length / sizeof(resistance_exercise_t);

                rex_classification_completed(res, count, accepted, timed_out, rejected);
                return;
            case 0xa0000004: // next up
            	rex_set_current((resistance_exercise_t*)t->value->data);
            	return;

            case 0xa0000005: // classifying mode
                main_ctx.training = false;
                accel_tap_service_unsubscribe();
                start(NULL);
        }
        t = dict_read_next(iterator);
    }
}

static void tap_handler(AccelAxisType axis, int32_t direction) {
    vibes_double_pulse();

    if (main_ctx.exercising) {
        // we are exercising; now's the time to stop
        APP_LOG(APP_LOG_LEVEL_DEBUG, "Stopping exercise.");
        ad_stop();
        rex_not_moving();
        am_send_simple(0xb3000003, 0);
        main_ctx.exercising = false;
    } else {
        APP_LOG(APP_LOG_LEVEL_DEBUG, "Starting exercise.");
        // we are not exercising; display suggestions.
        resistance_exercise_t res[] = {
                {.name = "Bicep curl", .confidence = 1, .repetitions = UNK_REPETITIONS, .weight = UNK_WEIGHT, .intensity = UNK_INTENSITY},
                {.name = "Chest fly", .confidence = 1, .repetitions = UNK_REPETITIONS, .weight = UNK_WEIGHT, .intensity = UNK_INTENSITY},
                {.name = "Shoulder press", .confidence = 1, .repetitions = UNK_REPETITIONS, .weight = UNK_WEIGHT, .intensity = UNK_INTENSITY}
        };
        uint8_t count = 3;

        rex_classification_completed(res, count, accepted, timed_out, rejected);
    }
}

static void init(void) {
    accel_tap_service_subscribe(tap_handler);
    main_ctx.rex_window = rex_init();
    window_stack_push(main_ctx.rex_window, true);
    app_message_register_inbox_received(app_message_received);

    main_ctx.exercising = false;
    main_ctx.training = true;

    main_ctx.message_callback = am_start(0xad, 50, 5);
    
#if 0
    resistance_exercise_t x[] = {
       {.name = "Bicep curl",   .repetitions = 10, .weight = 20},
       {.name = "Tricep extension", .repetitions = 10, .weight = 25}
    };
    rex_set_current(x);
    //rex_classification_completed(x, 2, accepted, timed_out, rejected);
#else
    //start(NULL);
#endif
}

static void deinit(void) {
    ad_stop();
    am_stop();

    accel_tap_service_unsubscribe();
    window_destroy(main_ctx.rex_window);
}

int main(void) {
    init();
    app_event_loop();
    deinit();
}
