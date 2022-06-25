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

void esp_hidd_send_consumer_value(uint16_t conn_id, uint8_t key_cmd, bool key_pressed)
{
    uint8_t buffer[HID_CC_IN_RPT_LEN] = {0, 0};
    if (key_pressed) {
        ESP_LOGD(HID_LE_PRF_TAG, "hid_consumer_build_report");
        hid_consumer_build_report(buffer, key_cmd);
    }
    ESP_LOGD(HID_LE_PRF_TAG, "buffer[0] = %x, buffer[1] = %x", buffer[0], buffer[1]);
    hid_dev_send_report(hidd_le_env.gatt_if, conn_id,
                        HID_RPT_ID_CC_IN, HID_REPORT_TYPE_INPUT, HID_CC_IN_RPT_LEN, buffer);
    return;
}

void esp_hidd_send_keyboard_value(uint16_t conn_id, key_mask_t special_key_mask, uint8_t *keyboard_cmd, uint8_t num_key)
{
    if (num_key > HID_KEYBOARD_IN_RPT_LEN - 2) {
        ESP_LOGE(HID_LE_PRF_TAG, "%s(), the number key should not be more than %d", __func__, HID_KEYBOARD_IN_RPT_LEN);
        return;
    }

    uint8_t buffer[HID_KEYBOARD_IN_RPT_LEN] = {0};

    buffer[0] = special_key_mask;

    for (int i = 0; i < num_key; i++) {
        buffer[i+2] = keyboard_cmd[i];
    }

    ESP_LOGD(HID_LE_PRF_TAG, "the key vaule = %d,%d,%d, %d, %d, %d,%d, %d", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6], buffer[7]);
    hid_dev_send_report(hidd_le_env.gatt_if, conn_id,
                        HID_RPT_ID_KEY_IN, HID_REPORT_TYPE_INPUT, HID_KEYBOARD_IN_RPT_LEN, buffer);
    return;
}

void esp_hidd_send_mouse_value(uint16_t conn_id, uint8_t mouse_button, int8_t mickeys_x, int8_t mickeys_y)
{
    uint8_t buffer[HID_MOUSE_IN_RPT_LEN];

    buffer[0] = mouse_button;   // Buttons
    buffer[1] = mickeys_x;           // X
    buffer[2] = mickeys_y;           // Y
    buffer[3] = 0;           // Wheel
    buffer[4] = 0;           // AC Pan

    hid_dev_send_report(hidd_le_env.gatt_if, conn_id,
                        HID_RPT_ID_MOUSE_IN, HID_REPORT_TYPE_INPUT, HID_MOUSE_IN_RPT_LEN, buffer);
    return;
}

// -------------------------------------
// From - Head Tracker Code joystick.cpp

#define JOYSTICK_BUTTONS
#define JOYSTICK_BUTTON_HIGH 1750
#define JOYSTICK_BUTTON_LOW 1250

struct {
#ifdef JOYSTICK_BUTTONS
    uint8_t but[2];
#endif
    uint16_t channels[8];
} report;

void set_JoystickChannels(uint16_t chans[16])
{
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

    hid_int_ep_write(hdev, (uint8_t*)&report, sizeof(report), NULL);
}

static const uint8_t hid_report_desc[] = {
    0x05, 0x01,                    //     USAGE_PAGE (Generic Desktop)
    0x09, 0x05,                    //     USAGE (Game Pad)
    0xa1, 0x01,                    //     COLLECTION (Application)
    0xa1, 0x00,                    //       COLLECTION (Physical)
#ifdef JOYSTICK_BUTTONS
    0x05, 0x09,                    //         USAGE_PAGE (Button)
    0x19, 0x01,                    //         USAGE_MINIMUM (Button 1)
    0x29, 0x10,                    //         USAGE_MAXIMUM (Button 8)
    0x15, 0x00,                    //         LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //         LOGICAL_MAXIMUM (1)
    0x95, 0x10,                    //         REPORT_COUNT (8)
    0x75, 0x01,                    //         REPORT_SIZE (1)
    0x81, 0x02,                    //         INPUT (Data,Var,Abs)
#endif
    0x05, 0x01,                    //         USAGE_PAGE (Generic Desktop)
    0x09, 0x30,                    //         USAGE (X)
    0x09, 0x31,                    //         USAGE (Y)
    0x09, 0x32,                    //         USAGE (Z)
    0x09, 0x33,                    //         USAGE (Rx)
    0x09, 0x34,                    //         USAGE (Ry)
    0x09, 0x35,                    //         USAGE (Rz)
    0x09, 0x36,                    //         USAGE (Slider)
    0x09, 0x36,                    //         USAGE (Slider)
    0x16, 0x00, 0x00,              //         LOGICAL_MINIMUM (0)
    0x26, 0xFF, 0x03,              //         LOGICAL_MAXIMUM (1024)
    0x75, 0x10,                    //         REPORT_SIZE (16)
    0x95, 0x08,                    //         REPORT_COUNT (8)
    0x81, 0x02,                    //         INPUT (Data,Var,Abs)
    0xc0,                          //       END_COLLECTION
    0xc0                           //     END_COLLECTION
};

void esp_hidd_send_joystick_value(uint16_t conn_id, uint16_t joystick_buttons, uint8_t joystick_x, uint8_t joystick_y, uint8_t joystick_z, uint8_t joystick_rx)
{
    uint8_t buffer[HID_GAMEPAD_IN_RPT_LEN];
    ESP_LOGD(HID_LE_PRF_TAG, "the buttons value = %d js1 = %d, %d js2 = %d, %d", joystick_buttons, joystick_x, joystick_y, joystick_z, joystick_rx);

    buffer[0]=joystick_buttons & 0xff;
    buffer[1] = ( joystick_buttons >> 8);
    buffer[2] = ( joystick_x ^ 0x80 );    // X
    buffer[3] = (( joystick_y ^ 0x80 ) * -1) - 1;    // Y
    buffer[4] = ( joystick_z ^ 0x80 );    // X
    buffer[5] = (( joystick_rx ^ 0x80 ) * -1) - 1;   // Y

    hid_dev_send_report(hidd_le_env.gatt_if, conn_id,
                        HID_RPT_ID_MOUSE_IN, HID_REPORT_TYPE_INPUT, HID_GAMEPAD_IN_RPT_LEN, buffer);
    return;
}