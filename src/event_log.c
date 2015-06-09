#include "event_log.h"

static Event eventLog[EVENT_LOG_MAX_SIZE];

static uint8_t eventLogSize = 0,
               firstEventIdx = 0;

uint8_t event_log_size() {
    return eventLogSize;
}

bool log_full() {
    return eventLogSize == EVENT_LOG_MAX_SIZE;
}

bool log_event(EventType type, time_t timestamp) {
    if (log_full()) {
        APP_LOG(APP_LOG_LEVEL_ERROR, "log capacity exhausted");
        return false;
    }

    eventLog[(firstEventIdx + eventLogSize++) % EVENT_LOG_MAX_SIZE] = (Event){
        .type = type,
        .timestamp = timestamp
    };

    LOG(APP_LOG_LEVEL_DEBUG, "recorded one event");

    return true;
}

Event* log_get_event(uint8_t index) {
    if (index >= EVENT_LOG_MAX_SIZE) {
        APP_LOG(APP_LOG_LEVEL_ERROR, "log index %d: out of range", (int)index);
        return NULL;
    }

    return &eventLog[(firstEventIdx + index) % EVENT_LOG_MAX_SIZE];
}

void log_purge_from_start(uint8_t count) {
    if (count > eventLogSize) {
        APP_LOG(APP_LOG_LEVEL_ERROR,
            "requested to purge %d events, but log contains only %d events",
            (int)count, (int)eventLogSize
        );
        return;
    }

    LOG(APP_LOG_LEVEL_DEBUG, "purged %d events", (int)count);

    firstEventIdx = (firstEventIdx + count) % EVENT_LOG_MAX_SIZE;
    eventLogSize -= count;
}

void log_serialize_event(Event *event, uint8_t *buffer) {
    *(buffer++) = event->type;

    time_t timestamp = event->timestamp;
    for (uint8_t i = 0; i < 4; i++) {
        *(buffer++) = timestamp & 0xFF;
        timestamp = timestamp >> 8;
    }
}
