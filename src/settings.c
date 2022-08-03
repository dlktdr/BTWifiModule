#include "esp_log.h"
#include "nvs_flash.h"
#include "settings.h"


#define LOG_SET "SETTINGS"

#define NVS_STRUCT_KEY "skey"

void _loadSettings(settings_t *set);

settings_t settings = {
  .mode = ESP_ROOT,
};
extern nvs_handle_t nvs_flsh_btw;
volatile bool settings_ok = false;

// TODO - Add a delay here, so multiple calls don't write twice
void saveSettings()
{
  settings_t flashsettings;
  _loadSettings(&flashsettings);

  if(!memcmp((void*)&flashsettings, (void*)&settings, sizeof(settings_t))) {
    ESP_LOGI(LOG_SET, "No data differs, not writing to flash");
    return;
  } else {
    ESP_LOGI(LOG_SET, "Data is different, FR=%d, MR=%d", flashsettings.mode, settings.mode);
  }

  esp_err_t err = nvs_set_blob(nvs_flsh_btw, NVS_STRUCT_KEY, (void *)&settings, sizeof(settings_t));
  if(err == ESP_OK) {
    ESP_LOGI(LOG_SET, "Blob store success");
  } else {
    ESP_LOGE(LOG_SET,"Unable to store blob - %s", esp_err_to_name(err));
  }
  err = nvs_commit(nvs_flsh_btw);
  if(err == ESP_OK) {
    ESP_LOGI(LOG_SET, "Settings written Successfully");
  } else {
    ESP_LOGE(LOG_SET,"Unable to write settings - %s", esp_err_to_name(err));
 }
}

void _loadSettings(settings_t *s)
{
  ESP_LOGI(LOG_SET,"Reading settings");
  size_t length = sizeof(settings_t);
  esp_err_t ret = nvs_get_blob(nvs_flsh_btw, NVS_STRUCT_KEY, (void*)s, &length);
  if(ret == ESP_OK && length == sizeof(settings_t)) {
    ESP_LOGI(LOG_SET, "Settings Read Successfully");
    return;
  } else {
    if(ret == ESP_OK) {
      ESP_LOGE(LOG_SET,"Incorrect Settings Length");
    } else {
      ESP_LOGE(LOG_SET,"Error (%s) reading\n", esp_err_to_name(ret));
    }
  }
  // Settings invalid, clear the memory
  memset(s, 0, sizeof(settings_t));
}

void loadSettings()
{
  // Load settings into global settings struct.
  _loadSettings(&settings);

  // Will be filled properly here or nulled out
  settings_ok = true;

  // Fill the Radio Specific Settings
  strcpy(espSettings.blemac, settings.blemac);
  strcpy(espSettings.ip, settings.ip);
  strcpy(espSettings.name, settings.name);
  strcpy(espSettings.ssid, settings.ssid);
  strcpy(espSettings.staticip, settings.staticip);
  strcpy(espSettings.subnet, settings.subnet);
  strcpy(espSettings.wifimac, settings.wifimac);
  espSettings.dhcpMode = settings.dhcpMode;
  espSettings.wifiStationMode = settings.wifiStationMode;
}