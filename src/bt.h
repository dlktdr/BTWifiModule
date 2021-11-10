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

void strtobtaddr(esp_bd_addr_t dest, char *src);
char *btaddrtostr(char dest[13], esp_bd_addr_t src);
extern uint8_t bt_scanned_address_cnt;
extern bool bt_connected;
extern esp_bd_addr_t bt_scanned_addresses[MAX_BLE_ADDRESSES];
void bt_connect(esp_bd_addr_t addr);
extern volatile bool bt_scan_complete;
void bt_init();
void bt_disconnect();
void bt_start_scan();
