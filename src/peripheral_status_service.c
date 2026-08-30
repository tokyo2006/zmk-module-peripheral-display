/*
 * Copyright (c) 2026 The zmk-module-peripheral-display Contributors
 * SPDX-License-Identifier: MIT
 *
 * Defines the custom GATT service + notify characteristic that the
 * central uses to push status to the peripheral. Only the central
 * declares the service; the peripheral only needs the UUIDs (T2).
 */
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/logging/log.h>
#include <zmk/peripheral_status.h>

LOG_MODULE_DECLARE(peripheral_status, CONFIG_ZMK_LOG_LEVEL);

static uint8_t notify_buf[PERIPHERAL_STATUS_PAYLOAD_SIZE];

static void nfy_changed_cb(const struct bt_gatt_attr *attr,
                           uint16_t value)
{
    /* No-op: CCC writes are tracked implicitly by bt_gatt_notify. */
    ARG_UNUSED(attr);
    ARG_UNUSED(value);
}

BT_GATT_SERVICE_DEFINE(peripheral_status_svc,
    BT_GATT_PRIMARY_SERVICE(PERIPHERAL_STATUS_SERVICE_UUID),
    BT_GATT_CHARACTERISTIC(PERIPHERAL_STATUS_CHRC_UUID,
                           BT_GATT_CHRC_NOTIFY,
                           BT_GATT_PERM_NONE,
                           NULL, NULL, notify_buf),
    BT_GATT_CCC(nfy_changed_cb, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
);

int peripheral_status_notify(const uint8_t *buf, size_t len)
{
    if (len != PERIPHERAL_STATUS_PAYLOAD_SIZE) {
        return -EINVAL;
    }
    /* attrs[] layout from BT_GATT_SERVICE_DEFINE above:
     *   attrs[0] = primary service
     *   attrs[1] = characteristic declaration  (BT_UUID_GATT_CHRC)
     *   attrs[2] = characteristic VALUE        (the one to notify on)
     *   attrs[3] = CCC descriptor
     */
    return bt_gatt_notify(NULL, &peripheral_status_svc.attrs[2], buf, len);
}