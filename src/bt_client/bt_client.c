/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/



/****************************************************************************
*
* This demo showcases BLE GATT client. It can scan BLE devices and connect to one device.
* Run the gatt_server demo, the client demo will automatically connect to the gatt_server demo.
* Client demo will enable gatt_server's notify after connection. The two devices will then exchange
* data.
*
****************************************************************************/

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gattc_api.h"
#include "esp_gatt_defs.h"
#include "esp_bt_main.h"
#include "esp_timer.h"
#include "esp_gatt_common_api.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "settings.h"

#include "bt.h"
#include "bt_client.h"
#include "frskybt.h"
#include "terminal.h"
#include "defines.h"

#include "esptrainer.h"

#define GATTC_TAG "BTCLIENT"
#define REMOTE_SERVICE_UUID        0xFFF0
#define REMOTE_FRSKY_CHAR_UUID     0xFFF6
#define REMOTE_HTRESET_CHAR_UUID   0xAFF2
#define PROFILE_NUM      1
#define PROFILE_A_APP_ID 0
#define INVALID_HANDLE   0

static bool get_server = false;
static esp_gattc_char_elem_t *char_elem_result   = NULL;
static esp_gattc_descr_elem_t *descr_elem_result = NULL;

static bool readytoscan = false;
uint8_t bt_scanned_address_cnt = 0;
volatile bool btc_validslavefound = false;
volatile bool btc_connected = false;
volatile bool btc_scan_complete = true;
volatile ble_board_type btc_board_type = BLE_BOARD_UNKNOWN;
volatile bool btc_ht_reset = false;
uint16_t bt_datahandle;
uint16_t bt_htresethandle;
esp_bd_addr_t rmtbtaddress;

esp_bd_addr_t btc_scanned_addresses[MAX_BLE_ADDRESSES];

/* Declare static functions */
static void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);
static void esp_gattc_cb(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param);
static void gattc_profile_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param);


static esp_bt_uuid_t remote_filter_service_uuid = {
    .len = ESP_UUID_LEN_16,
    .uuid = {.uuid16 = REMOTE_SERVICE_UUID,},
};

static esp_bt_uuid_t notify_descr_uuid = {
    .len = ESP_UUID_LEN_16,
    .uuid = {.uuid16 = ESP_GATT_UUID_CHAR_CLIENT_CONFIG,},
};

static esp_ble_scan_params_t ble_scan_params = {
    .scan_type              = BLE_SCAN_TYPE_ACTIVE,
    .own_addr_type          = BLE_ADDR_TYPE_PUBLIC,
    .scan_filter_policy     = BLE_SCAN_FILTER_ALLOW_ALL,
    .scan_interval          = 0x50,
    .scan_window            = 0x30,
    .scan_duplicate         = BLE_SCAN_DUPLICATE_ENABLE
};

struct gattc_profile_inst {
    esp_gattc_cb_t gattc_cb;
    uint16_t gattc_if;
    uint16_t app_id;
    uint16_t conn_id;
    uint16_t service_start_handle;
    uint16_t service_end_handle;
    uint16_t char_handle;
    esp_bd_addr_t remote_bda;
};

/* One gatt-based profile one app_id and one gattc_if, this array will store the gattc_if returned by ESP_GATTS_REG_EVT */
static struct gattc_profile_inst gl_profile_tab[PROFILE_NUM] = {
    [PROFILE_A_APP_ID] = {
        .gattc_cb = gattc_profile_event_handler,
        .gattc_if = ESP_GATT_IF_NONE,       /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */
    },
};

/* Common Bluetooth Code
 *
 */

#include <string.h>

#include "esp_err.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_log.h"
#include "bt.h"

void gattc_update_connection_params(esp_bd_addr_t *remote_bda)
{
    esp_ble_conn_update_params_t conn_params;
    memcpy(conn_params.bda, remote_bda, sizeof(esp_bd_addr_t));
    conn_params.min_int = 0x0A; // x 1.25ms
    conn_params.max_int = 0x0A; // x 1.25ms
    conn_params.latency = 0x00; //number of skippable connection events
    conn_params.timeout = 70; // x 6.25ms, time before peripheral will assume connection is dropped.

    esp_ble_gap_update_conn_params(&conn_params);
}

static void gattc_profile_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param)
{
    esp_ble_gattc_cb_param_t *p_data = (esp_ble_gattc_cb_param_t *)param;

    switch (event) {
    case ESP_GATTC_REG_EVT: {
        ESP_LOGI(GATTC_TAG, "REG_EVT");
        esp_err_t scan_ret = esp_ble_gap_set_scan_params(&ble_scan_params);
        if (scan_ret){
            ESP_LOGE(GATTC_TAG, "set scan params error, error code = %x", scan_ret);
        }
        break;
    }
    case ESP_GATTC_CONNECT_EVT:{
        btc_connected = true;
        esp_ble_conn_update_params_t conn_params = {0};
        memcpy(conn_params.bda, param->connect.remote_bda, sizeof(esp_bd_addr_t));
        ESP_LOGI(GATTC_TAG, "ESP_GATTC_CONNECT_EVT conn_id %d, if %d", p_data->connect.conn_id, gattc_if);
        gl_profile_tab[PROFILE_A_APP_ID].conn_id = p_data->connect.conn_id;
        memcpy(gl_profile_tab[PROFILE_A_APP_ID].remote_bda, p_data->connect.remote_bda, sizeof(esp_bd_addr_t));
        ESP_LOGI(GATTC_TAG, "REMOTE BDA:");
        esp_log_buffer_hex(GATTC_TAG, gl_profile_tab[PROFILE_A_APP_ID].remote_bda, sizeof(esp_bd_addr_t));
        esp_err_t mtu_ret = esp_ble_gattc_send_mtu_req (gattc_if, p_data->connect.conn_id);
        if (mtu_ret){
            ESP_LOGE(GATTC_TAG, "config MTU error, error code = %x", mtu_ret);
        }
        /*esp_ble_gap_set_prefered_phy(param->connect.remote_bda,
                                     ESP_BLE_GAP_NO_PREFER_TRANSMIT_PHY|ESP_BLE_GAP_NO_PREFER_RECEIVE_PHY,
                                     ESP_BLE_GAP_PHY_CODED_PREF_MASK,
                                     ESP_BLE_GAP_PHY_CODED_PREF_MASK,
                                     ESP_BLE_GAP_PHY_OPTIONS_PREF_S8_CODING);*/
        btc_scan_complete = false;
        ESP_LOGI(GATTC_TAG, "Starting Service Scan");
        esp_ble_gattc_search_service(gattc_if, param->cfg_mtu.conn_id, &remote_filter_service_uuid);
        gattc_update_connection_params(&param->connect.remote_bda);

        break;
    }
    case ESP_GATTC_OPEN_EVT: {
        if (param->open.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG, "open failed, status %d", p_data->open.status);
            break;
        }
        ESP_LOGI(GATTC_TAG, "open success");
        break;
    }
    case ESP_GATTC_DIS_SRVC_CMPL_EVT: {
        if (param->dis_srvc_cmpl.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG, "discover service failed, status %d", param->dis_srvc_cmpl.status);
            break;
        }
        ESP_LOGI(GATTC_TAG, "discover service complete conn_id %d", param->dis_srvc_cmpl.conn_id);
        esp_ble_gattc_search_service(gattc_if, param->cfg_mtu.conn_id, &remote_filter_service_uuid);
        break;
    }
    case ESP_GATTC_CFG_MTU_EVT: {
        if (param->cfg_mtu.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG,"config mtu failed, error status = %x", param->cfg_mtu.status);
        }
        ESP_LOGI(GATTC_TAG, "ESP_GATTC_CFG_MTU_EVT, Status %d, MTU %d, conn_id %d", param->cfg_mtu.status, param->cfg_mtu.mtu, param->cfg_mtu.conn_id);
        break;
    }
    case ESP_GATTC_SEARCH_RES_EVT: {
        ESP_LOGI(GATTC_TAG, "SEARCH RES: conn_id = %x is primary service %d", p_data->search_res.conn_id, p_data->search_res.is_primary);
        ESP_LOGI(GATTC_TAG, "start handle %d end handle %d current handle value %d", p_data->search_res.start_handle, p_data->search_res.end_handle, p_data->search_res.srvc_id.inst_id);
        if (p_data->search_res.srvc_id.uuid.len == ESP_UUID_LEN_16 && p_data->search_res.srvc_id.uuid.uuid.uuid16 == REMOTE_SERVICE_UUID) {
            ESP_LOGI(GATTC_TAG, "service found");
            get_server = true;
            gl_profile_tab[PROFILE_A_APP_ID].service_start_handle = p_data->search_res.start_handle;
            gl_profile_tab[PROFILE_A_APP_ID].service_end_handle = p_data->search_res.end_handle;
            ESP_LOGI(GATTC_TAG, "UUID16: %x", p_data->search_res.srvc_id.uuid.uuid.uuid16);
        }
        break;
    }
    case ESP_GATTC_SEARCH_CMPL_EVT: {
        if (p_data->search_cmpl.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG, "search service failed, error status = %x", p_data->search_cmpl.status);
            break;
        }
        if(p_data->search_cmpl.searched_service_source == ESP_GATT_SERVICE_FROM_REMOTE_DEVICE) {
            ESP_LOGI(GATTC_TAG, "Get service information from remote device");
        } else if (p_data->search_cmpl.searched_service_source == ESP_GATT_SERVICE_FROM_NVS_FLASH) {
            ESP_LOGI(GATTC_TAG, "Get service information from flash");
        } else {
            ESP_LOGI(GATTC_TAG, "unknown service source");
        }
        ESP_LOGI(GATTC_TAG, "ESP_GATTC_SEARCH_CMPL_EVT");
        if (get_server){
            uint16_t count = 0;
            esp_gatt_status_t status = esp_ble_gattc_get_attr_count( gattc_if,
                                                                     p_data->search_cmpl.conn_id,
                                                                     ESP_GATT_DB_CHARACTERISTIC,
                                                                     gl_profile_tab[PROFILE_A_APP_ID].service_start_handle,
                                                                     gl_profile_tab[PROFILE_A_APP_ID].service_end_handle,
                                                                     INVALID_HANDLE,
                                                                     &count);
            if (status != ESP_GATT_OK){
                ESP_LOGE(GATTC_TAG, "esp_ble_gattc_get_attr_count error");
            }

            ESP_LOGI(GATTC_TAG, "Attr Count %d", count);

            if (count > 0){
                char_elem_result = (esp_gattc_char_elem_t *)malloc(sizeof(esp_gattc_char_elem_t) * MAX_CHAR_TO_SCAN);
                if (!char_elem_result){
                    ESP_LOGE(GATTC_TAG, "gattc no mem");
                } else {
                    count = MAX_CHAR_TO_SCAN;
                    status = esp_ble_gattc_get_all_char ( gattc_if,
                                                          p_data->search_cmpl.conn_id,
                                                          gl_profile_tab[PROFILE_A_APP_ID].service_start_handle,
                                                          gl_profile_tab[PROFILE_A_APP_ID].service_end_handle,
                                                          char_elem_result,
                                                          &count,
                                                          0 );
                    if (status != ESP_GATT_OK){
                        ESP_LOGE(GATTC_TAG, "esp_ble_gattc_get_all_char error");
                    }

                    btc_validslavefound = false;
                    btc_board_type = BLE_BOARD_UNKNOWN;
                    for(int i=0; i < count; i++) {
                        if(char_elem_result[i].uuid.uuid.uuid16 == 0xFFF6) {
                            btc_validslavefound = true;
                            bt_datahandle = char_elem_result[i].char_handle;
                            ESP_LOGI(GATTC_TAG, "Found the Trainer Characteristic");
                            if(char_elem_result[i].properties & ESP_GATT_CHAR_PROP_BIT_NOTIFY) {
                                // Subscribing to the Notify
                                ESP_LOGI(GATTC_TAG, "Subscribing for Notifications");
                                gl_profile_tab[PROFILE_A_APP_ID].char_handle = char_elem_result[i].char_handle;
                                esp_ble_gattc_register_for_notify (gattc_if, gl_profile_tab[PROFILE_A_APP_ID].remote_bda, char_elem_result[i].char_handle);
                            }

                            // Update Flash to Save Last Connected BT Address
                            btaddrtostr(settings.rmtbtaddr, gl_profile_tab[PROFILE_A_APP_ID].remote_bda);
                            saveSettings();

                        } else if (char_elem_result[i].uuid.uuid.uuid16 == 0xAFF2) {
                            ESP_LOGI(GATTC_TAG, "Found the reset characteristic. This is a headtracker board");
                            btc_board_type = BLE_BOARD_HEADTRACKER;
                            bt_htresethandle = char_elem_result[i].char_handle;


                        } else if (char_elem_result[i].uuid.uuid.uuid16 == 0xAFF1) {
                            ESP_LOGI(GATTC_TAG, "Found the valid channels. This is a headtracker board");
                            btc_board_type = BLE_BOARD_HEADTRACKER;
                        }
                    }
                    btc_scan_complete = true;
                }
                /* free char_elem_result */
                free(char_elem_result);
            }else{
                ESP_LOGE(GATTC_TAG, "no char found");
            }
        }
         break;
    }
    case ESP_GATTC_REG_FOR_NOTIFY_EVT: {
        ESP_LOGI(GATTC_TAG, "ESP_GATTC_REG_FOR_NOTIFY_EVT");
        if (p_data->reg_for_notify.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG, "REG FOR NOTIFY failed: error status = %d", p_data->reg_for_notify.status);
        }else{
            uint16_t count = 0;
            uint16_t notify_en = 1;
            esp_gatt_status_t ret_status = esp_ble_gattc_get_attr_count( gattc_if,
                                                                         gl_profile_tab[PROFILE_A_APP_ID].conn_id,
                                                                         ESP_GATT_DB_DESCRIPTOR,
                                                                         gl_profile_tab[PROFILE_A_APP_ID].service_start_handle,
                                                                         gl_profile_tab[PROFILE_A_APP_ID].service_end_handle,
                                                                         gl_profile_tab[PROFILE_A_APP_ID].char_handle,
                                                                         &count);
            if (ret_status != ESP_GATT_OK){
                ESP_LOGE(GATTC_TAG, "esp_ble_gattc_get_attr_count error");
            }
            if (count > 0){
                descr_elem_result = malloc(sizeof(esp_gattc_descr_elem_t) * count);
                if (!descr_elem_result){
                    ESP_LOGE(GATTC_TAG, "malloc error, gattc no mem");
                }else{
                    ret_status = esp_ble_gattc_get_descr_by_char_handle( gattc_if,
                                                                         gl_profile_tab[PROFILE_A_APP_ID].conn_id,
                                                                         p_data->reg_for_notify.handle,
                                                                         notify_descr_uuid,
                                                                         descr_elem_result,
                                                                         &count);
                    if (ret_status != ESP_GATT_OK){
                        ESP_LOGE(GATTC_TAG, "esp_ble_gattc_get_descr_by_char_handle error");
                    }
                    /* Every char has only one descriptor in our 'ESP_GATTS_DEMO' demo, so we used first 'descr_elem_result' */
                    if (count > 0 && descr_elem_result[0].uuid.len == ESP_UUID_LEN_16 && descr_elem_result[0].uuid.uuid.uuid16 == ESP_GATT_UUID_CHAR_CLIENT_CONFIG){
                        ret_status = (esp_gatt_status_t)esp_ble_gattc_write_char_descr( gattc_if,
                                                                     gl_profile_tab[PROFILE_A_APP_ID].conn_id,
                                                                     descr_elem_result[0].handle,
                                                                     sizeof(notify_en),
                                                                     (uint8_t *)&notify_en,
                                                                     ESP_GATT_WRITE_TYPE_RSP,
                                                                     ESP_GATT_AUTH_REQ_NONE);
                    }

                    if (ret_status != ESP_GATT_OK){
                        ESP_LOGE(GATTC_TAG, "esp_ble_gattc_write_char_descr error");
                    }

                    /* free descr_elem_result */
                    free(descr_elem_result);
                }
            }
            else{
                ESP_LOGE(GATTC_TAG, "decsr not found");
            }

        }
        break;
    }
    case ESP_GATTC_NOTIFY_EVT: {
        if (p_data->notify.is_notify) {
          // TODO, verify what characteristic is being notified
#ifdef DEBUG_TIMERS
          processFrame(p_data->notify.value,p_data->notify.value_len); // Used to decode the channel data for debugging
#endif

          // TODO.. fixme
          //   espTrainerRFDataReceived()
          // uart_write_bytes(uart_num, (void*)p_data->notify.value, p_data->notify.value_len); // Write the received data to the UART port

           // if(p_data->notify.handle == bt_datahandle) // If notify coming from the data handle, send it to the UART port
          //  else
          //    ESP_LOGI(GATTC_TAG, "ESP_GATTC_NOTIFY_EVT, receive unknown notify value:");
        } else {
            ESP_LOGI(GATTC_TAG, "ESP_GATTC_NOTIFY_EVT, receive indicate value:");
        }

        break;
    }
    case ESP_GATTC_WRITE_DESCR_EVT: {
        if (p_data->write.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG, "write descr failed, error status = %x", p_data->write.status);
            break;
        }
        ESP_LOGI(GATTC_TAG, "write descr success ");
        uint8_t write_char_data[35];
        for (int i = 0; i < sizeof(write_char_data); ++i)
        {
            write_char_data[i] = i % 256;
        }
        esp_ble_gattc_write_char( gattc_if,
                                  gl_profile_tab[PROFILE_A_APP_ID].conn_id,
                                  gl_profile_tab[PROFILE_A_APP_ID].char_handle,
                                  sizeof(write_char_data),
                                  write_char_data,
                                  ESP_GATT_WRITE_TYPE_RSP,
                                  ESP_GATT_AUTH_REQ_NONE);
        break;
    }
    case ESP_GATTC_SRVC_CHG_EVT: {
        esp_bd_addr_t bda;
        memcpy(bda, p_data->srvc_chg.remote_bda, sizeof(esp_bd_addr_t));
        ESP_LOGI(GATTC_TAG, "ESP_GATTC_SRVC_CHG_EVT, bd_addr:");
        esp_log_buffer_hex(GATTC_TAG, bda, sizeof(esp_bd_addr_t));
        break;
    }
    case ESP_GATTC_WRITE_CHAR_EVT: {
        if (p_data->write.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG, "write char failed, error status = %x", p_data->write.status);
            break;
        }
        ESP_LOGI(GATTC_TAG, "write char success ");
        break;
    }
    case ESP_GATTC_DISCONNECT_EVT: {
        btc_connected = false;
        get_server = false;
        ESP_LOGI(GATTC_TAG, "ESP_GATTC_DISCONNECT_EVT, reason = %d", p_data->disconnect.reason);
        break;
    }
    default: {
        break;
    }
    }
}

static void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event) {
        case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT: {
            readytoscan = true;
            break;
        }
        case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT: {
            //scan start complete event to indicate scan start successfully or failed
            if (param->scan_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
                ESP_LOGE(GATTC_TAG, "scan start failed, error status = %x", param->scan_start_cmpl.status);
                break;
            }
            ESP_LOGI(GATTC_TAG, "scan start success");

            break;
        }
        case ESP_GAP_BLE_SCAN_RESULT_EVT: {
            esp_ble_gap_cb_param_t *scan_result = (esp_ble_gap_cb_param_t *)param;
            switch (scan_result->scan_rst.search_evt) {
            case ESP_GAP_SEARCH_INQ_RES_EVT: {
                // Add address to the list if it doesn't already exist
                if(bt_scanned_address_cnt < MAX_BLE_ADDRESSES && scan_result->scan_rst.rssi > MIN_BLE_RSSI) {
                    bool found=false;
                    for(int i=0; i < bt_scanned_address_cnt; i++) {
                        if(memcmp(btc_scanned_addresses[i], scan_result->scan_rst.bda, sizeof(esp_bd_addr_t)) == 0)  {
                            found = true;
                            break;
                        }
                    }
                    if(!found) {
                        memcpy(btc_scanned_addresses[bt_scanned_address_cnt++],
                        scan_result->scan_rst.bda, sizeof(esp_bd_addr_t));
                    }
                }
                char addr[13];
                btaddrtostr(addr, scan_result->scan_rst.bda);
                printf("Disc BT Address %s, RSSI=%d, Addr Type=%d\n",addr, scan_result->scan_rst.rssi,
                scan_result->scan_rst.ble_addr_type);
                espTrainerDeviceDiscovered(addr);
                break;
            }
            case ESP_GAP_SEARCH_INQ_CMPL_EVT: {
                espTrainerDiscoverComplete();
                break;
            }
            default: {
                break;
            }
            }
            break;
        }

        case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT: {
            if (param->scan_stop_cmpl.status != ESP_BT_STATUS_SUCCESS){
                ESP_LOGE(GATTC_TAG, "scan stop failed, error status = %x", param->scan_stop_cmpl.status);
                break;
            }
            ESP_LOGI(GATTC_TAG, "stop scan successfully");
            break;
        }

        case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT: {
            if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS){
                ESP_LOGE(GATTC_TAG, "adv stop failed, error status = %x", param->adv_stop_cmpl.status);
                break;
            }
            ESP_LOGI(GATTC_TAG, "stop adv successfully");
            break;
        }
        case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT: {
            ESP_LOGI(GATTC_TAG, "update connection params status = %d, min_int = %d, max_int = %d,conn_int = %d,latency = %d, timeout = %d",
                    param->update_conn_params.status,
                    param->update_conn_params.min_int,
                    param->update_conn_params.max_int,
                    param->update_conn_params.conn_int,
                    param->update_conn_params.latency,
                    param->update_conn_params.timeout);
            break;
        }
        default: {
            break;
        }
    }
}

static void esp_gattc_cb(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param)
{
    /* If event is register event, store the gattc_if for each profile */
    if (event == ESP_GATTC_REG_EVT) {
        if (param->reg.status == ESP_GATT_OK) {
            gl_profile_tab[param->reg.app_id].gattc_if = gattc_if;
        } else {
            ESP_LOGI(GATTC_TAG, "reg app failed, app_id %04x, status %d",
                    param->reg.app_id,
                    param->reg.status);
            return;
        }
    }

    /* If the gattc_if equal to profile A, call profile A cb handler,
     * so here call each profile's callback */
    do {
        int idx;
        for (idx = 0; idx < PROFILE_NUM; idx++) {
            if (gattc_if == ESP_GATT_IF_NONE || /* ESP_GATT_IF_NONE, not specify a certain gatt_if, need to call every profile cb function */
                    gattc_if == gl_profile_tab[idx].gattc_if) {
                if (gl_profile_tab[idx].gattc_cb) {
                    gl_profile_tab[idx].gattc_cb(event, gattc_if, param);
                }
            }
        }
    } while (0);
}

void btc_dohtreset()
{
    if(btc_connected && btc_scan_complete && btc_board_type == BLE_BOARD_HEADTRACKER) {
        esp_err_t status;
        status = esp_ble_gattc_write_char(gl_profile_tab[PROFILE_A_APP_ID].gattc_if,
                                            gl_profile_tab[PROFILE_A_APP_ID].conn_id,
                                            bt_htresethandle,
                                            1,
                                            (uint8_t*)"R",
                                            ESP_GATT_WRITE_TYPE_NO_RSP,
                                            ESP_GATT_AUTH_REQ_NONE);
        if(status != ESP_OK) {
            ESP_LOGE(GATTC_TAG, "Error Writing to Characteristic");
        }
    }
}

void btc_start_scan()
{
    if(!readytoscan) {
        return;
    }
    btc_scan_complete = false;
    bt_scanned_address_cnt = 0;
    printf("Clearing Addresses\r\n");
    uint32_t duration = 1;
    esp_ble_gap_start_scanning(duration);
}

void btc_connect(esp_bd_addr_t addr)
{
  if(btc_connected) return;
  btc_scan_complete = false;
  btc_validslavefound = false;
  char saddr[13];
  memcpy(rmtbtaddress, addr, sizeof(esp_bd_addr_t));
  printf("Connecting to %s\r\n", btaddrtostr(saddr,addr)); // TODO FIX ME
  esp_ble_gattc_open(gl_profile_tab[PROFILE_A_APP_ID].gattc_if, addr, BLE_ADDR_TYPE_RANDOM, true);
}

void btc_disconnect()
{
  if(btc_connected)
    esp_ble_gattc_close(gl_profile_tab[PROFILE_A_APP_ID].gattc_if,
                        gl_profile_tab[PROFILE_A_APP_ID].conn_id);
  btc_connected = false;
  btc_scan_complete = false;
  btc_validslavefound = false;
}

void btcInit()
{
  ESP_LOGI(GATTC_TAG, "Starting Central");

  //register the  callback function to the gap module
  esp_err_t ret = esp_ble_gap_register_callback(esp_gap_cb);
  if (ret){
      ESP_LOGE(GATTC_TAG, "%s gap register failed, error code = %x\n", __func__, ret);
      return;
  }

  //register the callback function to the gattc module
  ret = esp_ble_gattc_register_callback(esp_gattc_cb);
  if(ret){
      ESP_LOGE(GATTC_TAG, "%s gattc register failed, error code = %x\n", __func__, ret);
      return;
  }

  ret = esp_ble_gattc_app_register(PROFILE_A_APP_ID);
  if (ret){
      ESP_LOGE(GATTC_TAG, "%s gattc app register failed, error code = %x\n", __func__, ret);
  }
  esp_err_t local_mtu_ret = esp_ble_gatt_set_local_mtu(65);
  if (local_mtu_ret){
      ESP_LOGE(GATTC_TAG, "set local  MTU failed, error code = %x", local_mtu_ret);
  }

  // Update Local Address
  uint8_t adrtype;
  esp_ble_gap_get_local_used_addr(localbtaddress, &adrtype);

  vTaskDelay(pdMS_TO_TICKS(500));

  /*
  // Try to connect to saved address on startup.. // Leave me up to the radio
  esp_bd_addr_t addr;
  strtobtaddr(addr, settings.rmtbtaddr);*/
  //btc_connect(addr);
}