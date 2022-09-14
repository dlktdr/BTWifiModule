#include "espconmngr.h"
#include "esp_log.h"
#include "espdefs.h"
#include "terminal.h"

#include "esptrainer.h"
#include "esptelemetry.h"
#include "espaudio.h"
#include "espjoystick.h"

#include <stdio.h>

#define CON_MGR_TAG "CONMGR"

static void discoverStart();
static void discoverStop();

void connectionCommandRX(int conCmd, const uint8_t *data, int len) {
  switch (conCmd) {
  case ESP_CON_DISCOVER_START:
    ESP_LOGI(CON_MGR_TAG, "Discovery Start Req");
    discoverStart();
    break;
  case ESP_CON_DISCOVER_STOP:
    ESP_LOGI(CON_MGR_TAG, "Discovery Stop Req");
    void discoverStop();
    break;
  case ESP_CON_CONNECT:
    ESP_LOGI(CON_MGR_TAG, "Connect to device %s", (const char *)data);
    break;
  case ESP_CON_DISCONNECT:
    ESP_LOGI(CON_MGR_TAG, "Disconnect");
    break;
  }
}

void connectionEventRX(int event, const uint8_t *data, int len) {}

void sendEvent(int event, const uint8_t *data, int len) {
  writeCommand(ESP_ROOT, ESP_ROOTCMD_CON_EVENT, data, len);
}

void discoverStart()
{
  if(espTelemetryRunning())
    espTelemetryDiscoverStart();
  else if(espTrainerRunning())
    espTrainerDiscoverStart();
  else if(espJoystickRunning())
    espJoystickDiscoverStart();
}

void discoverStop()
{
  if(espTelemetryRunning())
    espTelemetryDiscoverStop();
  else if(espTrainerRunning())
    espTrainerDiscoverStop();
  else if(espJoystickRunning())
    espJoystickDiscoverStop();
}