#include "ui.h"

#define TEXT_SIZE 20

static Window *window;

static ClickConfigProvider clickConfigProvider;

// Texts

static TextLayer *bottleTextLayer;
static TextLayer *bottleSinceTextLayer;
static char bottleLayerText[TEXT_SIZE];
static char bottleSinceLayerText[TEXT_SIZE];

static TextLayer *diaperTextLayer;
static TextLayer *diaperSinceTextLayer;
static char diaperLayerText[TEXT_SIZE];
static char diaperSinceLayerText[TEXT_SIZE];

static TextLayer *moonTextLayer;
static TextLayer *moonSinceTextLayer;
static char moonLayerText[TEXT_SIZE];
static char moonSinceLayerText[TEXT_SIZE];

static GBitmap *actionBottle;
static GBitmap *actionDiaper;
static GBitmap *actionMoon;

static ActionBarLayer *actionBar;

static time_t lastFeed = 0,
              lastDiaperChange = 0,
              lastSleepStart = 0,
              lastSleepStop = 0;

static void set_time_text(time_t timestamp, char *text, TextLayer *textLayer) {
	if (timestamp == 0) {
		text[0] = '\0';
	} else {
		struct tm *time = localtime(&timestamp);
		strftime(text, TEXT_SIZE, (clock_is_24h_style()? "%H:%M" : "%I:%M"), time);
	}
	text_layer_set_text(textLayer, text);
}

static void set_time_since_text(time_t timestamp, char *text, TextLayer *textLayer) {
	
	if (timestamp > 0) {
		time_t now = time(NULL);
		time_t elapsed = now - timestamp;

		if (elapsed < 60) {
			strcpy(text, "(just now)");
		} else if (elapsed < 3600) {
			int minutes = ceil((double) elapsed / 60);
			snprintf(text, TEXT_SIZE, "(%d min ago)", minutes);
		} else {
			int hours = elapsed / 3600;
			snprintf(text, TEXT_SIZE, "(%d h ago)", hours);
		}
	} else {
		text[0] = '\0';
	}
	
	text_layer_set_text(textLayer, text);
}

static void set_time_range_text(time_t startTimestamp, time_t endTimestamp, char *text, TextLayer *textLayer) {
	char sleepStartStr[] = "00:00";
	char sleepEndStr[] = "00:00";
	
	if (startTimestamp == 0 && endTimestamp == 0) {
		text[0] = '\0';
	} else {
		struct tm *time = localtime(&startTimestamp);

		strftime(sleepStartStr, TEXT_SIZE, (clock_is_24h_style()? "%H:%M" : "%I:%M"), time);


		if (endTimestamp != 0 && endTimestamp >= startTimestamp) {
			time = localtime(&endTimestamp);
			strftime(sleepEndStr, sizeof(sleepEndStr), (clock_is_24h_style()? "%H:%M" : "%I:%M"), time);
		} else {
			strcpy(sleepEndStr, "...");
		}

		strncpy(text, sleepStartStr, sizeof(sleepStartStr));
		strncat(text, " - ", 4);
		strncat(text, sleepEndStr, sizeof(sleepEndStr));
	}
	
	text_layer_set_text(textLayer, text);
}

static void update_text_layers() {
    if (lastFeed) {
        set_time_text(lastFeed, bottleLayerText, bottleTextLayer);
        set_time_since_text(lastFeed, bottleSinceLayerText, bottleSinceTextLayer);
    } else {
        text_layer_set_text(bottleTextLayer, "");
        text_layer_set_text(bottleSinceTextLayer, "");
    }

    if (lastDiaperChange) {
        set_time_text(lastDiaperChange, diaperLayerText, diaperTextLayer);
        set_time_since_text(lastDiaperChange, diaperSinceLayerText, diaperSinceTextLayer);
    } else {
        text_layer_set_text(diaperTextLayer, "");
        text_layer_set_text(diaperSinceTextLayer, "");
    }

    if (lastSleepStart) {
        set_time_range_text(lastSleepStart, lastSleepStop, moonLayerText, moonTextLayer);
        set_time_since_text(MAX(lastSleepStart, lastSleepStop), moonSinceLayerText, moonSinceTextLayer);
    } else {
        text_layer_set_text(moonTextLayer, "");
        text_layer_set_text(moonSinceTextLayer, "");
    }
}

static void window_load(Window *window) {
	Layer *window_layer = window_get_root_layer(window);
	GRect bounds = layer_get_bounds(window_layer);
	bounds.size.h -= 6;

	// Text layers
	
	bottleSinceTextLayer = text_layer_create((GRect){ .origin = {0, bounds.size.h/3/2 + 2 }, .size = {bounds.size.w -  ACTION_BAR_WIDTH, 24} });
	text_layer_set_text_alignment(bottleSinceTextLayer, GTextAlignmentCenter);
	text_layer_set_font(bottleSinceTextLayer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
	layer_add_child(window_layer, text_layer_get_layer(bottleSinceTextLayer));
	
	diaperSinceTextLayer = text_layer_create((GRect){ .origin = {0, bounds.size.h/2 + 2 }, .size = {bounds.size.w -  ACTION_BAR_WIDTH, 24} });
	text_layer_set_font(diaperSinceTextLayer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
	text_layer_set_text_alignment(diaperSinceTextLayer, GTextAlignmentCenter);
	layer_add_child(window_layer, text_layer_get_layer(diaperSinceTextLayer));

	moonSinceTextLayer = text_layer_create((GRect){ .origin = {0, 5*bounds.size.h/3/2 + 2 }, .size = {bounds.size.w -  ACTION_BAR_WIDTH, 24} });
	text_layer_set_font(moonSinceTextLayer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
	text_layer_set_text_alignment(moonSinceTextLayer, GTextAlignmentCenter);
	layer_add_child(window_layer, text_layer_get_layer(moonSinceTextLayer));
	
	bottleTextLayer = text_layer_create((GRect){ .origin = {0, bounds.size.h/3/2 - 20 }, .size = {bounds.size.w -  ACTION_BAR_WIDTH, 24} });
	text_layer_set_text_alignment(bottleTextLayer, GTextAlignmentCenter);
	text_layer_set_font(bottleTextLayer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
	layer_add_child(window_layer, text_layer_get_layer(bottleTextLayer));
	
	diaperTextLayer = text_layer_create((GRect){ .origin = {0, bounds.size.h/2 - 20 }, .size = {bounds.size.w -  ACTION_BAR_WIDTH, 24} });
	text_layer_set_font(diaperTextLayer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
	text_layer_set_text_alignment(diaperTextLayer, GTextAlignmentCenter);
	layer_add_child(window_layer, text_layer_get_layer(diaperTextLayer));
	
	moonTextLayer = text_layer_create((GRect){ .origin = {0, 5*bounds.size.h/3/2 - 20 }, .size = {bounds.size.w -  ACTION_BAR_WIDTH, 24} });
	text_layer_set_font(moonTextLayer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
	text_layer_set_text_alignment(moonTextLayer, GTextAlignmentCenter);
	layer_add_child(window_layer, text_layer_get_layer(moonTextLayer));
	
	// Time values initialization
	if (persist_exists(PERSIST_BOTTLE)) {
        lastFeed = persist_read_int(PERSIST_BOTTLE);
	}
	
	if (persist_exists(PERSIST_DIAPER)) {
        lastDiaperChange = persist_read_int(PERSIST_DIAPER);
	}

    if (persist_exists(PERSIST_MOON_START)) {
        lastSleepStart = persist_read_int(PERSIST_MOON_START);
    }

    if (persist_exists(PERSIST_MOON_END)) {
        lastSleepStop = persist_read_int(PERSIST_MOON_END);
    }

    update_text_layers();
	
	// Action Bar
	
	// Initialize the action bar:
	actionBar = action_bar_layer_create();
	// Associate the action bar with the window:
	action_bar_layer_add_to_window(actionBar, window);
	// Set the click config provider:
	action_bar_layer_set_click_config_provider(actionBar, clickConfigProvider);
	
	// Set the icons:
	
	actionBottle = gbitmap_create_with_resource(RESOURCE_ID_ACTION_BOTTLE);
	actionDiaper = gbitmap_create_with_resource(RESOURCE_ID_ACTION_DIAPER);
	actionMoon = gbitmap_create_with_resource(RESOURCE_ID_ACTION_MOON);
	
	action_bar_layer_set_icon(actionBar, BUTTON_ID_UP, actionBottle);
	action_bar_layer_set_icon(actionBar, BUTTON_ID_SELECT, actionDiaper);
	action_bar_layer_set_icon(actionBar, BUTTON_ID_DOWN, actionMoon);
}

static void window_unload(Window *window) {
	text_layer_destroy(bottleTextLayer);
	text_layer_destroy(diaperTextLayer);
	text_layer_destroy(moonTextLayer);
	
	text_layer_destroy(bottleSinceTextLayer);
	text_layer_destroy(diaperSinceTextLayer);
	text_layer_destroy(moonSinceTextLayer);
	
	action_bar_layer_destroy(actionBar);

	gbitmap_destroy(actionBottle);
	gbitmap_destroy(actionDiaper);
	gbitmap_destroy(actionMoon);
}

void ui_init(ClickConfigProvider ccProvider) {
	window = window_create();

    clickConfigProvider = ccProvider;

	window_set_window_handlers(window, (WindowHandlers) {
		.load = window_load,
		.unload = window_unload
	});

	window_stack_push(window, true);
}

void ui_diaper_change(time_t timestamp) {
    lastDiaperChange = timestamp;
    update_text_layers();
}

void ui_feed(time_t timestamp) {
    lastFeed = timestamp;
    update_text_layers();
}

void ui_sleep_start(time_t timestamp) {
    lastSleepStart = timestamp;
    lastSleepStop = 0;
    update_text_layers();
}

void ui_sleep_stop(time_t timestamp) {
    lastSleepStop = timestamp;
    update_text_layers();
}

void ui_update() {
    update_text_layers();
}

void ui_deinit() {
    window_destroy(window);
}
