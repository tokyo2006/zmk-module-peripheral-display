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

#include <errno.h>
#include <string.h>

#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include <zmk/peripheral_status.h>

#if IS_ENABLED(CONFIG_ZMK_PERIPHERAL_STATUS_RECEIVE)

LOG_MODULE_DECLARE(peripheral_status, CONFIG_ZMK_LOG_LEVEL);

/*
 * Two separate discover params structs: `disc_char` drives the
 * characteristic discovery, `disc_desc` drives the CCC descriptor
 * discovery. They must be distinct so that starting the descriptor
 * discovery (from the characteristic discovery's completion path)
 * never clobbers the struct the first discovery is still walking.
 */
static struct bt_gatt_discover_params disc_char;
static struct bt_gatt_discover_params disc_desc;
static struct bt_gatt_subscribe_params subscribe_params;

static uint8_t notify_cb(struct bt_conn *conn, struct bt_gatt_subscribe_params *params,
                         const void *data, uint16_t length)
{
    struct peripheral_status_adv_data parsed;

    ARG_UNUSED(conn);
    ARG_UNUSED(params);

    LOG_INF("peripheral-display: notify_cb len=%u", length);

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
        if (params->type == BT_GATT_DISCOVER_CHARACTERISTIC) {
            /*
             * Characteristic discovery finished. Start the CCC
             * descriptor discovery using the second params struct.
             */
            disc_desc.uuid = BT_UUID_GATT_CCC;
            disc_desc.func = discover_func;
            disc_desc.start_handle = subscribe_params.value_handle + 1;
            disc_desc.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
            disc_desc.type = BT_GATT_DISCOVER_DESCRIPTOR;

            err = bt_gatt_discover(conn, &disc_desc);
            if (err) {
                LOG_WRN("CCC discovery failed (err %d)", err);
            }
        } else {
            LOG_DBG("Discovery complete");
        }
        return BT_GATT_ITER_STOP;
    }

    if (params->type == BT_GATT_DISCOVER_CHARACTERISTIC) {
        /*
         * Found the status characteristic. Record its value handle, then
         * let the characteristic discovery run to completion (attr == NULL)
         * before starting the CCC discovery, so the first discovery's
         * params struct is not reused mid-flight.
         */
        subscribe_params.value_handle = bt_gatt_attr_value_handle(attr);
        return BT_GATT_ITER_CONTINUE;
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

    LOG_INF("peripheral-display: BLE connected, starting status discovery");

    /* DEBUG: write a fake shadow so the widgets render immediately and we
     * can tell whether the issue is in the notify path or the display
     * pipeline. Remove once notify works. */
    {
        struct peripheral_status_adv_data fake = {
            .manufacturer_id = {0xFF, 0xFF},
            .service_uuid    = {0xAB, 0xCD},
            .version         = 0x22,
            .battery_level   = 99,
            .active_layer    = 1,
            .layer_name = "DBG",
            .wpm_value       = 0,
        };
        peripheral_status_shadow_set(&fake);
    }

    memset(&subscribe_params, 0, sizeof(subscribe_params));
    memset(&disc_char, 0, sizeof(disc_char));
    memset(&disc_desc, 0, sizeof(disc_desc));

    disc_char.uuid = PERIPHERAL_STATUS_CHRC_UUID;
    disc_char.func = discover_func;
    disc_char.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
    disc_char.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
    disc_char.type = BT_GATT_DISCOVER_CHARACTERISTIC;

    rc = bt_gatt_discover(conn, &disc_char);
    if (rc) {
        LOG_ERR("Characteristic discovery failed (err %d)", rc);
    } else {
        LOG_INF("peripheral-display: char discovery in flight");
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
