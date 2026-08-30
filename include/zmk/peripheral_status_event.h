/*
 * Copyright (c) 2026 The zmk-module-peripheral-display Contributors
 * SPDX-License-Identifier: MIT
 *
 * ZMK relay event that carries a full peripheral-status snapshot from the
 * central half to the peripheral half. The peripheral display module
 * subscribes to this on the peripheral and updates its local shadow state.
 */
#pragma once

#include <zephyr/kernel.h>
#include <zmk/event_manager.h>
#include <zmk/peripheral_status.h>

#ifdef __cplusplus
extern "C" {
#endif

struct peripheral_status_update {
    struct peripheral_status_adv_data data;
};

ZMK_EVENT_DECLARE(peripheral_status_update);

#ifdef __cplusplus
}
#endif
