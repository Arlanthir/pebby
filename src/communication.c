#include <pebble.h>

#include "communication.h"
#include "common.h"
#include "event_log.h"
#include "ui.h"

#define MESSAGE_KEY_EVENT_BLOB 1
#define MESSAGE_KEY_MESSAGE_TYPE 0

#define MESSAGE_TYPE_INVALID_MESSAGE 0
#define MESSAGE_TYPE_EVENT_TRANSMISSION 1
#define MESSAGE_TYPE_RESET_ACK 2
#define MESSAGE_TYPE_RESET_REQUEST 3

#define RETRY_TIMEOUT 5000

#define OUTGOING_MESSAGE_BUFFER_SIZE EVENT_LOG_MAX_SIZE * SERIALIZED_EVENT_SIZE + 2 + 15
#define INCOMING_MESSAGE_BUFFER_SIZE 10

static bool commReady = false;
static bool transmitInProgress = false;
static bool resetSignaled = false;

static uint8_t eventsInMessage;

static void bluetooth_connection_handler(bool state) {
    if (state) {
        LOG(APP_LOG_LEVEL_DEBUG, "Bluetooth connected, starting transmit");
        comm_transmit_events();
    } else {
        LOG(APP_LOG_LEVEL_DEBUG, "Bluetooth disconnected");
    }
}

static bool ready_to_transmit() {
    if (!commReady) {
        LOG(APP_LOG_LEVEL_ERROR, "message system not ready, aborting....");
        return false;
    }

    if (transmitInProgress) {
        LOG(APP_LOG_LEVEL_DEBUG, "transmit in progress, aborting...");
        return false;
    }

    if (!bluetooth_connection_service_peek()) {
        LOG(APP_LOG_LEVEL_DEBUG, "no connection, will retry once connection is ready");
        return false;
    }

    return true;
}

static DictionaryIterator* start_outbound_mesage(uint8_t type) {
    DictionaryIterator *iterator;

    uint8_t result = app_message_outbox_begin(&iterator);
    if (result != APP_MSG_OK) {
        LOG(APP_LOG_LEVEL_ERROR, "failed to start outgoing message, reason: %i", (int)result);
        return NULL;
    }

    if (!iterator) {
        LOG(APP_LOG_LEVEL_ERROR, "failed to allocate iterator for outgoing message");
        return NULL;
    }

    result = dict_write_uint8(iterator, MESSAGE_KEY_MESSAGE_TYPE, type);
    if (result != DICT_OK) {
        LOG(APP_LOG_LEVEL_ERROR, "unable to set message type, reason: %i", (int)result);
        return NULL;
    }

    return iterator;
}

static void transmit_reset_ack() {
    if (!ready_to_transmit()) {
        return;
    }

    DictionaryIterator *iterator = start_outbound_mesage(MESSAGE_TYPE_RESET_ACK);

    if (!iterator) return;

    uint8_t result = app_message_outbox_send();
    if (result != APP_MSG_OK) {
        LOG(APP_LOG_LEVEL_ERROR, "send failed, reason: %i", (int)result);
        return;
    }

    transmitInProgress = true;
}

static uint8_t get_message_type(DictionaryIterator *iterator) {
    Tuple* tuple = dict_find(iterator, MESSAGE_KEY_MESSAGE_TYPE);

    if (tuple == NULL) {
        LOG(APP_LOG_LEVEL_DEBUG, "message type missing");
        return MESSAGE_TYPE_INVALID_MESSAGE;
    }

    switch (tuple->type) {
        case TUPLE_INT:
        case TUPLE_UINT:
            break;

        default:
            LOG(APP_LOG_LEVEL_DEBUG, "invalid message type");
            return MESSAGE_TYPE_INVALID_MESSAGE;
    }

    switch (tuple->length) {
        case 1:
            return tuple->value[0].uint8;

        case 2:
            return tuple->value[0].uint16;

        case 4:
            return tuple->value[0].uint32;

        default:
            LOG(APP_LOG_LEVEL_DEBUG, "invalid message type length %i", (int)tuple->length);
            return MESSAGE_TYPE_INVALID_MESSAGE;

    }
}

void after_event_transmission() {
    LOG(APP_LOG_LEVEL_DEBUG, "message sent successfully, purging %i events", (int)eventsInMessage);
    log_purge_from_start(eventsInMessage);

    if (commReady && event_log_size() != 0) {
        LOG(APP_LOG_LEVEL_DEBUG, "log contains new events, sending...");
        comm_transmit_events();
    }
}

void after_reset_ack() {
    resetSignaled = false;
    log_reset();
    ui_reset();
}

static void outbox_sent(DictionaryIterator *iterator, void *context) {
    uint8_t messageType = get_message_type(iterator);
    
    transmitInProgress = false;
    LOG(APP_LOG_LEVEL_DEBUG, "message sent successful");

    switch (messageType) {
        case MESSAGE_TYPE_EVENT_TRANSMISSION:
            after_event_transmission();
            break;

        case MESSAGE_TYPE_RESET_ACK:
            after_reset_ack();
            break;
    }
}

static void retry_timer_handler(void *data) {
    if (!commReady) return;

    if (resetSignaled) {
        LOG(APP_LOG_LEVEL_DEBUG, "retransmitting reset ack");
        transmit_reset_ack();
    } else {
        LOG(APP_LOG_LEVEL_DEBUG, "retransmitting events");
        comm_transmit_events();
    }
}

static void outbox_failed(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
    LOG(APP_LOG_LEVEL_DEBUG, "message send failed, scheduling retransmit, reason: %i", (int)reason);

    transmitInProgress = false;

    app_timer_register(RETRY_TIMEOUT, retry_timer_handler, NULL);
}

static void start_reset() {
    resetSignaled = true;
    transmit_reset_ack();
}

static void inbox_received(DictionaryIterator *iterator, void *context) {
    uint8_t messageType = get_message_type(iterator);

    switch (messageType) {
        case MESSAGE_TYPE_RESET_REQUEST:
            start_reset();
            break;

        default:
            LOG(APP_LOG_LEVEL_ERROR, "invalid incoming message type %i", (int)messageType);
            break;
    }
}

static void inbox_dropped(AppMessageResult reason, void *context) {
    LOG(APP_LOG_LEVEL_DEBUG, "incoming message dropped");
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
        commReady = true;
        LOG(APP_LOG_LEVEL_DEBUG, "message system initialized");
    } else {
        commReady = false;
        LOG(APP_LOG_LEVEL_ERROR, "failed to start message system, reason %i", (int)openResult);
    }

    if (event_log_size() > 0) {
        comm_transmit_events();
    }
}

void comm_deinit() {
    commReady = false;
    
    bluetooth_connection_service_unsubscribe();
    app_message_deregister_callbacks();
}

void comm_transmit_events() {
    if (!ready_to_transmit()) {
        return;
    }

    if (resetSignaled) {
        LOG(APP_LOG_LEVEL_DEBUG, "reset signaled, refusing to send event log");
        return;
    }

    if (event_log_size() == 0) {
        LOG(APP_LOG_LEVEL_DEBUG, "event log is empty, nothing to transmit");
        return;
    }

    uint8_t bufferSize = log_calculate_serialized_size();
    uint8_t buffer[bufferSize];
    log_serialize(buffer);

    DictionaryIterator *iterator = start_outbound_mesage(MESSAGE_TYPE_EVENT_TRANSMISSION);

    if (!iterator) return;

    uint8_t result = dict_write_data(iterator, MESSAGE_KEY_EVENT_BLOB, buffer, bufferSize);
    if (result != DICT_OK) {
        LOG(APP_LOG_LEVEL_ERROR, "serialization to dictionary failed, reason: %i", (int)result);
        return;
    }

    result = app_message_outbox_send();
    if (result != APP_MSG_OK) {
        LOG(APP_LOG_LEVEL_ERROR, "send failed, reason: %i", (int)result);
        return;
    }

    transmitInProgress = true;
    eventsInMessage = event_log_size();
}
