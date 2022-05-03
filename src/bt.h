#include "esp_bt_defs.h"

extern esp_bd_addr_t rmtbtaddress;

void strtobtaddr(esp_bd_addr_t dest, char *src);
char *btaddrtostr(char dest[13], esp_bd_addr_t src);
void bt_disable();
void bt_init();