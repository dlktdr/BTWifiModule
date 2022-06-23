#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"

#include "esp_bt.h"
#include "bt_app_core.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_gap_bt_api.h"
#include "esp_a2dp_api.h"
#include "esp_avrc_api.h"


/* log tags */
#define BT_AV_TAG             "BT_AV"
#define BT_RC_CT_TAG          "RC_CT"

/* device name */
#define TARGET_DEVICE_NAME    "ESP_SPEAKER"
#define LOCAL_DEVICE_NAME     "ESP_A2DP_SRC"

/* AVRCP used transaction label */
#define APP_RC_CT_TL_GET_CAPS            (0)
#define APP_RC_CT_TL_RN_VOLUME_CHANGE    (1)

enum {
    BT_APP_STACK_UP_EVT   = 0x0000,    /* event for stack up */
    BT_APP_HEART_BEAT_EVT = 0xff00,    /* event for heart beat */
};

/* A2DP global states */
enum {
    APP_AV_STATE_IDLE,
    APP_AV_STATE_DISCOVERING,
    APP_AV_STATE_DISCOVERED,
    APP_AV_STATE_UNCONNECTED,
    APP_AV_STATE_CONNECTING,
    APP_AV_STATE_CONNECTED,
    APP_AV_STATE_DISCONNECTING,
};

/* sub states of APP_AV_STATE_CONNECTED */
enum {
    APP_AV_MEDIA_STATE_IDLE,
    APP_AV_MEDIA_STATE_STARTING,
    APP_AV_MEDIA_STATE_STARTED,
    APP_AV_MEDIA_STATE_STOPPING,
};

/*********************************
 * STATIC FUNCTION DECLARATIONS
 ********************************/

void bta2dpInit();

/* handler for bluetooth stack enabled events */
 void bt_av_hdl_stack_evt(uint16_t event, void *p_param);

/* avrc controller event handler */
 void bt_av_hdl_avrc_ct_evt(uint16_t event, void *p_param);

/* GAP callback function */
 void bt_app_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param);

/* callback function for A2DP source */
 void bt_app_a2d_cb(esp_a2d_cb_event_t event, esp_a2d_cb_param_t *param);

/* callback function for A2DP source audio data stream */
 int32_t bt_app_a2d_data_cb(uint8_t *data, int32_t len);

/* callback function for AVRCP controller */
 void bt_app_rc_ct_cb(esp_avrc_ct_cb_event_t event, esp_avrc_ct_cb_param_t *param);

/* handler for heart beat timer */
 void bt_app_a2d_heart_beat(TimerHandle_t arg);

/* A2DP application state machine */
 void bt_app_av_sm_hdlr(uint16_t event, void *param);

/* utils for transfer BLuetooth Deveice Address into string form */
 char *bda2str(esp_bd_addr_t bda, char *str, size_t size);

/* A2DP application state machine handler for each state */
void bt_app_av_state_unconnected_hdlr(uint16_t event, void *param);
void bt_app_av_state_connecting_hdlr(uint16_t event, void *param);
void bt_app_av_state_connected_hdlr(uint16_t event, void *param);
void bt_app_av_state_disconnecting_hdlr(uint16_t event, void *param);