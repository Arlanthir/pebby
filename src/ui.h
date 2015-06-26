#ifndef UI_H
#define UI_H

#include <pebble.h>
#include <time.h>
#include <math.h>
#include <string.h>

#include "common.h"

void ui_diaper_change(time_t timestamp);

void ui_feed(time_t timestamp);

void ui_sleep_start(time_t timestamp);

void ui_sleep_stop(time_t timestamp);

void ui_init(ClickConfigProvider clickConfigProvider);

void ui_update();

void ui_deinit();

void ui_reset();

#endif // UI_H
