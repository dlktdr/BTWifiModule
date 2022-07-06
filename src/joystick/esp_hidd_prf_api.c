// Copyright 2017-2018 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "esp_hidd_prf_api.h"
#include "hidd_le_prf_int.h"
#include "hid_dev.h"
#include <stdlib.h>
#include <string.h>
#include "esp_log.h"
#include "bt_joystick.h"

// HID keyboard input report length
#define HID_KEYBOARD_IN_RPT_LEN     8

// HID LED output report length
#define HID_LED_OUT_RPT_LEN         1

// HID mouse input report length
#define HID_MOUSE_IN_RPT_LEN        5

// HID consumer control input report length
#define HID_CC_IN_RPT_LEN           2

// HID gamepad input report length
#define HID_GAMEPAD_IN_RPT_LEN      6

esp_err_t esp_hidd_register_callbacks(esp_hidd_event_cb_t callbacks)
{
    esp_err_t hidd_status;

    if(callbacks != NULL) {
   	    hidd_le_env.hidd_cb = callbacks;
    } else {
        return ESP_FAIL;
    }

    if((hidd_status = hidd_register_cb()) != ESP_OK) {
        return hidd_status;
    }

    esp_ble_gatts_app_register(BATTRAY_APP_ID);

    if((hidd_status = esp_ble_gatts_app_register(HIDD_APP_ID)) != ESP_OK) {
        return hidd_status;
    }

    return hidd_status;
}

esp_err_t esp_hidd_profile_init(void)
{
     if (hidd_le_env.enabled) {
        ESP_LOGE(HID_LE_PRF_TAG, "HID device profile already initialized");
        return ESP_FAIL;
    }
    // Reset the hid device target environment
    memset(&hidd_le_env, 0, sizeof(hidd_le_env_t));
    hidd_le_env.enabled = true;
    return ESP_OK;
}

esp_err_t esp_hidd_profile_deinit(void)
{
    uint16_t hidd_svc_hdl = hidd_le_env.hidd_inst.att_tbl[HIDD_LE_IDX_SVC];
    if (!hidd_le_env.enabled) {
        ESP_LOGE(HID_LE_PRF_TAG, "HID device profile already initialized");
        return ESP_OK;
    }

    if(hidd_svc_hdl != 0) {
	esp_ble_gatts_stop_service(hidd_svc_hdl);
	esp_ble_gatts_delete_service(hidd_svc_hdl);
    } else {
	return ESP_FAIL;
   }

    /* register the HID device profile to the BTA_GATTS module*/
    esp_ble_gatts_app_unregister(hidd_le_env.gatt_if);

    return ESP_OK;
}

uint16_t esp_hidd_get_version(void)
{
	return HIDD_VERSION;
}

// -------------------------------------
// From - Head Tracker Code joystick.cpp

#define JOYSTICK_BUTTONS
#define JOYSTICK_BUTTON_HIGH 1750
#define JOYSTICK_BUTTON_LOW 1250

struct PACKED {
#ifdef JOYSTICK_BUTTONS
    uint8_t but[2];
#endif
    uint8_t channels[16];
} report;

void hid_SendJoystickChannels(uint16_t chans[8])
{
  uint16_t conn_id = btj_conn_id;
  ESP_LOGI("JOY","Sending HID Report ch0 - %d", chans[0]);  
  memcpy(report.channels, chans, sizeof(report.channels));

#ifdef JOYSTICK_BUTTONS
    report.but[0] = 0;
    report.but[1] = 0;
#endif

    for(int i=0; i < 8 ; i++) {
        if(report.channels[i] == 0) // If disabled, center it
            report.channels[i] = 1500;

#ifdef JOYSTICK_BUTTONS
        if(report.channels[i] >= JOYSTICK_BUTTON_HIGH) {
                report.but[0] |= 1<<(i * 2);
                report.but[1] |= 1<<((i - 4) * 2);
        }

        if(report.channels[i] <= JOYSTICK_BUTTON_LOW) {
                report.but[0] |= 1<<((i * 2) + 1);
                report.but[1] |= 1<<(((i - 4) * 2) + 1);
        }
#endif

        report.channels[i] -= 988; // Shift from center so it's 0-1024        
    }

    hid_dev_send_report(hidd_le_env.gatt_if, conn_id,
                        HID_RPT_ID_MOUSE_IN, HID_REPORT_TYPE_INPUT, sizeof(report), (uint8_t*)&report);
}

