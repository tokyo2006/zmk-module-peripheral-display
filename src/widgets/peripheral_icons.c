/*
 * Copyright (c) 2026 The zmk-module-peripheral-display Contributors
 * SPDX-License-Identifier: MIT
 *
 * 1-bit mono bitmap icons. Bitmap data copied from
 * englmaxi/zmk-dongle-display (MIT); see LICENSE-3RD-PARTY.
 */
#include <lvgl.h>

#ifndef LV_ATTRIBUTE_MEM_ALIGN
#define LV_ATTRIBUTE_MEM_ALIGN
#endif

/* ============ Modifier key icons (14x14) ============ */

#ifndef LV_ATTRIBUTE_IMG_CONTROL
#define LV_ATTRIBUTE_IMG_CONTROL
#endif
const LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST LV_ATTRIBUTE_IMG_CONTROL uint8_t control_map[] = {
  0xff, 0xff, 0xff, 0xff, 	/*Color of index 0*/
  0x00, 0x00, 0x00, 0xff, 	/*Color of index 1*/

  0x00, 0x00,
  0x00, 0x00,
  0x03, 0x00,
  0x07, 0x80,
  0x0c, 0xc0,
  0x18, 0x60,
  0x30, 0x30,
  0x20, 0x10,
  0x00, 0x00,
  0x00, 0x00,
  0x00, 0x00,
  0x00, 0x00,
  0x00, 0x00,
  0x00, 0x00,
};
const lv_img_dsc_t control_icon = {
  .header.cf = LV_COLOR_FORMAT_I1,
  .header.w = 14,
  .header.h = 14,
  .data_size = 36,
  .data = control_map,
};

#ifndef LV_ATTRIBUTE_IMG_SHIFT
#define LV_ATTRIBUTE_IMG_SHIFT
#endif
const LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST LV_ATTRIBUTE_IMG_SHIFT uint8_t shift_map[] = {
  0xff, 0xff, 0xff, 0xff, 	/*Color of index 0*/
  0x00, 0x00, 0x00, 0xff, 	/*Color of index 1*/

  0x00, 0x00,
  0x00, 0x00,
  0x03, 0x00,
  0x07, 0x80,
  0x0c, 0xc0,
  0x18, 0x60,
  0x30, 0x30,
  0x78, 0x78,
  0x08, 0x40,
  0x08, 0x40,
  0x08, 0x40,
  0x0f, 0xc0,
  0x00, 0x00,
  0x00, 0x00,
};
const lv_img_dsc_t shift_icon = {
  .header.cf = LV_COLOR_FORMAT_I1,
  .header.w = 14,
  .header.h = 14,
  .data_size = 36,
  .data = shift_map,
};

#ifndef LV_ATTRIBUTE_IMG_ALT
#define LV_ATTRIBUTE_IMG_ALT
#endif
const LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST LV_ATTRIBUTE_IMG_ALT uint8_t alt_map[] = {
  0xff, 0xff, 0xff, 0xff, 	/*Color of index 0*/
  0x00, 0x00, 0x00, 0xff, 	/*Color of index 1*/

  0x00, 0x00,
  0x00, 0x00,
  0x00, 0x00,
  0x00, 0x00,
  0x32, 0xf8,
  0x4a, 0x20,
  0x4a, 0x20,
  0x7a, 0x20,
  0x4a, 0x20,
  0x4b, 0xa0,
  0x00, 0x00,
  0x00, 0x00,
  0x00, 0x00,
  0x00, 0x00,
};
const lv_img_dsc_t alt_icon = {
  .header.cf = LV_COLOR_FORMAT_I1,
  .header.w = 14,
  .header.h = 14,
  .data_size = 36,
  .data = alt_map,
};

#ifndef LV_ATTRIBUTE_IMG_WIN
#define LV_ATTRIBUTE_IMG_WIN
#endif
const LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST LV_ATTRIBUTE_IMG_WIN uint8_t win_map[] = {
  0xff, 0xff, 0xff, 0xff, 	/*Color of index 0*/
  0x00, 0x00, 0x00, 0xff, 	/*Color of index 1*/

  0x00, 0x00,
  0x00, 0x00,
  0x00, 0xf0,
  0x0b, 0xf0,
  0x3b, 0xf0,
  0x3b, 0xf0,
  0x3b, 0xf0,
  0x00, 0x00,
  0x3b, 0xf0,
  0x3b, 0xf0,
  0x3b, 0xf0,
  0x03, 0xf0,
  0x00, 0x30,
  0x00, 0x00,
};
const lv_img_dsc_t win_icon = {
  .header.cf = LV_COLOR_FORMAT_I1,
  .header.w = 14,
  .header.h = 14,
  .data_size = 36,
  .data = win_map,
};

#ifndef LV_ATTRIBUTE_IMG_CMD
#define LV_ATTRIBUTE_IMG_CMD
#endif
const LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST LV_ATTRIBUTE_IMG_CMD uint8_t cmd_map[] = {
  0xff, 0xff, 0xff, 0xff, 	/*Color of index 0*/
  0x00, 0x00, 0x00, 0xff, 	/*Color of index 1*/

  0x00, 0x00,
  0x00, 0x00,
  0x18, 0x60,
  0x24, 0x90,
  0x24, 0x90,
  0x1f, 0xe0,
  0x04, 0x80,
  0x04, 0x80,
  0x1f, 0xe0,
  0x24, 0x90,
  0x24, 0x90,
  0x18, 0x60,
  0x00, 0x00,
  0x00, 0x00,
};
const lv_img_dsc_t cmd_icon = {
  .header.cf = LV_COLOR_FORMAT_I1,
  .header.w = 14,
  .header.h = 14,
  .data_size = 36,
  .data = cmd_map,
};

#ifndef LV_ATTRIBUTE_IMG_OPT
#define LV_ATTRIBUTE_IMG_OPT
#endif
const LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST LV_ATTRIBUTE_IMG_OPT uint8_t opt_map[] = {
  0xff, 0xff, 0xff, 0xff, 	/*Color of index 0*/
  0x00, 0x00, 0x00, 0xff, 	/*Color of index 1*/

  0x00, 0x00,
  0x00, 0x00,
  0x3c, 0xe0,
  0x3c, 0xe0,
  0x06, 0x00,
  0x06, 0x00,
  0x06, 0x00,
  0x03, 0x00,
  0x03, 0x00,
  0x03, 0x00,
  0x01, 0xe0,
  0x01, 0xe0,
  0x00, 0x00,
  0x00, 0x00,
};
const lv_img_dsc_t opt_icon = {
  .header.cf = LV_COLOR_FORMAT_I1,
  .header.w = 14,
  .header.h = 14,
  .data_size = 36,
  .data = opt_map,
};

/* ============ Output status icons (9x14) ============ */

#ifndef LV_ATTRIBUTE_IMG_SYM_BT
#define LV_ATTRIBUTE_IMG_SYM_BT
#endif
const LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST LV_ATTRIBUTE_IMG_SYM_BT uint8_t sym_bt_map[] = {
  0xff, 0xff, 0xff, 0xff, 	/*Color of index 0*/
  0x00, 0x00, 0x00, 0xff, 	/*Color of index 1*/

  0x3e, 0x00, 0x67, 0x00, 0xe3, 0x80, 0xe9,
  0x80, 0x8c, 0x80, 0xc9, 0x80, 0xe3, 0x80,
  0xe3, 0x80, 0xc9, 0x80, 0x8c, 0x80, 0xe9,
  0x80, 0xe3, 0x80, 0x67, 0x00, 0x3e, 0x00,
};
const lv_img_dsc_t sym_bt = {
  .header.cf = LV_COLOR_FORMAT_I1,
  .header.w = 9,
  .header.h = 14,
  .data_size = 36,
  .data = sym_bt_map,
};

#ifndef LV_ATTRIBUTE_IMG_SYM_USB
#define LV_ATTRIBUTE_IMG_SYM_USB
#endif
const LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST LV_ATTRIBUTE_IMG_SYM_USB uint8_t sym_usb_map[] = {
  0xff, 0xff, 0xff, 0xff, 	/*Color of index 0*/
  0x00, 0x00, 0x00, 0xff, 	/*Color of index 1*/

  0x7f, 0x00, 0x41, 0x00, 0x55, 0x00, 0x41,
  0x00, 0xff, 0x80, 0x80, 0x80, 0x80, 0x80,
  0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
  0x80, 0x80, 0x80, 0x80, 0x80, 0xff, 0x80,
};
const lv_img_dsc_t sym_usb = {
  .header.cf = LV_COLOR_FORMAT_I1,
  .header.w = 9,
  .header.h = 14,
  .data_size = 36,
  .data = sym_usb_map,
};

/* ============ Output status glyphs (5x5), profile numbers (5x6) ============
 * Verbatim from englmaxi/zmk-dongle-display (same source as sym_usb/sym_bt
 * above), used to mark USB-HID-ready / BLE-bonded-and-connected state next
 * to the usb/bt icons, and the active BLE profile slot number. */

#ifndef LV_ATTRIBUTE_IMG_SYM_OK
#define LV_ATTRIBUTE_IMG_SYM_OK
#endif
const LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST LV_ATTRIBUTE_IMG_SYM_OK uint8_t sym_ok_map[] = {
  0xff, 0xff, 0xff, 0xff, 	/*Color of index 0*/
  0x00, 0x00, 0x00, 0xff, 	/*Color of index 1*/

  0x08, 0x18, 0xb0, 0xe0, 0x40,
};
const lv_img_dsc_t sym_ok = {
  .header.cf = LV_COLOR_FORMAT_I1,
  .header.w = 5,
  .header.h = 5,
  .data_size = 13,
  .data = sym_ok_map,
};

#ifndef LV_ATTRIBUTE_IMG_SYM_NOK
#define LV_ATTRIBUTE_IMG_SYM_NOK
#endif
const LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST LV_ATTRIBUTE_IMG_SYM_NOK uint8_t sym_nok_map[] = {
  0xff, 0xff, 0xff, 0xff, 	/*Color of index 0*/
  0x00, 0x00, 0x00, 0xff, 	/*Color of index 1*/

  0x88, 0xd8, 0x70, 0xd8, 0x88,
};
const lv_img_dsc_t sym_nok = {
  .header.cf = LV_COLOR_FORMAT_I1,
  .header.w = 5,
  .header.h = 5,
  .data_size = 13,
  .data = sym_nok_map,
};

#ifndef LV_ATTRIBUTE_IMG_SYM_OPEN
#define LV_ATTRIBUTE_IMG_SYM_OPEN
#endif
const LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST LV_ATTRIBUTE_IMG_SYM_OPEN uint8_t sym_open_map[] = {
  0xff, 0xff, 0xff, 0xff, 	/*Color of index 0*/
  0x00, 0x00, 0x00, 0xff, 	/*Color of index 1*/

  0x20, 0x70, 0xd8, 0x70, 0x20,
};
const lv_img_dsc_t sym_open = {
  .header.cf = LV_COLOR_FORMAT_I1,
  .header.w = 5,
  .header.h = 5,
  .data_size = 13,
  .data = sym_open_map,
};

#ifndef LV_ATTRIBUTE_IMG_SYM_1
#define LV_ATTRIBUTE_IMG_SYM_1
#endif
const LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST LV_ATTRIBUTE_IMG_SYM_1 uint8_t sym_1_map[] = {
  0xff, 0xff, 0xff, 0xff, 	/*Color of index 0*/
  0x00, 0x00, 0x00, 0xff, 	/*Color of index 1*/

  0x30, 0x70, 0x70, 0x30, 0x30, 0x30,
};
const lv_img_dsc_t sym_1 = {
  .header.cf = LV_COLOR_FORMAT_I1,
  .header.w = 5,
  .header.h = 6,
  .data_size = 14,
  .data = sym_1_map,
};

#ifndef LV_ATTRIBUTE_IMG_SYM_2
#define LV_ATTRIBUTE_IMG_SYM_2
#endif
const LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST LV_ATTRIBUTE_IMG_SYM_2 uint8_t sym_2_map[] = {
  0xff, 0xff, 0xff, 0xff, 	/*Color of index 0*/
  0x00, 0x00, 0x00, 0xff, 	/*Color of index 1*/

  0x70, 0xd8, 0x18, 0x30, 0x60, 0xf8,
};
const lv_img_dsc_t sym_2 = {
  .header.cf = LV_COLOR_FORMAT_I1,
  .header.w = 5,
  .header.h = 6,
  .data_size = 14,
  .data = sym_2_map,
};

#ifndef LV_ATTRIBUTE_IMG_SYM_3
#define LV_ATTRIBUTE_IMG_SYM_3
#endif
const LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST LV_ATTRIBUTE_IMG_SYM_3 uint8_t sym_3_map[] = {
  0xff, 0xff, 0xff, 0xff, 	/*Color of index 0*/
  0x00, 0x00, 0x00, 0xff, 	/*Color of index 1*/

  0x70, 0x98, 0x30, 0x18, 0xd8, 0x70,
};
const lv_img_dsc_t sym_3 = {
  .header.cf = LV_COLOR_FORMAT_I1,
  .header.w = 5,
  .header.h = 6,
  .data_size = 14,
  .data = sym_3_map,
};

#ifndef LV_ATTRIBUTE_IMG_SYM_4
#define LV_ATTRIBUTE_IMG_SYM_4
#endif
const LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST LV_ATTRIBUTE_IMG_SYM_4 uint8_t sym_4_map[] = {
  0xff, 0xff, 0xff, 0xff, 	/*Color of index 0*/
  0x00, 0x00, 0x00, 0xff, 	/*Color of index 1*/

  0x10, 0x30, 0x70, 0xd0, 0xf8, 0x10,
};
const lv_img_dsc_t sym_4 = {
  .header.cf = LV_COLOR_FORMAT_I1,
  .header.w = 5,
  .header.h = 6,
  .data_size = 14,
  .data = sym_4_map,
};

#ifndef LV_ATTRIBUTE_IMG_SYM_5
#define LV_ATTRIBUTE_IMG_SYM_5
#endif
const LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST LV_ATTRIBUTE_IMG_SYM_5 uint8_t sym_5_map[] = {
  0xff, 0xff, 0xff, 0xff, 	/*Color of index 0*/
  0x00, 0x00, 0x00, 0xff, 	/*Color of index 1*/

  0x78, 0x40, 0x70, 0x18, 0xd8, 0x70,
};
const lv_img_dsc_t sym_5 = {
  .header.cf = LV_COLOR_FORMAT_I1,
  .header.w = 5,
  .header.h = 6,
  .data_size = 14,
  .data = sym_5_map,
};

/* ============ Battery icon (20x6, 2x width of the original 10x6) ============ */

#ifndef LV_ATTRIBUTE_IMG_SYM_BATTERY
#define LV_ATTRIBUTE_IMG_SYM_BATTERY
#endif
const LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST LV_ATTRIBUTE_IMG_SYM_BATTERY uint8_t sym_battery_map[] = {
  0xff, 0xff, 0xff, 0xff, 	/*Color of index 0*/
  0x00, 0x00, 0x00, 0xff, 	/*Color of index 1*/

  0xff, 0xff, 0x00,
  0xc0, 0x03, 0x00,
  0xc0, 0x03, 0xf0,
  0xc0, 0x03, 0xf0,
  0xc0, 0x03, 0x00,
  0xff, 0xff, 0x00,
};
const lv_img_dsc_t sym_battery = {
  .header.cf = LV_COLOR_FORMAT_I1,
  .header.w = 20,
  .header.h = 6,
  .data_size = 26,
  .data = sym_battery_map,
};
