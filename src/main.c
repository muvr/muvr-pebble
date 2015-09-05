#include <pebble.h>
#include "../core/main/ad.h"
#include "../core/main/am.h"
#include "resistance_exercise_layer.h"

enum mode {
    //  tap, select exercise, record data, tap to end;
    mode_training = 0,
    // tap, exercise, tap, confirm classification results;
    mode_assisted_classification = 1,
    // move / exercise, upon completing exercise, confirm classification results.
    mode_automatic_classification = 2
};

static struct {
    Window *rex_window;
    bool exercising;
    enum mode mode;
    message_callback_t message_callback;
} main_ctx;

static void start() {
    ad_start(main_ctx.message_callback, AD_SAMPLING_50HZ, 2050);
    rex_not_moving();

    main_ctx.exercising = true;
    if (main_ctx.mode != mode_automatic_classification) rex_exercising();
}

static void resume(void *data) {
    switch (main_ctx.mode) {
        case mode_automatic_classification: start(); break;
        case mode_training: main_ctx.exercising = true; rex_exercising(); break;
        case mode_assisted_classification: main_ctx.exercising = false; rex_not_moving(); break;
    }
}

static void accepted(const uint8_t index) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "accepted(%d)", index);
    am_send_simple(msg_accepted, index);
    app_timer_register(1500, resume, NULL);

    main_ctx.exercising = true;
    if (main_ctx.mode == mode_training) rex_exercising();
}

static void timed_out(const uint8_t index) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "timed_out(%d)", index);
    am_send_simple(msg_timed_out, index);
    app_timer_register(1500, resume, NULL);

    main_ctx.exercising = true;
    if (main_ctx.mode == mode_training) rex_exercising();
}

static void rejected(void) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "rejected()");
    am_send_simple(msg_rejected, 0);
    app_timer_register(1500, resume, NULL);

    main_ctx.exercising = true;
    if (main_ctx.mode == mode_training) rex_exercising();
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

            case 0xb0000000:
                main_ctx.mode = mode_training;
                return;
            case 0xb0000001:
                main_ctx.mode = mode_assisted_classification;
                return;
            case 0xb0000002:
                main_ctx.mode = mode_automatic_classification;
                accel_tap_service_unsubscribe();
                start();
                return;

        }
        t = dict_read_next(iterator);
    }
}

/**
 * The tap behaviour depends on the mode and the state in main_ctx:
 *
 * - if mode == mode_automatic_classification, do nothing
 * - if mode == mode_training && exercising: stop exercise, send training_completed
 * - if mode == mode_training && !exercising: show exercise selection
 * - if mode == mode_assisted_classificatoin && exercising: stop exercise, send exercise_completed
 * - if mode == mode_assisted_classification && !exercising: start exercise
 *
 */
static void tap_handler(AccelAxisType axis, int32_t direction) {
    if (main_ctx.mode == mode_automatic_classification) return;

    vibes_double_pulse();

    if (main_ctx.exercising) {
        // we are exercising; now's the time to stop
        APP_LOG(APP_LOG_LEVEL_DEBUG, "Stopping exercise.");
        ad_stop();
        rex_not_moving();

        if (main_ctx.mode == mode_assisted_classification) {
            am_send_simple(msg_exercise_completed, 0);
        } else if (main_ctx.mode == mode_training) {
            am_send_simple(msg_training_completed, 0);
        }
        main_ctx.exercising = false;
    } else {
        APP_LOG(APP_LOG_LEVEL_DEBUG, "Starting exercise.");
        if (main_ctx.mode == mode_training) {
            // we are not exercising; display suggestions.
            resistance_exercise_t res[] = {
                    {.name = "Lateral rise", .confidence = 1, .repetitions = UNK_REPETITIONS, .weight = UNK_WEIGHT, .intensity = UNK_INTENSITY},
                    {.name = "Triceps extension", .confidence = 1, .repetitions = UNK_REPETITIONS, .weight = UNK_WEIGHT, .intensity = UNK_INTENSITY},
                    {.name = "Biceps curl", .confidence = 1, .repetitions = UNK_REPETITIONS, .weight = UNK_WEIGHT, .intensity = UNK_INTENSITY}
            };
            uint8_t count = 3;

            rex_classification_completed(res, count, accepted, timed_out, rejected);
        } else if (main_ctx.mode == mode_assisted_classification) {
            start();
        }
    }
}

static void init(void) {
    accel_tap_service_subscribe(tap_handler);
    main_ctx.rex_window = rex_init();
    window_stack_push(main_ctx.rex_window, true);
    app_message_register_inbox_received(app_message_received);

    main_ctx.exercising = false;
    main_ctx.mode = mode_assisted_classification;

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
