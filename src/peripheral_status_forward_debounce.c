/*
 * Copyright (c) 2026 The zmk-module-peripheral-display Contributors
 * SPDX-License-Identifier: MIT
 */
#include "peripheral_status_forward_debounce.h"

#define DEBOUNCE_MS_LAYER    0
#define DEBOUNCE_MS_MODS     0
#define DEBOUNCE_MS_WPM      200
#define DEBOUNCE_MS_BATTERY  30000
#define DEBOUNCE_MS_OUTPUT   0
#define DEBOUNCE_MS_HID      0
#define DEBOUNCE_MS_ENDPOINT 0
#define DEBOUNCE_MS_ACTIVITY 0

static const uint32_t window_ms[KEY_COUNT] = {
    [KEY_LAYER]    = DEBOUNCE_MS_LAYER,
    [KEY_MODS]     = DEBOUNCE_MS_MODS,
    [KEY_WPM]      = DEBOUNCE_MS_WPM,
    [KEY_BATTERY]  = DEBOUNCE_MS_BATTERY,
    [KEY_OUTPUT]   = DEBOUNCE_MS_OUTPUT,
    [KEY_HID]      = DEBOUNCE_MS_HID,
    [KEY_ENDPOINT] = DEBOUNCE_MS_ENDPOINT,
    [KEY_ACTIVITY] = DEBOUNCE_MS_ACTIVITY,
};

bool peripheral_status_should_fire(struct peripheral_debounce_state *s,
                                   enum peripheral_debounce_key key,
                                   uint32_t now_ms, int force)
{
    if (force) {
        s->last_fired_ms[key] = now_ms;
        return true;
    }
    if (now_ms - s->last_fired_ms[key] >= window_ms[key]) {
        s->last_fired_ms[key] = now_ms;
        return true;
    }
    return false;
}
