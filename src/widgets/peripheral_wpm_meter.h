#pragma once
#include <zmk/peripheral_display.h>

struct peripheral_wpm_meter_state {
    uint8_t wpm;
};

ZMK_PERIPHERAL_DISPLAY_WIDGET(wpm_meter, struct peripheral_wpm_meter_state)

int  zmk_widget_peripheral_wpm_meter_init(
    struct zmk_widget_peripheral_wpm_meter *w, lv_obj_t *p);
void zmk_widget_peripheral_wpm_meter_update(
    struct zmk_widget_peripheral_wpm_meter *w,
    const struct peripheral_status_adv_data *s);
