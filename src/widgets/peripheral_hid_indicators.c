#include "peripheral_hid_indicators.h"
#include <lvgl.h>
#include <string.h>

static lv_obj_t *labels[3];
static const char *const names[3] = {"CAP", "NUM", "SCR"};

int zmk_widget_peripheral_hid_indicators_init(
    struct zmk_widget_peripheral_hid_indicators *w, lv_obj_t *p)
{
    lv_obj_t *row = lv_obj_create(p);
    lv_obj_set_size(row, 48, 12);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);

    for (int i = 0; i < 3; i++) {
        labels[i] = lv_label_create(row);
        lv_label_set_text(labels[i], names[i]);
        lv_obj_align(labels[i], LV_ALIGN_LEFT_MID, i * 16, 0);
        lv_obj_set_style_text_opa(labels[i], LV_OPA_30, 0);
    }
    /* Top-left, just to the right of output_status widget. */
    lv_obj_align(row, LV_ALIGN_TOP_LEFT, 48, 0);
    w->obj = row;
    return 0;
}

void zmk_widget_peripheral_hid_indicators_update(
    struct zmk_widget_peripheral_hid_indicators *w,
    const struct peripheral_status_adv_data *s)
{
    bool caps = (s->status_flags & PERIPHERAL_STATUS_FLAG_CAPS_WORD) != 0;
    bool num  = false;
    bool scrl = false;

    struct peripheral_hid_indicators_state new_state = {
        .caps = caps, .num = num, .scroll = scrl,
    };
    if (memcmp(&w->state, &new_state, sizeof(new_state)) == 0) return;
    w->state = new_state;

    lv_obj_set_style_text_opa(labels[0], caps ? LV_OPA_COVER : LV_OPA_30, 0);
    lv_obj_set_style_text_opa(labels[1], num  ? LV_OPA_COVER : LV_OPA_30, 0);
    lv_obj_set_style_text_opa(labels[2], scrl ? LV_OPA_COVER : LV_OPA_30, 0);
}
