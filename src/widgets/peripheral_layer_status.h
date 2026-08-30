#pragma once
#include <zmk/peripheral_display.h>

struct peripheral_layer_status_state {
    uint8_t index;
    char name[5];
};

ZMK_PERIPHERAL_DISPLAY_WIDGET(layer_status, struct peripheral_layer_status_state)

int  zmk_widget_peripheral_layer_status_init(
    struct zmk_widget_peripheral_layer_status *w, lv_obj_t *p);
void zmk_widget_peripheral_layer_status_update(
    struct zmk_widget_peripheral_layer_status *w,
    const struct peripheral_status_adv_data *s);
