#include <pebble.h>

static Window *main_window;
static TextLayer *time_layer;
static char time_buffer[64] = "";
static char num_names[][10] = {
  "", "one", "two", "three", "four", "five", "six", "seven", "eight", "nine", "ten", "eleven", "twelve", "thirteen", "fourteen", "fifteen", "sixteen", "seventeen", "eighteen", "nineteen"
};
static char multi_names[][7] = {
  "twenty", "thirty", "forty", "fifty"
};

static void tick_handler(struct tm *tm, TimeUnits units_changed) {
  char minutes[16] = "";
  if (tm->tm_hour == 0 && tm->tm_min == 0) {
    /* Beginning of the day is the exception */
    snprintf(time_buffer, sizeof(time_buffer), "twenty-four hundred hours");
  } else {
    if (tm->tm_min == 0) {
      minutes[0] = '\0';
    } else if (tm->tm_min < 10) {
      snprintf(minutes, sizeof(minutes), "oh %s", num_names[tm->tm_min]);
    } else if (tm->tm_min < 20) {
      snprintf(minutes, sizeof(minutes), "%s", num_names[tm->tm_min]);
    } else {
      snprintf(minutes, sizeof(minutes), "%s %s", multi_names[tm->tm_min / 10 - 2], num_names[tm->tm_min % 10]);
    }
    
    if (tm->tm_hour < 20) {
      snprintf(time_buffer, sizeof(time_buffer), "%s %s%shundred %s hours", tm->tm_hour < 10 ? "oh" : "", num_names[tm->tm_hour], tm->tm_hour == 0 ? "" : " ", minutes);
    } else {
      snprintf(time_buffer, sizeof(time_buffer), "twenty %s%shundred %s hours", num_names[tm->tm_hour - 20], tm->tm_hour == 20 ? "" : " ", minutes);
    }
  }
  text_layer_set_text(time_layer, time_buffer);
}

static void main_window_load(Window *w) {
  time_t now = time(NULL);
  struct tm *tm;
  time_layer = text_layer_create(GRect(0, 10, 144, 200));
  text_layer_set_background_color(time_layer, GColorClear);
  text_layer_set_text_color(time_layer, GColorBlack);
  text_layer_set_font(time_layer, fonts_get_system_font(FONT_KEY_DROID_SERIF_28_BOLD));
  text_layer_set_text_alignment(time_layer, GTextAlignmentCenter);
  
  layer_add_child(window_get_root_layer(main_window), text_layer_get_layer(time_layer));
  tm = localtime(&now);
  tick_handler(tm, (TimeUnits) NULL);
}

static void main_window_unload(Window *w) {
  text_layer_destroy(time_layer);
}

static void init() {
  main_window = window_create();
  window_set_window_handlers(main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
  });
  window_stack_push(main_window, true);
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
}

static void deinit() {
  window_destroy(main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}