/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

/****************************************************************************
 *
 * Slave Mode, Peripheral, Server
 *
 ****************************************************************************/

#include "bt_server.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bt.h"
#include "esp_bt.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"
#include "esp_gatt_common_api.h"
#include "esp_gatts_api.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "sdkconfig.h"

#define GATTS_TAG "BTSERVER"

/// Declare the static function
static void gatts_profile_a_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if,
                                          esp_ble_gatts_cb_param_t *param);

#define GATTS_SERVICE_FRSKY_UUID 0xFFF0
#define GATTS_CHAR_FRSKY_UUID 0xFFF6
#define GATTS_DESCR_UUID_TEST_A 0x3333
#define GATTS_NUM_HANDLE_TEST_A 8

#define GATTS_DEMO_CHAR_VAL_LEN_MAX 0x40

#define PREPARE_BUF_MAX_SIZE 1024

volatile bool btp_connected = false;

static uint8_t char1_str[] = {0x11, 0x22, 0x33};
static esp_gatt_char_prop_t a_property = 0;

static esp_attr_value_t gatts_demo_char1_val = {
    .attr_max_len = GATTS_DEMO_CHAR_VAL_LEN_MAX,
    .attr_len = sizeof(char1_str),
    .attr_value = char1_str,
};

static uint8_t adv_config_done = 0;
#define adv_config_flag (1 << 0)
#define scan_rsp_config_flag (1 << 1)

// FORMAT  <Bytes> <Flag> <Data>
static uint8_t raw_adv_data[] = {
    0x02, 0x01, 0x06,                       // General | NoBREDR
    0x03, 0x02, 0xF0, 0xFF,                 // Available Service
    0x06, 0x09, 'H',  'e',  'l', 'l', 'o',  // Name
    0x02, 0x0A, 0x00                        // TX Power
};

// No Scan Response
/*
static uint8_t raw_scan_rsp_data[] = {
        0x0f, 0x09, 0x45, 0x53, 0x50, 0x5f, 0x47, 0x41, 0x54, 0x54, 0x53, 0x5f, 0x44,
        0x45, 0x4d, 0x4f
};*/

static esp_ble_adv_params_t adv_params = {
    .adv_int_min = 0x20,
    .adv_int_max = 0x40,
    .adv_type = ADV_TYPE_IND,
    .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
    .channel_map = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

#define PROFILE_NUM 1
#define PROFILE_TRAINER_SL_ID 0

struct gatts_profile_inst {
  esp_gatts_cb_t gatts_cb;
  uint16_t gatts_if;
  uint16_t app_id;
  uint16_t conn_id;
  uint16_t service_handle;
  esp_gatt_srvc_id_t service_id;
  uint16_t char_handle;
  esp_bt_uuid_t char_uuid;
  esp_gatt_perm_t perm;
  esp_gatt_char_prop_t property;
  uint16_t descr_handle;
  esp_bt_uuid_t descr_uuid;
};

/* One gatt-based profile one app_id and one gatts_if, this array will store the gatts_if returned
 * by ESP_GATTS_REG_EVT */
static struct gatts_profile_inst gl_profile_tab[PROFILE_NUM] = {
    [PROFILE_TRAINER_SL_ID] = {
        .gatts_cb = gatts_profile_a_event_handler,
        .gatts_if = ESP_GATT_IF_NONE, /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */
    }};

typedef struct {
  uint8_t *prepare_buf;
  int prepare_len;
} prepare_type_env_t;

static prepare_type_env_t a_prepare_write_env;

void example_write_event_env(esp_gatt_if_t gatts_if, prepare_type_env_t *prepare_write_env,
                             esp_ble_gatts_cb_param_t *param);
void example_exec_write_event_env(prepare_type_env_t *prepare_write_env,
                                  esp_ble_gatts_cb_param_t *param);

static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
  switch (event) {
    case ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT:
      adv_config_done &= (~adv_config_flag);
      if (adv_config_done == 0) {
        esp_ble_gap_start_advertising(&adv_params);
      }
      break;
    case ESP_GAP_BLE_SCAN_RSP_DATA_RAW_SET_COMPLETE_EVT:
      adv_config_done &= (~scan_rsp_config_flag);
      if (adv_config_done == 0) {
        esp_ble_gap_start_advertising(&adv_params);
      }
      break;
    case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
      // advertising start complete event to indicate advertising start successfully or failed
      if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
        ESP_LOGE(GATTS_TAG, "Advertising start failed\n");
      }
      break;
    case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
      if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS) {
        ESP_LOGE(GATTS_TAG, "Advertising stop failed\n");
      } else {
        ESP_LOGI(GATTS_TAG, "Stop adv successfully\n");
      }
      break;
    case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
      ESP_LOGI(GATTS_TAG,
               "update connection params status = %d, min_int = %d, max_int = %d,conn_int = "
               "%d,latency = %d, timeout = %d",
               param->update_conn_params.status, param->update_conn_params.min_int,
               param->update_conn_params.max_int, param->update_conn_params.conn_int,
               param->update_conn_params.latency, param->update_conn_params.timeout);
      break;
    default:
      break;
  }
}

void example_write_event_env(esp_gatt_if_t gatts_if, prepare_type_env_t *prepare_write_env,
                             esp_ble_gatts_cb_param_t *param)
{
  esp_gatt_status_t status = ESP_GATT_OK;
  if (param->write.need_rsp) {
    if (param->write.is_prep) {
      if (prepare_write_env->prepare_buf == NULL) {
        prepare_write_env->prepare_buf = (uint8_t *)malloc(PREPARE_BUF_MAX_SIZE * sizeof(uint8_t));
        prepare_write_env->prepare_len = 0;
        if (prepare_write_env->prepare_buf == NULL) {
          ESP_LOGE(GATTS_TAG, "Gatt_server prep no mem\n");
          status = ESP_GATT_NO_RESOURCES;
        }
      } else {
        if (param->write.offset > PREPARE_BUF_MAX_SIZE) {
          status = ESP_GATT_INVALID_OFFSET;
        } else if ((param->write.offset + param->write.len) > PREPARE_BUF_MAX_SIZE) {
          status = ESP_GATT_INVALID_ATTR_LEN;
        }
      }

      esp_gatt_rsp_t *gatt_rsp = (esp_gatt_rsp_t *)malloc(sizeof(esp_gatt_rsp_t));
      gatt_rsp->attr_value.len = param->write.len;
      gatt_rsp->attr_value.handle = param->write.handle;
      gatt_rsp->attr_value.offset = param->write.offset;
      gatt_rsp->attr_value.auth_req = ESP_GATT_AUTH_REQ_NONE;
      memcpy(gatt_rsp->attr_value.value, param->write.value, param->write.len);
      esp_err_t response_err = esp_ble_gatts_send_response(gatts_if, param->write.conn_id,
                                                           param->write.trans_id, status, gatt_rsp);
      if (response_err != ESP_OK) {
        ESP_LOGE(GATTS_TAG, "Send response error\n");
      }
      free(gatt_rsp);
      if (status != ESP_GATT_OK) {
        return;
      }
      memcpy(prepare_write_env->prepare_buf + param->write.offset, param->write.value,
             param->write.len);
      prepare_write_env->prepare_len += param->write.len;

    } else {
      esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, status,
                                  NULL);
    }
  }
}

void example_exec_write_event_env(prepare_type_env_t *prepare_write_env,
                                  esp_ble_gatts_cb_param_t *param)
{
  if (param->exec_write.exec_write_flag == ESP_GATT_PREP_WRITE_EXEC) {
    esp_log_buffer_hex(GATTS_TAG, prepare_write_env->prepare_buf, prepare_write_env->prepare_len);
  } else {
    ESP_LOGI(GATTS_TAG, "ESP_GATT_PREP_WRITE_CANCEL");
  }
  if (prepare_write_env->prepare_buf) {
    free(prepare_write_env->prepare_buf);
    prepare_write_env->prepare_buf = NULL;
  }
  prepare_write_env->prepare_len = 0;
}

static void gatts_profile_a_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if,
                                          esp_ble_gatts_cb_param_t *param)
{
  switch (event) {
    case ESP_GATTS_REG_EVT:
      ESP_LOGI(GATTS_TAG, "REGISTER_APP_EVT, status %d, app_id %d\n", param->reg.status,
               param->reg.app_id);
      gl_profile_tab[PROFILE_TRAINER_SL_ID].service_id.is_primary = true;
      gl_profile_tab[PROFILE_TRAINER_SL_ID].service_id.id.inst_id = 0x00;
      gl_profile_tab[PROFILE_TRAINER_SL_ID].service_id.id.uuid.len = ESP_UUID_LEN_16;
      gl_profile_tab[PROFILE_TRAINER_SL_ID].service_id.id.uuid.uuid.uuid16 =
          GATTS_SERVICE_FRSKY_UUID;

      /*esp_err_t set_dev_name_ret = esp_ble_gap_set_device_name(devicename);
      if (set_dev_name_ret){
          ESP_LOGE(GATTS_TAG, "set device name failed, error code = %x", set_dev_name_ret);
      }*/
      esp_err_t raw_adv_ret = esp_ble_gap_config_adv_data_raw(raw_adv_data, sizeof(raw_adv_data));
      if (raw_adv_ret) {
        ESP_LOGE(GATTS_TAG, "config raw adv data failed, error code = %x ", raw_adv_ret);
      }
      adv_config_done |= adv_config_flag;
      /*esp_err_t raw_scan_ret = esp_ble_gap_config_scan_rsp_data_raw(raw_scan_rsp_data,
      sizeof(raw_scan_rsp_data)); if (raw_scan_ret){ ESP_LOGE(GATTS_TAG, "config raw scan rsp data
      failed, error code = %x", raw_scan_ret);
      }
      adv_config_done |= scan_rsp_config_flag;*/

      esp_ble_gatts_create_service(gatts_if, &gl_profile_tab[PROFILE_TRAINER_SL_ID].service_id,
                                   GATTS_NUM_HANDLE_TEST_A);
      break;
    case ESP_GATTS_READ_EVT: {
      ESP_LOGI(GATTS_TAG, "GATT_READ_EVT, conn_id %d, trans_id %ld, handle %d\n",
               param->read.conn_id, param->read.trans_id, param->read.handle);
      esp_gatt_rsp_t rsp;
      memset(&rsp, 0, sizeof(esp_gatt_rsp_t));
      rsp.attr_value.handle = param->read.handle;
      rsp.attr_value.len = 4;
      rsp.attr_value.value[0] = 0xde;
      rsp.attr_value.value[1] = 0xed;
      rsp.attr_value.value[2] = 0xbe;
      rsp.attr_value.value[3] = 0xef;
      esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id, ESP_GATT_OK,
                                  &rsp);
      break;
    }
    case ESP_GATTS_WRITE_EVT: {
      ESP_LOGI(GATTS_TAG, "GATT_WRITE_EVT, conn_id %d, trans_id %ld, handle %d",
               param->write.conn_id, param->write.trans_id, param->write.handle);
      if (!param->write.is_prep) {
        ESP_LOGI(GATTS_TAG, "GATT_WRITE_EVT, value len %d, value :", param->write.len);
        esp_log_buffer_hex(GATTS_TAG, param->write.value, param->write.len);
        if (gl_profile_tab[PROFILE_TRAINER_SL_ID].descr_handle == param->write.handle &&
            param->write.len == 2) {
          uint16_t descr_value = param->write.value[1] << 8 | param->write.value[0];
          if (descr_value == 0x0001) {
            if (a_property & ESP_GATT_CHAR_PROP_BIT_NOTIFY) {
              ESP_LOGI(GATTS_TAG, "notify enable");
              uint8_t notify_data[15];
              for (int i = 0; i < sizeof(notify_data); ++i) {
                notify_data[i] = i % 0xff;
              }
              // the size of notify_data[] need less than MTU size
              esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id,
                                          gl_profile_tab[PROFILE_TRAINER_SL_ID].char_handle,
                                          sizeof(notify_data), notify_data, false);
            }
          } else if (descr_value == 0x0002) {
            if (a_property & ESP_GATT_CHAR_PROP_BIT_INDICATE) {
              ESP_LOGI(GATTS_TAG, "indicate enable");
              uint8_t indicate_data[15];
              for (int i = 0; i < sizeof(indicate_data); ++i) {
                indicate_data[i] = i % 0xff;
              }
              // the size of indicate_data[] need less than MTU size
              esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id,
                                          gl_profile_tab[PROFILE_TRAINER_SL_ID].char_handle,
                                          sizeof(indicate_data), indicate_data, true);
            }
          } else if (descr_value == 0x0000) {
            ESP_LOGI(GATTS_TAG, "notify/indicate disable ");
          } else {
            ESP_LOGE(GATTS_TAG, "unknown descr value");
            esp_log_buffer_hex(GATTS_TAG, param->write.value, param->write.len);
          }
        }
      }
      example_write_event_env(gatts_if, &a_prepare_write_env, param);
      break;
    }
    case ESP_GATTS_EXEC_WRITE_EVT:
      ESP_LOGI(GATTS_TAG, "ESP_GATTS_EXEC_WRITE_EVT");
      esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id,
                                  ESP_GATT_OK, NULL);
      example_exec_write_event_env(&a_prepare_write_env, param);
      break;
    case ESP_GATTS_MTU_EVT:
      ESP_LOGI(GATTS_TAG, "ESP_GATTS_MTU_EVT, MTU %d", param->mtu.mtu);
      break;
    case ESP_GATTS_UNREG_EVT:
      break;
    case ESP_GATTS_CREATE_EVT:
      ESP_LOGI(GATTS_TAG, "CREATE_SERVICE_EVT, status %d,  service_handle %d\n",
               param->create.status, param->create.service_handle);
      gl_profile_tab[PROFILE_TRAINER_SL_ID].service_handle = param->create.service_handle;
      gl_profile_tab[PROFILE_TRAINER_SL_ID].char_uuid.len = ESP_UUID_LEN_16;
      gl_profile_tab[PROFILE_TRAINER_SL_ID].char_uuid.uuid.uuid16 = GATTS_CHAR_FRSKY_UUID;

      esp_ble_gatts_start_service(gl_profile_tab[PROFILE_TRAINER_SL_ID].service_handle);
      a_property = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE |
                   ESP_GATT_CHAR_PROP_BIT_NOTIFY;
      esp_err_t add_char_ret = esp_ble_gatts_add_char(
          gl_profile_tab[PROFILE_TRAINER_SL_ID].service_handle,
          &gl_profile_tab[PROFILE_TRAINER_SL_ID].char_uuid,
          ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, a_property, &gatts_demo_char1_val, NULL);
      if (add_char_ret) {
        ESP_LOGE(GATTS_TAG, "add char failed, error code =%x", add_char_ret);
      }
      break;
    case ESP_GATTS_ADD_INCL_SRVC_EVT:
      break;
    case ESP_GATTS_ADD_CHAR_EVT: {
      uint16_t length = 0;
      const uint8_t *prf_char;

      ESP_LOGI(GATTS_TAG, "ADD_CHAR_EVT, status %d,  attr_handle %d, service_handle %d\n",
               param->add_char.status, param->add_char.attr_handle, param->add_char.service_handle);
      gl_profile_tab[PROFILE_TRAINER_SL_ID].char_handle = param->add_char.attr_handle;
      gl_profile_tab[PROFILE_TRAINER_SL_ID].descr_uuid.len = ESP_UUID_LEN_16;
      gl_profile_tab[PROFILE_TRAINER_SL_ID].descr_uuid.uuid.uuid16 =
          ESP_GATT_UUID_CHAR_CLIENT_CONFIG;
      esp_err_t get_attr_ret =
          esp_ble_gatts_get_attr_value(param->add_char.attr_handle, &length, &prf_char);
      if (get_attr_ret == ESP_FAIL) {
        ESP_LOGE(GATTS_TAG, "ILLEGAL HANDLE");
      }

      ESP_LOGI(GATTS_TAG, "the gatts demo char length = %x\n", length);
      for (int i = 0; i < length; i++) {
        ESP_LOGI(GATTS_TAG, "prf_char[%x] =%x\n", i, prf_char[i]);
      }
      esp_err_t add_descr_ret =
          esp_ble_gatts_add_char_descr(gl_profile_tab[PROFILE_TRAINER_SL_ID].service_handle,
                                       &gl_profile_tab[PROFILE_TRAINER_SL_ID].descr_uuid,
                                       ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, NULL, NULL);
      if (add_descr_ret) {
        ESP_LOGE(GATTS_TAG, "add char descr failed, error code =%x", add_descr_ret);
      }
      break;
    }
    case ESP_GATTS_ADD_CHAR_DESCR_EVT:
      gl_profile_tab[PROFILE_TRAINER_SL_ID].descr_handle = param->add_char_descr.attr_handle;
      ESP_LOGI(GATTS_TAG, "ADD_DESCR_EVT, status %d, attr_handle %d, service_handle %d\n",
               param->add_char_descr.status, param->add_char_descr.attr_handle,
               param->add_char_descr.service_handle);
      break;
    case ESP_GATTS_DELETE_EVT:
      break;
    case ESP_GATTS_START_EVT:
      ESP_LOGI(GATTS_TAG, "SERVICE_START_EVT, status %d, service_handle %d\n", param->start.status,
               param->start.service_handle);
      break;
    case ESP_GATTS_STOP_EVT:
      break;
    case ESP_GATTS_CONNECT_EVT: {
      esp_ble_conn_update_params_t conn_params = {0};
      memcpy(conn_params.bda, param->connect.remote_bda, sizeof(esp_bd_addr_t));
      /* For the IOS system, please reference the apple official documents about the ble connection
       * parameters restrictions. */
      conn_params.latency = 0;
      conn_params.max_int = BT_CON_INT_MIN;
      conn_params.min_int = BT_CON_INT_MAX;
      conn_params.timeout = BT_CON_TIMEOUT;
      ESP_LOGI(GATTS_TAG,
               "ESP_GATTS_CONNECT_EVT, conn_id %d, remote %02x:%02x:%02x:%02x:%02x:%02x:",
               param->connect.conn_id, param->connect.remote_bda[0], param->connect.remote_bda[1],
               param->connect.remote_bda[2], param->connect.remote_bda[3],
               param->connect.remote_bda[4], param->connect.remote_bda[5]);
      gl_profile_tab[PROFILE_TRAINER_SL_ID].conn_id = param->connect.conn_id;
      // start sent the update connection parameters to the peer device.
      esp_ble_gap_update_conn_params(&conn_params);
      mempcpy(rmtbtaddress, param->connect.remote_bda, sizeof(esp_bd_addr_t));

      btp_connected = true;
      break;
    }
    case ESP_GATTS_DISCONNECT_EVT:
      ESP_LOGI(GATTS_TAG, "ESP_GATTS_DISCONNECT_EVT, disconnect reason 0x%x",
               param->disconnect.reason);
      esp_ble_gap_start_advertising(&adv_params);
      btp_connected = false;
      break;
    case ESP_GATTS_CONF_EVT:
    case ESP_GATTS_OPEN_EVT:
    case ESP_GATTS_CANCEL_OPEN_EVT:
    case ESP_GATTS_CLOSE_EVT:
    case ESP_GATTS_LISTEN_EVT:
    case ESP_GATTS_CONGEST_EVT:
    default:
      break;
  }
}

static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if,
                                esp_ble_gatts_cb_param_t *param)
{
  /* If event is register event, store the gatts_if for each profile */
  if (event == ESP_GATTS_REG_EVT) {
    if (param->reg.status == ESP_GATT_OK) {
      gl_profile_tab[param->reg.app_id].gatts_if = gatts_if;
    } else {
      ESP_LOGI(GATTS_TAG, "Reg app failed, app_id %04x, status %d\n", param->reg.app_id,
               param->reg.status);
      return;
    }
  }

  /* If the gatts_if equal to profile A, call profile A cb handler,
   * so here call each profile's callback */
  do {
    int idx;
    for (idx = 0; idx < PROFILE_NUM; idx++) {
      if (gatts_if == ESP_GATT_IF_NONE || /* ESP_GATT_IF_NONE, not specify a certain gatt_if, need
                                             to call every profile cb function */
          gatts_if == gl_profile_tab[idx].gatts_if) {
        if (gl_profile_tab[idx].gatts_cb) {
          gl_profile_tab[idx].gatts_cb(event, gatts_if, param);
        }
      }
    }
  } while (0);
}

int btp_sendChannelData(uint8_t *data, int len)
{
  if (!btp_connected) return -1;

  if (gl_profile_tab[PROFILE_TRAINER_SL_ID].gatts_if == ESP_GATT_IF_NONE) return -1;

  // the size of notify_data[] need less than MTU size
  esp_ble_gatts_send_indicate(gl_profile_tab[PROFILE_TRAINER_SL_ID].gatts_if,
                              gl_profile_tab[PROFILE_TRAINER_SL_ID].conn_id,
                              gl_profile_tab[PROFILE_TRAINER_SL_ID].char_handle, len, data, false);
  return 0;
}

void btpInit(void)
{
  ESP_LOGI(GATTS_TAG, "Starting Peripherial");

  esp_err_t ret = esp_ble_gatts_register_callback(gatts_event_handler);
  if (ret) {
    ESP_LOGE(GATTS_TAG, "gatts register error, error code = %x", ret);
    return;
  }
  ret = esp_ble_gap_register_callback(gap_event_handler);
  if (ret) {
    ESP_LOGE(GATTS_TAG, "gap register error, error code = %x", ret);
    return;
  }
  ret = esp_ble_gatts_app_register(PROFILE_TRAINER_SL_ID);
  if (ret) {
    ESP_LOGE(GATTS_TAG, "gatts app register error, error code = %x", ret);
    return;
  }

  esp_err_t local_mtu_ret = esp_ble_gatt_set_local_mtu(85);
  if (local_mtu_ret) {
    ESP_LOGE(GATTS_TAG, "set local  MTU failed, error code = %x", local_mtu_ret);
  }

  // Update Local Address
  uint8_t adrtype;
  esp_ble_gap_get_local_used_addr(localbtaddress, &adrtype);
}
