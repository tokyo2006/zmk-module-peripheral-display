/*
 * Copyright (c) 2026 The zmk-module-peripheral-display Contributors
 * SPDX-License-Identifier: MIT
 *
 * Central-side forward: subscribe to ZMK status events, rebuild the current
 * state, and send it to the peripheral over the ASDC BLE L2CAP data channel
 * (see src/asdc). The peripheral's ASDC receive callback feeds the local
 * shadow state that the display widgets poll.
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/sys/util.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>

#include <zmk/peripheral_status.h>
#include <zmk/asdc.h>
#include "peripheral_status_forward_debounce.h"

LOG_MODULE_DECLARE(peripheral_status, CONFIG_ZMK_LOG_LEVEL);

#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL) || !IS_ENABLED(CONFIG_ZMK_SPLIT)
#include <zmk/keymap.h>
#endif
#include <zmk/battery.h>
#include <zmk/wpm.h>
#include <zmk/ble.h>
#include <zmk/usb.h>
#include <zmk/endpoints.h>
#include <zmk/hid.h>
#include <zmk/activity.h>

#include <zmk/event_manager.h>
#include <zmk/keys.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/events/wpm_state_changed.h>
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/events/hid_indicators_changed.h>
#include <zmk/events/activity_state_changed.h>
#include <zmk/events/endpoint_changed.h>

#if IS_ENABLED(CONFIG_ZMK_PERIPHERAL_STATUS_FORWARD)

/* Wire-format version. Matches prospector's PROSPECTOR_ENCODE_VERSION()
 * for v2.2 (major=2, minor=2). */
#define PERIPHERAL_STATUS_WIRE_VERSION 0x22

static void fill_current_state(struct peripheral_status_adv_data *s) {
    memset(s, 0, sizeof(*s));
    s->manufacturer_id[0] = 0xFF;
    s->manufacturer_id[1] = 0xFF;
    s->service_uuid[0]    = 0xAB;
    s->service_uuid[1]    = 0xCD;
    s->version = PERIPHERAL_STATUS_WIRE_VERSION;

    uint8_t level = zmk_battery_state_of_charge();
    if (level > 100) level = 100;
    s->battery_level = level;

#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL) || !IS_ENABLED(CONFIG_ZMK_SPLIT)
    uint8_t layer = zmk_keymap_highest_layer_active();
    s->active_layer = layer;
    const char *name = zmk_keymap_layer_name(zmk_keymap_layer_index_to_id(layer));
    if (name && name[0] != '\0') {
        size_t len = MIN(strlen(name), sizeof(s->layer_name));
        memcpy(s->layer_name, name, len);
    }
#endif

#if IS_ENABLED(CONFIG_ZMK_BLE)
    s->profile_slot = (uint8_t)zmk_ble_active_profile_index();
#endif

    uint8_t flags = 0;
#if IS_ENABLED(CONFIG_ZMK_USB)
    if (zmk_usb_is_powered())   flags |= PERIPHERAL_STATUS_FLAG_USB_CONNECTED;
    if (zmk_usb_is_hid_ready()) flags |= PERIPHERAL_STATUS_FLAG_USB_HID_READY;
#endif
#if IS_ENABLED(CONFIG_ZMK_BLE)
    if (zmk_ble_active_profile_is_connected()) flags |= PERIPHERAL_STATUS_FLAG_BLE_CONNECTED;
    if (!zmk_ble_active_profile_is_open())     flags |= PERIPHERAL_STATUS_FLAG_BLE_BONDED;
#endif
    enum zmk_transport selected = zmk_endpoint_get_selected().transport;
    if (selected == ZMK_TRANSPORT_NONE) {
        selected = zmk_endpoint_get_preferred_transport();
    }
    if (selected == ZMK_TRANSPORT_USB) {
        flags |= PERIPHERAL_STATUS_FLAG_OUTPUT_USB_SELECTED;
    }
    s->status_flags = flags;

    s->device_role = 1; /* CENTRAL */
    s->device_index = 0;

    struct zmk_hid_keyboard_report *report = zmk_hid_get_keyboard_report();
    if (report) {
        uint8_t mods = report->body.modifiers;
        if (mods & 0x01) s->modifier_flags |= PERIPHERAL_MOD_FLAG_LCTL;
        if (mods & 0x10) s->modifier_flags |= PERIPHERAL_MOD_FLAG_RCTL;
        if (mods & 0x02) s->modifier_flags |= PERIPHERAL_MOD_FLAG_LSFT;
        if (mods & 0x20) s->modifier_flags |= PERIPHERAL_MOD_FLAG_RSFT;
        if (mods & 0x04) s->modifier_flags |= PERIPHERAL_MOD_FLAG_LALT;
        if (mods & 0x40) s->modifier_flags |= PERIPHERAL_MOD_FLAG_RALT;
        if (mods & 0x08) s->modifier_flags |= PERIPHERAL_MOD_FLAG_LGUI;
        if (mods & 0x80) s->modifier_flags |= PERIPHERAL_MOD_FLAG_RGUI;
    }

    int wpm = zmk_wpm_get_state();
    if (wpm < 0)   wpm = 0;
    if (wpm > 255) wpm = 255;
    s->wpm_value = (uint8_t)wpm;
}

static struct peripheral_debounce_state debounce = {0};

static void raise_status(enum peripheral_debounce_key key, bool force) {
    /* Debounce per event type (battery 30 s, wpm 200 ms, others immediate)
     * so a burst of events doesn't spam the ASDC queue. The heartbeat forces
     * a send regardless. */
    if (!peripheral_status_should_fire(&debounce, key, k_uptime_get_32(), force)) {
        return;
    }

    struct peripheral_status_adv_data s;
    fill_current_state(&s);

    const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(asdc0));
    if (!device_is_ready(dev)) {
        LOG_WRN("peripheral-display: asdc0 not ready");
        return;
    }

    LOG_INF("peripheral-display: send batt=%u layer=%u flags=0x%02x "
            "mods=0x%02x wpm=%u",
            (unsigned)s.battery_level, (unsigned)s.active_layer,
            (unsigned)s.status_flags, (unsigned)s.modifier_flags,
            (unsigned)s.wpm_value);

    int err = asdc_send(dev, (const uint8_t *)&s, sizeof(s), 0);
    if (err < 0) {
        LOG_WRN("peripheral-display: asdc send failed err=%d", err);
    }
}

/* --- Heartbeat: periodically re-send so the peripheral always has fresh state --- */

static void heartbeat_work_handler(struct k_work *work) {
    ARG_UNUSED(work);
    raise_status(KEY_ACTIVITY, true);
}

K_WORK_DEFINE(heartbeat_work, heartbeat_work_handler);

static void heartbeat_timer_handler(struct k_timer *timer) {
    ARG_UNUSED(timer);
    k_work_submit(&heartbeat_work);
}

K_TIMER_DEFINE(heartbeat_timer, heartbeat_timer_handler, NULL);

/* --- Event listeners: re-send a full status snapshot on any change --- */

static int on_layer_changed(const zmk_event_t *eh) {
    ARG_UNUSED(eh);
    raise_status(KEY_LAYER, false);
    return ZMK_EV_EVENT_BUBBLE;
}
ZMK_LISTENER(peripheral_status_layer, on_layer_changed);
ZMK_SUBSCRIPTION(peripheral_status_layer, zmk_layer_state_changed);

/* zmk_modifiers_state_changed is never actually raised by ZMK core (no
 * caller in hid.c/hid_listener.c) -- plain &kp LSHIFT-style modifier presses
 * only flow through zmk_keycode_state_changed, so we watch that instead and
 * filter to modifier keycodes with is_mod() to avoid firing on every key. */

/* ZMK's own hid_listener is subscribed to the same event and is the one
 * that actually updates zmk_hid's modifier bits -- listener order across
 * this module and core ZMK isn't something we control, and in practice
 * ours can run first. Reading zmk_hid_get_keyboard_report() synchronously
 * here would then see the state from *before* this key's press/release,
 * always one step behind. Defer the actual read to a work item so it runs
 * after the whole synchronous event-dispatch chain (hid_listener included)
 * has finished. */
static void mods_raise_work_handler(struct k_work *work) {
    ARG_UNUSED(work);
    raise_status(KEY_MODS, false);
}
K_WORK_DEFINE(mods_raise_work, mods_raise_work_handler);

static int on_mods_changed(const zmk_event_t *eh) {
    const struct zmk_keycode_state_changed *ev = as_zmk_keycode_state_changed(eh);
    if (!ev) {
        return ZMK_EV_EVENT_BUBBLE;
    }
    /* Either the keycode itself is a modifier (plain &kp LSHIFT), or it
     * carries modifier bits alongside a non-mod key (e.g. &kp LC(A)). */
    bool is_mod_related = is_mod(ev->usage_page, ev->keycode) ||
                          ev->explicit_modifiers != 0 || ev->implicit_modifiers != 0;
    if (!is_mod_related) {
        return ZMK_EV_EVENT_BUBBLE;
    }
    k_work_submit(&mods_raise_work);
    return ZMK_EV_EVENT_BUBBLE;
}
ZMK_LISTENER(peripheral_status_mods, on_mods_changed);
ZMK_SUBSCRIPTION(peripheral_status_mods, zmk_keycode_state_changed);

static int on_battery_changed(const zmk_event_t *eh) {
    ARG_UNUSED(eh);
    raise_status(KEY_BATTERY, false);
    return ZMK_EV_EVENT_BUBBLE;
}
ZMK_LISTENER(peripheral_status_battery, on_battery_changed);
ZMK_SUBSCRIPTION(peripheral_status_battery, zmk_battery_state_changed);

static int on_wpm_changed(const zmk_event_t *eh) {
    ARG_UNUSED(eh);
    raise_status(KEY_WPM, false);
    return ZMK_EV_EVENT_BUBBLE;
}
ZMK_LISTENER(peripheral_status_wpm, on_wpm_changed);
ZMK_SUBSCRIPTION(peripheral_status_wpm, zmk_wpm_state_changed);

static int on_output_changed(const zmk_event_t *eh) {
    ARG_UNUSED(eh);
    raise_status(KEY_OUTPUT, false);
    return ZMK_EV_EVENT_BUBBLE;
}
ZMK_LISTENER(peripheral_status_output, on_output_changed);
ZMK_SUBSCRIPTION(peripheral_status_output, zmk_usb_conn_state_changed);

static int on_endpoint_changed(const zmk_event_t *eh) {
    ARG_UNUSED(eh);
    raise_status(KEY_ENDPOINT, false);
    return ZMK_EV_EVENT_BUBBLE;
}
ZMK_LISTENER(peripheral_status_endpoint, on_endpoint_changed);
ZMK_SUBSCRIPTION(peripheral_status_endpoint, zmk_endpoint_changed);

static int on_hid_changed(const zmk_event_t *eh) {
    ARG_UNUSED(eh);
    raise_status(KEY_HID, false);
    return ZMK_EV_EVENT_BUBBLE;
}
ZMK_LISTENER(peripheral_status_hid, on_hid_changed);
ZMK_SUBSCRIPTION(peripheral_status_hid, zmk_hid_indicators_changed);

static int on_activity_changed(const zmk_event_t *eh) {
    ARG_UNUSED(eh);
    raise_status(KEY_ACTIVITY, false);
    return ZMK_EV_EVENT_BUBBLE;
}
ZMK_LISTENER(peripheral_status_activity, on_activity_changed);
ZMK_SUBSCRIPTION(peripheral_status_activity, zmk_activity_state_changed);

static int peripheral_status_forward_init(const struct device *device) {
    ARG_UNUSED(device);
    /* Event listeners register themselves via ZMK_LISTENER/ZMK_SUBSCRIPTION
     * linker sections. Start a 1 Hz heartbeat so the peripheral always has a
     * fresh snapshot even if nothing changes. */
    k_timer_start(&heartbeat_timer, K_SECONDS(1), K_SECONDS(1));
    return 0;
}
SYS_INIT(peripheral_status_forward_init, APPLICATION, 99);

#endif /* IS_ENABLED(CONFIG_ZMK_PERIPHERAL_STATUS_FORWARD) */
