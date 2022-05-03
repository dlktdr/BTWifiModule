/* Common Bluetooth Code
 *
 */

#include <string.h>

#include "esp_err.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_log.h"
#include "bt.h"

esp_bd_addr_t rmtbtaddress;

void strtobtaddr(esp_bd_addr_t dest, char *src)
{
  for(int i=0; i < 6; i++) {
    char str[3] = "  ";
    memcpy(str, src, 2);
    src += 2;
    dest[i] = strtoul(str,NULL, 16);
  }
}

char *btaddrtostr(char dest[13], esp_bd_addr_t src)
{
  sprintf(dest, "%02X%02X%02X%02X%02X%02X\r\n",
          src[0],
          src[1],
          src[2],
          src[3],
          src[4],
          src[5]
      );
  dest[12] = '\0';
  return dest;
}

void bt_init()
{
  esp_err_t ret;
  ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

  esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
  ret = esp_bt_controller_init(&bt_cfg);
  if (ret) {
    ESP_LOGE("BLE", "%s initialize controller failed: %s\n", __func__, esp_err_to_name(ret));
    return;
  }
}

void bt_disable()
{
  esp_bt_controller_disable();
  esp_bluedroid_disable();
  esp_bluedroid_deinit();

}
