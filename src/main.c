#include <pebble.h>
#include "../core/main/ad.h"
#include "../core/main/am.h"

#include "main_window.h"

static void app_message_received(DictionaryIterator *iterator, void *context) {
    Tuple *t = dict_read_first(iterator);
    while (t != NULL) {
        APP_LOG(APP_LOG_LEVEL_DEBUG, "Received %lx", t->key);
        switch (t->key) {
            case 0xa0000000: // notify-not-moving
                main_window_set_text("...");
                break;
            case 0xa0000001: // notify-moving
                main_window_set_text("Moving");
                break;
            case 0xa0000002: // notify-exercising
                main_window_set_text("Exercising");
                break;
            case 0xa0000003: // classification completed
                main_window_set_text("** Done **");
                break;
            case 0xa0000004: // notify simple current
                main_window_set_text("** Done **");

                break;
            case 0xb0000000: {
                // start recording
                message_callback_t message_callback = am_start(0x516c6174, 50, sizeof(struct threed_data));
                ad_start(message_callback, 50, 1000);
                main_window_set_text("Ready");
                }
                break;
            case 0xb0000001: // stop recording
                main_window_set_text("Stopped");
                break;
            default:
                main_window_set_text("???");
                break;
        }
        t = dict_read_next(iterator);
    }
}

static void init(void) {
    main_window_init();
    app_message_open(APP_MESSAGE_INBOX_SIZE_MINIMUM, APP_MESSAGE_OUTBOX_SIZE);
    app_message_register_inbox_received(app_message_received);
}

static void deinit(void) {
    ad_stop();
    am_stop();
    app_message_deregister_callbacks();

    main_window_deinit();
}

int main(void) {
    init();
    app_event_loop();
    deinit();
}
