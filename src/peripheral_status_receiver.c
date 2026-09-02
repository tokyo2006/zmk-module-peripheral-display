/*
 * Copyright (c) 2026 The zmk-module-peripheral-display Contributors
 * SPDX-License-Identifier: MIT
 *
 * Peripheral-side receiver: registers an ASDC receive callback. When the
 * central sends a status snapshot over the BLE L2CAP data channel, the
 * callback writes it into the local shadow state that the display widgets
 * poll. No custom GATT service involved.
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>

#include <zmk/peripheral_status.h>
#include <zmk/asdc.h>

LOG_MODULE_DECLARE(peripheral_status, CONFIG_ZMK_LOG_LEVEL);

#if IS_ENABLED(CONFIG_ZMK_PERIPHERAL_STATUS_RECEIVE)

static void on_status_data(const struct device *dev, void *sender_conn,
                           uint8_t *data, size_t len)
{
    ARG_UNUSED(dev);
    ARG_UNUSED(sender_conn);

    if (len != sizeof(struct peripheral_status_adv_data)) {
        LOG_WRN("peripheral-display: unexpected status len %u", (unsigned)len);
        return;
    }

    const struct peripheral_status_adv_data *s =
        (const struct peripheral_status_adv_data *)data;
    if (peripheral_status_shadow_set(s) != 0) {
        LOG_WRN("peripheral-display: shadow set failed");
    } else {
        LOG_INF("peripheral-display: recv batt=%u layer=%u flags=0x%02x",
                (unsigned)s->battery_level, (unsigned)s->active_layer,
                (unsigned)s->status_flags);
    }
}

static int peripheral_status_receiver_init(const struct device *device) {
    ARG_UNUSED(device);
    const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(asdc0));
    if (!device_is_ready(dev)) {
        LOG_WRN("peripheral-display: asdc0 not ready");
        return -ENODEV;
    }
    asdc_register_recv_cb(dev, on_status_data);
    LOG_INF("peripheral-display: registered ASDC receive callback");
    return 0;
}
SYS_INIT(peripheral_status_receiver_init, APPLICATION, 99);

#endif /* IS_ENABLED(CONFIG_ZMK_PERIPHERAL_STATUS_RECEIVE) */
