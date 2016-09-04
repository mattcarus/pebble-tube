#include "pebble.h"

static Window *s_main_window;

static TextLayer *s_name_layer;
static TextLayer *s_zone_layer;
static TextLayer *s_distance_layer;
static TextLayer *s_direction_layer;

static AppSync s_sync;
static uint8_t s_sync_buffer[64];

enum WeatherKey {
  TUBE_STATION_NAME_KEY = 0x0,         // TUPLE_CSTRING
  TUBE_STATION_ZONE_KEY = 0x1,         // TUPLE_CSTRING
  TUBE_STATION_DISTANCE_KEY = 0x2,     // TUPLE_CSTRING
  TUBE_STATION_DIRECTION_KEY = 0x3,    // TUPLE_CSTRING
};

static void sync_error_callback(DictionaryResult dict_error, AppMessageResult app_message_error, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Sync Error: %d", app_message_error);
}

static void sync_tuple_changed_callback(const uint32_t key, const Tuple* new_tuple, const Tuple* old_tuple, void* context) {
  switch (key) {
    case TUBE_STATION_NAME_KEY:
      // App Sync keeps new_tuple in s_sync_buffer, so we may use it directly
      text_layer_set_text(s_name_layer, new_tuple->value->cstring);
      break;

    case TUBE_STATION_ZONE_KEY:
      // App Sync keeps new_tuple in s_sync_buffer, so we may use it directly
      text_layer_set_text(s_zone_layer, new_tuple->value->cstring);
      break;

    case TUBE_STATION_DISTANCE_KEY:
      text_layer_set_text(s_distance_layer, new_tuple->value->cstring);
      break;

    case TUBE_STATION_DIRECTION_KEY:
      text_layer_set_text(s_direction_layer, new_tuple->value->cstring);
      break;
  }
}

static void request_nearest_tube_station(void) {
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);

  if (!iter) {
    // Error creating outbound message
    return;
  }

  int value = 1;
  dict_write_int(iter, 1, &value, sizeof(int), true);
  dict_write_end(iter);

  app_message_outbox_send();
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_name_layer = text_layer_create(GRect(bounds.size.w * 0.2, 20, bounds.size.w * 0.6, 32));
  text_layer_set_text_color(s_name_layer, GColorWhite);
  text_layer_set_background_color(s_name_layer, GColorClear);
  text_layer_set_font(s_name_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(s_name_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_name_layer));

  s_zone_layer = text_layer_create(GRect(bounds.size.w * 0.2, 132, bounds.size.w * 0.6, 64));
  text_layer_set_text_color(s_zone_layer, GColorWhite);
  text_layer_set_background_color(s_zone_layer, GColorClear);
  text_layer_set_font(s_zone_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(s_zone_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_zone_layer));

  s_distance_layer = text_layer_create(GRect(0, 75, bounds.size.w * 0.45, 32));
  text_layer_set_text_color(s_distance_layer, GColorWhite);
  text_layer_set_background_color(s_distance_layer, GColorClear);
  text_layer_set_font(s_distance_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_text_alignment(s_distance_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_distance_layer));

  s_direction_layer = text_layer_create(GRect(bounds.size.w * 0.55, 75, bounds.size.w * 0.45, 32));
  text_layer_set_text_color(s_direction_layer, GColorWhite);
  text_layer_set_background_color(s_direction_layer, GColorClear);
  text_layer_set_font(s_direction_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_text_alignment(s_direction_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_direction_layer));

  Tuplet initial_values[] = {
    TupletCString(TUBE_STATION_NAME_KEY, "No Station"),
    TupletCString(TUBE_STATION_ZONE_KEY, "Zone: none"),
    TupletCString(TUBE_STATION_DISTANCE_KEY, "9999m"),
    TupletCString(TUBE_STATION_DIRECTION_KEY, "360\u00B0"),
  };

  app_sync_init(&s_sync, s_sync_buffer, sizeof(s_sync_buffer),
      initial_values, ARRAY_LENGTH(initial_values),
      sync_tuple_changed_callback, sync_error_callback, NULL);

  request_nearest_tube_station();
}

static void window_unload(Window *window) {
  text_layer_destroy(s_name_layer);
  text_layer_destroy(s_zone_layer);
  text_layer_destroy(s_distance_layer);
  text_layer_destroy(s_direction_layer);
}

static void init(void) {
  s_main_window = window_create();
  window_set_background_color(s_main_window, PBL_IF_COLOR_ELSE(GColorIndigo, GColorBlack));
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload
  });
  window_stack_push(s_main_window, true);

  app_message_open(64, 64);
}

static void deinit(void) {
  window_destroy(s_main_window);

  app_sync_deinit(&s_sync);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
