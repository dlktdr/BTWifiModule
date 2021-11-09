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

#define MAX_BLE_ADDRESSES 20

extern uint8_t bt_scanned_address_cnt;
extern char bt_scanned_addresses[MAX_BLE_ADDRESSES][13];
extern volatile bool bt_scan_complete;
void bt_init();
void bt_start_scan();
