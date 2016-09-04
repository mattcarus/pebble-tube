#include "pebble.h"
#include "tube.h"

static Window *s_main_window;

static TextLayer *s_name_layer;
static TextLayer *s_zone_layer;
static TextLayer *s_distance_layer;
static TextLayer *s_direction_layer;
static Layer *s_pointer_layer;
static Layer *s_lines_layer;

static GPath *s_pointer_arrow;

static AppSync s_sync;
static uint8_t s_sync_buffer[64];

static uint16_t bearing;
static uint16_t currentHeading;

enum WeatherKey {
  NAME_KEY = 0x0,         // TUPLE_CSTRING
  LINES_KEY = 0x1,        // TUPLE_INT
  ZONE_KEY = 0x2,         // TUPLE_CSTRING
  DISTANCE_KEY = 0x3,     // TUPLE_CSTRING
  DIRECTION_KEY = 0x4,    // TUPLE_INT
};

bool lines[12];

enum Lines {              // record position of line in lines array
  BAKERLOO = 0x0,
  CENTRAL = 0x1,
  CIRCLE = 0x2,
  DISTRICT = 0x3,
  HAMMERSMITH_CITY = 0x4,
  JUBILEE = 0x5,
  METROPOLITAN = 0x6,
  NORTHERN = 0x7,
  PICCADILLY = 0x8,
  VICTORIA = 0x9,
  WATERLOO_CITY = 0xA,
  DLR = 0xB,
};

static void sync_error_callback(DictionaryResult dict_error, AppMessageResult app_message_error, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Sync Error: %d", app_message_error);
}

static void pointer_update_proc(Layer *layer, GContext *ctx) {
  int bearing_to_destination = bearing - currentHeading;

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Re-drawing with bearing %d, heading %d and bearing to destination %d", bearing, currentHeading, bearing_to_destination);
  
  graphics_context_set_fill_color(ctx, GColorYellow);
  
  gpath_rotate_to(s_pointer_arrow, (TRIG_MAX_ANGLE / 360) * bearing_to_destination);
  gpath_draw_filled(ctx, s_pointer_arrow);
}

static void compass_heading_handler(CompassHeadingData heading_data) {
  // Is the compass calibrated?
  switch(heading_data.compass_status) {
    case CompassStatusDataInvalid:
      APP_LOG(APP_LOG_LEVEL_INFO, "Not yet calibrated.");
      break;
    case CompassStatusUnavailable:
      APP_LOG(APP_LOG_LEVEL_INFO, "Compass unavailable!");
      break;
    case CompassStatusCalibrating:
      currentHeading = TRIGANGLE_TO_DEG(TRIG_MAX_ANGLE - heading_data.magnetic_heading);
      layer_mark_dirty(window_get_root_layer(s_main_window));
      break;
    case CompassStatusCalibrated:
      currentHeading = TRIGANGLE_TO_DEG(TRIG_MAX_ANGLE - heading_data.magnetic_heading);
      layer_mark_dirty(window_get_root_layer(s_main_window));
      break;
  }
}

static void sync_tuple_changed_callback(const uint32_t key, const Tuple* new_tuple, const Tuple* old_tuple, void* context) {
  switch (key) {
    case NAME_KEY:
      // App Sync keeps new_tuple in s_sync_buffer, so we may use it directly
      text_layer_set_text(s_name_layer, new_tuple->value->cstring);
      break;

    case LINES_KEY:
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Lines value: %d", new_tuple->value->uint16);
      uint16_t rawLines = new_tuple->value->uint16;
      int numLines = 0;

      if ( rawLines >= 2048 ) {
        lines[DLR] = true;
        rawLines -= 2048;
        numLines++;
      } else {
        lines[DLR] = false;
      }
    
      if ( rawLines >= 1024 ) {
        lines[WATERLOO_CITY] = true;
        rawLines -= 1024;
        numLines++;
      } else {
        lines[WATERLOO_CITY] = false;
      }
    
      if ( rawLines >= 512 ) {
        lines[VICTORIA] = true;
        rawLines -= 512;
        numLines++;
      } else {
        lines[VICTORIA] = false;
      }
    
      if ( rawLines >= 256 ) {
        lines[PICCADILLY] = true;
        rawLines -= 256;
        numLines++;
      } else {
        lines[PICCADILLY] = false;
      }
    
      if ( rawLines >= 128 ) {
        lines[NORTHERN] = true;
        rawLines -= 128;
        numLines++;
      } else {
        lines[NORTHERN] = false;
      }
    
      if ( rawLines >= 64 ) {
        lines[METROPOLITAN] = true;
        rawLines -= 64;
        numLines++;
      } else {
        lines[METROPOLITAN] = false;
      }

      if ( rawLines >= 32 ) {
        lines[JUBILEE] = true;
        rawLines -= 32;
        numLines++;
      } else {
        lines[JUBILEE] = false;
      }
    
      if ( rawLines >= 16 ) {
        lines[HAMMERSMITH_CITY] = true;
        rawLines -= 16;
        numLines++;
      } else {
        lines[HAMMERSMITH_CITY] = false;
      }
    
      if ( rawLines >= 8 ) {
        lines[DISTRICT] = true;
        rawLines -= 8;
        numLines++;
      } else {
        lines[DISTRICT] = false;
      }
    
      if ( rawLines >= 4 ) {
        lines[CIRCLE] = true;
        rawLines -= 4;
        numLines++;
      } else {
        lines[CIRCLE] = false;
      }
    
      if ( rawLines >= 2 ) {
        lines[CENTRAL] = true;
        rawLines -= 2;
        numLines++;
      } else {
        lines[CENTRAL] = false;
      }
    
      if ( rawLines >= 1 ) {
        lines[BAKERLOO] = true;
        rawLines -= 1;
        numLines++;
      } else {
        lines[BAKERLOO] = false;
      }
    
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Number of Lines: %d", numLines);
    
      // TODO: Display lines here
    
      break;

    case ZONE_KEY:
      // App Sync keeps new_tuple in s_sync_buffer, so we may use it directly
      text_layer_set_text(s_zone_layer, new_tuple->value->cstring);
      break;

    case DISTANCE_KEY:
      text_layer_set_text(s_distance_layer, new_tuple->value->cstring);
      break;

    case DIRECTION_KEY:
      //text_layer_set_text(s_direction_layer, (*new_tuple->value->uint8).c_str());
      bearing = new_tuple->value->uint8;
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Bearing now %d", bearing);
      layer_mark_dirty(window_get_root_layer(s_main_window));
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

  s_name_layer = text_layer_create(GRect(bounds.size.w * 0.2, 20, bounds.size.w * 0.6, 64));
  text_layer_set_text_color(s_name_layer, GColorWhite);
  text_layer_set_background_color(s_name_layer, GColorClear);
  text_layer_set_font(s_name_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(s_name_layer, GTextAlignmentCenter);
  text_layer_set_overflow_mode(s_name_layer, GTextOverflowModeWordWrap);
  layer_add_child(window_layer, text_layer_get_layer(s_name_layer));

  s_zone_layer = text_layer_create(GRect(bounds.size.w * 0.2, 140, bounds.size.w * 0.6, 64));
  text_layer_set_text_color(s_zone_layer, GColorWhite);
  text_layer_set_background_color(s_zone_layer, GColorClear);
  text_layer_set_font(s_zone_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(s_zone_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_zone_layer));

  s_distance_layer = text_layer_create(GRect(0, 75, bounds.size.w, 32));
  text_layer_set_text_color(s_distance_layer, GColorWhite);
  text_layer_set_background_color(s_distance_layer, GColorClear);
  text_layer_set_font(s_distance_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_text_alignment(s_distance_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_distance_layer));

  s_lines_layer = layer_create(GRect(bounds.size.w * 0.1, 50, bounds.size.w * 0.8, 10));
  layer_add_child(window_layer, s_lines_layer);

  s_pointer_layer = layer_create(bounds);
  layer_set_update_proc(s_pointer_layer, pointer_update_proc);
  layer_add_child(window_layer, s_pointer_layer);

  Tuplet initial_values[] = {
    TupletCString(NAME_KEY, "No Station Nearby"),
    TupletInteger(LINES_KEY, (uint16_t) 0),
    TupletCString(ZONE_KEY, ""),
    TupletCString(DISTANCE_KEY, "?m"),
    TupletInteger(DIRECTION_KEY, (uint16_t) 1),
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
  window_set_background_color(s_main_window, PBL_IF_COLOR_ELSE(GColorOxfordBlue, GColorBlack));
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload
  });
  window_stack_push(s_main_window, true);
  
  // init hand paths
  s_pointer_arrow = gpath_create(&DIRECTION_POINTER_POINTS);
  
  Layer *window_layer = window_get_root_layer(s_main_window);
  GRect bounds = layer_get_bounds(window_layer);
  GPoint center = grect_center_point(&bounds);
  gpath_move_to(s_pointer_arrow, center);
  
  // Subscribe to compass heading updates
  compass_service_subscribe(compass_heading_handler);

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
