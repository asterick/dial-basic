#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>

#include "nordic_common.h"
#include "nrf.h"
#include "app_timer.h"
#include "app_usbd.h"
#include "app_usbd_hid_mouse.h"

#define APP_USBD_INTERFACE_MOUSE    6

#define CONFIG_MOUSE_BUTTON_COUNT   5

static void hid_mouse_user_ev_handler(app_usbd_class_inst_t const * p_inst,
                                      app_usbd_hid_user_event_t event);

APP_USBD_HID_MOUSE_GLOBAL_DEF(m_app_hid_mouse,
                              APP_USBD_INTERFACE_MOUSE,
                              NRF_DRV_USBD_EPIN7,
                              CONFIG_MOUSE_BUTTON_COUNT,
                              hid_mouse_user_ev_handler,
                              APP_USBD_HID_SUBCLASS_BOOT);


static void hid_mouse_user_ev_handler(app_usbd_class_inst_t const * p_inst,
                                      app_usbd_hid_user_event_t event)
{
    UNUSED_PARAMETER(p_inst);
    switch (event) {
        case APP_USBD_HID_USER_EVT_OUT_REPORT_READY:
            /* No output report defined for HID mouse.*/
            ASSERT(0);
            break;
        case APP_USBD_HID_USER_EVT_IN_REPORT_DONE:
            break;
        case APP_USBD_HID_USER_EVT_SET_BOOT_PROTO:
        case APP_USBD_HID_USER_EVT_SET_REPORT_PROTO:
            hid_mouse_clear_buffer(p_inst);
            break;
        default:
            break;
    }
}

void app_usbd_mouse_init() {
    ret_code_t err_code;
    app_usbd_class_inst_t const * class_inst;

    class_inst = app_usbd_hid_mouse_class_inst_get(&m_app_hid_mouse);
    err_code = app_usbd_class_append(class_inst);
    APP_ERROR_CHECK(err_code);
}
