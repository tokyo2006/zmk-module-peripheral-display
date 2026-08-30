#pragma once
#include <zmk/peripheral_display.h>

struct peripheral_wpm_status_state {
    uint8_t wpm;
};

ZMK_PERIPHERAL_DISPLAY_WIDGET(wpm_status, struct peripheral_wpm_status_state)

int  zmk_widget_peripheral_wpm_status_init(
    struct zmk_widget_peripheral_wpm_status *w, lv_obj_t *p);
void zmk_widget_peripheral_wpm_status_update(
    struct zmk_widget_peripheral_wpm_status *w,
    const struct peripheral_status_adv_data *s);
