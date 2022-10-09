#pragma once

#include "esp_bt_defs.h"

#define BT_PAUSE_BEFORE_RESTART 250  // ms

#define BT_CON_INT_MIN 10
#define BT_CON_INT_MAX 10
#define BT_CON_TIMEOUT 70


extern esp_bd_addr_t rmtbtaddress;
extern esp_bd_addr_t localbtaddress;
extern char btname[];

void strtobtaddr(esp_bd_addr_t dest, char *src);
char *btaddrtostr(char dest[13], esp_bd_addr_t src);
void bt_disable();
void bt_init();
void btSetName(const char *name);
