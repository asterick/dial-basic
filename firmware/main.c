#include <stdint.h>

#include "nrf_drv_clock.h"
#include "app_timer.h"
#include "app_usbd.h"

#include "headphones.h"
#include "keyboard.h"
#include "bluetooth.h"


static void usbd_user_ev_handler(app_usbd_event_type_t event)
{
    switch (event)
    {
        case APP_USBD_EVT_DRV_SOF:
            break;
        case APP_USBD_EVT_DRV_SUSPEND:
            app_usbd_suspend_req(); // Allow the library to put the peripheral into sleep modey
            break;
        case APP_USBD_EVT_DRV_RESUME:
            break;
        case APP_USBD_EVT_STARTED:
            break;
        case APP_USBD_EVT_STOPPED:
            app_usbd_disable();
            break;
        default:
            break;
    }
}

void app_usbd_headphones_init();
void app_usbd_keyboard_init();

static void usb_stack_init()
{
    ret_code_t err_code;

    static const app_usbd_config_t usbd_config = {
        .ev_state_proc = usbd_user_ev_handler,
        .enable_sof = true
    };

    err_code = app_usbd_init(&usbd_config);
    APP_ERROR_CHECK(err_code);

    app_usbd_headphones_init();
    app_usbd_keyboard_init();

    app_usbd_enable();
    app_usbd_start();
}

int main(void)
{
    ret_code_t err_code;

    // Bring up clocks
    err_code = nrf_drv_clock_init();
    APP_ERROR_CHECK(err_code);

    nrf_drv_clock_lfclk_request(NULL);
    while(!nrf_drv_clock_lfclk_is_running()) ;

    // Initialize board
    err_code = app_timer_init();
    APP_ERROR_CHECK(err_code);

    // Initialize Stacks
    usb_stack_init();
    ble_stack_init();

    // Start execution.
    ble_advertising_start();

    // Enter main loop.
    for (;;)
    {
        while (app_usbd_event_queue_process()) ;
        __WFE();
    }
}
