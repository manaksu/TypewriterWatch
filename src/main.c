#include <pebble.h>

/*
 * TypeWriter Watch
 * - Paper texture background
 * - Special Elite typewriter font
 * - Top-left: HH:MM ~ bluetooth indicator
 * - Bottom-right: MM.DD.YYYY date
 */

static Window      *s_window;
static BitmapLayer *s_bg_layer;
static GBitmap     *s_bg_bitmap;

static TextLayer   *s_time_layer;
static TextLayer   *s_bt_layer;
static TextLayer   *s_date_layer;

static GFont s_font_28;
static GFont s_font_18;
static GFont s_font_14;

static char s_time_buf[6];   /* HH:MM */
static char s_bt_buf[4];     /* ~ or   */
static char s_date_buf[12];  /* MM.DD.YYYY */

/* ── update ── */
static void update_time(struct tm *t) {
  strftime(s_time_buf, sizeof(s_time_buf),
           clock_is_24h_style() ? "%H:%M" : "%I:%M", t);
  strftime(s_date_buf, sizeof(s_date_buf), "%m.%d.%Y", t);
  text_layer_set_text(s_time_layer, s_time_buf);
  text_layer_set_text(s_date_layer, s_date_buf);
}

static void update_bt(bool connected) {
  snprintf(s_bt_buf, sizeof(s_bt_buf), connected ? "~" : "");
  text_layer_set_text(s_bt_layer, s_bt_buf);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time(tick_time);
}

static void bt_handler(bool connected) {
  update_bt(connected);
}

/* ── window load ── */
static void window_load(Window *window) {
  Layer *root   = window_get_root_layer(window);
  GRect  bounds = layer_get_bounds(root);

  /* load fonts */
  s_font_28 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_SPECIAL_ELITE_28));
  s_font_18 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_SPECIAL_ELITE_18));
  s_font_14 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_SPECIAL_ELITE_14));

  /* paper background */
  s_bg_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_PAPER_BG);
  s_bg_layer  = bitmap_layer_create(bounds);
  bitmap_layer_set_bitmap(s_bg_layer, s_bg_bitmap);
  bitmap_layer_set_compositing_mode(s_bg_layer, GCompOpAssign);
  layer_add_child(root, bitmap_layer_get_layer(s_bg_layer));

  /* ── time — top left ── */
  /* GRect(x, y, w, h) */
  s_time_layer = text_layer_create(GRect(4, 6, 140, 38));
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorBlack);
  text_layer_set_font(s_time_layer, s_font_28);
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentLeft);
  layer_add_child(root, text_layer_get_layer(s_time_layer));

  /* ── bluetooth indicator — top left below time ── */
  s_bt_layer = text_layer_create(GRect(4, 44, 80, 20));
  text_layer_set_background_color(s_bt_layer, GColorClear);
  text_layer_set_text_color(s_bt_layer, GColorDarkGray);
  text_layer_set_font(s_bt_layer, s_font_14);
  text_layer_set_text_alignment(s_bt_layer, GTextAlignmentLeft);
  layer_add_child(root, text_layer_get_layer(s_bt_layer));

  /* ── date — bottom right ── */
  s_date_layer = text_layer_create(GRect(4, 136, 136, 26));
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_text_color(s_date_layer, GColorBlack);
  text_layer_set_font(s_date_layer, s_font_18);
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentRight);
  layer_add_child(root, text_layer_get_layer(s_date_layer));

  /* initial draw */
  time_t now = time(NULL);
  update_time(localtime(&now));
  update_bt(connection_service_peek_pebble_app_connection());
}

static void window_unload(Window *window) {
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_bt_layer);
  text_layer_destroy(s_date_layer);
  bitmap_layer_destroy(s_bg_layer);
  gbitmap_destroy(s_bg_bitmap);
  fonts_unload_custom_font(s_font_28);
  fonts_unload_custom_font(s_font_18);
  fonts_unload_custom_font(s_font_14);
}

static void init(void) {
  s_window = window_create();
  window_set_background_color(s_window, GColorWhite);
  window_set_window_handlers(s_window, (WindowHandlers){
    .load   = window_load,
    .unload = window_unload,
  });
  window_stack_push(s_window, true);
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  connection_service_subscribe((ConnectionHandlers){
    .pebble_app_connection_handler = bt_handler
  });
}

static void deinit(void) {
  tick_timer_service_unsubscribe();
  connection_service_unsubscribe();
  window_destroy(s_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
