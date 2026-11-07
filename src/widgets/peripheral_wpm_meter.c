#include "peripheral_wpm_meter.h"
#include <lvgl.h>
#include <string.h>

/* Operator-style segmented WPM meter: 16 bars across 128 px. */
#define WPM_BAR_COUNT 16
#define WPM_BAR_WIDTH 6
#define WPM_BAR_GAP 2
#define WPM_BAR_HEIGHT 16
#define WPM_MAX 120

static lv_obj_t *bars[WPM_BAR_COUNT];

static void set_bar(int index, bool active) {
    lv_obj_set_style_bg_color(bars[index], active ? lv_color_black() : lv_color_white(),
                              LV_PART_MAIN);
    lv_obj_set_style_bg_opa(bars[index], LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_color(bars[index], lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_border_width(bars[index], 1, LV_PART_MAIN);
}

int zmk_widget_peripheral_wpm_meter_init(
    struct zmk_widget_peripheral_wpm_meter *w, lv_obj_t *p)
{
    lv_obj_t *row = lv_obj_create(p);
    lv_obj_set_size(row, WPM_BAR_COUNT * WPM_BAR_WIDTH + (WPM_BAR_COUNT - 1) * WPM_BAR_GAP,
                    WPM_BAR_HEIGHT);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(row, 0, LV_PART_MAIN);

    for (int i = 0; i < WPM_BAR_COUNT; i++) {
        bars[i] = lv_obj_create(row);
        lv_obj_set_size(bars[i], WPM_BAR_WIDTH, WPM_BAR_HEIGHT);
        lv_obj_set_pos(bars[i], i * (WPM_BAR_WIDTH + WPM_BAR_GAP), 0);
        lv_obj_set_style_radius(bars[i], 0, LV_PART_MAIN);
        lv_obj_set_style_pad_all(bars[i], 0, LV_PART_MAIN);
        set_bar(i, false);
    }

    w->obj = row;
    return 0;
}

void zmk_widget_peripheral_wpm_meter_update(
    struct zmk_widget_peripheral_wpm_meter *w,
    const struct peripheral_status_adv_data *s)
{
    uint8_t wpm = s->wpm_value;
    struct peripheral_wpm_meter_state new_state = { .wpm = wpm };
    if (memcmp(&w->state, &new_state, sizeof(new_state)) == 0) return;
    w->state = new_state;

    if (wpm > WPM_MAX) wpm = WPM_MAX;
    int active = (wpm * WPM_BAR_COUNT) / WPM_MAX;
    if (wpm > 0 && active < 1) active = 1;
    if (active > WPM_BAR_COUNT) active = WPM_BAR_COUNT;

    for (int i = 0; i < WPM_BAR_COUNT; i++) {
        set_bar(i, i < active);
    }
}
