#include <pebble.h>
#include "../core/main/ad.h"
#include "../core/main/am.h"
#include "resistance_exercise_layer.h"

static void click_config_provider(void *context);
static void back_click_handler(ClickRecognizerRef recognizer, void *context);

static struct {
    Window *rex_window;
    bool exercising;
    message_callback_t message_callback;
    time_t vibes_start_time;
} main_ctx;

static void safe_vibes_double_pulse(void) {
    time(&main_ctx.vibes_start_time);
    vibes_double_pulse();
}

static void start(void) {
    ad_start(main_ctx.message_callback, AD_SAMPLING_50HZ, 1050);

    main_ctx.exercising = true;
}

static void resume(void *data) {
    start();
}

static void app_message_received(DictionaryIterator *iterator, void *context) {
    Tuple *t = dict_read_first(iterator);
    while (t != NULL) {
        APP_LOG(APP_LOG_LEVEL_DEBUG, "Received %lx", t->key);
        switch (t->key) {
            case 0xa0000000: // notify-not-moving
                rex_not_moving();
                return;
            case 0xa0000001: // notify-moving
                rex_moving();
                return;
            case 0xa0000002: // notify-exercising
                rex_exercising();
                return;
            case 0xa0000003: // classification completed
                ad_stop();
                safe_vibes_double_pulse();
                return;
            case 0xa0000004: // notify simple current
                rex_set_current((resistance_exercise_t*)t->value->data);
                return;
            case 0xb0000000: // start recording
                rex_moving();
                start();
                return;
            case 0xb0000001: // stop recording
                main_ctx.exercising = false;
                rex_empty();
                ad_stop();
                return;
        }
        t = dict_read_next(iterator);
    }
}

/**
 * The back button behavior depends on the mode and the state in main_ctx:
 */
static void back_click_handler(ClickRecognizerRef recognizer, void *context) {

    if (main_ctx.exercising) {
        safe_vibes_double_pulse();
        // we are exercising; now's the time to stop
        APP_LOG(APP_LOG_LEVEL_DEBUG, "Stopping exercise.");
        ad_stop();
        rex_empty();

        am_send_simple(msg_training_completed, 0);
        main_ctx.exercising = false;
    } else {
        APP_LOG(APP_LOG_LEVEL_DEBUG, "Doing nothing. Already as back as we get.");
    }
}

static void rex_dismissed(void) {
    window_set_click_config_provider(main_ctx.rex_window, click_config_provider);
}

static void click_config_provider(void *context) {
    // Register the ClickHandlers
    window_single_click_subscribe(BUTTON_ID_BACK, back_click_handler);
}

static void init(void) {
    main_ctx.rex_window = rex_init(rex_dismissed);
    window_set_click_config_provider(main_ctx.rex_window, click_config_provider);
    window_stack_push(main_ctx.rex_window, true);
    app_message_register_inbox_received(app_message_received);

    main_ctx.exercising = false;
    main_ctx.vibes_start_time = 0;

    main_ctx.message_callback = am_start(0x516c6174, 50, 5);
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
