#include "peripheral_central_name.h"
#include <lvgl.h>
#include <string.h>

static lv_obj_t *label;

/* Placeholder: render the 4-byte keyboard_id as hex.
 * A real impl looks up the id in NVS or a hardcoded table to show
 * the user's chosen name. */
static void set_hex(const uint8_t id[4]) {
    lv_label_set_text_fmt(label, "%02X%02X", id[0], id[1]);
}

int zmk_widget_peripheral_central_name_init(
    struct zmk_widget_peripheral_central_name *w, lv_obj_t *p)
{
    label = lv_label_create(p);
    lv_obj_set_style_text_font(label, &lv_font_unscii_8, 0);
    /* Below the battery column. */
    lv_obj_align(label, LV_ALIGN_TOP_RIGHT, 0, 36);
    w->obj = label;
    return 0;
}

void zmk_widget_peripheral_central_name_update(
    struct zmk_widget_peripheral_central_name *w,
    const struct peripheral_status_adv_data *s)
{
    struct peripheral_central_name_state new_state;
    memcpy(new_state.id, s->keyboard_id, 4);
    if (memcmp(&w->state, &new_state, sizeof(new_state)) == 0) return;
    w->state = new_state;
    set_hex(new_state.id);
}
