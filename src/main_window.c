#include "main_window.h"

#define TEXT_LAYER_TEXT_LEN 10

static Window* main_window;
static Layer *text_layer;
static char text_layer_text[TEXT_LAYER_TEXT_LEN];

static void text_layer_update_callback(Layer *layer, GContext *context) {
    // -- Display main image
    graphics_context_set_text_color(context, GColorWhite);

    // -- Display UI help text, e.g. "Stop Session" or "Idle"
    char* ui_text;
    if(strlen(text_layer_text) > 0) {
        ui_text = text_layer_text;
    } else {
        ui_text = "Idle";
    }

    GRect bounds = layer_get_frame(layer);
    graphics_draw_text(context,
                       ui_text,
                       fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD),
                       GRect(5, 40, bounds.size.w - 10, 100),
                       GTextOverflowModeWordWrap,
                       GTextAlignmentCenter,
                       NULL);

}

static void window_load(Window *window) {
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);

    text_layer = layer_create(bounds);

    layer_set_update_proc(text_layer, text_layer_update_callback);
    layer_add_child(window_layer, text_layer);
}

static void window_unload(Window *window) {
    layer_destroy(text_layer);
}

void main_window_init(void) {
    if (main_window != NULL) {
        APP_LOG(APP_LOG_LEVEL_ERROR, "main_window_init() already called. Ignoring.");
        return;
    }
    main_window = window_create();
    window_set_window_handlers(main_window, (WindowHandlers) {
            .load = window_load,
            .unload = window_unload
    });

    window_set_background_color(main_window, GColorBlue);
    window_stack_push(main_window, true);
}

void main_window_deinit(void) {
    if (main_window == NULL) {
        APP_LOG(APP_LOG_LEVEL_ERROR, "main_window_deinit() called before main_window_init(). Ignoring");
        return;
    }

    window_destroy(main_window);
}

void main_window_set_text(char* text) {
    strncpy(text_layer_text, text, TEXT_LAYER_TEXT_LEN);
    layer_mark_dirty(text_layer);
}
