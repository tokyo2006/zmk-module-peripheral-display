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
#if IS_ENABLED(CONFIG_ZMK_PERIPHERAL_DISPLAY_WIDGET_BONGO_CAT)
#include "widgets/peripheral_bongo_cat.h"
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
#if IS_ENABLED(CONFIG_ZMK_PERIPHERAL_DISPLAY_WIDGET_BONGO_CAT)
extern int zmk_widget_peripheral_bongo_cat_init(
    struct zmk_widget_peripheral_bongo_cat *w, lv_obj_t *p);
static struct zmk_widget_peripheral_bongo_cat bongo_cat_w;
#endif

#define POLL_MS 100

static lv_obj_t *no_link_label;
static bool had_link;

static void set_link_visible(bool linked) {
    if (linked) {
        lv_obj_add_flag(no_link_label, LV_OBJ_FLAG_HIDDEN);
#if IS_ENABLED(CONFIG_ZMK_PERIPHERAL_DISPLAY_WIDGET_BONGO_CAT)
        lv_obj_clear_flag(bongo_cat_w.obj, LV_OBJ_FLAG_HIDDEN);
#endif
    } else {
        lv_obj_clear_flag(no_link_label, LV_OBJ_FLAG_HIDDEN);
#if IS_ENABLED(CONFIG_ZMK_PERIPHERAL_DISPLAY_WIDGET_BONGO_CAT)
        lv_obj_add_flag(bongo_cat_w.obj, LV_OBJ_FLAG_HIDDEN);
#endif
    }
}

static void poll_shadow(lv_timer_t *t) {
    (void)t;
    struct peripheral_status_shadow s;
    bool ok = peripheral_status_shadow_get(&s);

    if (!ok) {
        if (had_link) {
            had_link = false;
            set_link_visible(false);
        }
        return;
    }

    if (!had_link) {
        had_link = true;
        set_link_visible(true);
    }

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
#if IS_ENABLED(CONFIG_ZMK_PERIPHERAL_DISPLAY_WIDGET_BONGO_CAT)
    zmk_widget_peripheral_bongo_cat_update(&bongo_cat_w, &s.data);
#endif
}

int peripheral_display_init(lv_obj_t *parent) {
    /* The status screen must be sized before children align against it.
     * `zmk_display_status_screen()` creates the screen with lv_obj_create(NULL)
     * and `lv_scr_load()` (which normally sizes it to the display) runs AFTER
     * this init, so children were aligning against a 0x0 parent and all piled
     * up top-left. Pin it to the panel's 128x128 here so LV_ALIGN_* works. */
    lv_obj_set_size(parent, 128, 128);

    /* Global style.
     *
     * NOTE: colors are intentionally inverted. LVGL 4.1's Zephyr mono
     * integration (`lvgl_display_mono.c` / `lvgl_transform_buffer`) inverts
     * the draw buffer relative to the MONO01 pixel format the Sharp driver
     * reports. So to get a black background + white text on the panel we set
     * bg = white and text = black here. Bitmap widgets (bongo cat) are
     * unaffected because they carry their own pixel data. */
    static lv_style_t style;
    lv_style_init(&style);
    lv_style_set_bg_color(&style, lv_color_white());
    lv_style_set_bg_opa(&style, LV_OPA_COVER);
    lv_style_set_text_color(&style, lv_color_black());
    lv_obj_add_style(parent, &style, LV_PART_MAIN);

#if IS_ENABLED(CONFIG_ZMK_PERIPHERAL_DISPLAY_WIDGET_LAYER)
    zmk_widget_peripheral_layer_status_init(&layer_w, parent);
    lv_obj_align(layer_w.obj, LV_ALIGN_CENTER, 0, 0);
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
#if IS_ENABLED(CONFIG_ZMK_PERIPHERAL_DISPLAY_WIDGET_CENTRAL_NAME)
    zmk_widget_peripheral_central_name_init(&central_name_w, parent);
#endif
#if IS_ENABLED(CONFIG_ZMK_PERIPHERAL_DISPLAY_WIDGET_BONGO_CAT)
    zmk_widget_peripheral_bongo_cat_init(&bongo_cat_w, parent);
    /* bongo cat sits slightly above the bottom-right so layer/WPM can go below it */
    lv_obj_align(bongo_cat_w.obj, LV_ALIGN_BOTTOM_RIGHT, 0, -7);
#endif
#if IS_ENABLED(CONFIG_ZMK_PERIPHERAL_DISPLAY_WIDGET_WPM)
    zmk_widget_peripheral_wpm_status_init(&wpm_w, parent);
    lv_obj_align_to(wpm_w.obj, bongo_cat_w.obj, LV_ALIGN_BOTTOM_RIGHT, 0, 5);
#endif

    no_link_label = lv_label_create(parent);
    lv_label_set_text(no_link_label, "NO LINK");
    lv_obj_set_style_text_font(no_link_label, &lv_font_unscii_8, 0);
    /* Offset above the center layer label so the two don't overlap. */
    lv_obj_align(no_link_label, LV_ALIGN_CENTER, 0, -24);

    /* Visible initially: shows "NO LINK" until the first status arrives. */
    had_link = false;
    set_link_visible(false);

    lv_timer_create(poll_shadow, POLL_MS, NULL);
    return 0;
}
