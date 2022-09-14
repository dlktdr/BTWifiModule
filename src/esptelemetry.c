#include "espTelemetry.h"
#include "bt.h"
#include "esp_log.h"
#include "joystick/bt_joystick.h"
#include "joystick/hidd_le_prf_int.h"
#include <stdio.h>

#define TELEMETRY_TAG "TELEM"

bool telemetrystarted = false;

int espTelemetryStart() {
  ESP_LOGI(TELEMETRY_TAG, "Telemetry Starting");
  if (telemetrystarted)
    return -1;
  telemetrystarted = true;
  //BTInit();
  //BTJoyInit();
  return 0;
}

void espTelemetryStop() {
  ESP_LOGI(TELEMETRY_TAG, "Telemetry Stop");
  if (telemetrystarted) {
    //bt_disable();
  }
  telemetrystarted = false;
}

bool espTelemetryRunning() { return telemetrystarted; }

bool espTelemetryExec()
{

}

void espTelemetryData(const uint8_t *data, uint8_t len)
{

}

void espTelemetryCommand(uint8_t command, const uint8_t *data, uint8_t len) {}

void espTelemetryDiscoverStart() {}
void espTelemetryDiscoverStop() {}