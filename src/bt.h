#pragma once

#include "esp_bt_defs.h"

extern esp_bd_addr_t rmtbtaddress;
extern esp_bd_addr_t localbtaddress;
extern char btname[];

void strtobtaddr(esp_bd_addr_t dest, char *src);
char *btaddrtostr(char dest[13], esp_bd_addr_t src);
void bt_disable();
void bt_init();
void btSetName(const char *name);
