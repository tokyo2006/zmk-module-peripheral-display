#include "peripheral_bongo_cat.h"
#include <lvgl.h>
#include <string.h>

static lv_obj_t *img;

int zmk_widget_peripheral_bongo_cat_init(
    struct zmk_widget_peripheral_bongo_cat *w, lv_obj_t *p)
{
    img = lv_img_create(p);
    lv_img_set_src(img, &peripheral_bongo_cat_none);
    lv_obj_align(img, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
    w->obj = img;
    return 0;
}

void zmk_widget_peripheral_bongo_cat_update(
    struct zmk_widget_peripheral_bongo_cat *w,
    const struct peripheral_status_adv_data *s)
{
    struct peripheral_bongo_cat_state new_state = { .wpm = s->wpm_value };
    if (memcmp(&w->state, &new_state, sizeof(new_state)) == 0) return;
    w->state = new_state;

    lv_img_set_src(img, s->wpm_value > 0 ? &peripheral_bongo_cat_both1
                                         : &peripheral_bongo_cat_none);
}
