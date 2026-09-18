#include <pebble.h>
#include <tgmath.h>

#define SETTINGS_KEY 1

typedef struct ClaySettings {
  bool TemperatureUnit; // false = Celsius, true = Fahrenheit
  bool ShowDate;
  bool HRTEnabled;
  int HRTDay;
  time_t LastDose;
} ClaySettings;

static ClaySettings settings;

static Window *s_main_window;

static Layer *s_window_layer;

static Layer *s_trans_layer;

static TextLayer *s_time_layer;

static TextLayer *s_date_layer;

static TextLayer *s_battery_layer;
static int s_battery_level;

static BitmapLayer *s_bt_icon_layer;
static GBitmap *s_bt_icon_bitmap;

static TextLayer *s_weather_layer;

static TextLayer *s_hrt_reminder_layer;

static time_t get_current_hrt_day() {
  time_t temp = time(NULL);
  struct tm *hrt_day = localtime(&temp);

  hrt_day->tm_hour = 0;
  hrt_day->tm_min = 0;
  hrt_day->tm_sec = 0;

  int days_to_subtract = hrt_day->tm_wday - settings.HRTDay;

  if (days_to_subtract < 0) {
    days_to_subtract += 7;
  }

  hrt_day->tm_mday -= days_to_subtract;

  return mktime(hrt_day);
}

static void prv_default_settings() {
  settings.TemperatureUnit = false;
  settings.ShowDate = true;
  settings.HRTEnabled = false;
  settings.HRTDay = 0;
  settings.LastDose = 0;
}

static void prv_save_settings() {
  persist_write_data(SETTINGS_KEY, &settings, sizeof(settings));
}

static void prv_load_settings() {
  prv_default_settings();
  persist_read_data(SETTINGS_KEY, &settings, sizeof(settings));
}

static void update_hrt_reminder() {
  time_t hrt_day = get_current_hrt_day();

  if (!settings.HRTEnabled || settings.LastDose > hrt_day) {
    layer_set_hidden(text_layer_get_layer(s_hrt_reminder_layer), true);
  } else {
    layer_set_hidden(text_layer_get_layer(s_hrt_reminder_layer), false);

    time_t now = time(NULL);
    int difference = ceil(difftime(now, hrt_day) / (60.0 * 60.0 * 24.0)) - 1;

    GRect bounds = layer_get_bounds(s_window_layer);

    GRect frame = layer_get_frame(text_layer_get_layer(s_hrt_reminder_layer));
    frame.origin.x = 25;
    frame.size.w = bounds.size.w - 50;

    if (difference < 1) {
      text_layer_set_background_color(s_hrt_reminder_layer, GColorClear);
      text_layer_set_text_color(s_hrt_reminder_layer, GColorWhite);
      text_layer_set_text(s_hrt_reminder_layer, "It's HRT Day!");
    } else if (difference < 2) {
      text_layer_set_background_color(s_hrt_reminder_layer, GColorWhite);
      text_layer_set_text_color(s_hrt_reminder_layer, GColorRed);
      text_layer_set_text(s_hrt_reminder_layer, "! HRT 1 day late !");
    } else {
      text_layer_set_background_color(s_hrt_reminder_layer, GColorWhite);
      text_layer_set_text_color(s_hrt_reminder_layer, GColorRed);
      static char s_battery_buffer[24];
      snprintf(s_battery_buffer, sizeof(s_battery_buffer), "! ! HRT %d days late ! !", difference);
      text_layer_set_text(s_hrt_reminder_layer, s_battery_buffer);
      frame.origin.x = 15;
      frame.size.w = bounds.size.w - 30;
    }

    layer_set_frame(text_layer_get_layer(s_hrt_reminder_layer), frame);
  }
  layer_mark_dirty(s_trans_layer);
}

static void prv_update_display() {

  layer_set_hidden(text_layer_get_layer(s_date_layer), !settings.ShowDate);
  update_hrt_reminder();
}

static void battery_callback(BatteryChargeState state) {
  // Record the new battery level
  s_battery_level = state.charge_percent;

  // Update the meter
  static char s_battery_buffer[5];
  snprintf(s_battery_buffer, sizeof(s_battery_buffer), "%d%%", s_battery_level);
  text_layer_set_text(s_battery_layer, s_battery_buffer);
}

static void bluetooth_callback(bool connected) {
  // Show icon if disconnected
  layer_set_hidden(bitmap_layer_get_layer(s_bt_icon_layer), connected);

  if (!connected) {
    // Issue a vibrating alert
    vibes_double_pulse();
  }
}


static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

  // Write the current hours and minutes into a buffer
  static char s_time_buffer[8];
  strftime(s_time_buffer, sizeof(s_time_buffer), clock_is_24h_style() ?
                                                    "%H:%M" : "%I:%M", tick_time);

  // Display this time on the TextLayer
  text_layer_set_text(s_time_layer, s_time_buffer);

  // Write the current date into a buffer
  static char s_date_buffer[16];
  strftime(s_date_buffer, sizeof(s_date_buffer), "%a %b %d", tick_time);

  // Display the date
  text_layer_set_text(s_date_layer, s_date_buffer);
}


static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();

  // Get weather update every 30 minutes
  if (tick_time->tm_min % 30 == 0) {
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
    dict_write_uint8(iter, MESSAGE_KEY_REQUEST_WEATHER, 1);
    app_message_outbox_send();
  }

  if (tick_time->tm_hour == 0) {
    update_hrt_reminder();
  }
}

static void trans_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);

  int section_height = bounds.size.h / 5;

  int width_side = 5;
  int width_top = 5;


  time_t hrt_day = get_current_hrt_day();

  if (settings.HRTEnabled && settings.LastDose <= hrt_day) {
    time_t now = time(NULL);
    int difference = ceil(difftime(now, hrt_day) / (60.0 * 60.0 * 24.0)) - 1;

    if (difference < 1) {
      graphics_context_set_fill_color(ctx, GColorOxfordBlue);
      graphics_fill_rect(ctx, GRect(0, 0, bounds.size.w, bounds.size.h), 0, GCornerNone);
      graphics_context_set_fill_color(ctx, GColorBulgarianRose);
      graphics_fill_rect(ctx, GRect(0, section_height, bounds.size.w, bounds.size.h - section_height * 2), 0, GCornerNone);
      graphics_context_set_fill_color(ctx, GColorDarkGray);
      graphics_fill_rect(ctx, GRect(0, section_height * 2, bounds.size.w, section_height), 0, GCornerNone);
    } else if (difference < 2) {
      graphics_context_set_fill_color(ctx, GColorBulgarianRose);
      graphics_fill_rect(ctx, GRect(0, 0, bounds.size.w, bounds.size.h), 0, GCornerNone);
    } else {
      graphics_context_set_fill_color(ctx, GColorDarkCandyAppleRed);
      graphics_fill_rect(ctx, GRect(0, 0, bounds.size.w, bounds.size.h), 0, GCornerNone);
    }
  }

  graphics_context_set_fill_color(ctx, GColorBlue);
  graphics_fill_rect(ctx, GRect(0, 0, width_side, bounds.size.h), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(bounds.size.w - width_side, 0, width_side, bounds.size.h), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(0, 0, bounds.size.w, width_top), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(0, bounds.size.h - width_top, bounds.size.w, width_top), 0, GCornerNone);
  graphics_context_set_fill_color(ctx, GColorRed);
  graphics_fill_rect(ctx, GRect(0, section_height, width_side, bounds.size.h - section_height * 2), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(bounds.size.w - width_side, section_height, width_side, bounds.size.h - section_height * 2), 0, GCornerNone);
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, GRect(0, section_height * 2, width_side, section_height), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(bounds.size.w - width_side, section_height * 2, width_side, section_height), 0, GCornerNone);
}

static void prv_unobstructed_will_change(GRect final_unobstructed_screen_area,
                                         void *context) {
  // Hide BT icon during the transition to reduce clutter
  layer_set_hidden(bitmap_layer_get_layer(s_bt_icon_layer), true);
  layer_set_hidden(text_layer_get_layer(s_battery_layer), true);
}

static void prv_unobstructed_change(AnimationProgress progress, void *context) {
  GRect bounds = layer_get_unobstructed_bounds(s_window_layer);
}

static void prv_unobstructed_did_change(void *context) {
  GRect full_bounds = layer_get_bounds(s_window_layer);
  GRect bounds = layer_get_unobstructed_bounds(s_window_layer);
  bool obstructed = !grect_equal(&full_bounds, &bounds);

  // Keep BT icon hidden when obstructed, otherwise restore based on connection
  if (obstructed) {
    layer_set_hidden(bitmap_layer_get_layer(s_bt_icon_layer), true);
    layer_set_hidden(text_layer_get_layer(s_battery_layer), true);
  } else {
    layer_set_hidden(bitmap_layer_get_layer(s_bt_icon_layer),
      connection_service_peek_pebble_app_connection());
    layer_set_hidden(text_layer_get_layer(s_battery_layer), false);
  }
}

static void main_window_load(Window *window) {
  // Get information about the Window
  s_window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(s_window_layer);

  int date_height = 30;
  int time_height = 60;
  int battery_height = 25;
  int bluetooth_height = 30;
  int hrt_height = 35;
  int time_y = (bounds.size.h / 2) - (time_height / 2) - 10;
  int date_y = 53;
  int hrt_y = time_y + 75;
  int battery_y = bounds.size.h - battery_height - 5;
  int bluetooth_y = bounds.size.h - bluetooth_height - 5;
  int weather_y = 10;

  // Create trans layer
  s_trans_layer = layer_create(GRect(0, 0, bounds.size.w, bounds.size.h));
  layer_set_update_proc(s_trans_layer, trans_update_proc);

  // Create weather TextLayer - aligned to the bottom of the screen
  s_weather_layer = text_layer_create(
      GRect(0, weather_y, bounds.size.w, 25));
  text_layer_set_background_color(s_weather_layer, GColorClear);
  text_layer_set_text_color(s_weather_layer, GColorWhite);
  text_layer_set_font(s_weather_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text_alignment(s_weather_layer, GTextAlignmentCenter);
  text_layer_set_text(s_weather_layer, "Loading...");

  // Create the time TextLayer
  s_time_layer = text_layer_create(
      GRect(0, time_y, bounds.size.w, time_height));
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorWhite);
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_LECO_60_NUMBERS_AM_PM));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);

  // Create the date TextLayer
  s_date_layer = text_layer_create(
      GRect(0, date_y, bounds.size.w, date_height));
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_text_color(s_date_layer, GColorWhite);
  text_layer_set_font(s_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);

  // Create the battery TextLayer
  s_battery_layer = text_layer_create(
      GRect(0, battery_y, bounds.size.w, battery_height));
  text_layer_set_background_color(s_battery_layer, GColorClear);
  text_layer_set_text_color(s_battery_layer, GColorLightGray);
  text_layer_set_font(s_battery_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text_alignment(s_battery_layer, GTextAlignmentCenter);

  // Create the Bluetooth icon GBitmap
  s_bt_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BT_ICON);

  // Create the BitmapLayer to display the GBitmap
  s_bt_icon_layer = bitmap_layer_create(GRect(bounds.size.w - 40, bluetooth_y, 30, bluetooth_height));
  bitmap_layer_set_bitmap(s_bt_icon_layer, s_bt_icon_bitmap);
  bitmap_layer_set_compositing_mode(s_bt_icon_layer, GCompOpSet);

  // Create the HRT Reminder TextLayer
  s_hrt_reminder_layer = text_layer_create(
      GRect(25, hrt_y, bounds.size.w - 50, hrt_height));
  text_layer_set_font(s_hrt_reminder_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(s_hrt_reminder_layer, GTextAlignmentCenter);

  // Add layers to the Window
  layer_add_child(s_window_layer, s_trans_layer);
  layer_add_child(s_window_layer, text_layer_get_layer(s_weather_layer));
  layer_add_child(s_window_layer, text_layer_get_layer(s_time_layer));
  layer_add_child(s_window_layer, text_layer_get_layer(s_date_layer));
  layer_add_child(s_window_layer, text_layer_get_layer(s_battery_layer));
  layer_add_child(s_window_layer, bitmap_layer_get_layer(s_bt_icon_layer));
  layer_add_child(s_window_layer, text_layer_get_layer(s_hrt_reminder_layer));

  layer_mark_dirty(s_trans_layer);

  // Apply correct layout in case Quick View is already active
  prv_unobstructed_change(0, NULL);
  prv_unobstructed_did_change(NULL);

  UnobstructedAreaHandlers handlers = {
    .will_change = prv_unobstructed_will_change,
    .change = prv_unobstructed_change,
    .did_change = prv_unobstructed_did_change
  };
  unobstructed_area_service_subscribe(handlers, NULL);

  prv_update_display();
}

static void main_window_unload(Window *window) {
  layer_destroy(s_trans_layer);
  text_layer_destroy(s_weather_layer);
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_date_layer);
  text_layer_destroy(s_battery_layer);
  gbitmap_destroy(s_bt_icon_bitmap);
  bitmap_layer_destroy(s_bt_icon_layer);
  text_layer_destroy(s_hrt_reminder_layer);
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  // Check for weather data
  Tuple *temp_tuple = dict_find(iterator, MESSAGE_KEY_TEMPERATURE);
  Tuple *conditions_tuple = dict_find(iterator, MESSAGE_KEY_CONDITIONS);

  if (temp_tuple && conditions_tuple) {
    static char temperature_buffer[8];
    static char conditions_buffer[32];
    static char weather_layer_buffer[42];

    int temp_value = (int)temp_tuple->value->int32;

    // Convert to Fahrenheit if setting is enabled
    if (settings.TemperatureUnit) {
      temp_value = (temp_value * 9 / 5) + 32;
      snprintf(temperature_buffer, sizeof(temperature_buffer), "%d°F", temp_value);
    } else {
      snprintf(temperature_buffer, sizeof(temperature_buffer), "%d°C", temp_value);
    }

    snprintf(conditions_buffer, sizeof(conditions_buffer), "%s", conditions_tuple->value->cstring);
    snprintf(weather_layer_buffer, sizeof(weather_layer_buffer), "%s %s", temperature_buffer, conditions_buffer);
    text_layer_set_text(s_weather_layer, weather_layer_buffer);
  }

  Tuple *log_hrt_tuple = dict_find(iterator, MESSAGE_KEY_LOG_HRT);

  if (log_hrt_tuple && log_hrt_tuple->value->int32 == 1) {
    settings.LastDose = time(NULL);
  }

  // Check for Clay settings
  Tuple *temp_unit_t = dict_find(iterator, MESSAGE_KEY_TemperatureUnit);
  if (temp_unit_t) {
    settings.TemperatureUnit = temp_unit_t->value->int32 == 1;
  }

  Tuple *show_date_t = dict_find(iterator, MESSAGE_KEY_ShowDate);
  if (show_date_t) {
    settings.ShowDate = show_date_t->value->int32 == 1;
  }

  Tuple *hrt_enabled_t = dict_find(iterator, MESSAGE_KEY_HRTEnabled);
  if (hrt_enabled_t) {
    settings.HRTEnabled = hrt_enabled_t->value->int32 == 1;
  }

  Tuple *hrt_day_t = dict_find(iterator, MESSAGE_KEY_HRTDay);
  if (hrt_day_t) {
    char *str_value = hrt_day_t->value->cstring;
    int value = 0;
    if (strcmp("mon", str_value) == 0) {
      value = 1;
    } else if (strcmp("tue", str_value) == 0) {
      value = 2;
    } else if (strcmp("wed", str_value) == 0) {
      value = 3;
    } else if (strcmp("thu", str_value) == 0) {
      value = 4;
    } else if (strcmp("fri", str_value) == 0) {
      value = 5;
    } else if (strcmp("sat", str_value) == 0) {
      value = 6;
    }
    settings.HRTDay = value;
  }

  // Save and apply if any settings were changed
  if (temp_unit_t || show_date_t || hrt_enabled_t || hrt_day_t || (log_hrt_tuple && log_hrt_tuple->value->int32 == 1)) {
    prv_save_settings();
    prv_update_display();

    // Refetch weather if the temperature unit changed so the display updates
    if (temp_unit_t) {
      DictionaryIterator *iter;
      app_message_outbox_begin(&iter);
      dict_write_uint8(iter, MESSAGE_KEY_REQUEST_WEATHER, 1);
      app_message_outbox_send();
    }
  }
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

static void init() {
  prv_load_settings();

  // Create main Window element and assign to pointer
  s_main_window = window_create();

  // Set the background color
  window_set_background_color(s_main_window, GColorBlack);

  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);

  // Make sure the time is displayed from the start
  update_time();

  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);

  // Ensure battery level is displayed from the start
  battery_callback(battery_state_service_peek());

  // Register for battery level updates
  battery_state_service_subscribe(battery_callback);

  // Register for Bluetooth connection updates
  connection_service_subscribe((ConnectionHandlers) {
    .pebble_app_connection_handler = bluetooth_callback
  });

  // Register AppMessage callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);

  // Open AppMessage
  const int inbox_size = 256;
  const int outbox_size = 256;
  app_message_open(inbox_size, outbox_size);
}

static void deinit() {
  // Destroy Window
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
