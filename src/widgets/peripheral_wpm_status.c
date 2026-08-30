#include "peripheral_wpm_status.h"
#include <lvgl.h>
#include <string.h>

static lv_obj_t *label;

int zmk_widget_peripheral_wpm_status_init(
    struct zmk_widget_peripheral_wpm_status *w, lv_obj_t *p)
{
    label = lv_label_create(p);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_font(label, &lv_font_unscii_8, 0);
    w->obj = label;
    return 0;
}

void zmk_widget_peripheral_wpm_status_update(
    struct zmk_widget_peripheral_wpm_status *w,
    const struct peripheral_status_adv_data *s)
{
    struct peripheral_wpm_status_state new_state = { .wpm = s->wpm_value };
    if (memcmp(&w->state, &new_state, sizeof(new_state)) == 0) return;
    w->state = new_state;
    if (s->wpm_value == 0) {
        lv_label_set_text(label, "");
    } else {
        lv_label_set_text_fmt(label, "WPM %u", s->wpm_value);
    }
}
