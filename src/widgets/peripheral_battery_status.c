#include "peripheral_battery_status.h"
#include "peripheral_icons.h"
#include <lvgl.h>
#include <string.h>
#include <zmk/battery.h>

static lv_obj_t *central_row;
static lv_obj_t *central_label;
static lv_obj_t *central_icon;
static lv_obj_t *central_fill;
static lv_obj_t *peripheral_row;
static lv_obj_t *peripheral_label;
static lv_obj_t *peripheral_icon;
static lv_obj_t *peripheral_fill;

/* sym_battery's interior (inside the 2px-wide/1px-tall border) is 12x4 px
 * starting at (2,1). Size the fill bar to that fraction of pct so the icon
 * actually reflects charge instead of always drawing empty. */
static void set_battery_fill(lv_obj_t *fill, lv_obj_t *icon, uint8_t pct) {
    int w = (pct * 12) / 100;
    if (pct > 0 && w < 1) w = 1;
    lv_obj_set_width(fill, w);
    lv_obj_align_to(fill, icon, LV_ALIGN_TOP_LEFT, 2, 1);
}

static void set_row(lv_obj_t *label, lv_obj_t *icon, lv_obj_t *fill, uint8_t pct) {
    if (pct > 100) pct = 100;
    lv_label_set_text_fmt(label, "%u%%", pct);
    /* Re-align every update: the label's rendered width changes with the
     * digit count, so the icon needs to follow it to stay right after it. */
    lv_obj_align_to(icon, label, LV_ALIGN_OUT_RIGHT_MID, 2, 0);
    set_battery_fill(fill, icon, pct);
}

int zmk_widget_peripheral_battery_status_init(
    struct zmk_widget_peripheral_battery_status *w, lv_obj_t *p)
{
    lv_obj_t *col = lv_obj_create(p);
    lv_obj_set_size(col, 64, 32);
    lv_obj_set_style_bg_opa(col, LV_OPA_TRANSP, 0);

    central_row = lv_obj_create(col);
    lv_obj_set_size(central_row, 64, 16);
    lv_obj_set_style_bg_opa(central_row, LV_OPA_TRANSP, 0);
    lv_obj_align(central_row, LV_ALIGN_TOP_LEFT, 0, 0);
    central_label = lv_label_create(central_row);
    lv_obj_align(central_label, LV_ALIGN_LEFT_MID, 0, 0);
    central_icon = lv_img_create(central_row);
    lv_img_set_src(central_icon, &sym_battery);
    central_fill = lv_obj_create(central_row);
    lv_obj_set_style_bg_color(central_fill, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(central_fill, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(central_fill, 0, 0);
    lv_obj_set_size(central_fill, 0, 4);

    peripheral_row = lv_obj_create(col);
    lv_obj_set_size(peripheral_row, 64, 16);
    lv_obj_set_style_bg_opa(peripheral_row, LV_OPA_TRANSP, 0);
    lv_obj_align(peripheral_row, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    peripheral_label = lv_label_create(peripheral_row);
    lv_obj_align(peripheral_label, LV_ALIGN_LEFT_MID, 0, 0);
    peripheral_icon = lv_img_create(peripheral_row);
    lv_img_set_src(peripheral_icon, &sym_battery);
    peripheral_fill = lv_obj_create(peripheral_row);
    lv_obj_set_style_bg_color(peripheral_fill, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(peripheral_fill, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(peripheral_fill, 0, 0);
    lv_obj_set_size(peripheral_fill, 0, 4);

    lv_obj_align(col, LV_ALIGN_TOP_RIGHT, 0, 0);
    w->obj = col;

    /* Position the icons against the initial "0%" text. */
    lv_obj_align_to(central_icon, central_label, LV_ALIGN_OUT_RIGHT_MID, 2, 0);
    lv_obj_align_to(peripheral_icon, peripheral_label, LV_ALIGN_OUT_RIGHT_MID, 2, 0);
    return 0;
}

void zmk_widget_peripheral_battery_status_update(
    struct zmk_widget_peripheral_battery_status *w,
    const struct peripheral_status_adv_data *s)
{
    uint8_t own = zmk_battery_state_of_charge();
    if (own > 100) own = 100;
    struct peripheral_battery_status_state new_state = {
        .central_pct    = s->battery_level,
        .peripheral_pct = own,
    };
    if (memcmp(&w->state, &new_state, sizeof(new_state)) == 0) return;
    w->state = new_state;
    set_row(central_label,    central_icon,    central_fill,    new_state.central_pct);
    set_row(peripheral_label, peripheral_icon, peripheral_fill, new_state.peripheral_pct);
}
