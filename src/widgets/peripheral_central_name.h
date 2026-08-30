#pragma once
#include <zmk/peripheral_display.h>

struct peripheral_central_name_state {
    uint8_t id[4];
};

ZMK_PERIPHERAL_DISPLAY_WIDGET(central_name, struct peripheral_central_name_state)

int  zmk_widget_peripheral_central_name_init(
    struct zmk_widget_peripheral_central_name *w, lv_obj_t *p);
void zmk_widget_peripheral_central_name_update(
    struct zmk_widget_peripheral_central_name *w,
    const struct peripheral_status_adv_data *s);
