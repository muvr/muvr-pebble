#include "pebble.h"
#include "resistance_exercise_layer.h"

static struct {
    Window *window;
    GBitmap *bitmap;
    GBitmap *action_up_bitmap;
    GBitmap *action_down_bitmap;
    GBitmap *action_select_bitmap;
    GBitmap *arrow;
    Layer *text_layer;
    BitmapLayer *bitmap_layer;
    ActionBarLayer *action_bar;
    resistance_exercise_t *current_exercise;
    screen_back_callback_t dismissed;
    char show_help[10];
} ui;

/// tidies up the display window: removes all counters and hides the action bar
static void zero() {
    ui.current_exercise = NULL;
    strcpy(ui.show_help, "");
    action_bar_layer_remove_from_window(ui.action_bar);
    if (ui.dismissed != NULL) ui.dismissed();
}

/// sets the main bitmap to be displayed
static void load_and_set_bitmap(uint32_t resource_id) {
    if (ui.bitmap != NULL) {
        gbitmap_destroy(ui.bitmap);
        ui.bitmap = NULL;
    }
    GBitmap *bitmap = NULL;
    if (resource_id != 0) bitmap = gbitmap_create_with_resource(resource_id);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "bitmap=%p", bitmap);
    ui.bitmap = bitmap;

    layer_mark_dirty(ui.text_layer);
}

static void text_layer_update_callback(Layer *layer, GContext *context) {
    // -- Display main image
    graphics_context_set_text_color(context, GColorWhite);
    if (ui.bitmap != NULL) {
        bitmap_layer_set_bitmap(ui.bitmap_layer, ui.bitmap);
        bitmap_layer_set_compositing_mode(ui.bitmap_layer, GCompOpAssign);
    }

    // -- Display UI help text, e.g. "Stop Session" or "Idle"
    char* ui_text;
    if(strlen(ui.show_help) > 0) {
        graphics_context_set_compositing_mode(context, GCompOpAssign);
        graphics_draw_bitmap_in_rect(context, ui.arrow, GRect(0, 10, 70, 34));
        ui_text = ui.show_help;
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

    // -- Display current exercise information
    if (ui.current_exercise != NULL) {
        graphics_context_set_compositing_mode(context, GCompOpAssign);

        // Display the exercise name at the bottom of the screen
        GFont exercise_font = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
        GRect exercise_rect = GRect(5, 142, bounds.size.w - 10, 100);
        char *exercise_text = ui.current_exercise->name;
        graphics_draw_text(context, exercise_text, exercise_font, exercise_rect, GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
    }
}

static void window_load(Window *window) {
    Layer *window_layer = window_get_root_layer(window);
    StatusBarLayer *s_status_bar = status_bar_layer_create();
    GRect bounds = layer_get_bounds(window_layer);

    ui.action_bar = action_bar_layer_create();
    ui.text_layer = layer_create(bounds);
    ui.bitmap_layer = bitmap_layer_create(GRect(10, 70, 120, 75));

    layer_set_update_proc(ui.text_layer, text_layer_update_callback);

    layer_add_child(window_layer, ui.text_layer);
    layer_add_child(window_layer, bitmap_layer_get_layer(ui.bitmap_layer));

    action_bar_layer_set_background_color(ui.action_bar, GColorWhite);

    // set the action bar bitmaps
    ui.action_up_bitmap = gbitmap_create_with_resource(RESOURCE_ID_UP);
    ui.action_select_bitmap = gbitmap_create_with_resource(RESOURCE_ID_SELECT);
    ui.action_down_bitmap = gbitmap_create_with_resource(RESOURCE_ID_DOWN);
    ui.arrow = gbitmap_create_with_resource(RESOURCE_ID_LEFTARROW);
    action_bar_layer_set_icon(ui.action_bar, BUTTON_ID_UP, ui.action_up_bitmap);
    action_bar_layer_set_icon(ui.action_bar, BUTTON_ID_SELECT, ui.action_select_bitmap);
    action_bar_layer_set_icon(ui.action_bar, BUTTON_ID_DOWN, ui.action_down_bitmap);
    layer_add_child(window_layer, status_bar_layer_get_layer(s_status_bar));
    // load_and_set_bitmap(RESOURCE_ID_NOTMOVING);
}

static void window_unload(Window *window) {
    layer_destroy(ui.text_layer);
    bitmap_layer_destroy(ui.bitmap_layer);
    if (ui.bitmap != NULL) gbitmap_destroy(ui.bitmap);
    gbitmap_destroy(ui.action_up_bitmap);
    gbitmap_destroy(ui.action_select_bitmap);
    gbitmap_destroy(ui.action_down_bitmap);
    gbitmap_destroy(ui.arrow);
}

Window* rex_init(screen_back_callback_t dismissed) {
    ui.dismissed = NULL;
    zero();
    ui.bitmap = NULL;
    ui.window = window_create();
    window_set_window_handlers(ui.window, (WindowHandlers) {
            .load = window_load,
            .unload = window_unload
    });
    ui.dismissed = dismissed;

    window_set_background_color(ui.window, GColorBlack);

    return ui.window;
}

void rex_set_current(resistance_exercise_t *exercise) {
    if (exercise == NULL) {
        ui.current_exercise = NULL;
    }
    static resistance_exercise_t current;
    memcpy(&current, exercise, sizeof(resistance_exercise_t));
    ui.current_exercise = &current;
    layer_mark_dirty(ui.text_layer);
}

void rex_empty(void) {
    zero();
    load_and_set_bitmap(0);
}

void rex_moving(void) {
    strcpy(ui.show_help, "Stop Session");
    load_and_set_bitmap(RESOURCE_ID_MOVING);
}

void rex_exercising(void) {
    strcpy(ui.show_help , "Stop Session");
    load_and_set_bitmap(RESOURCE_ID_EXERCISING);
}

void rex_not_moving(void) {
    strcpy(ui.show_help, "Stop Session");
    load_and_set_bitmap(RESOURCE_ID_NOTMOVING);
}
