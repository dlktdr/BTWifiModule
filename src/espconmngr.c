#include "espconmngr.h"
#include "esp_log.h"
#include "espdefs.h"
#include "terminal.h"
#include <stdio.h>

#define CON_MGR_TAG "CONMGR"

void connectionCommandRX(int conCmd, const uint8_t *data, int len) {
  switch (conCmd) {
  case ESP_CON_DISCOVER_START:
    ESP_LOGI(CON_MGR_TAG, "Discovery Start Req");
    break;
  case ESP_CON_DISCOVER_STOP:
    ESP_LOGI(CON_MGR_TAG, "Discovery Stop Req");
    break;
  case ESP_CON_CONNECT:
    ESP_LOGI(CON_MGR_TAG, "Connect to device %s", (const char *)data);
    break;
  case ESP_CON_DISCONNECT:
    ESP_LOGI(CON_MGR_TAG, "Disconnect");
    break;
  case ESP_CON_SET_NAME:
    ESP_LOGI(CON_MGR_TAG, "Setting name=%s", (const char *)data);
    break;
  case ESP_CON_SET_MAC:
    ESP_LOGI(CON_MGR_TAG, "Setting mac=%s", (const char *)data);
    break;
  case ESP_CON_SET_SSID:
    ESP_LOGI(CON_MGR_TAG, "Setting ssid=%s", (const char *)data);
    break;
  case ESP_CON_SET_IP:
    ESP_LOGI(CON_MGR_TAG, "Setting id=%s", (const char *)data);
    break;
  case ESP_CON_SET_SUBNET_MASK:
    ESP_LOGI(CON_MGR_TAG, "Setting subnetmask=%s", (const char *)data);
    break;
  case ESP_CON_SET_STATIC_IP:
    ESP_LOGI(CON_MGR_TAG, "Setting to static ip mode");
    break;
  case ESP_CON_SET_DHCP:
    ESP_LOGI(CON_MGR_TAG, "Setting DHCP=%s", (const char *)data);
    break;
  case ESP_CON_SET_WIFI_STATION:
    ESP_LOGI(CON_MGR_TAG, "Setting WIFI to station mode");
    break;
  case ESP_CON_SET_WIFI_AP:
    ESP_LOGI(CON_MGR_TAG, "Setting WIFI to ap mode");
    break;
  }
}

void connectionEventRX(int event, const uint8_t *data, int len) {}

void sendEvent(int event, const uint8_t *data, int len) {
  writeCommand(ESP_ROOT, ESP_ROOTCMD_CON_EVENT, data, len);
}