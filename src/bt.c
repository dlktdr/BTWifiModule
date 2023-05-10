/* Common Bluetooth Code
 *
 */

#include "bt.h"

#include <string.h>

#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/task.h"


#define LOG_BT "BT"
#define MAX_BTNAME_LEN 50

esp_bd_addr_t localbtaddress;
esp_bd_addr_t rmtbtaddress;

char btname[MAX_BTNAME_LEN] = "Hello";

void strtobtaddr(esp_bd_addr_t dest, char *src)
{
  for (int i = 0; i < 6; i++) {
    char str[3] = "  ";
    memcpy(str, src, 2);
    src += 2;
    dest[i] = strtoul(str, NULL, 16);
  }
}

char *btaddrtostr(char dest[13], esp_bd_addr_t src)
{
  sprintf(dest, "%02X%02X%02X%02X%02X%02X", src[0], src[1], src[2], src[3], src[4], src[5]);
  dest[12] = '\0';
  return dest;
}

bool memreleased = false;

void bt_init()
{
  esp_err_t ret;

  if (!memreleased) {
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));
    memreleased = true;
  }

  esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
  ret = esp_bt_controller_init(&bt_cfg);
  if (ret) {
    ESP_LOGE(LOG_BT, "%s initialize controller failed: %s\n", __func__, esp_err_to_name(ret));
    return;
  }

  ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
  if (ret) {
    ESP_LOGE(LOG_BT, "%s enable controller failed: %s\n", __func__, esp_err_to_name(ret));
    return;
  }

  ret = esp_bluedroid_init();
  if (ret) {
    ESP_LOGE(LOG_BT, "%s init bluetooth failed: %s\n", __func__, esp_err_to_name(ret));
    return;
  }

  ret = esp_bluedroid_enable();
  if (ret) {
    ESP_LOGE(LOG_BT, "%s enable bluetooth failed: %s\n", __func__, esp_err_to_name(ret));
    return;
  }

  esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_DEFAULT, ESP_PWR_LVL_P9);
}

void bt_disable()
{
  ESP_LOGI(LOG_BT, "Disabling Bluetooth");
  esp_bluedroid_disable();
  esp_bluedroid_deinit();
  esp_bt_controller_disable();
  esp_bt_controller_deinit();

  ESP_LOGI(LOG_BT, "Pausing to shutdown");
  vTaskDelay(pdMS_TO_TICKS(BT_PAUSE_BEFORE_RESTART));
}

void btSetName(const char *name)
{
  strncpy(btname, name, sizeof(btname));
  btname[sizeof(btname) - 1] = '\0';
  ESP_LOGI(LOG_BT, "Setting BT Name %s", name);
}