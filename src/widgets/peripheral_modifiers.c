#include "peripheral_modifiers.h"
#include "peripheral_icons.h"
#include <lvgl.h>
#include <string.h>

/*
 * Modifier icons (14x14 bitmaps from zmk-dongle-display). Each symbol
 * covers a left/right modifier pair (see PERIPHERAL_MOD_FLAG_* bit order):
 *   LCTL/RCTL = 0x11, LSFT/RSFT = 0x22, LALT/RALT = 0x44, LGUI/RGUI = 0x88
 */
struct modifier_symbol {
    uint8_t modifier;
    const lv_img_dsc_t *symbol_dsc;
    lv_obj_t *symbol;
    lv_obj_t *underline;
};

#if IS_ENABLED(CONFIG_ZMK_PERIPHERAL_DISPLAY_MODIFIERS_STYLE_MAC)
static struct modifier_symbol symbols[] = {
    { .modifier = 0x11, .symbol_dsc = &control_icon },
    { .modifier = 0x44, .symbol_dsc = &opt_icon },
    { .modifier = 0x88, .symbol_dsc = &cmd_icon },
    { .modifier = 0x22, .symbol_dsc = &shift_icon },
};
#else
static struct modifier_symbol symbols[] = {
    { .modifier = 0x88, .symbol_dsc = &win_icon },
    { .modifier = 0x44, .symbol_dsc = &alt_icon },
    { .modifier = 0x11, .symbol_dsc = &control_icon },
    { .modifier = 0x22, .symbol_dsc = &shift_icon },
};
#endif

#define NUM_SYMBOLS (sizeof(symbols) / sizeof(symbols[0]))
#define ICON_SIZE 14
#define ICON_GAP 2

int zmk_widget_peripheral_modifiers_init(
    struct zmk_widget_peripheral_modifiers *w, lv_obj_t *p)
{
    lv_obj_t *row = lv_obj_create(p);
    lv_obj_set_size(row, NUM_SYMBOLS * (ICON_SIZE + ICON_GAP), ICON_SIZE + 5);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);

    for (int i = 0; i < NUM_SYMBOLS; i++) {
        symbols[i].symbol = lv_img_create(row);
        lv_obj_align(symbols[i].symbol, LV_ALIGN_TOP_LEFT,
                     i * (ICON_SIZE + ICON_GAP), 1);
        lv_img_set_src(symbols[i].symbol, symbols[i].symbol_dsc);
        lv_obj_set_style_img_opa(symbols[i].symbol, LV_OPA_60, 0);

        /* Underline that lights up under the icon while its modifier is
         * held, same visual language as the output-status selection bar. */
        symbols[i].underline = lv_obj_create(row);
        lv_obj_set_size(symbols[i].underline, ICON_SIZE, 2);
        lv_obj_set_style_bg_color(symbols[i].underline, lv_color_black(), 0);
        lv_obj_set_style_bg_opa(symbols[i].underline, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(symbols[i].underline, 0, 0);
        /* The mono theme's default object style sets radius=2 on every
         * plain lv_obj_create(); on a 2px-tall bar that rounds it away to
         * nothing visible. */
        lv_obj_set_style_radius(symbols[i].underline, 0, 0);
        lv_obj_align_to(symbols[i].underline, symbols[i].symbol,
                        LV_ALIGN_OUT_BOTTOM_MID, 0, 1);
        lv_obj_add_flag(symbols[i].underline, LV_OBJ_FLAG_HIDDEN);
    }

    lv_obj_align(row, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    w->obj = row;
    return 0;
}

void zmk_widget_peripheral_modifiers_update(
    struct zmk_widget_peripheral_modifiers *w,
    const struct peripheral_status_adv_data *s)
{
    struct peripheral_modifiers_state new_state = { .flags = s->modifier_flags };
    if (memcmp(&w->state, &new_state, sizeof(new_state)) == 0) return;
    w->state = new_state;
    for (int i = 0; i < NUM_SYMBOLS; i++) {
        bool on = (s->modifier_flags & symbols[i].modifier) != 0;
        lv_obj_set_style_img_opa(symbols[i].symbol,
                                 on ? LV_OPA_COVER : LV_OPA_60, 0);
        if (on) {
            lv_obj_clear_flag(symbols[i].underline, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(symbols[i].underline, LV_OBJ_FLAG_HIDDEN);
        }
    }
}
