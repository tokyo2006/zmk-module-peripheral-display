/*
 * Copyright (c) 2026 The zmk-module-peripheral-display Contributors
 * SPDX-License-Identifier: MIT
 *
 * Peripheral-side receiver: the central half ships a full status snapshot as
 * a relay event (`peripheral_status_update`). This file receives it over the
 * standard ZMK split relay transport and writes it into the local shadow
 * state, which the display widgets poll. No custom GATT service involved.
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <zmk/event_manager.h>
#include <zmk/peripheral_status.h>
#include <zmk/peripheral_status_event.h>

LOG_MODULE_DECLARE(peripheral_status, CONFIG_ZMK_LOG_LEVEL);

#if IS_ENABLED(CONFIG_ZMK_PERIPHERAL_STATUS_RECEIVE)

/* Receive the relayed `peripheral_status_update` event from the central.
 * On arrival the macro re-raises a `peripheral_status_update` event, which
 * our listener below consumes to refresh the shadow state. */
ZMK_RELAY_EVENT_HANDLE(peripheral_status_update, pd, )

static int on_status_update(const zmk_event_t *eh) {
    const struct peripheral_status_update *ev =
        as_peripheral_status_update(eh);
    if (ev == NULL) {
        return ZMK_EV_EVENT_BUBBLE;
    }
    if (peripheral_status_shadow_set(&ev->data) != 0) {
        LOG_WRN("peripheral-display: shadow set failed");
    } else {
        LOG_INF("peripheral-display: recv batt=%u layer=%u flags=0x%02x",
                (unsigned)ev->data.battery_level, (unsigned)ev->data.active_layer,
                (unsigned)ev->data.status_flags);
    }
    return ZMK_EV_EVENT_HANDLED;
}
ZMK_LISTENER(peripheral_status_handle, on_status_update);
ZMK_SUBSCRIPTION(peripheral_status_handle, peripheral_status_update);

#endif /* IS_ENABLED(CONFIG_ZMK_PERIPHERAL_STATUS_RECEIVE) */
