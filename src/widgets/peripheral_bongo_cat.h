#pragma once
#include <zmk/peripheral_display.h>

struct peripheral_bongo_cat_state {
    uint8_t wpm;
};

ZMK_PERIPHERAL_DISPLAY_WIDGET(bongo_cat, struct peripheral_bongo_cat_state)

int  zmk_widget_peripheral_bongo_cat_init(
    struct zmk_widget_peripheral_bongo_cat *w, lv_obj_t *p);
void zmk_widget_peripheral_bongo_cat_update(
    struct zmk_widget_peripheral_bongo_cat *w,
    const struct peripheral_status_adv_data *s);

extern const lv_img_dsc_t peripheral_bongo_cat_none;
extern const lv_img_dsc_t peripheral_bongo_cat_left1;
extern const lv_img_dsc_t peripheral_bongo_cat_left2;
extern const lv_img_dsc_t peripheral_bongo_cat_right1;
extern const lv_img_dsc_t peripheral_bongo_cat_right2;
extern const lv_img_dsc_t peripheral_bongo_cat_both1;
extern const lv_img_dsc_t peripheral_bongo_cat_both1_open;
extern const lv_img_dsc_t peripheral_bongo_cat_both2;
