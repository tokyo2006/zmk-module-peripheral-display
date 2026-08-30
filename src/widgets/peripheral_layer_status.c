#include "peripheral_layer_status.h"
#include <lvgl.h>
#include <string.h>

static lv_obj_t *label;

static void set_layer_text(const struct peripheral_layer_status_state *s) {
    if (s->name[0] == '\0') {
        lv_label_set_text_fmt(label, "L%u", s->index);
    } else {
        lv_label_set_text(label, s->name);
    }
}

int zmk_widget_peripheral_layer_status_init(
    struct zmk_widget_peripheral_layer_status *w, lv_obj_t *p)
{
    label = lv_label_create(p);
    lv_obj_set_style_text_font(label, &lv_font_unscii_8, 0);
    w->obj = label;
    return 0;
}

void zmk_widget_peripheral_layer_status_update(
    struct zmk_widget_peripheral_layer_status *w,
    const struct peripheral_status_adv_data *s)
{
    struct peripheral_layer_status_state new_state = {
        .index = s->active_layer,
    };
    memcpy(new_state.name, s->layer_name, 4);
    new_state.name[4] = '\0';
    if (memcmp(&w->state, &new_state, sizeof(new_state)) == 0) return;
    w->state = new_state;
    set_layer_text(&w->state);
}
