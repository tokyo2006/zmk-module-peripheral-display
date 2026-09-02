#include "peripheral_hid_indicators.h"
#include <lvgl.h>
#include <string.h>
#include <dt-bindings/zmk/hid_indicators.h>

static lv_obj_t *labels[2];
static lv_obj_t *underlines[2];
static const char *const names[2] = {"CAP", "NUM"};

int zmk_widget_peripheral_hid_indicators_init(
    struct zmk_widget_peripheral_hid_indicators *w, lv_obj_t *p)
{
    lv_obj_t *row = lv_obj_create(p);
    lv_obj_set_size(row, 24, 20);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);

    /* Stacked vertically, not side by side: without an explicit font these
     * labels used LVGL's (much wider than unscii_8) default font, so the
     * old i*16 horizontal spacing wasn't enough and all three drew on top
     * of each other. Stacking avoids re-measuring text width entirely, and
     * keeps this widget inside x:0-24 (battery claims x:64-128). Each label
     * gets an 11-row slot (8px text + 1px underline + 2px gap) so the
     * underline doesn't touch the next label -- 9 (no gap) had it doing
     * exactly that. Scroll Lock dropped -- nothing needs it yet. */
    for (int i = 0; i < 2; i++) {
        labels[i] = lv_label_create(row);
        lv_label_set_text(labels[i], names[i]);
        lv_obj_set_style_text_font(labels[i], &lv_font_unscii_8, 0);
        lv_obj_align(labels[i], LV_ALIGN_TOP_LEFT, 0, i * 11);
        lv_obj_set_style_text_opa(labels[i], LV_OPA_60, 0);

        /* Underline that lights up while this indicator's LED is on, same
         * visual language as the modifier icons' underline. */
        underlines[i] = lv_obj_create(row);
        lv_obj_set_size(underlines[i], 24, 1);
        lv_obj_set_style_bg_color(underlines[i], lv_color_black(), 0);
        lv_obj_set_style_bg_opa(underlines[i], LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(underlines[i], 0, 0);
        /* The mono theme's default object style sets radius=2 on every
         * plain lv_obj_create(); on a 1px-tall bar that rounds it away to
         * nothing visible. */
        lv_obj_set_style_radius(underlines[i], 0, 0);
        lv_obj_align_to(underlines[i], labels[i], LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
        lv_obj_add_flag(underlines[i], LV_OBJ_FLAG_HIDDEN);
    }
    /* Top-left, below the output_status row (now 20px tall: icons + the
     * selection bar above them + the HID-ready glyph below them). */
    lv_obj_align(row, LV_ALIGN_TOP_LEFT, 0, 20);
    w->obj = row;
    return 0;
}

void zmk_widget_peripheral_hid_indicators_update(
    struct zmk_widget_peripheral_hid_indicators *w,
    const struct peripheral_status_adv_data *s)
{
    bool caps = (s->hid_indicators & HID_INDICATOR_CAPS_LOCK) != 0;
    bool num  = (s->hid_indicators & HID_INDICATOR_NUM_LOCK) != 0;

    struct peripheral_hid_indicators_state new_state = {
        .caps = caps, .num = num,
    };
    if (memcmp(&w->state, &new_state, sizeof(new_state)) == 0) return;
    w->state = new_state;

    bool on[2] = { caps, num };
    for (int i = 0; i < 2; i++) {
        lv_obj_set_style_text_opa(labels[i], on[i] ? LV_OPA_COVER : LV_OPA_60, 0);
        if (on[i]) {
            lv_obj_clear_flag(underlines[i], LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(underlines[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
}
