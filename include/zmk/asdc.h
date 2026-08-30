/*
 * Copyright (c) 2026 The zmk-module-peripheral-display Contributors
 * SPDX-License-Identifier: MIT
 *
 * Arbitrary split data channel (ASDC): a BLE L2CAP data pipe from the split
 * central to the peripheral. Integrated from dmhuisma/zmk_arbitrary_split_data_channel
 * (MIT). API is plain inlined functions (ZMK runs in kernel space, no syscalls).
 */
#ifndef ZMK_PERIPHERAL_DISPLAY_ASDC_H_
#define ZMK_PERIPHERAL_DISPLAY_ASDC_H_

#include <zephyr/device.h>
#include <zephyr/kernel.h>

/* device config structure */
struct asdc_config {
    int channel_id;
};

/* sender_conn can be used for identification of the connection the data came from */
typedef void (*asdc_rx_cb)(const struct device *dev, void *sender_conn,
                           uint8_t *buf, size_t buflen);

typedef int (*asdc_tx)(const struct device *dev, const uint8_t *data,
                       size_t len, uint32_t delay_ms);
typedef void (*asdc_register_rx_cb)(const struct device *dev, asdc_rx_cb cb);

/* device runtime data structure */
struct asdc_data {
    asdc_rx_cb recv_cb;
};

struct asdc_packet {
    uint32_t channel_id;
    uint32_t len;
    uint8_t data[];
} __packed;

__subsystem struct asdc_driver_api {
    asdc_tx send;
    asdc_register_rx_cb register_recv_cb;
};

static inline int asdc_send(const struct device *dev, const uint8_t *data,
                            size_t len, uint32_t delay_ms)
{
    const struct asdc_driver_api *api = (const struct asdc_driver_api *)dev->api;
    if (api->send == NULL) {
        return -ENOSYS;
    }
    return api->send(dev, data, len, delay_ms);
}

static inline void asdc_register_recv_cb(const struct device *dev, asdc_rx_cb cb)
{
    const struct asdc_driver_api *api = (const struct asdc_driver_api *)dev->api;
    if (api->register_recv_cb == NULL) {
        return;
    }
    api->register_recv_cb(dev, cb);
}

void asdc_on_data_received(void *conn, uint8_t *data, size_t len);

#endif /* ZMK_PERIPHERAL_DISPLAY_ASDC_H_ */
