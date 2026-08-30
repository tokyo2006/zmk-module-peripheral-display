/*
 * Copyright (c) 2026 The zmk-module-peripheral-display Contributors
 * SPDX-License-Identifier: MIT
 */
#include <string.h>
#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zmk/peripheral_status.h>

LOG_MODULE_REGISTER(peripheral_status, CONFIG_ZMK_LOG_LEVEL);

int peripheral_status_pack(const struct peripheral_status_adv_data *data,
                           uint8_t *buf, size_t buf_len)
{
    if (!data || !buf || buf_len < PERIPHERAL_STATUS_PAYLOAD_SIZE) {
        return -EINVAL;
    }
    memcpy(buf, data, PERIPHERAL_STATUS_PAYLOAD_SIZE);
    return 0;
}

int peripheral_status_unpack(const uint8_t *buf, size_t buf_len,
                             struct peripheral_status_adv_data *data)
{
    if (!buf || !data || buf_len < PERIPHERAL_STATUS_PAYLOAD_SIZE) {
        return -EINVAL;
    }
    memcpy(data, buf, PERIPHERAL_STATUS_PAYLOAD_SIZE);
    return 0;
}

/* ---- Shadow state (mutex-protected) ------------------------------- */

static struct peripheral_status_shadow shadow;
static struct k_mutex shadow_mutex = Z_MUTEX_INITIALIZER(shadow_mutex);
static bool shadow_initialized;

bool peripheral_status_shadow_get(struct peripheral_status_shadow *out)
{
    if (!out) return false;
    k_mutex_lock(&shadow_mutex, K_FOREVER);
    *out = shadow;
    bool ok = shadow.valid;
    k_mutex_unlock(&shadow_mutex);
    return ok;
}

int peripheral_status_shadow_set(const struct peripheral_status_adv_data *data)
{
    if (!data) return -EINVAL;
    k_mutex_lock(&shadow_mutex, K_FOREVER);
    shadow.data = *data;
    shadow.valid = true;
    shadow.last_update_ms = k_uptime_get_32();
    k_mutex_unlock(&shadow_mutex);
    return 0;
}