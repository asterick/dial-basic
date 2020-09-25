#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>

#include "nordic_common.h"
#include "nrf.h"
#include "app_timer.h"
#include "app_usbd.h"
#include "app_usbd_hid_kbd.h"

#define APP_USBD_INTERFACE_KBD1     0
#define APP_USBD_INTERFACE_KBD2     1
#define APP_USBD_INTERFACE_KBD3     2
#define APP_USBD_INTERFACE_KBD4     3
#define APP_USBD_INTERFACE_KBD5     4
#define APP_USBD_INTERFACE_KBD6     5

static void hid_kbd_user_ev_handler(app_usbd_class_inst_t const * p_inst,
                                    app_usbd_hid_user_event_t event);

APP_TIMER_DEF(m_repeated_timer_id);

APP_USBD_HID_KBD_GLOBAL_DEF(m_app_hid_kbd1,
                            APP_USBD_INTERFACE_KBD1,
                            NRF_DRV_USBD_EPIN1,
                            hid_kbd_user_ev_handler,
                            APP_USBD_HID_SUBCLASS_BOOT);

APP_USBD_HID_KBD_GLOBAL_DEF(m_app_hid_kbd2,
                            APP_USBD_INTERFACE_KBD2,
                            NRF_DRV_USBD_EPIN2,
                            hid_kbd_user_ev_handler,
                            APP_USBD_HID_SUBCLASS_BOOT);

APP_USBD_HID_KBD_GLOBAL_DEF(m_app_hid_kbd3,
                            APP_USBD_INTERFACE_KBD3,
                            NRF_DRV_USBD_EPIN3,
                            hid_kbd_user_ev_handler,
                            APP_USBD_HID_SUBCLASS_BOOT);

APP_USBD_HID_KBD_GLOBAL_DEF(m_app_hid_kbd4,
                            APP_USBD_INTERFACE_KBD4,
                            NRF_DRV_USBD_EPIN4,
                            hid_kbd_user_ev_handler,
                            APP_USBD_HID_SUBCLASS_BOOT);

APP_USBD_HID_KBD_GLOBAL_DEF(m_app_hid_kbd5,
                            APP_USBD_INTERFACE_KBD5,
                            NRF_DRV_USBD_EPIN5,
                            hid_kbd_user_ev_handler,
                            APP_USBD_HID_SUBCLASS_BOOT);

APP_USBD_HID_KBD_GLOBAL_DEF(m_app_hid_kbd6,
                            APP_USBD_INTERFACE_KBD6,
                            NRF_DRV_USBD_EPIN6,
                            hid_kbd_user_ev_handler,
                            APP_USBD_HID_SUBCLASS_BOOT);

static app_usbd_hid_kbd_t const* keyboards[] = {
    &m_app_hid_kbd6,
    &m_app_hid_kbd5,
    &m_app_hid_kbd4,
    &m_app_hid_kbd3,
    &m_app_hid_kbd2,
    &m_app_hid_kbd1,
};

/*
static const uint8_t KEYCODES[] = {
    0x04, 0x05, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19,
    0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23,
    0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d,
    0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
    0x38, 0x3a, 0x3b, 0x3c, 0x3d, 0x3f, 0x40, 0x41, 0x44, 0x45,
    0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 0x50, 0x51, 0x52,
    0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d,
    0x5e, 0x5f, 0x60, 0x61, 0x62, 0x63, 0x64, 0x66, 0x67, 0x68,
    0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 0x70, 0x71, 0x72,
    0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x7b, 0x7c,
    0x7d, 0x7e, 0x7f, 0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86,
    0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f, 0x90,
    0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a,
    0x9b, 0x9c, 0x9d, 0x9e, 0x9f, 0xa0, 0xa1, 0xa2, 0xa3, 0xa4,
    0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae,
    0xaf, 0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8,
    0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf, 0xc0, 0xc1, 0xc2,
    0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb, 0xcc,
    0xcd, 0xce, 0xcf, 0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6,
    0xd7, 0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf, 0xe8,
    0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef, 0xf0, 0xf1, 0xf2,
    0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc,
    0xfd, 0xfe, 0xff, 0x00
};
*/

static void repeated_timer_handler(void * p_context)
{
    /*
    static bool toggle = true;

    for (int i = 0; i < 6; i++) {
        app_usbd_hid_access_lock(&keyboards[i]->specific.p_data->ctx.hid_ctx);
    }

    m_app_hid_kbd1.specific.p_data->ctx.rep.modifier = (not_zero << 1) | (toggle ? 1 : 0);
    for (int i = 0; i < 6; i++) {
        app_usbd_hid_kbd_ctx_t * p_kbd_ctx = &keyboards[i]->specific.p_data->ctx;
        
        for (int k = 0; k < 6; k++)
            p_kbd_ctx->rep.key_table[k] = toggle ? KEYCODES[i+k*6] : 0;
    }

    for (int i = 0; i < 6; i++) {
        app_usbd_hid_access_unlock(&keyboards[i]->specific.p_data->ctx.hid_ctx);
    }
   
    for (int i = 0; i < 6; i++) {
        if (app_usbd_hid_trans_required(&keyboards[i]->specific.p_data->ctx.hid_ctx))
        {
            hid_kbd_transfer_set(keyboards[i]);
        }
    }

    toggle = !toggle;
    */
}

static void hid_kbd_user_ev_handler(app_usbd_class_inst_t const * p_inst,
                                    app_usbd_hid_user_event_t event)
{
    UNUSED_PARAMETER(p_inst);
    switch (event) {
        case APP_USBD_HID_USER_EVT_OUT_REPORT_READY:
            break;
        case APP_USBD_HID_USER_EVT_IN_REPORT_DONE:
            break;
        case APP_USBD_HID_USER_EVT_SET_BOOT_PROTO:
        case APP_USBD_HID_USER_EVT_SET_REPORT_PROTO:
            hid_kbd_clear_buffer(p_inst);
            break;
        default:
            break;
    }
}

void app_usbd_keyboard_init() {
    ret_code_t err_code;
    app_usbd_class_inst_t const * class_inst;
    
    for (int i = 0; i < 6; i++) {
        class_inst = app_usbd_hid_kbd_class_inst_get(keyboards[i]);
        err_code = app_usbd_class_append(class_inst);
        APP_ERROR_CHECK(err_code);
    }

    err_code = app_timer_create(&m_repeated_timer_id,
                                APP_TIMER_MODE_REPEATED,
                                repeated_timer_handler);

    app_timer_start(m_repeated_timer_id, APP_TIMER_TICKS(17), NULL);
}
