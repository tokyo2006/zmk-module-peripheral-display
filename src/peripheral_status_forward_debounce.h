/*
 * Debounce helper for central-side forward. Pulled into a header so
 * unit tests can call it without pulling in BLE stack.
 */
#pragma once
#include <stdbool.h>
#include <stdint.h>

enum peripheral_debounce_key {
    KEY_LAYER = 0,
    KEY_MODS,
    KEY_WPM,
    KEY_BATTERY,
    KEY_OUTPUT,
    KEY_HID,
    KEY_ENDPOINT,
    KEY_ACTIVITY,
    KEY_COUNT,
};

struct peripheral_debounce_state {
    uint32_t last_fired_ms[KEY_COUNT];
};

/* Returns true if a new notify should be sent for @p key at @p now_ms.
 * Pass @p force=1 to bypass debounce (used by the 1Hz heartbeat). */
bool peripheral_status_should_fire(struct peripheral_debounce_state *s,
                                   enum peripheral_debounce_key key,
                                   uint32_t now_ms, int force);
