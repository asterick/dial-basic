#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>

#include "nordic_common.h"
#include "nrf.h"
#include "app_usbd_core.h"
#include "app_usbd_audio.h"

/* USB Definitions */
#define HP_INTERFACES_CONFIG()      APP_USBD_AUDIO_CONFIG_OUT(7, 8)

static void hp_audio_user_ev_handler(app_usbd_class_inst_t const * p_inst,
                                     app_usbd_audio_user_event_t   event);

#define BUFFER_SIZE (48)

#define HP_TERMINAL_CH_CONFIG()                                                                       \
        (APP_USBD_AUDIO_IN_TERM_CH_CONFIG_LEFT_FRONT | APP_USBD_AUDIO_IN_TERM_CH_CONFIG_RIGHT_FRONT)

#define HP_FEATURE_CONTROLS()                                               \
        APP_USBD_U16_TO_RAW_DSC(APP_USBD_AUDIO_FEATURE_UNIT_CONTROL_MUTE),  \
        APP_USBD_U16_TO_RAW_DSC(APP_USBD_AUDIO_FEATURE_UNIT_CONTROL_MUTE),  \
        APP_USBD_U16_TO_RAW_DSC(APP_USBD_AUDIO_FEATURE_UNIT_CONTROL_MUTE)

APP_USBD_AUDIO_FORMAT_DESCRIPTOR(m_hp_form_desc, 
                                 APP_USBD_AUDIO_AS_FORMAT_III_DSC(    /* Format type 3 descriptor */
                                    2,                                /* Number of channels */
                                    2,                                /* Subframe size */
                                    16,                               /* Bit resolution */
                                    1,                                /* Frequency type */
                                    APP_USBD_U24_TO_RAW_DSC(48000))   /* Frequency */
                                );

APP_USBD_AUDIO_INPUT_DESCRIPTOR(m_hp_inp_desc, 
                                APP_USBD_AUDIO_INPUT_TERMINAL_DSC(
                                    1,                                     /* Terminal ID */
                                    APP_USBD_AUDIO_TERMINAL_USB_STREAMING, /* Terminal type */
                                    2,                                     /* Number of channels */
                                    HP_TERMINAL_CH_CONFIG())               /* Channels config */
                               );

APP_USBD_AUDIO_OUTPUT_DESCRIPTOR(m_hp_out_desc, 
                                 APP_USBD_AUDIO_OUTPUT_TERMINAL_DSC(
                                    3,                                      /* Terminal ID */
                                    APP_USBD_AUDIO_TERMINAL_OUT_HEADPHONES, /* Terminal type */
                                    2)                                      /* Source ID */
                                );

APP_USBD_AUDIO_FEATURE_DESCRIPTOR(m_hp_fea_desc, 
                                  APP_USBD_AUDIO_FEATURE_UNIT_DSC(
                                    2,                     /* Unit ID */
                                    1,                     /* Source ID */
                                    HP_FEATURE_CONTROLS()) /* List of controls */
                                 );

/* Interfaces lists */
APP_USBD_AUDIO_GLOBAL_DEF(m_app_audio_headphone,
                          HP_INTERFACES_CONFIG(),
                          hp_audio_user_ev_handler,
                          &m_hp_form_desc,
                          &m_hp_inp_desc,
                          &m_hp_out_desc,
                          &m_hp_fea_desc,
                          0,
                          APP_USBD_AUDIO_AS_IFACE_FORMAT_PCM,
                          192,
                          APP_USBD_AUDIO_SUBCLASS_AUDIOSTREAMING,
                          1);

static uint8_t m_mute_hp;
static uint32_t m_freq_hp;

static void hp_audio_user_class_req(app_usbd_class_inst_t const * p_inst)
{
    app_usbd_audio_t const * p_audio = app_usbd_audio_class_get(p_inst);
    app_usbd_audio_req_t * p_req = app_usbd_audio_class_request_get(p_audio);

    UNUSED_VARIABLE(m_mute_hp);
    UNUSED_VARIABLE(m_freq_hp);

    switch (p_req->req_target)
    {
        case APP_USBD_AUDIO_CLASS_REQ_IN:

            if (p_req->req_type == APP_USBD_AUDIO_REQ_SET_CUR)
            {
                //Only mute control is defined
                p_req->payload[0] = m_mute_hp;
            }

            break;
        case APP_USBD_AUDIO_CLASS_REQ_OUT:

            if (p_req->req_type == APP_USBD_AUDIO_REQ_SET_CUR)
            {
                //Only mute control is defined
                m_mute_hp = p_req->payload[0];
            }

            break;
        case APP_USBD_AUDIO_EP_REQ_IN:
            break;
        case APP_USBD_AUDIO_EP_REQ_OUT:

            if (p_req->req_type == APP_USBD_AUDIO_REQ_SET_CUR)
            {
                //Only set frequency is supported
                m_freq_hp = uint24_decode(p_req->payload);
            }

            break;
        default:
            break;
    }
}

static int16_t  m_temp_buffer[2 * BUFFER_SIZE];
uint8_t   not_zero = 0;

static void hp_sof_ev_handler(uint16_t framecnt)
{
    UNUSED_VARIABLE(framecnt);
    if (APP_USBD_STATE_Configured != app_usbd_core_state_get())
    {
        return;
    }
    size_t rx_size = app_usbd_audio_class_rx_size_get(&m_app_audio_headphone.base);
    
    if (rx_size > 0)
    {
        ASSERT(rx_size <= sizeof(m_temp_buffer));

        ret_code_t err_code;
        err_code = app_usbd_audio_class_rx_start(&m_app_audio_headphone.base, m_temp_buffer, rx_size);
        APP_ERROR_CHECK(err_code);

        for (int i = 0; i < BUFFER_SIZE*2; i++) {
            if (m_temp_buffer[i] != 0) not_zero++;
        }
    }
}

static void hp_audio_user_ev_handler(app_usbd_class_inst_t const * p_inst,
                                     app_usbd_audio_user_event_t   event)
{
    app_usbd_audio_t const * p_audio = app_usbd_audio_class_get(p_inst);
    UNUSED_VARIABLE(p_audio);
    switch (event)
    {
        case APP_USBD_AUDIO_USER_EVT_CLASS_REQ:
            hp_audio_user_class_req(p_inst);
            break;
        case APP_USBD_AUDIO_USER_EVT_RX_DONE:
            break;
        default:
            break;
    }
}

void app_usbd_headphones_init()
{
    ret_code_t err_code;

    app_usbd_class_inst_t const * class_inst = app_usbd_audio_class_inst_get(&m_app_audio_headphone);
    err_code = app_usbd_audio_sof_interrupt_register(class_inst, hp_sof_ev_handler);
    APP_ERROR_CHECK(err_code);
    err_code = app_usbd_class_append(class_inst);
    APP_ERROR_CHECK(err_code);
}
