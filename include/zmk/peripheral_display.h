/*
 * Copyright (c) 2026 The zmk-module-peripheral-display Contributors
 * SPDX-License-Identifier: MIT
 */
#pragma once

#include <lvgl.h>
#include <zmk/peripheral_status.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward-declared widget structs (defined by each widget's header in T9) */
struct zmk_widget_peripheral_layer_status;
struct zmk_widget_peripheral_output_status;
struct zmk_widget_peripheral_battery_status;
struct zmk_widget_peripheral_modifiers;
struct zmk_widget_peripheral_hid_indicators;
struct zmk_widget_peripheral_wpm_status;
struct zmk_widget_peripheral_central_name;
struct zmk_widget_peripheral_bongo_cat;

/**
 * @brief Initialize all enabled peripheral display widgets and add
 *        them to @p parent.
 *
 * Called from zmk_display_status_screen() in T11.
 */
int peripheral_display_init(lv_obj_t *parent);

/* ZMK_PERIPHERAL_DISPLAY_WIDGET base macro — used by every widget in T9
 * to avoid copy-paste boilerplate. Defines the standard fields
 * (obj, sys_slist_node, last_state) the widget update functions expect. */
#define ZMK_PERIPHERAL_DISPLAY_WIDGET(name, state_t)                         \
    struct zmk_widget_peripheral_##name {                                    \
        lv_obj_t *obj;                                                       \
        sys_slist_node_t node;                                               \
        state_t state;                                                       \
    }

#ifdef __cplusplus
}
#endif
