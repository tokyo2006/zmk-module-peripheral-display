#include "peripheral_output_status.h"
#include "peripheral_icons.h"
#include <lvgl.h>
#include <string.h>

static lv_obj_t *usb_img;
static lv_obj_t *ble_img;

int zmk_widget_peripheral_output_status_init(
    struct zmk_widget_peripheral_output_status *w, lv_obj_t *p)
{
    lv_obj_t *row = lv_obj_create(p);
    lv_obj_set_size(row, 24, 16);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);

    usb_img = lv_img_create(row);
    lv_obj_align(usb_img, LV_ALIGN_LEFT_MID, 0, 0);
    lv_img_set_src(usb_img, &sym_usb);

    ble_img = lv_img_create(row);
    lv_obj_align(ble_img, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_img_set_src(ble_img, &sym_bt);

    lv_obj_align(row, LV_ALIGN_TOP_LEFT, 0, 0);
    w->obj = row;
    return 0;
}

void zmk_widget_peripheral_output_status_update(
    struct zmk_widget_peripheral_output_status *w,
    const struct peripheral_status_adv_data *s)
{
    bool usb = s->status_flags & PERIPHERAL_STATUS_FLAG_USB_CONNECTED;
    bool ble = s->status_flags & PERIPHERAL_STATUS_FLAG_BLE_CONNECTED;
    struct peripheral_output_status_state new_state = {
        .usb_connected = usb,
        .ble_connected = ble,
    };
    if (memcmp(&w->state, &new_state, sizeof(new_state)) == 0) return;
    w->state = new_state;
    /* Dim the inactive transport; keep both icons visible. */
    lv_obj_set_style_img_opa(usb_img, usb ? LV_OPA_COVER : LV_OPA_60, 0);
    lv_obj_set_style_img_opa(ble_img, ble ? LV_OPA_COVER : LV_OPA_60, 0);
}
