#include "pebble.h"
#include "resistance_exercise_layer.h"

#define RESISTANCE_EXERCISES_MAX 8
#define TIMER_MS 75
#define COUNTER_ZERO 110

#define MIN(a, b) ((a < b) ? a : b)

static struct {
    Window *window;
    GBitmap *bitmap;
    GBitmap *action_up_bitmap;
    GBitmap *action_down_bitmap;
    GBitmap *action_select_bitmap;
    Layer *text_layer;
    ActionBarLayer *action_bar;
} ui;

static struct {
    resistance_exercise_t exercises[RESISTANCE_EXERCISES_MAX];
    uint8_t count;
} resistance_exercises;

static struct {
    classification_accepted_callback_t accepted;
    classification_rejected_callback_t rejected;
    classification_timedout_callback_t timed_out;
} callbacks;

static struct {
    AppTimer *timer;
    uint8_t index;
    uint8_t counter;
} selection;

/// tidies up the display window: removes all counters and hides the action bar
static void zero() {
    callbacks.accepted = NULL;
    callbacks.rejected = NULL;
    callbacks.timed_out = NULL;
    selection.index = selection.counter = 0;
    resistance_exercises.count = 0;
    if (selection.timer != NULL) app_timer_cancel(selection.timer);
    selection.timer = NULL;
    action_bar_layer_remove_from_window(ui.action_bar);
}

/// sets the main bitmap to be displayed
static void load_and_set_bitmap(uint32_t resource_id) {
    if (ui.bitmap != NULL) {
        gbitmap_destroy(ui.bitmap);
        ui.bitmap = NULL;
    }

    if (resource_id != 0) ui.bitmap = gbitmap_create_with_resource(resource_id);

    layer_mark_dirty(ui.text_layer);
}

static void accept_once(const uint8_t index) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "accept_once");
    if (callbacks.accepted != NULL) {
        load_and_set_bitmap(RESOURCE_ID_THUMBSUP);
        callbacks.accepted(index);
    }
    zero();
}

static void reject_once(void) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "reject_once");
    if (callbacks.rejected != NULL) {
        load_and_set_bitmap(RESOURCE_ID_THUMBSDOWN);
        callbacks.rejected();
    }
    zero();
}

static void timed_out_once(const uint8_t index) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "timed_out_once");
    if (callbacks.timed_out != NULL) {
        load_and_set_bitmap(RESOURCE_ID_THUMBSUP);
        callbacks.timed_out(index);
    }
    zero();
}

static void draw_progress_bar(GContext *context, uint8_t y, uint8_t counter) {
    GPoint start = {.x = 5, .y = y};
    GPoint end   = {.x = start.x + selection.counter, .y = y};
    graphics_context_set_stroke_color(context, GColorBlack);
    graphics_draw_line(context, start, end);

}

static void text_layer_update_callback(Layer *layer, GContext *context) {
    if (resistance_exercises.count > 0) {
        resistance_exercise_t re = resistance_exercises.exercises[selection.index];
        graphics_context_set_text_color(context, GColorBlack);
        GRect bounds = layer_get_frame(layer);

        // the name
        GFont *exercise_font = fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD);
        GRect exercise_rect = GRect(5, 5, bounds.size.w - 10, 100);
        char *exercise_text = re.name;
        //GSize exercise_cs = graphics_text_layout_get_content_size(exercise_text, exercise_font, exercise_rect, GTextOverflowModeWordWrap, GTextAlignmentLeft);
        graphics_draw_text(context, exercise_text, exercise_font, exercise_rect, GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);

        // the details
        char header_text[30];
        char x[10];
        if (re.repetitions > 0) snprintf(x, 10, "%d reps\n", re.repetitions); else strcpy(x, "\n");
        strcpy(header_text, x);

        if (re.weight > 0) snprintf(x, 10, "%d kg", re.weight);
        strcat(header_text, x);
        graphics_draw_text(context, header_text, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD), GRect(5, 80, bounds.size.w - 10, 100), GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);

        // counter indicator
        uint8_t y = 75;
        draw_progress_bar(context, y, selection.counter);
        draw_progress_bar(context, y + 1, selection.counter);
        draw_progress_bar(context, y + 2, selection.counter);

    }

    if (ui.bitmap != NULL) {
        graphics_context_set_compositing_mode(context, GCompOpClear);
        graphics_draw_bitmap_in_rect(context, ui.bitmap, GRect(10, 40, 120, 75));
    }
}

static void down_single_click_handler(ClickRecognizerRef recognizer, void *context) {
    selection.counter = COUNTER_ZERO;
    if (selection.index < resistance_exercises.count - 1) selection.index++;
    layer_mark_dirty(ui.text_layer);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "selection.index = %d", selection.index);
}

static void up_single_click_handler(ClickRecognizerRef recognizer, void *context) {
    selection.counter = COUNTER_ZERO;
    if (selection.index > 0) selection.index--;
    layer_mark_dirty(ui.text_layer);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "selection.index = %d", selection.index);
}

static void select_single_click_handler(ClickRecognizerRef recognizer, void *context) {
    accept_once(selection.index);
}

static void back_single_click_handler(ClickRecognizerRef recognizer, void *context) {
    reject_once();
}

static void click_config_provider(Window *window) {
    // single click / repeat-on-hold config:
    window_single_click_subscribe(BUTTON_ID_DOWN,   down_single_click_handler);
    window_single_click_subscribe(BUTTON_ID_UP,     up_single_click_handler);
    window_single_click_subscribe(BUTTON_ID_SELECT, select_single_click_handler);
    window_single_click_subscribe(BUTTON_ID_BACK,   back_single_click_handler);

    // long click config:
    // window_long_click_subscribe(BUTTON_ID_DOWN, 700, select_long_click_handler, select_long_click_release_handler);
    // window_long_click_subscribe(BUTTON_ID_UP,   700, select_long_click_handler, select_long_click_release_handler);
}

static void window_load(Window *window) {
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);

    ui.action_bar = action_bar_layer_create();
    ui.text_layer = layer_create(bounds);
    layer_set_update_proc(ui.text_layer, text_layer_update_callback);

    layer_add_child(window_layer, ui.text_layer);

    action_bar_layer_set_background_color(ui.action_bar, GColorBlack);
    action_bar_layer_set_click_config_provider(ui.action_bar, (ClickConfigProvider)click_config_provider);

    // set the action bar bitmaps
    ui.action_up_bitmap = gbitmap_create_with_resource(RESOURCE_ID_UP);
    ui.action_select_bitmap = gbitmap_create_with_resource(RESOURCE_ID_SELECT);
    ui.action_down_bitmap = gbitmap_create_with_resource(RESOURCE_ID_DOWN);
    action_bar_layer_set_icon(ui.action_bar, BUTTON_ID_UP, ui.action_up_bitmap);
    action_bar_layer_set_icon(ui.action_bar, BUTTON_ID_SELECT, ui.action_select_bitmap);
    action_bar_layer_set_icon(ui.action_bar, BUTTON_ID_DOWN, ui.action_down_bitmap);

    load_and_set_bitmap(RESOURCE_ID_NOTMOVING);
}

static void window_unload(Window *window) {
    layer_destroy(ui.text_layer);
    if (ui.bitmap != NULL) gbitmap_destroy(ui.bitmap);
    gbitmap_destroy(ui.action_up_bitmap);
    gbitmap_destroy(ui.action_select_bitmap);
    gbitmap_destroy(ui.action_down_bitmap);
}

Window* rex_init(void) {
    zero();
    ui.bitmap = NULL;
    ui.window = window_create();
    window_set_window_handlers(ui.window, (WindowHandlers) {
            .load = window_load,
            .unload = window_unload
    });

#ifdef PBL_COLOR
    window_set_background_color(ui.window, GColorWhite);
#else
    window_set_background_color(ui.window, GColorWhite);
#endif

    return ui.window;
}

static void timer_callback(void *data) {
    selection.counter--;
    if (selection.counter == 0) {
        // we have reached countdown without making a selection
        timed_out_once(selection.index);
        resistance_exercises.count = 0;
    } else {
        selection.timer = app_timer_register(TIMER_MS, timer_callback, NULL);
    }
    layer_mark_dirty(ui.text_layer);
}

void rex_classification_completed(resistance_exercise_t *exercises, uint8_t count,
                                  classification_accepted_callback_t accepted,
                                  classification_timedout_callback_t timed_out,
                                  classification_rejected_callback_t rejected) {
    if (count == 0) {
        rejected();
    } else {
        action_bar_layer_add_to_window(ui.action_bar, ui.window);
        callbacks.accepted = accepted;
        callbacks.timed_out = timed_out;
        callbacks.rejected = rejected;

        resistance_exercises.count = MIN(RESISTANCE_EXERCISES_MAX, count);
        memcpy(resistance_exercises.exercises, exercises, resistance_exercises.count * sizeof(resistance_exercise_t));
        selection.counter = COUNTER_ZERO;
        selection.timer = app_timer_register(TIMER_MS, timer_callback, NULL);

        APP_LOG(APP_LOG_LEVEL_DEBUG, "r_e.count = %d", resistance_exercises.count);

        load_and_set_bitmap(0);
        layer_mark_dirty(ui.text_layer);
    }
}

void rex_moving(void) {
    load_and_set_bitmap(RESOURCE_ID_MOVING);
    layer_mark_dirty(ui.text_layer);
}

void rex_exercising(void) {
    load_and_set_bitmap(RESOURCE_ID_EXERCISING);
    layer_mark_dirty(ui.text_layer);
}

void rex_not_moving(void) {
    load_and_set_bitmap(RESOURCE_ID_NOTMOVING);
    layer_mark_dirty(ui.text_layer);
}
