#pragma once

#if defined(USE_NIMBLE)
#include <stdint.h>
typedef uint8_t esp_bd_addr_t[6];
#else
#include "esp_bt_defs.h"
#endif

#define BT_PAUSE_BEFORE_RESTART 250  // ms

#define BT_CON_INT_MIN 10
#define BT_CON_INT_MAX 10
#define BT_CON_TIMEOUT 70


extern esp_bd_addr_t rmtbtaddress;
extern esp_bd_addr_t localbtaddress;

void strtobtaddr(esp_bd_addr_t dest, char *src);
char *btaddrtostr(char dest[13], esp_bd_addr_t src);
void bt_disable();
void bt_init();
void btSetName(const char *name);

#if defined(USE_NIMBLE)
/* NimBLE host task (runs nimble_port_run), shared by server and client. */
void bt_host_task(void *param);
/* Reverses a 6 byte address between NimBLE wire order and display order. */
void btReverseAddr(const uint8_t *src, uint8_t *dst);
#endif
