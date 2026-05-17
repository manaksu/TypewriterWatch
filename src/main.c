#include <pebble.h>

/*
 * TypeWriter Watch
 *   Key 0: KEY_PAPER  0=Paper  1=Concrete  2=Cream  3=Aged
 */

#define KEY_PAPER  0

#define PAPER_ORIGINAL  0
#define PAPER_CONCRETE  1
#define PAPER_CREAM     2
#define PAPER_AGED      3

static int s_paper = PAPER_ORIGINAL;

static Window      *s_window;
static BitmapLayer *s_bg_layer;
static GBitmap     *s_bg_bitmap;

static TextLayer   *s_time_layer;
static TextLayer   *s_bt_layer;
static TextLayer   *s_city_layer;
static TextLayer   *s_date_layer;

static GFont s_font_20;
static GFont s_font_16;
static GFont s_font_13;

static char s_time_buf[32];
static char s_date_buf[12];
static const char *s_city = "Atlanta";

/* ── background resource ── */
static uint32_t bg_res(void) {
  switch (s_paper) {
    case PAPER_CONCRETE: return RESOURCE_ID_IMAGE_BG_CONCRETE;
    case PAPER_CREAM:    return RESOURCE_ID_IMAGE_BG_CREAM;
    case PAPER_AGED:     return RESOURCE_ID_IMAGE_BG_AGED;
    default:             return RESOURCE_ID_IMAGE_BG_PAPER;
  }
}

/* ── apply paper ── */
static void apply_paper(void) {
  if (s_bg_bitmap) gbitmap_destroy(s_bg_bitmap);
  s_bg_bitmap = gbitmap_create_with_resource(bg_res());
  bitmap_layer_set_bitmap(s_bg_layer, s_bg_bitmap);
  layer_mark_dirty(bitmap_layer_get_layer(s_bg_layer));
}

/* ── update ── */
static void update_time(struct tm *t) {
  char day_buf[12], hm_buf[6];
  strftime(day_buf, sizeof(day_buf), "%A", t);
  strftime(hm_buf,  sizeof(hm_buf),
           clock_is_24h_style() ? "%H:%M" : "%I:%M", t);
  snprintf(s_time_buf, sizeof(s_time_buf), "%s, %s", day_buf, hm_buf);
  strftime(s_date_buf, sizeof(s_date_buf), "%m.%d.%Y", t);
  text_layer_set_text(s_time_layer, s_time_buf);
  text_layer_set_text(s_date_layer, s_date_buf);
}

static void update_bt(bool connected) {
  text_layer_set_text(s_bt_layer, connected ? "[bt]" : "[--]");
  text_layer_set_text_color(s_bt_layer,
    connected ? GColorBlack : GColorLightGray);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time(tick_time);
}

static void bt_handler(bool connected) {
  update_bt(connected);
}

/* ── AppMessage ── */
static void inbox_received(DictionaryIterator *iter, void *ctx) {
  Tuple *t = dict_find(iter, KEY_PAPER);
  if (t) {
    s_paper = (int)t->value->int32;
    persist_write_int(KEY_PAPER, s_paper);
    apply_paper();
  }
}

/* ── window load ── */
static void window_load(Window *window) {
  Layer *root   = window_get_root_layer(window);
  GRect  bounds = layer_get_bounds(root);

  s_font_20 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_SPECIAL_ELITE_20));
  s_font_16 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_SPECIAL_ELITE_16));
  s_font_13 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_SPECIAL_ELITE_13));

  /* background */
  s_bg_bitmap = gbitmap_create_with_resource(bg_res());
  s_bg_layer  = bitmap_layer_create(bounds);
  bitmap_layer_set_bitmap(s_bg_layer, s_bg_bitmap);
  bitmap_layer_set_compositing_mode(s_bg_layer, GCompOpAssign);
  layer_add_child(root, bitmap_layer_get_layer(s_bg_layer));

  /* time — top left, 20px, wraps to 2 lines */
  s_time_layer = text_layer_create(GRect(4, 6, 118, 56));
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorBlack);
  text_layer_set_font(s_time_layer, s_font_20);
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentLeft);
  text_layer_set_overflow_mode(s_time_layer, GTextOverflowModeWordWrap);
  layer_add_child(root, text_layer_get_layer(s_time_layer));

  /* bluetooth — top right, bold GOTHIC_18 */
  s_bt_layer = text_layer_create(GRect(94, 6, 46, 22));
  text_layer_set_background_color(s_bt_layer, GColorClear);
  text_layer_set_font(s_bt_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(s_bt_layer, GTextAlignmentRight);
  layer_add_child(root, text_layer_get_layer(s_bt_layer));

  /* city — bottom right above date, 16px */
  s_city_layer = text_layer_create(GRect(4, 126, 136, 22));
  text_layer_set_background_color(s_city_layer, GColorClear);
  text_layer_set_text_color(s_city_layer, GColorBlack);
  text_layer_set_font(s_city_layer, s_font_16);
  text_layer_set_text_alignment(s_city_layer, GTextAlignmentRight);
  text_layer_set_text(s_city_layer, s_city);
  layer_add_child(root, text_layer_get_layer(s_city_layer));

  /* date — bottom right, 16px */
  s_date_layer = text_layer_create(GRect(4, 147, 136, 22));
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_text_color(s_date_layer, GColorBlack);
  text_layer_set_font(s_date_layer, s_font_16);
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentRight);
  layer_add_child(root, text_layer_get_layer(s_date_layer));

  time_t now = time(NULL);
  update_time(localtime(&now));
  update_bt(connection_service_peek_pebble_app_connection());
}

static void window_unload(Window *window) {
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_bt_layer);
  text_layer_destroy(s_city_layer);
  text_layer_destroy(s_date_layer);
  bitmap_layer_destroy(s_bg_layer);
  gbitmap_destroy(s_bg_bitmap);
  fonts_unload_custom_font(s_font_20);
  fonts_unload_custom_font(s_font_16);
  fonts_unload_custom_font(s_font_13);
}

static void init(void) {
  s_paper = persist_exists(KEY_PAPER) ? persist_read_int(KEY_PAPER) : PAPER_ORIGINAL;

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
  app_message_open(64, 64);
  app_message_register_inbox_received(inbox_received);
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
