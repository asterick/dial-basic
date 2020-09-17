#include <math.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>

#include "nordic_common.h"
#include "nrf.h"
#include "nrf_drv_usbd.h"
#include "nrf_drv_clock.h"
#include "app_util.h"
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
#include "app_usbd_audio.h"
#include "app_usbd_hid_kbd.h"
#include "app_error.h"

#define DEVICE_NAME                     "DialBasic"                             /**< Name of device. Will be included in the advertising data. */

#define APP_BLE_OBSERVER_PRIO           3                                       /**< Application's BLE observer priority. You shouldn't need to modify this value. */
#define APP_BLE_CONN_CFG_TAG            1                                       /**< A tag identifying the SoftDevice BLE configuration. */

#define APP_ADV_INTERVAL                64                                      /**< The advertising interval (in units of 0.625 ms; this value corresponds to 40 ms). */
#define APP_ADV_DURATION                BLE_GAP_ADV_TIMEOUT_GENERAL_UNLIMITED   /**< The advertising time-out (in units of seconds). When set to 0, we will never time out. */

#define MIN_CONN_INTERVAL               MSEC_TO_UNITS( 16, UNIT_1_25_MS)        /**< Minimum acceptable connection interval (20ms). */
#define MAX_CONN_INTERVAL               MSEC_TO_UNITS(200, UNIT_1_25_MS)        /**< Maximum acceptable connection interval (250ms). */
#define SLAVE_LATENCY                   0                                       /**< Slave latency. */
#define CONN_SUP_TIMEOUT                MSEC_TO_UNITS(4000, UNIT_10_MS)         /**< Connection supervisory time-out (4 seconds). */

#define FIRST_CONN_PARAMS_UPDATE_DELAY  APP_TIMER_TICKS(20000)                  /**< Time from initiating event (connect or start of notification) to first time sd_ble_gap_conn_param_update is called (15 seconds). */
#define NEXT_CONN_PARAMS_UPDATE_DELAY   APP_TIMER_TICKS(5000)                   /**< Time between each call to sd_ble_gap_conn_param_update after the first call (5 seconds). */
#define MAX_CONN_PARAMS_UPDATE_COUNT    3                                       /**< Number of attempts before giving up the connection parameter negotiation. */

#define DEAD_BEEF                       0xDEADBEEF                              /**< Value used as error code on stack dump, can be used to identify stack location on stack unwind. */

#define SERIAL_UUID_BASE        {0x68, 0x3f, 0x2c, 0x08, 0xf8, 0x71, 0x11, 0xea, \
                                 0xa1, 0x3a, 0x70, 0x85, 0xc2, 0xcc, 0x42, 0x5a}
#define SERIAL_UUID_SERVICE     0xBA5C
#define SERIAL_UUID_READ_CHAR   0xBA5D
#define SERIAL_UUID_WRITE_CHAR  0xBA5E
#define BLE_SERIAL_BLE_OBSERVER_PRIO 2

uint8_t VERSION_NUMBER[] = {0x20, 0x20, 0x09, 0x16};

/**@brief Serial service structure. This structure contains various status information for the service. */
typedef struct ble_serial_s
{
    uint16_t                   service_handle;      /**< Handle of serial service (as provided by the BLE stack). */
    ble_gatts_char_handles_t   write_char_handles;  /**< Handles related to the write Characteristic. */
    ble_gatts_char_handles_t   read_char_handles;   /**< Handles related to the read Characteristic. */
    uint8_t                    uuid_type;           /**< UUID type for the serial service. */
} ble_serial_t;

static ble_serial_t m_serial;                                                                             
NRF_BLE_GATT_DEF(m_gatt);                                                       /**< GATT module instance. */

static uint16_t m_conn_handle = BLE_CONN_HANDLE_INVALID;                        /**< Handle of the current connection. */

static uint8_t m_adv_handle = BLE_GAP_ADV_SET_HANDLE_NOT_SET;                   /**< Advertising handle used to identify an advertising set. */
static uint8_t m_enc_advdata[BLE_GAP_ADV_SET_DATA_SIZE_MAX];                    /**< Buffer for storing an encoded advertising set. */
static uint8_t m_enc_scan_response_data[BLE_GAP_ADV_SET_DATA_SIZE_MAX];         /**< Buffer for storing an encoded scan data. */

/* USB Definitions */
#define APP_USBD_INTERFACE_KBD1     0
#define APP_USBD_INTERFACE_KBD2     1
#define APP_USBD_INTERFACE_KBD3     2
#define APP_USBD_INTERFACE_KBD4     3
#define APP_USBD_INTERFACE_KBD5     4
#define APP_USBD_INTERFACE_KBD6     5
#define HP_INTERFACES_CONFIG()      APP_USBD_AUDIO_CONFIG_OUT(6, 7)

static void hid_kbd_user_ev_handler(app_usbd_class_inst_t const * p_inst,
                                    app_usbd_hid_user_event_t event);
static void hp_audio_user_ev_handler(app_usbd_class_inst_t const * p_inst,
                                     app_usbd_audio_user_event_t   event);

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

/**@brief Struct that contains pointers to the encoded advertising data. */
static ble_gap_adv_data_t m_adv_data =
{
    .adv_data =
    {
        .p_data = m_enc_advdata,
        .len    = BLE_GAP_ADV_SET_DATA_SIZE_MAX
    },
    .scan_rsp_data =
    {
        .p_data = m_enc_scan_response_data,
        .len    = BLE_GAP_ADV_SET_DATA_SIZE_MAX

    }
};

/**@brief Function for assert macro callback.
 *
 * @details This function will be called in case of an assert in the SoftDevice.
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyze
 *          how your product is supposed to react in case of Assert.
 * @warning On assert from the SoftDevice, the system can only recover on reset.
 *
 * @param[in] line_num    Line number of the failing ASSERT call.
 * @param[in] p_file_name File name of the failing ASSERT call.
 */
void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name)
{
    app_error_handler(DEAD_BEEF, line_num, p_file_name);
}

/**@brief Function for handling the Connection Parameters Module.
 *
 * @details This function will be called for all events in the Connection Parameters Module that
 *          are passed to the application.
 *
 * @note All this function does is to disconnect. This could have been done by simply
 *       setting the disconnect_on_fail config parameter, but instead we use the event
 *       handler mechanism to demonstrate its use.
 *
 * @param[in] p_evt  Event received from the Connection Parameters Module.
 */
static void on_conn_params_evt(ble_conn_params_evt_t * p_evt)
{
    ret_code_t err_code;

    if (p_evt->evt_type == BLE_CONN_PARAMS_EVT_FAILED)
    {
        err_code = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_CONN_INTERVAL_UNACCEPTABLE);
        APP_ERROR_CHECK(err_code);
    }
}


/**@brief Function for handling a Connection Parameters error.
 *
 * @param[in] nrf_error  Error code containing information about what went wrong.
 */
static void conn_params_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}

/**@brief Function for starting advertising.
 */
static void advertising_start(void)
{
    ret_code_t           err_code;

    err_code = sd_ble_gap_adv_start(m_adv_handle, APP_BLE_CONN_CFG_TAG);
    APP_ERROR_CHECK(err_code);
}

static void ble_serial_on_ble_evt(ble_evt_t const * p_ble_evt, void * p_context)
{
    ble_gatts_evt_write_t const * p_evt_write = &p_ble_evt->evt.gatts_evt.params.write;

    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GATTS_EVT_WRITE:
            if (   (p_evt_write->handle == m_serial.write_char_handles.value_handle)
                && (p_evt_write->len >= 1))
            {
                // TODO: HANDLE WRITE HERE
            }
            break;

        default:
            // No implementation needed.
            break;
    }
}

uint32_t ble_serial_write(uint16_t conn_handle, uint8_t *data, uint16_t len)
{
    ble_gatts_hvx_params_t params;

    memset(&params, 0, sizeof(params));
    params.type   = BLE_GATT_HVX_NOTIFICATION;
    params.handle = m_serial.read_char_handles.value_handle;
    params.p_data = data;
    params.p_len  = &len;

    return sd_ble_gatts_hvx(conn_handle, &params);
}

/**@brief Function for handling BLE events.
 *
 * @param[in]   p_ble_evt   Bluetooth stack event.
 * @param[in]   p_context   Unused.
 */
static void ble_evt_handler(ble_evt_t const * p_ble_evt, void * p_context)
{
    ret_code_t err_code;

    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
            m_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
            break;

        case BLE_GAP_EVT_DISCONNECTED:
            m_conn_handle = BLE_CONN_HANDLE_INVALID;
            advertising_start();
            break;

        case BLE_GAP_EVT_SEC_PARAMS_REQUEST:
            // Pairing not supported
            err_code = sd_ble_gap_sec_params_reply(m_conn_handle,
                                                   BLE_GAP_SEC_STATUS_PAIRING_NOT_SUPP,
                                                   NULL,
                                                   NULL);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GAP_EVT_PHY_UPDATE_REQUEST:
        {
            ble_gap_phys_t const phys =
            {
                .rx_phys = BLE_GAP_PHY_AUTO,
                .tx_phys = BLE_GAP_PHY_AUTO,
            };
            err_code = sd_ble_gap_phy_update(p_ble_evt->evt.gap_evt.conn_handle, &phys);
            APP_ERROR_CHECK(err_code);
        } break;

        case BLE_GATTS_EVT_SYS_ATTR_MISSING:
            // No system attributes have been stored.
            err_code = sd_ble_gatts_sys_attr_set(m_conn_handle, NULL, 0, 0);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GATTC_EVT_TIMEOUT:
            // Disconnect on GATT Client timeout event.
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gattc_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GATTS_EVT_TIMEOUT:
            // Disconnect on GATT Server timeout event.
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gatts_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break;

        default:
            // No implementation needed.
            break;
    }
}

static void ble_serial_init()
{
    uint32_t              err_code;
    ble_uuid_t            ble_uuid;
    ble_add_char_params_t add_char_params;

    // Add service.
    ble_uuid128_t base_uuid = {SERIAL_UUID_BASE};
    err_code = sd_ble_uuid_vs_add(&base_uuid, &m_serial.uuid_type);
    APP_ERROR_CHECK(err_code);

    ble_uuid.type = m_serial.uuid_type;
    ble_uuid.uuid = SERIAL_UUID_SERVICE;

    err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &ble_uuid, &m_serial.service_handle);
    APP_ERROR_CHECK(err_code);

    // Add read characteristic.
    memset(&add_char_params, 0, sizeof(add_char_params));
    add_char_params.uuid              = SERIAL_UUID_READ_CHAR;
    add_char_params.uuid_type         = m_serial.uuid_type;
    add_char_params.init_len          = sizeof(uint8_t);
    add_char_params.max_len           = sizeof(uint8_t);
    add_char_params.char_props.read   = 1;
    add_char_params.char_props.notify = 1;

    add_char_params.read_access       = SEC_OPEN;
    add_char_params.cccd_write_access = SEC_OPEN;

    err_code = characteristic_add(m_serial.service_handle,
                                  &add_char_params,
                                  &m_serial.read_char_handles);
    APP_ERROR_CHECK(err_code);

    // Add write characteristic.
    memset(&add_char_params, 0, sizeof(add_char_params));
    add_char_params.uuid             = SERIAL_UUID_WRITE_CHAR;
    add_char_params.uuid_type        = m_serial.uuid_type;
    add_char_params.init_len         = sizeof(uint8_t);
    add_char_params.max_len          = sizeof(uint8_t);
    add_char_params.char_props.read  = 1;
    add_char_params.char_props.write = 1;

    add_char_params.read_access  = SEC_OPEN;
    add_char_params.write_access = SEC_OPEN;

    err_code = characteristic_add(m_serial.service_handle, &add_char_params, &m_serial.write_char_handles);
    APP_ERROR_CHECK(err_code);
}

static void ble_stack_init(void)
{
    static const ble_gap_conn_params_t gap_conn_params = {
        .min_conn_interval = MIN_CONN_INTERVAL,
        .max_conn_interval = MAX_CONN_INTERVAL,
        .slave_latency     = SLAVE_LATENCY,
        .conn_sup_timeout  = CONN_SUP_TIMEOUT,
    };

    ble_gap_conn_sec_mode_t sec_mode;
    ret_code_t err_code;

    err_code = nrf_sdh_enable_request();
    APP_ERROR_CHECK(err_code);

    // Configure the BLE stack using the default settings.
    // Fetch the start address of the application RAM.
    uint32_t ram_start = 0;
    err_code = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);
    APP_ERROR_CHECK(err_code);

    // Enable BLE stack.
    err_code = nrf_sdh_ble_enable(&ram_start);
    APP_ERROR_CHECK(err_code);

    // Register a handler for BLE events.
    NRF_SDH_BLE_OBSERVER(m_ble_observer, APP_BLE_OBSERVER_PRIO, ble_evt_handler, NULL);
    NRF_SDH_BLE_OBSERVER(m_serial_observer, BLE_SERIAL_BLE_OBSERVER_PRIO, ble_serial_on_ble_evt, NULL);

    // init gap parameters
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

    err_code = sd_ble_gap_device_name_set(&sec_mode,
                                          (const uint8_t *)DEVICE_NAME,
                                          strlen(DEVICE_NAME));
    APP_ERROR_CHECK(err_code);

    err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
    APP_ERROR_CHECK(err_code);

    // Gatt init
    err_code = nrf_ble_gatt_init(&m_gatt, NULL);
    APP_ERROR_CHECK(err_code);

    // Initialize BLE serial.
    ble_serial_init();

    // Setup advertising
    ble_uuid_t adv_uuids[] = {{SERIAL_UUID_SERVICE, m_serial.uuid_type}};
    
    ble_advdata_manuf_data_t manfudata = {
        .company_identifier = 0xDEAD,
        .data = {
            .size = sizeof(VERSION_NUMBER),
            .p_data = VERSION_NUMBER,
        }
    };

    const ble_advdata_t advdata = {
        .name_type          = BLE_ADVDATA_FULL_NAME,
        .include_appearance = true,
        .p_manuf_specific_data = &manfudata,
        .flags              = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE,
    };

    const ble_advdata_t srdata = {
        .uuids_complete = {
            .uuid_cnt = sizeof(adv_uuids) / sizeof(adv_uuids[0]),
            .p_uuids  = adv_uuids,
        },
    };

    // Build and set advertising data.
    err_code = ble_advdata_encode(&advdata, m_adv_data.adv_data.p_data, &m_adv_data.adv_data.len);
    APP_ERROR_CHECK(err_code);

    err_code = ble_advdata_encode(&srdata, m_adv_data.scan_rsp_data.p_data, &m_adv_data.scan_rsp_data.len);
    APP_ERROR_CHECK(err_code);

    static const ble_gap_adv_params_t adv_params = {
        .primary_phy     = BLE_GAP_PHY_1MBPS,
        .duration        = APP_ADV_DURATION,
        .properties.type = BLE_GAP_ADV_TYPE_CONNECTABLE_SCANNABLE_UNDIRECTED,
        .p_peer_addr     = NULL,
        .filter_policy   = BLE_GAP_ADV_FP_ANY,
        .interval        = APP_ADV_INTERVAL,
    };

    // Set advertising parameters.

    err_code = sd_ble_gap_adv_set_configure(&m_adv_handle, &m_adv_data, &adv_params);
    APP_ERROR_CHECK(err_code);

    // Set connection parameters
    static const ble_conn_params_init_t cp_init = {
        .p_conn_params                  = NULL,
        .first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY,
        .next_conn_params_update_delay  = NEXT_CONN_PARAMS_UPDATE_DELAY,
        .max_conn_params_update_count   = MAX_CONN_PARAMS_UPDATE_COUNT,
        .start_on_notify_cccd_handle    = BLE_GATT_HANDLE_INVALID,
        .disconnect_on_fail             = false,
        .evt_handler                    = on_conn_params_evt,
        .error_handler                  = conn_params_error_handler,
    };

    err_code = ble_conn_params_init(&cp_init);
    APP_ERROR_CHECK(err_code);
}

APP_TIMER_DEF(m_repeated_timer_id);

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

static void repeated_timer_handler(void * p_context)
{
    static bool toggle = true;

    for (int i = 0; i < 6; i++) {
        app_usbd_hid_access_lock(&keyboards[i]->specific.p_data->ctx.hid_ctx);
    }

    m_app_hid_kbd1.specific.p_data->ctx.rep.modifier = toggle ? 0xFF : 0;
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
            /*New transfer need to be triggered*/
            hid_kbd_transfer_set(keyboards[i]);
        }
    }

    toggle = !toggle;
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
            UNUSED_RETURN_VALUE(hid_kbd_clear_buffer(p_inst));
            break;
        case APP_USBD_HID_USER_EVT_SET_REPORT_PROTO:
            UNUSED_RETURN_VALUE(hid_kbd_clear_buffer(p_inst));
            break;
        default:
            break;
    }
}

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

static void usb_stack_init()
{
    ret_code_t err_code;

    static const app_usbd_config_t usbd_config = {
        .ev_state_proc = usbd_user_ev_handler,
        .enable_sof = true
    };

    err_code = app_usbd_init(&usbd_config);
    APP_ERROR_CHECK(err_code);

    app_usbd_class_inst_t const * class_inst;

    class_inst = app_usbd_audio_class_inst_get(&m_app_audio_headphone);
    err_code = app_usbd_audio_sof_interrupt_register(class_inst, hp_sof_ev_handler);
    APP_ERROR_CHECK(err_code);
    err_code = app_usbd_class_append(class_inst);
    APP_ERROR_CHECK(err_code);

    for (int i = 0; i < 6; i++) {
        class_inst = app_usbd_hid_kbd_class_inst_get(keyboards[i]);
        err_code = app_usbd_class_append(class_inst);
        APP_ERROR_CHECK(err_code);
    }
    
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
    advertising_start();

    err_code = app_timer_create(&m_repeated_timer_id,
                                APP_TIMER_MODE_REPEATED,
                                repeated_timer_handler);

    app_timer_start(m_repeated_timer_id, APP_TIMER_TICKS(8*3), NULL);

    // Enter main loop.
    for (;;)
    {
        while (app_usbd_event_queue_process()) ;
        __WFE();
    }
}
