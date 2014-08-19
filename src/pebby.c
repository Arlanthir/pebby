#include "pebble.h"
#include <string.h>
#include <time.h>

// Persist Keys
#define PERSIST_BOTTLE 1
#define PERSIST_DIAPER 2
#define PERSIST_MOON_START 3
#define PERSIST_MOON_END 4

	
static Window *window;

// Texts

static TextLayer *bottleTextLayer;
static char timeTextUp[] = "00:00";	// Used by the system later

static TextLayer *diaperTextLayer;
static char timeTextMiddle[] = "00:00";	// Used by the system later

static TextLayer *moonTextLayer;
static char timeTextDown[14] = "";	// Used by the system later

// Bitmaps

static GBitmap *bottleBlackImage;
static GBitmap *diaperBlackImage;
static GBitmap *moonBlackImage;

static BitmapLayer *bottleBlacklayer;
static BitmapLayer *diaperBlacklayer;
static BitmapLayer *moonBlacklayer;

static int sleeping = 0;
static time_t sleepStart;



static void setTimeText(time_t timestamp, char *text, TextLayer *textLayer) {
	struct tm *time = localtime(&timestamp);
	strftime(text, sizeof(timeTextUp), (clock_is_24h_style()? "%H:%M" : "%I:%M"), time);
	text_layer_set_text(textLayer, text);
}


static void setTimeRangeText(time_t startTimestamp, time_t endTimestamp, char *text, TextLayer *textLayer) {
	char sleepStartStr[] = "00:00";
	char sleepEndStr[] = "00:00";
	
	struct tm *time = localtime(&startTimestamp);
		
	strftime(sleepStartStr, sizeof(sleepStartStr), (clock_is_24h_style()? "%H:%M" : "%I:%M"), time);

	
	if (endTimestamp != 0) {
		time = localtime(&endTimestamp);
		strftime(sleepEndStr, sizeof(sleepEndStr), (clock_is_24h_style()? "%H:%M" : "%I:%M"), time);
	} else {
		strcpy(sleepEndStr, "...");
	}
	
	strncpy(text, sleepStartStr, sizeof(sleepStartStr));
	strncat(text, " - ", 4);
	strncat(text, sleepEndStr, sizeof(sleepEndStr));
	
	text_layer_set_text(textLayer, text);
}


static void window_load(Window *window) {
	Layer *window_layer = window_get_root_layer(window);
	GRect bounds = layer_get_bounds(window_layer);

	// Text layers
	
	bottleTextLayer = text_layer_create((GRect){ .origin = {0, bounds.size.h/3/2 - 16 }, .size = {100, 24} });
	text_layer_set_text_alignment(bottleTextLayer, GTextAlignmentCenter);
	text_layer_set_text(bottleTextLayer, "");
	text_layer_set_font(bottleTextLayer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
	layer_add_child(window_layer, text_layer_get_layer(bottleTextLayer));
	
	diaperTextLayer = text_layer_create((GRect){ .origin = {0, bounds.size.h/2 - 16 }, .size = {100, 24} });
	text_layer_set_font(diaperTextLayer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
	text_layer_set_text_alignment(diaperTextLayer, GTextAlignmentCenter);
	text_layer_set_text(diaperTextLayer, "");
	layer_add_child(window_layer, text_layer_get_layer(diaperTextLayer));
	
	moonTextLayer = text_layer_create((GRect){ .origin = {0, 5*bounds.size.h/3/2 - 16 }, .size = {105, 24} });
	text_layer_set_font(moonTextLayer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
	text_layer_set_text_alignment(moonTextLayer, GTextAlignmentCenter);
	text_layer_set_text(moonTextLayer, "");
	layer_add_child(window_layer, text_layer_get_layer(moonTextLayer));
	
	// Time values initialization
	if (persist_exists(PERSIST_BOTTLE)) {
		time_t t = persist_read_int(PERSIST_BOTTLE);
		setTimeText(t, timeTextUp, bottleTextLayer);
	}
	
	if (persist_exists(PERSIST_DIAPER)) {
		time_t t = persist_read_int(PERSIST_DIAPER);
		setTimeText(t, timeTextMiddle, diaperTextLayer);
	}
	
	if (persist_exists(PERSIST_MOON_START)) {
		sleepStart = persist_read_int(PERSIST_MOON_START);
		
		time_t t = persist_exists(PERSIST_MOON_END)? persist_read_int(PERSIST_MOON_END) : 0;

		if (t == 0) {
			sleeping = 1;
		}
		
		setTimeRangeText(sleepStart, t, timeTextDown, moonTextLayer);
	}
	
	// Image layers
	
	bottleBlackImage = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BOTTLE_BLACK);
	diaperBlackImage = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_DIAPER_BLACK);
	moonBlackImage = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_MOON_BLACK);

	const GPoint center = grect_center_point(&bounds);
	GRect image_frame = (GRect) { .origin = center, .size = bottleBlackImage->bounds.size };

	image_frame.size = bottleBlackImage->bounds.size;
	image_frame.origin.x = bounds.size.w - image_frame.size.w - 5;
	image_frame.origin.y = bounds.size.h/3/2 - image_frame.size.h/2;
	bottleBlacklayer = bitmap_layer_create(image_frame);
	bitmap_layer_set_bitmap(bottleBlacklayer, bottleBlackImage);
	bitmap_layer_set_compositing_mode(bottleBlacklayer, GCompOpClear);
	layer_add_child(window_layer, bitmap_layer_get_layer(bottleBlacklayer));
	
	image_frame.size = diaperBlackImage->bounds.size;
	image_frame.origin.x = bounds.size.w - image_frame.size.w - 5;
	image_frame.origin.y = center.y - image_frame.size.h/2;
	diaperBlacklayer = bitmap_layer_create(image_frame);
	bitmap_layer_set_bitmap(diaperBlacklayer, diaperBlackImage);
	bitmap_layer_set_compositing_mode(diaperBlacklayer, GCompOpClear);
	layer_add_child(window_layer, bitmap_layer_get_layer(diaperBlacklayer));
	
	image_frame.size = moonBlackImage->bounds.size;
	image_frame.origin.x = bounds.size.w - image_frame.size.w - 5;
	image_frame.origin.y = 5*bounds.size.h/3/2 - image_frame.size.h/2;
	moonBlacklayer = bitmap_layer_create(image_frame);
	bitmap_layer_set_bitmap(moonBlacklayer, moonBlackImage);
	bitmap_layer_set_compositing_mode(moonBlacklayer, GCompOpClear);
	layer_add_child(window_layer, bitmap_layer_get_layer(moonBlacklayer));
}

static void window_unload(Window *window) {
	text_layer_destroy(bottleTextLayer);
	text_layer_destroy(diaperTextLayer);
	text_layer_destroy(moonTextLayer);
	
	bitmap_layer_destroy(bottleBlacklayer);
	bitmap_layer_destroy(diaperBlacklayer);
	bitmap_layer_destroy(moonBlacklayer);

	gbitmap_destroy(bottleBlackImage);
	gbitmap_destroy(diaperBlackImage);
	gbitmap_destroy(moonBlackImage);
}


void up_single_click_handler(ClickRecognizerRef recognizer, void *context) {
	//Window *window = (Window *)context;
	//app_log(APP_LOG_LEVEL_DEBUG, "pebby.c", 101, "Click Up");

	ButtonId bt = click_recognizer_get_button_id(recognizer);
	char *targetText = timeTextUp;
	TextLayer *targetLayer = bottleTextLayer;
	int persistKey = PERSIST_BOTTLE;
	
	if (bt == BUTTON_ID_SELECT) {
		targetText = timeTextMiddle;
		targetLayer = diaperTextLayer;
		persistKey = PERSIST_DIAPER;
	}
	
	time_t t = time(NULL);
	persist_write_int(persistKey, t);
	
	setTimeText(t, targetText, targetLayer);
}

void down_single_click_handler(ClickRecognizerRef recognizer, void *context) {
	//Window *window = (Window *)context;
	//app_log(APP_LOG_LEVEL_DEBUG, "pebby.c", 101, "Click Down");
	
	time_t sleepEnd = 0;
	
	if (sleeping) {
		sleepEnd = time(NULL);
		persist_write_int(PERSIST_MOON_END, sleepEnd);
		sleeping = 0;
	} else {
		sleepStart = time(NULL);
		persist_write_int(PERSIST_MOON_START, sleepStart);
		persist_write_int(PERSIST_MOON_END, 0);
		sleeping = 1;
	}
	
	setTimeRangeText(sleepStart, sleepEnd, timeTextDown, moonTextLayer);
}

void config_provider(Window *window) {
	window_single_click_subscribe(BUTTON_ID_UP, up_single_click_handler);
	window_single_click_subscribe(BUTTON_ID_SELECT, up_single_click_handler);
	window_single_click_subscribe(BUTTON_ID_DOWN, down_single_click_handler);
}

static void init(void) {
	window = window_create();
	window_set_window_handlers(window, (WindowHandlers) {
		.load = window_load,
		.unload = window_unload
	});
	window_set_click_config_provider(window, (ClickConfigProvider) config_provider);
	window_stack_push(window, true /* Animated */);
}

static void deinit(void) {
	window_destroy(window);
}

int main(void) {
	init();
	app_event_loop();
	deinit();
}
