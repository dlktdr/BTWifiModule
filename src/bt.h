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
   BLE_BOARD_COUNT
} ble_board_type;

extern char *str_ble_board_types[BLE_BOARD_COUNT];

void strtobtaddr(esp_bd_addr_t dest, char *src);
char *btaddrtostr(char dest[13], esp_bd_addr_t src);
extern uint8_t bt_scanned_address_cnt;

extern esp_bd_addr_t bt_scanned_addresses[MAX_BLE_ADDRESSES];
void bt_connect(esp_bd_addr_t addr);
extern volatile bool bt_connected;
extern volatile bool bt_scan_complete;
extern volatile bool bt_validslavefound;
extern volatile bool ht_reset;
extern volatile ble_board_type bt_board_type;
void bt_init();
void bt_disconnect();
void bt_start_scan();
void bt_dohtreset();
