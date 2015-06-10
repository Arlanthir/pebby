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

void log_serialize(uint8_t *buffer) {
    *(buffer++) = eventLogSize;

    for (uint8_t idx = 0; idx < eventLogSize; idx++) {
        Event *event = log_get_event(idx);

        *(buffer++) = event->type;
        uint32_t timestamp = event->timestamp;

        for (uint8_t i = 0; i < 4; i++) {
            *(buffer++) = timestamp & 0xFF;
            timestamp = timestamp >> 8;
        }
    }
}

bool log_unserialize(const uint8_t *buffer) {
    uint8_t event_count = *(buffer++);

    if (event_count > EVENT_LOG_MAX_SIZE) {
        LOG(APP_LOG_LEVEL_ERROR, "too many events in serialized buffer, unable to unserialize: %i vs. %i",
            (int)event_count, (int)EVENT_LOG_MAX_SIZE);

        return false;
    }

    eventLogSize = event_count;
    firstEventIdx = 0;

    for (uint8_t idx = 0; idx < event_count; idx++) {
        uint8_t type = *(buffer++);
        uint32_t timestamp = buffer[0] | buffer[1] << 8 | buffer[2] << 16 | buffer[3] << 24;

        buffer += 4;

        if (type > 3) {
            LOG(APP_LOG_LEVEL_ERROR, "invalid event type %i in serialized buffer at index %i, corrupt data? Aborting...",
                (int)type, (int)idx);

            event_count = 0;

            return false;
        }

        eventLog[idx] = (Event){
            .type = type,
            .timestamp = timestamp
        };
    }

    return true;
}

uint8_t log_calculate_serialized_size() {
    return eventLogSize * SERIALIZED_EVENT_SIZE + 1;
}

void log_init() {
    if (persist_exists(PERSIST_LOG)) {
        LOG(APP_LOG_LEVEL_DEBUG, "found persisted events, reading...");

        uint8_t buffer[255];

        persist_read_data(PERSIST_LOG, buffer, 255);

        if (!log_unserialize(buffer)) {
            LOG(APP_LOG_LEVEL_ERROR, "unable to deserialize persisted events");
        } else {
            LOG(APP_LOG_LEVEL_DEBUG, "successfully unserialized %i events", (int)eventLogSize);
        }
    }
}

void log_deinit() {
    if (eventLogSize > 0) {
        LOG(APP_LOG_LEVEL_DEBUG, "log not clean on exit, persisting remaining events");

        uint8_t bufferSize = log_calculate_serialized_size();
        uint8_t buffer[bufferSize];

        log_serialize(buffer);

        if (persist_write_data(PERSIST_LOG, buffer, bufferSize) != bufferSize) {
            LOG(APP_LOG_LEVEL_ERROR, "persist failed");
        } else {
            LOG(APP_LOG_LEVEL_DEBUG, "persisted %i events", (int)eventLogSize);
        }
    } else {
        persist_delete(PERSIST_LOG);
    }
}
