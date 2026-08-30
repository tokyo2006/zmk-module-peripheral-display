#include "peripheral_battery_status.h"
#include <lvgl.h>
#include <string.h>
#include <zmk/battery.h>

static lv_obj_t *central_row;
static lv_obj_t *central_label;
static lv_obj_t *peripheral_row;
static lv_obj_t *peripheral_label;

static void set_row(lv_obj_t *label, uint8_t pct) {
    if (pct > 100) pct = 100;
    lv_label_set_text_fmt(label, "%u%%", pct);
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

    peripheral_row = lv_obj_create(col);
    lv_obj_set_size(peripheral_row, 64, 16);
    lv_obj_set_style_bg_opa(peripheral_row, LV_OPA_TRANSP, 0);
    lv_obj_align(peripheral_row, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    peripheral_label = lv_label_create(peripheral_row);
    lv_obj_align(peripheral_label, LV_ALIGN_LEFT_MID, 0, 0);

    lv_obj_align(col, LV_ALIGN_TOP_RIGHT, 0, 0);
    w->obj = col;
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
    set_row(central_label,    new_state.central_pct);
    set_row(peripheral_label, new_state.peripheral_pct);
}
