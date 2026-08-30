/*
 * Copyright (c) 2026 The zmk-module-peripheral-display Contributors
 * SPDX-License-Identifier: MIT
 */
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zmk/peripheral_status.h>
#include <zmk/peripheral_display.h>

#if IS_ENABLED(CONFIG_ZMK_PERIPHERAL_DISPLAY_WIDGET_LAYER)
#include "widgets/peripheral_layer_status.h"
#endif
#if IS_ENABLED(CONFIG_ZMK_PERIPHERAL_DISPLAY_WIDGET_OUTPUT)
#include "widgets/peripheral_output_status.h"
#endif
#if IS_ENABLED(CONFIG_ZMK_PERIPHERAL_DISPLAY_WIDGET_BATTERY)
#include "widgets/peripheral_battery_status.h"
#endif
#if IS_ENABLED(CONFIG_ZMK_PERIPHERAL_DISPLAY_WIDGET_MODIFIERS)
#include "widgets/peripheral_modifiers.h"
#endif
#if IS_ENABLED(CONFIG_ZMK_PERIPHERAL_DISPLAY_WIDGET_HID_INDICATORS)
#include "widgets/peripheral_hid_indicators.h"
#endif
#if IS_ENABLED(CONFIG_ZMK_PERIPHERAL_DISPLAY_WIDGET_WPM)
#include "widgets/peripheral_wpm_status.h"
#endif
#if IS_ENABLED(CONFIG_ZMK_PERIPHERAL_DISPLAY_WIDGET_CENTRAL_NAME)
#include "widgets/peripheral_central_name.h"
#endif

LOG_MODULE_REGISTER(peripheral_display, CONFIG_ZMK_LOG_LEVEL);

#if IS_ENABLED(CONFIG_ZMK_PERIPHERAL_DISPLAY_WIDGET_LAYER)
extern int zmk_widget_peripheral_layer_status_init(
    struct zmk_widget_peripheral_layer_status *w, lv_obj_t *p);
static struct zmk_widget_peripheral_layer_status layer_w;
#endif
#if IS_ENABLED(CONFIG_ZMK_PERIPHERAL_DISPLAY_WIDGET_OUTPUT)
extern int zmk_widget_peripheral_output_status_init(
    struct zmk_widget_peripheral_output_status *w, lv_obj_t *p);
static struct zmk_widget_peripheral_output_status output_w;
#endif
#if IS_ENABLED(CONFIG_ZMK_PERIPHERAL_DISPLAY_WIDGET_BATTERY)
extern int zmk_widget_peripheral_battery_status_init(
    struct zmk_widget_peripheral_battery_status *w, lv_obj_t *p);
static struct zmk_widget_peripheral_battery_status battery_w;
#endif
#if IS_ENABLED(CONFIG_ZMK_PERIPHERAL_DISPLAY_WIDGET_MODIFIERS)
extern int zmk_widget_peripheral_modifiers_init(
    struct zmk_widget_peripheral_modifiers *w, lv_obj_t *p);
static struct zmk_widget_peripheral_modifiers modifiers_w;
#endif
#if IS_ENABLED(CONFIG_ZMK_PERIPHERAL_DISPLAY_WIDGET_HID_INDICATORS)
extern int zmk_widget_peripheral_hid_indicators_init(
    struct zmk_widget_peripheral_hid_indicators *w, lv_obj_t *p);
static struct zmk_widget_peripheral_hid_indicators hid_indicators_w;
#endif
#if IS_ENABLED(CONFIG_ZMK_PERIPHERAL_DISPLAY_WIDGET_WPM)
extern int zmk_widget_peripheral_wpm_status_init(
    struct zmk_widget_peripheral_wpm_status *w, lv_obj_t *p);
static struct zmk_widget_peripheral_wpm_status wpm_w;
#endif
#if IS_ENABLED(CONFIG_ZMK_PERIPHERAL_DISPLAY_WIDGET_CENTRAL_NAME)
extern int zmk_widget_peripheral_central_name_init(
    struct zmk_widget_peripheral_central_name *w, lv_obj_t *p);
static struct zmk_widget_peripheral_central_name central_name_w;
#endif

#define POLL_MS 100

static void poll_shadow(lv_timer_t *t) {
    (void)t;
    struct peripheral_status_shadow s;
    if (!peripheral_status_shadow_get(&s)) return;

#if IS_ENABLED(CONFIG_ZMK_PERIPHERAL_DISPLAY_WIDGET_LAYER)
    zmk_widget_peripheral_layer_status_update(&layer_w, &s.data);
#endif
#if IS_ENABLED(CONFIG_ZMK_PERIPHERAL_DISPLAY_WIDGET_OUTPUT)
    zmk_widget_peripheral_output_status_update(&output_w, &s.data);
#endif
#if IS_ENABLED(CONFIG_ZMK_PERIPHERAL_DISPLAY_WIDGET_BATTERY)
    zmk_widget_peripheral_battery_status_update(&battery_w, &s.data);
#endif
#if IS_ENABLED(CONFIG_ZMK_PERIPHERAL_DISPLAY_WIDGET_MODIFIERS)
    zmk_widget_peripheral_modifiers_update(&modifiers_w, &s.data);
#endif
#if IS_ENABLED(CONFIG_ZMK_PERIPHERAL_DISPLAY_WIDGET_HID_INDICATORS)
    zmk_widget_peripheral_hid_indicators_update(&hid_indicators_w, &s.data);
#endif
#if IS_ENABLED(CONFIG_ZMK_PERIPHERAL_DISPLAY_WIDGET_WPM)
    zmk_widget_peripheral_wpm_status_update(&wpm_w, &s.data);
#endif
#if IS_ENABLED(CONFIG_ZMK_PERIPHERAL_DISPLAY_WIDGET_CENTRAL_NAME)
    zmk_widget_peripheral_central_name_update(&central_name_w, &s.data);
#endif
}

int peripheral_display_init(lv_obj_t *parent) {
    /* Global style: black bg, white text (mono LCD convention). */
    static lv_style_t style;
    lv_style_init(&style);
    lv_style_set_bg_color(&style, lv_color_black());
    lv_style_set_text_color(&style, lv_color_white());
    lv_obj_add_style(parent, &style, LV_PART_MAIN);

#if IS_ENABLED(CONFIG_ZMK_PERIPHERAL_DISPLAY_WIDGET_LAYER)
    zmk_widget_peripheral_layer_status_init(&layer_w, parent);
    lv_obj_align(layer_w.obj, LV_ALIGN_BOTTOM_RIGHT, -28, -2);
#endif
#if IS_ENABLED(CONFIG_ZMK_PERIPHERAL_DISPLAY_WIDGET_OUTPUT)
    zmk_widget_peripheral_output_status_init(&output_w, parent);
#endif
#if IS_ENABLED(CONFIG_ZMK_PERIPHERAL_DISPLAY_WIDGET_BATTERY)
    zmk_widget_peripheral_battery_status_init(&battery_w, parent);
#endif
#if IS_ENABLED(CONFIG_ZMK_PERIPHERAL_DISPLAY_WIDGET_MODIFIERS)
    zmk_widget_peripheral_modifiers_init(&modifiers_w, parent);
#endif
#if IS_ENABLED(CONFIG_ZMK_PERIPHERAL_DISPLAY_WIDGET_HID_INDICATORS)
    zmk_widget_peripheral_hid_indicators_init(&hid_indicators_w, parent);
#endif
#if IS_ENABLED(CONFIG_ZMK_PERIPHERAL_DISPLAY_WIDGET_WPM)
    zmk_widget_peripheral_wpm_status_init(&wpm_w, parent);
#endif
#if IS_ENABLED(CONFIG_ZMK_PERIPHERAL_DISPLAY_WIDGET_CENTRAL_NAME)
    zmk_widget_peripheral_central_name_init(&central_name_w, parent);
#endif

    lv_timer_create(poll_shadow, POLL_MS, NULL);
    return 0;
}
