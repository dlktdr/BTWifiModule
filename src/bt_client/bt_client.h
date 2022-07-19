#pragma once

/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "esp_bt_defs.h"

#define MAX_BLE_ADDRESSES 20
#define MIN_BLE_RSSI -85
#define MAX_CHAR_TO_SCAN 15

typedef enum {
   BLE_BOARD_UNKNOWN,
   BLE_BOARD_FRSKY_CC2540,
   BLE_BOARD_FRSKY_PARA,
   BLE_BOARD_HEADTRACKER,
   BLE_BOARD_FLYSKY,
   BLE_BOARD_HM10,
   BLE_BOARD_COUNT
} ble_board_type;

extern char *str_ble_board_types[BLE_BOARD_COUNT];
extern uint8_t bt_scanned_address_cnt;

extern esp_bd_addr_t btc_scanned_addresses[MAX_BLE_ADDRESSES];
void btc_connect(esp_bd_addr_t addr);
extern volatile bool btc_connected;
extern volatile bool btc_scan_complete;
extern volatile bool btc_validslavefound;
extern volatile bool btc_ht_reset;
extern volatile ble_board_type btc_board_type;
void btcInit();
void btc_disconnect();
void btc_start_scan();
void btc_dohtreset();
