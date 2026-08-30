/*
 * Test: debounce logic for forward trigger.
 */
#include <zephyr/ztest.h>
#include <zephyr/kernel.h>

/* Forward declaration of the function under test (implemented in
 * src/peripheral_status_forward.c, but we test it via a thin shim
 * included inline). For testability the debounce function lives in
 * a header so it can be unit-tested. */
#include "../../../src/peripheral_status_forward_debounce.h"

ZTEST_SUITE(peripheral_status_debounce, NULL, NULL, NULL, NULL, NULL);

ZTEST(peripheral_status_debounce, test_wpm_pass)
{
    struct peripheral_debounce_state s = {0};
    /* First call on a non-zero-window key: passes (no prior fire). */
    zassert_true(peripheral_status_should_fire(&s, KEY_WPM, 1000, 0));
    /* Within debounce window: should NOT fire. */
    zassert_false(peripheral_status_should_fire(&s, KEY_WPM, 1100, 0));
}

ZTEST(peripheral_status_debounce, test_wpm_window)
{
    struct peripheral_debounce_state s = {0};
    /* WPM: 200ms window. */
    zassert_true(peripheral_status_should_fire(&s, KEY_WPM, 1000, 0));
    zassert_false(peripheral_status_should_fire(&s, KEY_WPM, 1100, 0));
    zassert_true(peripheral_status_should_fire(&s, KEY_WPM, 1201, 0));
}

ZTEST(peripheral_status_debounce, test_battery_long_window)
{
    struct peripheral_debounce_state s = {0};
    /* Battery: 30s window. Start at t=31000ms so initial check passes. */
    zassert_true(peripheral_status_should_fire(&s, KEY_BATTERY, 31000, 0));
    zassert_false(peripheral_status_should_fire(&s, KEY_BATTERY, 41000, 0));
    zassert_true(peripheral_status_should_fire(&s, KEY_BATTERY, 61001, 0));
}

ZTEST(peripheral_status_debounce, test_different_keys_independent)
{
    struct peripheral_debounce_state s = {0};
    zassert_true(peripheral_status_should_fire(&s, KEY_LAYER, 1000, 0));
    zassert_true(peripheral_status_should_fire(&s, KEY_MODS, 1000, 0)); /* independent */
}
