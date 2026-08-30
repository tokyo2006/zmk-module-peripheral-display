#pragma once
#include <zmk/peripheral_display.h>

struct peripheral_battery_status_state {
    uint8_t central_pct;
    uint8_t peripheral_pct;
};

ZMK_PERIPHERAL_DISPLAY_WIDGET(battery_status, struct peripheral_battery_status_state)

int  zmk_widget_peripheral_battery_status_init(
    struct zmk_widget_peripheral_battery_status *w, lv_obj_t *p);
void zmk_widget_peripheral_battery_status_update(
    struct zmk_widget_peripheral_battery_status *w,
    const struct peripheral_status_adv_data *s);
