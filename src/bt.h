#pragma once

#include "esp_bt_defs.h"

#define BT_PAUSE_BEFORE_RESTART 500 // ms

extern esp_bd_addr_t rmtbtaddress;
extern esp_bd_addr_t localbtaddress;
extern char btname[];

void strtobtaddr(esp_bd_addr_t dest, char *src);
char *btaddrtostr(char dest[13], esp_bd_addr_t src);
void bt_disable();
void BTInit();
void btSetName(const char *name);
