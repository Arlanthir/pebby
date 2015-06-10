#include <pebble.h>

#include "communication.h"
#include "common.h"
#include "event_log.h"

#define MESSAGE_KEY_EVENT_BLOB 0

#define RETRY_TIMEOUT 5000

#define OUTGOING_MESSAGE_BUFFER_SIZE EVENT_LOG_MAX_SIZE * SERIALIZED_EVENT_SIZE + 9
#define INCOMING_MESSAGE_BUFFER_SIZE 10

static bool comm_ready = false;
static bool transmit_in_progress = false;
static uint8_t eventsInMessage;

static void bluetooth_connection_handler(bool state) {
    if (state) {
        LOG(APP_LOG_LEVEL_DEBUG, "Bluetooth connected, starting transmit");
        comm_transmit();
    } else {
        LOG(APP_LOG_LEVEL_DEBUG, "Bluetooth disconnected");
    }
}

static void outbox_sent(DictionaryIterator *iterator, void *context) {
    LOG(APP_LOG_LEVEL_DEBUG, "message sent successfully, purging %i events", (int)eventsInMessage);
    log_purge_from_start(eventsInMessage);
    transmit_in_progress = false;

    if (comm_ready && event_log_size() != 0) {
        LOG(APP_LOG_LEVEL_DEBUG, "log contains new events, sending...");
        comm_transmit();
    }
}

static void retry_timer_handler(void *data) {
    if (comm_ready) {
        LOG(APP_LOG_LEVEL_DEBUG, "retransmitting");
        comm_transmit();
    }
}

static void outbox_failed(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
    LOG(APP_LOG_LEVEL_DEBUG, "message send failed, scheduling retransmit, reason: %i", (int)reason);

    transmit_in_progress = false;

    app_timer_register(RETRY_TIMEOUT, retry_timer_handler, NULL);
}

static void inbox_received(DictionaryIterator *iterator, void *context) {
}

static void inbox_dropped(AppMessageResult reason, void *context) {
}

void comm_init() {
    bluetooth_connection_service_subscribe(bluetooth_connection_handler);

    app_message_register_inbox_received(inbox_received);
    app_message_register_inbox_dropped(inbox_dropped);
    app_message_register_outbox_sent(outbox_sent);
    app_message_register_outbox_failed(outbox_failed);

    AppMessageResult openResult =
        app_message_open(INCOMING_MESSAGE_BUFFER_SIZE, OUTGOING_MESSAGE_BUFFER_SIZE);

    if (openResult == APP_MSG_OK) {
        comm_ready = true;
        LOG(APP_LOG_LEVEL_DEBUG, "message system initialized");
    } else {
        comm_ready = false;
        LOG(APP_LOG_LEVEL_ERROR, "failed to start message system, reason %i", (int)openResult);
    }

    if (event_log_size() > 0) {
        comm_transmit();
    }
}

void comm_deinit() {
    comm_ready = false;
    
    bluetooth_connection_service_unsubscribe();
    app_message_deregister_callbacks();
}

void comm_transmit() {
    if (!comm_ready) {
        LOG(APP_LOG_LEVEL_ERROR, "message system not ready, aborting....");
        return;
    }

    if (transmit_in_progress) {
        LOG(APP_LOG_LEVEL_DEBUG, "transmit in progress, aborting...");
        return;
    }

    if (!bluetooth_connection_service_peek()) {
        LOG(APP_LOG_LEVEL_DEBUG, "no connection, will retry once connection is ready");
        return;
    }

    if (event_log_size() == 0) {
        LOG(APP_LOG_LEVEL_DEBUG, "event log is empty, nothing to transmit");
    }

    uint8_t bufferSize = log_calculate_serialized_size();
    uint8_t buffer[bufferSize];
    log_serialize(buffer);

    DictionaryIterator *iterator;

    uint8_t result = app_message_outbox_begin(&iterator);
    if (result != APP_MSG_OK) {
        LOG(APP_LOG_LEVEL_ERROR, "failed to start outgoing message, reason: %i", (int)result);
        return;
    }

    if (!iterator) {
        LOG(APP_LOG_LEVEL_ERROR, "failed to allocate iterator for outgoing message");
        return;
    }

    result = dict_write_data(iterator, MESSAGE_KEY_EVENT_BLOB, buffer, bufferSize);
    if (result != DICT_OK) {
        LOG(APP_LOG_LEVEL_ERROR, "serialization to dictionary failed, reason: %i", (int)result);
        return;
    }

    result = app_message_outbox_send();
    if (result != APP_MSG_OK) {
        LOG(APP_LOG_LEVEL_ERROR, "send failed, reason: %i", (int)result);
        return;
    }

    transmit_in_progress = true;
    eventsInMessage = event_log_size();
}
