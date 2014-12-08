#include <pebble.h>

static Window *main_window;
static TextLayer *time_layer;
static BitmapLayer *battery_layer;
static char time_buffer[64] = "";
static GBitmap *battery_bitmap;

static const char num_names[][10] = {
  "", "one", "two", "three", "four", "five", "six", "seven", "eight", "nine", "ten", "eleven", "twelve", "thirteen", "fourteen", "fifteen", "sixteen", "seventeen", "eighteen", "nineteen"
};
static const char multi_names[][7] = {
  "twenty", "thirty", "forty", "fifty"
};

/* We have a gauge from 0 to 100, on which we need to plot on a grid of 144x2. */
static void battery_state_handler(BatteryChargeState batt)
{
	int i;
	unsigned char *bitfield = (unsigned char *) battery_bitmap->addr;

	/* 144 is 18 bytes, 8 bits apiece. */
	for (i = 0; i < 72; i++) {
		short corresponding_percentage = i * 144 / 100;
		short bitnum = i * 2;
		short byte_offset = bitnum / 8;
		short bit_offset = bitnum % 8;
		if (batt.charge_percent > corresponding_percentage) {
			/* Set black (unset bit) */
			bitfield[ 0 + byte_offset] &= ~(3 << bit_offset);
			bitfield[20 + byte_offset] &= ~(3 << bit_offset);
		} else {
			/* Set white */
			bitfield[ 0 + byte_offset] |= (3 << bit_offset);
			bitfield[20 + byte_offset] |= (3 << bit_offset);
		}
	}
	bitmap_layer_set_bitmap(battery_layer, battery_bitmap);
}

static void tick_handler(struct tm *tm, TimeUnits units_changed) {
  char minutes[16] = "";
  if (tm->tm_hour == 0 && tm->tm_min == 0) {
    /* Beginning of the day is the exception */
    snprintf(time_buffer, sizeof(time_buffer), "twenty-four hundred hours");
  } else {
    if (tm->tm_min == 0) {
      snprintf(minutes, sizeof(minutes), "hundred");
    } else if (tm->tm_min < 10) {
      snprintf(minutes, sizeof(minutes), "oh %s", num_names[tm->tm_min]);
    } else if (tm->tm_min < 20) {
      snprintf(minutes, sizeof(minutes), "%s", num_names[tm->tm_min]);
    } else {
      snprintf(minutes, sizeof(minutes), "%s %s", multi_names[tm->tm_min / 10 - 2], num_names[tm->tm_min % 10]);
    }

    if (tm->tm_hour == 0) {
      snprintf(time_buffer, sizeof(time_buffer), "oh zero %s hours", minutes);
    } else if (tm->tm_hour < 20) {
      snprintf(time_buffer, sizeof(time_buffer), "%s %s %s hours", tm->tm_hour < 10 ? "oh" : "", num_names[tm->tm_hour], minutes);
    } else {
      snprintf(time_buffer, sizeof(time_buffer), "twenty %s%s%s hours", num_names[tm->tm_hour - 20], tm->tm_hour == 20 ? "" : " ", minutes);
    }
  }
  text_layer_set_text(time_layer, time_buffer);
}

static void main_window_load(Window *w) {
  time_t now = time(NULL);
  struct tm *tm;
  BatteryChargeState batt = battery_state_service_peek();

  time_layer = text_layer_create(GRect(0, 10, 144, 168));
  text_layer_set_background_color(time_layer, GColorClear);
  text_layer_set_text_color(time_layer, GColorBlack);
  text_layer_set_font(time_layer, fonts_get_system_font(FONT_KEY_DROID_SERIF_28_BOLD));
  text_layer_set_text_alignment(time_layer, GTextAlignmentCenter);

  /* Bottom of the screen gets a battery indicator */
  battery_layer = bitmap_layer_create(GRect(0, 165, 144, 2));

  layer_add_child(window_get_root_layer(main_window), text_layer_get_layer(time_layer));
  layer_add_child(window_get_root_layer(main_window), bitmap_layer_get_layer(battery_layer));

  /* Set the initial states */
  tm = localtime(&now);
  tick_handler(tm, (TimeUnits) NULL);

  battery_state_handler(batt);
}

static void main_window_unload(Window *w) {
  text_layer_destroy(time_layer);
  bitmap_layer_destroy(battery_layer);
}

static void init() {
  /* Set up the memory for the bitmap before we load the window */
  battery_bitmap = gbitmap_create_blank(GSize(144, 2));
  memset(battery_bitmap->addr, 0xff, (144 / 8) * 2);

  main_window = window_create();
  window_set_window_handlers(main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
  });
  window_stack_push(main_window, true);

  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  battery_state_service_subscribe(battery_state_handler);
}

static void deinit() {
  window_destroy(main_window);
  gbitmap_destroy(battery_bitmap);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
