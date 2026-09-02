#include "peripheral_bongo_cat.h"
#include <lvgl.h>
#include <string.h>

#define SRC(array) (const void **)array, sizeof(array) / sizeof(lv_img_dsc_t *)

static const lv_img_dsc_t *idle_imgs[] = {
    &peripheral_bongo_cat_both1_open,
    &peripheral_bongo_cat_both1_open,
    &peripheral_bongo_cat_both1_open,
    &peripheral_bongo_cat_both1,
};
#define ANIMATION_SPEED_IDLE 10000

static const lv_img_dsc_t *slow_imgs[] = {
    &peripheral_bongo_cat_left1,
    &peripheral_bongo_cat_both1,
    &peripheral_bongo_cat_both1,
    &peripheral_bongo_cat_right1,
    &peripheral_bongo_cat_both1,
    &peripheral_bongo_cat_both1,
    &peripheral_bongo_cat_left1,
    &peripheral_bongo_cat_both1,
    &peripheral_bongo_cat_both1,
};
#define ANIMATION_SPEED_SLOW 2000

static const lv_img_dsc_t *mid_imgs[] = {
    &peripheral_bongo_cat_left2,
    &peripheral_bongo_cat_left1,
    &peripheral_bongo_cat_none,
    &peripheral_bongo_cat_right2,
    &peripheral_bongo_cat_right1,
    &peripheral_bongo_cat_none,
};
#define ANIMATION_SPEED_MID 500

static const lv_img_dsc_t *fast_imgs[] = {
    &peripheral_bongo_cat_both2,
    &peripheral_bongo_cat_both1,
    &peripheral_bongo_cat_none,
    &peripheral_bongo_cat_none,
};
#define ANIMATION_SPEED_FAST 200

enum anim_state {
    ANIM_STATE_NONE,
    ANIM_STATE_IDLE,
    ANIM_STATE_SLOW,
    ANIM_STATE_MID,
    ANIM_STATE_FAST,
};
static enum anim_state current_anim_state = ANIM_STATE_NONE;

static void set_animation(lv_obj_t *animimg, uint8_t wpm)
{
    enum anim_state target;

    if (wpm < 5) {
        target = ANIM_STATE_IDLE;
    } else if (wpm < 30) {
        target = ANIM_STATE_SLOW;
    } else if (wpm < 70) {
        target = ANIM_STATE_MID;
    } else {
        target = ANIM_STATE_FAST;
    }

    if (target == current_anim_state) {
        return;
    }
    current_anim_state = target;

    switch (target) {
    case ANIM_STATE_IDLE:
        lv_animimg_set_src(animimg, SRC(idle_imgs));
        lv_animimg_set_duration(animimg, ANIMATION_SPEED_IDLE);
        break;
    case ANIM_STATE_SLOW:
        lv_animimg_set_src(animimg, SRC(slow_imgs));
        lv_animimg_set_duration(animimg, ANIMATION_SPEED_SLOW);
        break;
    case ANIM_STATE_MID:
        lv_animimg_set_src(animimg, SRC(mid_imgs));
        lv_animimg_set_duration(animimg, ANIMATION_SPEED_MID);
        break;
    case ANIM_STATE_FAST:
        lv_animimg_set_src(animimg, SRC(fast_imgs));
        lv_animimg_set_duration(animimg, ANIMATION_SPEED_FAST);
        break;
    default:
        return;
    }

    lv_animimg_set_repeat_count(animimg, LV_ANIM_REPEAT_INFINITE);
    lv_animimg_start(animimg);
}

int zmk_widget_peripheral_bongo_cat_init(
    struct zmk_widget_peripheral_bongo_cat *w, lv_obj_t *p)
{
    w->obj = lv_animimg_create(p);
    /* 1.3x zoom (LVGL: 256 = 1:1) so the cat reads better on this 128x128
     * panel without hand-redrawing all 8 frames at a larger native size.
     * Zoom is applied around the object's own center, growing its visual
     * footprint symmetrically beyond its nominal 50x26 box -- the align
     * offset below is shifted inward (rather than 0,0) so that growth
     * lands inside the screen instead of clipping off the right/bottom
     * edge. Picked 1.3x over a larger zoom specifically because the grown
     * left edge starts to reach toward the modifier row's rightmost
     * (Shift) icon at the bottom-left; a bigger zoom would need the
     * modifier row moved too. Untested on hardware: if the mono display's
     * fast path doesn't honor image zoom, or the offset needs tuning,
     * this is the first value to revisit. */
    lv_img_set_zoom(w->obj, 332);
    lv_obj_align(w->obj, LV_ALIGN_BOTTOM_RIGHT, -10, -7);
    current_anim_state = ANIM_STATE_NONE;
    set_animation(w->obj, 0); /* start idle */
    return 0;
}

void zmk_widget_peripheral_bongo_cat_update(
    struct zmk_widget_peripheral_bongo_cat *w,
    const struct peripheral_status_adv_data *s)
{
    struct peripheral_bongo_cat_state new_state = { .wpm = s->wpm_value };
    if (memcmp(&w->state, &new_state, sizeof(new_state)) == 0) {
        return;
    }
    w->state = new_state;
    set_animation(w->obj, s->wpm_value);
}
