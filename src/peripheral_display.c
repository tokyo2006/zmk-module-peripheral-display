/*
 * Copyright (c) 2026 The zmk-module-peripheral-display Contributors
 * SPDX-License-Identifier: MIT
 */
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zmk/peripheral_status.h>
#include <zmk/peripheral_display.h>

LOG_MODULE_REGISTER(peripheral_display, CONFIG_ZMK_LOG_LEVEL);

/* Widget declarations are stubbed until T9 defines each widget struct
 * (via ZMK_PERIPHERAL_DISPLAY_WIDGET) and its init function. Uncomment
 * per widget as T9 lands. Pattern:
 *
 * #if IS_ENABLED(CONFIG_ZMK_PERIPHERAL_DISPLAY_WIDGET_LAYER)
 * extern int zmk_widget_peripheral_layer_status_init(
 *     struct zmk_widget_peripheral_layer_status *w, lv_obj_t *p);
 * static struct zmk_widget_peripheral_layer_status layer_w;
 * #endif
 */

#define POLL_MS 100

static void poll_shadow(lv_timer_t *t) {
    (void)t;
    struct peripheral_status_shadow s;
    if (!peripheral_status_shadow_get(&s)) return;

    /* Each widget has an update fn. Pattern (filled in T9):
     *   zmk_widget_peripheral_layer_status_update(&layer_w, &s.data);
     */
}

int peripheral_display_init(lv_obj_t *parent) {
    /* Global style: black bg, white text (mono LCD convention). */
    static lv_style_t style;
    lv_style_init(&style);
    lv_style_set_bg_color(&style, lv_color_black());
    lv_style_set_text_color(&style, lv_color_white());
    lv_obj_add_style(parent, &style, LV_PART_MAIN);

    /* Init widgets (stubs compile until T9 lands).
     *
     * #if IS_ENABLED(CONFIG_ZMK_PERIPHERAL_DISPLAY_WIDGET_LAYER)
     *     zmk_widget_peripheral_layer_status_init(&layer_w, parent);
     *     lv_obj_align(layer_w.obj, LV_ALIGN_BOTTOM_RIGHT, 0, -16);
     * #endif
     */

    lv_timer_create(poll_shadow, POLL_MS, NULL);
    return 0;
}
