#ifndef EVENT_LOG_H
#define EVENT_LOG_H

#include <pebble.h>

#include "common.h"

#define EVENT_LOG_MAX_SIZE 50

typedef enum {
    EVENT_TYPE_FEED = 0,
    EVENT_TYPE_DIAPER_CHANGE = 1,
    EVENT_TYPE_SLEEP_START = 2,
    EVENT_TYPE_SLEEP_STOP = 3
} EventType;

typedef struct {
    EventType type;
    time_t timestamp;
} Event;

uint8_t event_log_size();

uint8_t log_full();

uint8_t log_event(EventType type, time_t timestamp);

Event* get_event(uint8_t index);

void purge_from_start(uint8_t count);

uint8_t serialize_event(Event* event, uint8_t *buffer);

#endif // EVENT_LOG_H
