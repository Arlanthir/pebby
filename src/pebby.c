#include "pebble.h"

static Window *window;

// Texts

static TextLayer *text_layer;

// Bitmaps

static GBitmap *bottleBlackImage;
static GBitmap *diaperBlackImage;
static GBitmap *moonBlackImage;

static BitmapLayer *bottleBlacklayer;
static BitmapLayer *diaperBlacklayer;
static BitmapLayer *moonBlacklayer;


static void window_load(Window *window) {
	Layer *window_layer = window_get_root_layer(window);
	GRect bounds = layer_get_bounds(window_layer);

	// We do this to account for the offset due to the status bar
	// at the top of the app window.
	GRect layer_frame_description = layer_get_frame(window_layer);
	layer_frame_description.origin.x = 0;
	layer_frame_description.origin.y = 0;

	// Text layers
	text_layer = text_layer_create(layer_frame_description);
	text_layer_set_text(text_layer, "");
	layer_add_child(window_layer, text_layer_get_layer(text_layer));

	
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
	text_layer_destroy(text_layer);
	
	bitmap_layer_destroy(bottleBlacklayer);
	bitmap_layer_destroy(diaperBlacklayer);
	bitmap_layer_destroy(moonBlacklayer);

	gbitmap_destroy(bottleBlackImage);
	gbitmap_destroy(diaperBlackImage);
	gbitmap_destroy(moonBlackImage);

}

static void init(void) {
	window = window_create();
	window_set_window_handlers(window, (WindowHandlers) {
		.load = window_load,
		.unload = window_unload
	});
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
