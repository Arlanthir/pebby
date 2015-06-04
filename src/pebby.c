#include "pebble.h"
#include <math.h>
#include <string.h>
#include <time.h>

/***** Useful Macros *****/
	
#define MAX(a, b) (( a > b)? a : b)
	
/***** Persist Keys *****/
#define PERSIST_BOTTLE 1
#define PERSIST_DIAPER 2
#define PERSIST_MOON_START 3
#define PERSIST_MOON_END 4


/***** Variables *****/	

static Window *window;

static Window *loadingWindow;
static TextLayer *loadingTextLayer;

// Texts

static TextLayer *bottleTextLayer;
static char timeTextUp[] = "00:00";	// Used by the system later
static TextLayer *bottleSinceTextLayer;
static char timeSinceTextUp[] = "(99 minutes ago)";	// Used by the system later

static TextLayer *diaperTextLayer;
static char timeTextMiddle[] = "00:00";	// Used by the system later
static TextLayer *diaperSinceTextLayer;
static char timeSinceTextMiddle[] = "(99 minutes ago)";	// Used by the system later

static TextLayer *moonTextLayer;
static char timeTextDown[14] = "";	// Used by the system later
static TextLayer *moonSinceTextLayer;
static char timeSinceTextDown[] = "(99 minutes ago)";	// Used by the system later

// Action Bar

static GBitmap *actionBottle;
static GBitmap *actionDiaper;
static GBitmap *actionMoon;

static ActionBarLayer *actionBar;

// Data

static int sleeping = 0;
static time_t bottleStart = 0;
static time_t diaperStart = 0;
static time_t sleepStart = 0;
static time_t sleepEnd = 0;


/***** Util *****/

static void setTimeText(time_t timestamp, char *text, TextLayer *textLayer) {
	if (timestamp == 0) {
		text[0] = '\0';
	} else {
		struct tm *time = localtime(&timestamp);
		strftime(text, sizeof(timeTextUp), (clock_is_24h_style()? "%H:%M" : "%I:%M"), time);
	}
	text_layer_set_text(textLayer, text);
}

static void setTimeSinceText(time_t timestamp, char *text, TextLayer *textLayer) {
	
	if (timestamp > 0) {
		time_t now = time(NULL);
		time_t elapsed = now - timestamp;

		if (elapsed < 60) {
			strcpy(text, "(just now)");
		} else if (elapsed < 3600) {
			int minutes = ceil((double) elapsed / 60);
			snprintf(text, sizeof(timeSinceTextUp), "(%d min ago)", minutes);
		} else {
			int hours = elapsed / 3600;
			snprintf(text, sizeof(timeSinceTextUp), "(%d h ago)", hours);
		}
	} else {
		text[0] = '\0';
	}
	
	// strftime(text, sizeof(timeTextUp), (clock_is_24h_style()? "%H:%M" : "%I:%M"), time);
	text_layer_set_text(textLayer, text);
}


static void setTimeRangeText(time_t startTimestamp, time_t endTimestamp, char *text, TextLayer *textLayer) {
	char sleepStartStr[] = "00:00";
	char sleepEndStr[] = "00:00";
	
	if (startTimestamp == 0 && endTimestamp == 0) {
		text[0] = '\0';
	} else {
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
	}
	
	text_layer_set_text(textLayer, text);
}


/***** Click Provider *****/

bool isLoading() {
    return window_stack_get_top_window() == loadingWindow;
}

void showSendingNotification() {
    text_layer_set_text(loadingTextLayer, "Sending...");

    if (!isLoading()) {
        window_stack_push(loadingWindow, true);
    }
}

void showFailedNotification() {
    text_layer_set_text(loadingTextLayer, "Failed");

    if (!isLoading()) {
        window_stack_push(loadingWindow, true);
    }
}

void hideNotification() {
    if (!isLoading()) {
        return;
    }

    window_stack_pop(true);
}

void sendToPhone(int key, uint32_t timestamp) {
	// Send value to phone
	DictionaryIterator *iter;

	if (app_message_outbox_begin(&iter) != APP_MSG_OK) {
        app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "unable to start outbound message");
        showFailedNotification();
        return;
    };

    if (!iter) {
        app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "unable to start outbound message");
        showFailedNotification();
        return;
    }

	dict_write_uint32(iter, key, timestamp);
	
    if (app_message_outbox_send() != APP_MSG_OK) {
        showFailedNotification();
        app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "unable to send outbound message");
        return;
    }

    showSendingNotification();
}

void up_single_click_handler(ClickRecognizerRef recognizer, void *context) {
	//Window *window = (Window *)context;
	//app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Click Up");

	ButtonId bt = click_recognizer_get_button_id(recognizer);
	char *targetText = timeTextUp;
	TextLayer *targetLayer = bottleTextLayer;
	int persistKey = PERSIST_BOTTLE;
	
	time_t t = time(NULL);
	
	if (bt == BUTTON_ID_SELECT) {
		targetText = timeTextMiddle;
		targetLayer = diaperTextLayer;
		persistKey = PERSIST_DIAPER;
		diaperStart = t;
		setTimeSinceText(diaperStart, timeSinceTextMiddle, diaperSinceTextLayer);
	} else {
		bottleStart = t;
		setTimeSinceText(bottleStart, timeSinceTextUp, bottleSinceTextLayer);
	}
	
	
	persist_write_int(persistKey, t);
	sendToPhone(persistKey, t);
	
	setTimeText(t, targetText, targetLayer);
}

void down_single_click_handler(ClickRecognizerRef recognizer, void *context) {
	//Window *window = (Window *)context;
	//app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Click Down");
	
	if (sleeping) {
		sleepEnd = time(NULL);
		persist_write_int(PERSIST_MOON_END, sleepEnd);
		sendToPhone(PERSIST_MOON_END, sleepEnd);
		sleeping = 0;
	} else {
		sleepStart = time(NULL);
		sleepEnd = 0;
		persist_write_int(PERSIST_MOON_START, sleepStart);
		sendToPhone(PERSIST_MOON_START, sleepStart);
		persist_write_int(PERSIST_MOON_END, sleepEnd);
		sleeping = 1;
	}
	
	setTimeRangeText(sleepStart, sleepEnd, timeTextDown, moonTextLayer);
	setTimeSinceText(MAX(sleepStart, sleepEnd), timeSinceTextDown, moonSinceTextLayer);
}

void config_provider(Window *window) {
	window_single_click_subscribe(BUTTON_ID_UP, up_single_click_handler);
	window_single_click_subscribe(BUTTON_ID_SELECT, up_single_click_handler);
	window_single_click_subscribe(BUTTON_ID_DOWN, down_single_click_handler);
}

/***** Tick Events *****/

static void handle_tick(struct tm *tick_time, TimeUnits units_changed) {
	if (bottleStart != 0) setTimeSinceText(bottleStart, timeSinceTextUp, bottleSinceTextLayer);
	if (diaperStart != 0) setTimeSinceText(diaperStart, timeSinceTextMiddle, diaperSinceTextLayer);
	if (sleepStart != 0 || sleepEnd != 0) setTimeSinceText(MAX(sleepStart, sleepEnd), timeSinceTextDown, moonSinceTextLayer);
}

/***** Watch-phone communication *****/

void out_sent_handler(DictionaryIterator *sent, void *context) {
	// Outgoing message was delivered
	app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Pebble: Out message delivered");
    hideNotification();
}


void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
	// Outgoing message failed
	char logMsg[64];
	snprintf(logMsg, 64, "Pebble: Out message failed, reason: %d", reason);
	app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, logMsg);

	if (reason != APP_MSG_SEND_TIMEOUT || !isLoading()) {
        showFailedNotification();
		return;
	}
	app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Retrying message send...");
	
    Tuple* failedMessage = dict_read_first(failed);

    if (!failedMessage) {
        app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "failed to reread failed message");
        showFailedNotification();
        return;
    }

    if (failedMessage->type != TUPLE_UINT || failedMessage->length != 4) {
        app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "invalid message type");
        showFailedNotification();
        return;
    }

    if (dict_read_next(failed)) {
        app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "invalid message content");
        showFailedNotification();
        return;
    }

    sendToPhone(failedMessage->key, failedMessage->value[0].uint32);
}


void in_received_handler(DictionaryIterator *received, void *context) {
	// Incoming message received
	app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Pebble: In message received");
	
	// Check for fields you expect to receive
	Tuple *text_tuple = dict_find(received, 0);
	
	// Act on the found fields received
	if (text_tuple) {
		app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Received message: %s", text_tuple->value->cstring);
		if (strcmp(text_tuple->value->cstring, "reset") == 0) {
			persist_write_int(PERSIST_BOTTLE, 0);
			persist_write_int(PERSIST_DIAPER, 0);
			persist_write_int(PERSIST_MOON_START, 0);
			persist_write_int(PERSIST_MOON_END, 0);
			
			timeTextUp[0] = '\0';
			timeTextMiddle[0] = '\0';
			timeTextDown[0] = '\0';
			
			text_layer_set_text(bottleTextLayer, timeTextUp);
			text_layer_set_text(diaperTextLayer, timeTextMiddle);
			text_layer_set_text(moonTextLayer, timeTextDown);
			
			timeSinceTextUp[0] = '\0';
			timeSinceTextMiddle[0] = '\0';
			timeSinceTextDown[0] = '\0';
			
			text_layer_set_text(bottleSinceTextLayer, timeSinceTextUp);
			text_layer_set_text(diaperSinceTextLayer, timeSinceTextMiddle);
			text_layer_set_text(moonSinceTextLayer, timeSinceTextDown);
		}
	}
}


void in_dropped_handler(AppMessageResult reason, void *context) {
	// Incoming message dropped
	app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Pebble: In message failed");
}



/***** App *****/

static void window_load(Window *window) {
	Layer *window_layer = window_get_root_layer(window);
	GRect bounds = layer_get_bounds(window_layer);
	bounds.size.h -= 6;

	// Text layers
	
	bottleSinceTextLayer = text_layer_create((GRect){ .origin = {0, bounds.size.h/3/2 + 2 }, .size = {bounds.size.w -  ACTION_BAR_WIDTH, 24} });
	text_layer_set_text_alignment(bottleSinceTextLayer, GTextAlignmentCenter);
	text_layer_set_text(bottleSinceTextLayer, "");
	text_layer_set_font(bottleSinceTextLayer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
	layer_add_child(window_layer, text_layer_get_layer(bottleSinceTextLayer));
	
	diaperSinceTextLayer = text_layer_create((GRect){ .origin = {0, bounds.size.h/2 + 2 }, .size = {bounds.size.w -  ACTION_BAR_WIDTH, 24} });
	text_layer_set_font(diaperSinceTextLayer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
	text_layer_set_text_alignment(diaperSinceTextLayer, GTextAlignmentCenter);
	text_layer_set_text(diaperSinceTextLayer, "");
	layer_add_child(window_layer, text_layer_get_layer(diaperSinceTextLayer));

	moonSinceTextLayer = text_layer_create((GRect){ .origin = {0, 5*bounds.size.h/3/2 + 2 }, .size = {bounds.size.w -  ACTION_BAR_WIDTH, 24} });
	text_layer_set_font(moonSinceTextLayer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
	text_layer_set_text_alignment(moonSinceTextLayer, GTextAlignmentCenter);
	text_layer_set_text(moonSinceTextLayer, "");
	layer_add_child(window_layer, text_layer_get_layer(moonSinceTextLayer));
	
	
	bottleTextLayer = text_layer_create((GRect){ .origin = {0, bounds.size.h/3/2 - 20 }, .size = {bounds.size.w -  ACTION_BAR_WIDTH, 24} });
	text_layer_set_text_alignment(bottleTextLayer, GTextAlignmentCenter);
	text_layer_set_text(bottleTextLayer, "");
	text_layer_set_font(bottleTextLayer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
	layer_add_child(window_layer, text_layer_get_layer(bottleTextLayer));
	
	diaperTextLayer = text_layer_create((GRect){ .origin = {0, bounds.size.h/2 - 20 }, .size = {bounds.size.w -  ACTION_BAR_WIDTH, 24} });
	text_layer_set_font(diaperTextLayer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
	text_layer_set_text_alignment(diaperTextLayer, GTextAlignmentCenter);
	text_layer_set_text(diaperTextLayer, "");
	layer_add_child(window_layer, text_layer_get_layer(diaperTextLayer));
	
	moonTextLayer = text_layer_create((GRect){ .origin = {0, 5*bounds.size.h/3/2 - 20 }, .size = {bounds.size.w -  ACTION_BAR_WIDTH, 24} });
	text_layer_set_font(moonTextLayer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
	text_layer_set_text_alignment(moonTextLayer, GTextAlignmentCenter);
	text_layer_set_text(moonTextLayer, "");
	layer_add_child(window_layer, text_layer_get_layer(moonTextLayer));
	
	
	// Time values initialization
	if (persist_exists(PERSIST_BOTTLE)) {
		bottleStart = persist_read_int(PERSIST_BOTTLE);
		setTimeText(bottleStart, timeTextUp, bottleTextLayer);
		setTimeSinceText(bottleStart, timeSinceTextUp, bottleSinceTextLayer);
	}
	
	if (persist_exists(PERSIST_DIAPER)) {
		diaperStart = persist_read_int(PERSIST_DIAPER);
		setTimeText(diaperStart, timeTextMiddle, diaperTextLayer);
		setTimeSinceText(diaperStart, timeSinceTextMiddle, diaperSinceTextLayer);
	}
	
	if (persist_exists(PERSIST_MOON_START)) {
		sleepStart = persist_read_int(PERSIST_MOON_START);
		
		sleepEnd = persist_exists(PERSIST_MOON_END)? persist_read_int(PERSIST_MOON_END) : 0;

		if (sleepEnd == 0 && sleepStart != 0) {
			sleeping = 1;
		}
		
		setTimeRangeText(sleepStart, sleepEnd, timeTextDown, moonTextLayer);
		setTimeSinceText(MAX(sleepStart, sleepEnd), timeSinceTextDown, moonSinceTextLayer);
	}
	
	// Action Bar
	
	// Initialize the action bar:
	actionBar = action_bar_layer_create();
	// Associate the action bar with the window:
	action_bar_layer_add_to_window(actionBar, window);
	// Set the click config provider:
	action_bar_layer_set_click_config_provider(actionBar, (ClickConfigProvider) config_provider);
	
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


static void createLoadingWindow() {
    loadingWindow = window_create();

    Layer *windowLayer = window_get_root_layer(loadingWindow);
    GRect bounds = layer_get_bounds(windowLayer);

    loadingTextLayer = text_layer_create((GRect){
        .origin = {0, 70},
        .size = {bounds.size.w, 20}
    });

    text_layer_set_text(loadingTextLayer, "Sending...");
    text_layer_set_text_alignment(loadingTextLayer, GTextAlignmentCenter);

    layer_add_child(windowLayer, text_layer_get_layer(loadingTextLayer));
}

static void init(void) {
	window = window_create();
	window_set_window_handlers(window, (WindowHandlers) {
		.load = window_load,
		.unload = window_unload
	});
	window_set_click_config_provider(window, (ClickConfigProvider) config_provider);
	tick_timer_service_subscribe(MINUTE_UNIT, handle_tick);

    createLoadingWindow();

	// Watch-phone communication
	
	app_message_register_inbox_received(in_received_handler);
	app_message_register_inbox_dropped(in_dropped_handler);
	app_message_register_outbox_sent(out_sent_handler);
	app_message_register_outbox_failed(out_failed_handler);
	
	const uint32_t inbound_size = 64;
	const uint32_t outbound_size = 64;
	app_message_open(inbound_size, outbound_size);
	
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
