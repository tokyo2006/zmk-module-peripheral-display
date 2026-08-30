/*
 * Copyright (c) 2026 The zmk-module-peripheral-display Contributors
 * SPDX-License-Identifier: MIT
 */
#include <string.h>
#include <errno.h>
#include <zmk/peripheral_status.h>

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