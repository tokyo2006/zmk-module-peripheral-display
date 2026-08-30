/*
 * Copyright (c) 2026 The zmk-module-peripheral-display Contributors
 * SPDX-License-Identifier: MIT
 *
 * Peripheral-side receiver. Here the peripheral acts as a GATT *client*
 * (the reverse of the normal ZMK split role, where the peripheral is the
 * server): on connect it discovers the central's status characteristic and
 * subscribes to notifications, then unpacks each 26-byte payload into the
 * shadow state.
 */
#if IS_ENABLED(CONFIG_ZMK_PERIPHERAL_STATUS_RECEIVE)

#include <errno.h>
#include <string.h>

#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include <zmk/peripheral_status.h>

LOG_MODULE_DECLARE(peripheral_status, CONFIG_ZMK_LOG_LEVEL);

static struct bt_gatt_discover_params discover_params;
static struct bt_gatt_subscribe_params subscribe_params;

static uint8_t notify_cb(struct bt_conn *conn, struct bt_gatt_subscribe_params *params,
                         const void *data, uint16_t length)
{
    struct peripheral_status_adv_data parsed;

    ARG_UNUSED(conn);
    ARG_UNUSED(params);

    if (length != PERIPHERAL_STATUS_PAYLOAD_SIZE) {
        LOG_WRN("status notify: unexpected length %u", length);
        return BT_GATT_ITER_CONTINUE;
    }

    if (peripheral_status_unpack(data, length, &parsed) != 0) {
        LOG_WRN("status notify: unpack failed");
        return BT_GATT_ITER_CONTINUE;
    }

    if (peripheral_status_shadow_set(&parsed) != 0) {
        LOG_WRN("status notify: shadow set failed");
    }

    return BT_GATT_ITER_CONTINUE;
}

static uint8_t discover_func(struct bt_conn *conn,
                             const struct bt_gatt_attr *attr,
                             struct bt_gatt_discover_params *params)
{
    int err;

    if (!attr) {
        LOG_DBG("Discovery complete");
        (void)memset(params, 0, sizeof(*params));
        return BT_GATT_ITER_STOP;
    }

    if (params->type == BT_GATT_DISCOVER_CHARACTERISTIC) {
        /*
         * Found the status characteristic. Record its value handle, then
         * discover the CCC descriptor that follows it.
         */
        subscribe_params.value_handle = bt_gatt_attr_value_handle(attr);

        discover_params.uuid = BT_UUID_GATT_CCC;
        discover_params.start_handle = attr->handle + 2;
        discover_params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
        discover_params.type = BT_GATT_DISCOVER_DESCRIPTOR;

        err = bt_gatt_discover(conn, &discover_params);
        if (err) {
            LOG_WRN("CCC discovery failed (err %d)", err);
        }

        return BT_GATT_ITER_STOP;
    }

    /* Found the CCC descriptor: subscribe to notifications. */
    subscribe_params.notify = notify_cb;
    subscribe_params.value = BT_GATT_CCC_NOTIFY;
    subscribe_params.ccc_handle = attr->handle;

    err = bt_gatt_subscribe(conn, &subscribe_params);
    if (err && err != -EALREADY) {
        LOG_WRN("Subscribe failed (err %d)", err);
    } else {
        LOG_INF("Subscribed to central status notifications");
    }

    return BT_GATT_ITER_STOP;
}

static void connected(struct bt_conn *conn, uint8_t err)
{
    int rc;

    if (err) {
        LOG_ERR("Connection failed (err %u)", err);
        return;
    }

    memset(&subscribe_params, 0, sizeof(subscribe_params));

    discover_params.uuid = PERIPHERAL_STATUS_CHRC_UUID;
    discover_params.func = discover_func;
    discover_params.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
    discover_params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
    discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;

    rc = bt_gatt_discover(conn, &discover_params);
    if (rc) {
        LOG_ERR("Discovery failed (err %d)", rc);
    }
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
    ARG_UNUSED(conn);
    ARG_UNUSED(reason);
    memset(&subscribe_params, 0, sizeof(subscribe_params));
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected = connected,
    .disconnected = disconnected,
};

#endif /* IS_ENABLED(CONFIG_ZMK_PERIPHERAL_STATUS_RECEIVE) */
