/*
 * Copyright (c) 2026 The zmk-module-peripheral-display Contributors
 * SPDX-License-Identifier: MIT
 */
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <zephyr/bluetooth/uuid.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 26-byte status payload sent from central to peripheral.
 *
 * Field layout is byte-identical to prospector's
 * `struct zmk_status_adv_data` (prospector-zmk-module v2.2.2).
 * We redefine it here to avoid coupling the modules.
 */
struct peripheral_status_adv_data {
    uint8_t manufacturer_id[2];   /* 0xFF 0xFF */
    uint8_t service_uuid[2];      /* 0xAB 0xCD */
    uint8_t version;
    uint8_t battery_level;
    uint8_t active_layer;
    uint8_t profile_slot;
    uint8_t connection_count;
    uint8_t status_flags;
    uint8_t device_role;
    uint8_t device_index;
    uint8_t peripheral_battery[3];
    char     layer_name[4];
    uint8_t keyboard_id[4];
    uint8_t modifier_flags;
    uint8_t wpm_value;
    uint8_t channel;
} __packed;

#define PERIPHERAL_STATUS_PAYLOAD_SIZE sizeof(struct peripheral_status_adv_data)
_Static_assert(PERIPHERAL_STATUS_PAYLOAD_SIZE == 26,
               "peripheral_status_adv_data must be exactly 26 bytes");

/* Status flags (mirror prospector's bits) */
#define PERIPHERAL_STATUS_FLAG_CAPS_WORD     (1 << 0)
#define PERIPHERAL_STATUS_FLAG_CHARGING      (1 << 1)
#define PERIPHERAL_STATUS_FLAG_USB_CONNECTED (1 << 2)
#define PERIPHERAL_STATUS_FLAG_USB_HID_READY (1 << 3)
#define PERIPHERAL_STATUS_FLAG_BLE_CONNECTED (1 << 4)
#define PERIPHERAL_STATUS_FLAG_BLE_BONDED    (1 << 5)
/* Set when USB is the actively selected output transport; clear means BLE
 * is selected. Distinct from USB_CONNECTED/BLE_CONNECTED, which just track
 * physical link state -- both can be true while only one is in use. */
#define PERIPHERAL_STATUS_FLAG_OUTPUT_USB_SELECTED (1 << 6)

#define PERIPHERAL_MOD_FLAG_LCTL (1 << 0)
#define PERIPHERAL_MOD_FLAG_LSFT (1 << 1)
#define PERIPHERAL_MOD_FLAG_LALT (1 << 2)
#define PERIPHERAL_MOD_FLAG_LGUI (1 << 3)
#define PERIPHERAL_MOD_FLAG_RCTL (1 << 4)
#define PERIPHERAL_MOD_FLAG_RSFT (1 << 5)
#define PERIPHERAL_MOD_FLAG_RALT (1 << 6)
#define PERIPHERAL_MOD_FLAG_RGUI (1 << 7)

#define PERIPHERAL_STATUS_SERVICE_UUID \
    BT_UUID_DECLARE_128(0x9e,0xca,0xdc,0x24,0x0e,0xe5,0xa9,0x89, \
                       0x9c,0x9b,0x42,0x4f,0x7b,0x5e,0xab,0xcd)

#define PERIPHERAL_STATUS_CHRC_UUID \
    BT_UUID_DECLARE_128(0x9e,0xca,0xdc,0x24,0x0e,0xe5,0xa9,0x89, \
                       0x9c,0x9b,0x42,0x4f,0x7c,0x5e,0xab,0xcd)

/**
 * @brief Pack a status payload into a 26-byte buffer.
 * @param data  Source data.
 * @param buf   Destination buffer (must be >= 26 bytes).
 * @return      0 on success.
 */
int peripheral_status_pack(const struct peripheral_status_adv_data *data,
                           uint8_t *buf, size_t buf_len);

/**
 * @brief Unpack a 26-byte buffer into a status payload.
 * @param buf   Source buffer.
 * @param buf_len Source buffer length (must be >= 26).
 * @param data  Destination.
 * @return      0 on success, -EINVAL if buf_len < 26.
 */
int peripheral_status_unpack(const uint8_t *buf, size_t buf_len,
                             struct peripheral_status_adv_data *data);

/**
 * @brief Shadow state holding the most recent status received.
 *
 * Single source of truth for widget rendering on the peripheral side.
 * All access goes through the getter/setter (mutex-protected).
 */
struct peripheral_status_shadow {
    struct peripheral_status_adv_data data;
    bool valid;            /* True after first successful write */
    uint32_t last_update_ms; /* k_uptime_get_32() of last successful update */
};

/**
 * @brief Get a snapshot of the current shadow state.
 *
 * Copies the shadow into @p out under the shadow mutex.
 * @return true if shadow has been written at least once.
 */
bool peripheral_status_shadow_get(struct peripheral_status_shadow *out);

/**
 * @brief Replace the shadow state. Called by the peripheral receiver.
 *
 * @return 0 on success.
 */
int peripheral_status_shadow_set(const struct peripheral_status_adv_data *data);

#ifdef __cplusplus
}
#endif
