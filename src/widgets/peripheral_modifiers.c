#include "peripheral_modifiers.h"
#include <lvgl.h>
#include <string.h>

/* Bit order: LCTL LSFT LALT LGUI RCTL RSFT RALT RGUI.
 * Each bit renders its glyph in a small box. */
static lv_obj_t *row;
static lv_obj_t *boxes[8];

/* Single-char glyphs. The mono lv_font_unscii_8 has no ⌘⌥⌃⇧ codepoints,
 * so letters are used for both styles. */
static char glyph_for(uint8_t bit, bool mac) {
    static const char win_g[8] = {'C','S','A','W','c','s','a','w'};
    static const char mac_g[8] = {'C','S','A','W','c','s','a','w'};
    return (mac ? mac_g : win_g)[bit];
}

int zmk_widget_peripheral_modifiers_init(
    struct zmk_widget_peripheral_modifiers *w, lv_obj_t *p)
{
    row = lv_obj_create(p);
    lv_obj_set_size(row, 64, 12);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);

    bool mac = IS_ENABLED(CONFIG_ZMK_PERIPHERAL_DISPLAY_MODIFIERS_STYLE_MAC);

    for (int i = 0; i < 8; i++) {
        boxes[i] = lv_label_create(row);
        char g = glyph_for((uint8_t)i, mac);
        lv_label_set_text_fmt(boxes[i], "%c", g);
        lv_obj_align(boxes[i], LV_ALIGN_LEFT_MID, i * 8, 0);
        lv_obj_set_style_text_opa(boxes[i], LV_OPA_30, 0);
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
    for (int i = 0; i < 8; i++) {
        bool on = (s->modifier_flags >> i) & 1;
        lv_obj_set_style_text_opa(boxes[i],
            on ? LV_OPA_COVER : LV_OPA_30, 0);
    }
}
