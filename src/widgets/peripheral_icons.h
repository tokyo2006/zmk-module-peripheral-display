/*
 * Copyright (c) 2026 The zmk-module-peripheral-display Contributors
 * SPDX-License-Identifier: MIT
 *
 * 1-bit mono bitmap icons (LV_COLOR_FORMAT_I1). Bitmap data copied from
 * englmaxi/zmk-dongle-display (MIT); see LICENSE-3RD-PARTY.
 */
#pragma once

#include <lvgl.h>

/* Modifier key icons (14x14) */
extern const lv_img_dsc_t control_icon;
extern const lv_img_dsc_t shift_icon;
extern const lv_img_dsc_t alt_icon;
extern const lv_img_dsc_t win_icon;
extern const lv_img_dsc_t cmd_icon;
extern const lv_img_dsc_t opt_icon;

/* Output status icons (9x14) */
extern const lv_img_dsc_t sym_usb;
extern const lv_img_dsc_t sym_bt;

/* Output status glyphs (5x5): HID-ready / bonded+connected / bonded-but-open */
extern const lv_img_dsc_t sym_ok;
extern const lv_img_dsc_t sym_nok;
extern const lv_img_dsc_t sym_open;

/* BLE profile slot number (5x6), 1-indexed */
extern const lv_img_dsc_t sym_1;
extern const lv_img_dsc_t sym_2;
extern const lv_img_dsc_t sym_3;
extern const lv_img_dsc_t sym_4;
extern const lv_img_dsc_t sym_5;

/* Battery icon (10x6) */
extern const lv_img_dsc_t sym_battery;
