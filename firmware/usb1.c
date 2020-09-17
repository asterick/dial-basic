#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>

#include "nordic_common.h"
#include "nrf.h"
#include "nrf_drv_usbd.h"
#include "nrf_drv_clock.h"
#include "app_error.h"
#include "ble.h"
#include "ble_err.h"
#include "ble_hci.h"
#include "ble_srv_common.h"
#include "ble_advdata.h"
#include "ble_conn_params.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "nrf_ble_gatt.h"
#include "app_timer.h"
#include "app_usbd.h"
#include "app_usbd_core.h"
#include "app_usbd_hid_mouse.h"
#include "app_usbd_hid_kbd.h"
#include "app_error.h"

#define APP_USBD_INTERFACE_KBD   0

app_usbd_hid_kbd_t const * kbd1;

static void hid_kbd_user_ev_handler(app_usbd_class_inst_t const * p_inst,
                                    app_usbd_hid_user_event_t event);
APP_USBD_HID_KBD_GLOBAL_DEF(m_app_hid_kbd1,
                            APP_USBD_INTERFACE_KBD,
                            NRF_DRV_USBD_EPIN1,
                            hid_kbd_user_ev_handler,
                            APP_USBD_HID_SUBCLASS_BOOT);

static void hid_kbd_user_ev_handler(app_usbd_class_inst_t const * p_inst,
                                    app_usbd_hid_user_event_t event)
{
    UNUSED_PARAMETER(p_inst);
    switch (event) {
        case APP_USBD_HID_USER_EVT_OUT_REPORT_READY:
            break;
        case APP_USBD_HID_USER_EVT_IN_REPORT_DONE:
            app_usbd_hid_kbd_key_control(&m_app_hid_kbd1, APP_USBD_HID_KBD_A, false);
            break;
        case APP_USBD_HID_USER_EVT_SET_BOOT_PROTO:
            UNUSED_RETURN_VALUE(hid_kbd_clear_buffer(p_inst));
            break;
        case APP_USBD_HID_USER_EVT_SET_REPORT_PROTO:
            UNUSED_RETURN_VALUE(hid_kbd_clear_buffer(p_inst));
            break;
        default:
            break;
    }
}

app_usbd_class_inst_t const *  usb_add_keyboard1() {
    kbd1 = &m_app_hid_kbd1;
    return app_usbd_hid_kbd_class_inst_get(&m_app_hid_kbd1);
}
