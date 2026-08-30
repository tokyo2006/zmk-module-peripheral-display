#pragma once
#include <zmk/peripheral_display.h>

struct peripheral_modifiers_state {
    uint8_t flags;       /* 8 bits, see PERIPHERAL_MOD_FLAG_* */
};

ZMK_PERIPHERAL_DISPLAY_WIDGET(modifiers, struct peripheral_modifiers_state)

int  zmk_widget_peripheral_modifiers_init(
    struct zmk_widget_peripheral_modifiers *w, lv_obj_t *p);
void zmk_widget_peripheral_modifiers_update(
    struct zmk_widget_peripheral_modifiers *w,
    const struct peripheral_status_adv_data *s);
