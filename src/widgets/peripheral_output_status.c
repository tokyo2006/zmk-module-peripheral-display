#include "peripheral_output_status.h"
#include "peripheral_icons.h"
#include <lvgl.h>
#include <string.h>

static const lv_img_dsc_t *const profile_syms[] = {&sym_1, &sym_2, &sym_3, &sym_4, &sym_5};
#define NUM_PROFILE_SYMS (sizeof(profile_syms) / sizeof(profile_syms[0]))

static lv_obj_t *usb_img;
static lv_obj_t *usb_hid_status;
static lv_obj_t *bt_img;
static lv_obj_t *bt_number;
static lv_obj_t *bt_status;
static lv_obj_t *selection_bar;

int zmk_widget_peripheral_output_status_init(
    struct zmk_widget_peripheral_output_status *w, lv_obj_t *p)
{
    lv_obj_t *row = lv_obj_create(p);
    lv_obj_set_size(row, 32, 20);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);

    /* usb/bt icons start 3px down to leave room for the selection bar
     * that sits above whichever one is active. */
    usb_img = lv_img_create(row);
    lv_obj_align(usb_img, LV_ALIGN_TOP_LEFT, 0, 3);
    lv_img_set_src(usb_img, &sym_usb);

    usb_hid_status = lv_img_create(row);
    lv_obj_align_to(usb_hid_status, usb_img, LV_ALIGN_CENTER, 0, 0);
    lv_img_set_src(usb_hid_status, &sym_nok);

    bt_img = lv_img_create(row);
    lv_obj_align_to(bt_img, usb_img, LV_ALIGN_OUT_RIGHT_TOP, 4, 0);
    lv_img_set_src(bt_img, &sym_bt);

    bt_status = lv_img_create(row);
    lv_obj_align_to(bt_status, bt_img, LV_ALIGN_OUT_RIGHT_TOP, 1, 1);
    lv_img_set_src(bt_status, &sym_open);

    bt_number = lv_img_create(row);
    lv_obj_align_to(bt_number, bt_img, LV_ALIGN_OUT_RIGHT_TOP, 1, 8);
    lv_img_set_src(bt_number, &sym_1);

    /* Short bar above whichever transport is actively selected. */
    selection_bar = lv_obj_create(row);
    lv_obj_set_size(selection_bar, 9, 2);
    lv_obj_set_style_bg_color(selection_bar, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(selection_bar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(selection_bar, 0, 0);
    lv_obj_align_to(selection_bar, usb_img, LV_ALIGN_OUT_TOP_MID, 0, -1);

    lv_obj_align(row, LV_ALIGN_TOP_LEFT, 0, 0);
    w->obj = row;
    return 0;
}

void zmk_widget_peripheral_output_status_update(
    struct zmk_widget_peripheral_output_status *w,
    const struct peripheral_status_adv_data *s)
{
    struct peripheral_output_status_state new_state = {
        .usb_hid_ready = s->status_flags & PERIPHERAL_STATUS_FLAG_USB_HID_READY,
        .ble_connected = s->status_flags & PERIPHERAL_STATUS_FLAG_BLE_CONNECTED,
        .ble_bonded    = s->status_flags & PERIPHERAL_STATUS_FLAG_BLE_BONDED,
        .usb_selected  = s->status_flags & PERIPHERAL_STATUS_FLAG_OUTPUT_USB_SELECTED,
        .profile_slot  = s->profile_slot,
    };
    if (memcmp(&w->state, &new_state, sizeof(new_state)) == 0) return;
    w->state = new_state;

    lv_img_set_src(usb_hid_status, new_state.usb_hid_ready ? &sym_ok : &sym_nok);

    if (!new_state.ble_bonded) {
        lv_img_set_src(bt_status, &sym_open);
    } else {
        lv_img_set_src(bt_status, new_state.ble_connected ? &sym_ok : &sym_nok);
    }

    if (new_state.profile_slot < NUM_PROFILE_SYMS) {
        lv_img_set_src(bt_number, profile_syms[new_state.profile_slot]);
    } else {
        lv_img_set_src(bt_number, &sym_nok);
    }

    lv_obj_align_to(selection_bar, new_state.usb_selected ? usb_img : bt_img,
                    LV_ALIGN_OUT_TOP_MID, 0, -1);
}
