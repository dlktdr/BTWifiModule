#include "esproot.h"
#include "espaudio.h"
#include "espconmngr.h"
#include "espdefs.h"
#include "espjoystick.h"
#include "esptrainer.h"
#include "terminal.h"
#include <stdint.h>
#include <string.h>

#define LOG_ESPR "ESPROOT"

// TODO, Replace me with git SHA
espversion espVersion = {1, 0, 0, "GITTAG"};

// Global ESP Settings
espsettings espSettings;

// Settings must be 4 characters!
const espsettingslink espSettingsIndex[] = {
    SETTING_LINK_ARR("name", espSettings.name),
    SETTING_LINK_ARR("wmac", espSettings.wifimac),
    SETTING_LINK_ARR("btma", espSettings.blemac),
    SETTING_LINK_ARR("ssid", espSettings.ssid),
    SETTING_LINK_ARR("ip  ", espSettings.ip),
    SETTING_LINK_ARR("subn", espSettings.subnet),
    SETTING_LINK_ARR("stip", espSettings.staticip),
    SETTING_LINK_VAR("dhcp", espSettings.dhcpMode),
    SETTING_LINK_VAR("wimd", espSettings.wifiStationMode)};

int g_rv;
uint8_t g_m;
bool g_radioIsBTEDR = false;
bool g_radioIsBLE = false;
bool g_radioIsWIFI = false;

#define STARTBLE_MODE(m, x)                                                    \
  g_rv = x;                                                                    \
  g_m = m;                                                                     \
  if (g_rv == 0) {                                                             \
    g_radioIsBLE = true;                                                       \
    writeCommand(ESP_ROOT, ESP_ROOTCMD_START_MODE, &g_m, 1);                   \
  } else {                                                                     \
    writeCommand(ESP_ROOT, ESP_ROOTCMD_STOP_MODE, &g_m, 1);                    \
  }

#define STARTBTEDR_MODE(m, x)                                                  \
  g_rv = x;                                                                    \
  g_m = m;                                                                     \
  if (g_rv == 0) {                                                             \
    g_radioIsBTEDR = true;                                                     \
    writeCommand(ESP_ROOT, ESP_ROOTCMD_START_MODE, &g_m, 1);                   \
  } else {                                                                     \
    writeCommand(ESP_ROOT, ESP_ROOTCMD_STOP_MODE, &g_m, 1);                    \
  }

#define STARTWIFI_MODE(m, x)                                                   \
  g_rv = x;                                                                    \
  g_m = m;                                                                     \
  if (g_rv == 0) {                                                             \
    g_radioIsWIFI = true;                                                      \
    writeCommand(ESP_ROOT, ESP_ROOTCMD_START_MODE, &g_m, 1);                   \
  } else {                                                                     \
    writeCommand(ESP_ROOT, ESP_ROOTCMD_STOP_MODE, &g_m, 1);                    \
  }

#define START_MODE(m, x) writeAckNak(m, x, "");
#define RADIO_USED (g_radioIsBTEDR || g_radioIsBLE || g_radioIsWIFI)

uint8_t runningModes;

void espRootData(const uint8_t *data, uint8_t len) {}

void espRootCommand(uint8_t command, const uint8_t *data, uint8_t len) {
  //ESP_LOGI(LOG_ESPR, "Root Command %d", command);

  switch (command) {
  case ESP_ROOTCMD_START_MODE:
    if (len != 1)
      break;
    ESP_LOGI(LOG_ESPR, "Root Cmd RX: Starting Mode %d", data[0]);
    if (RADIO_USED && ((data[0] == ESP_TELEMETRY || data[0] == ESP_TRAINER ||
                          data[0] == ESP_JOYSTICK || data[0] == ESP_AUDIO ||
                          data[0] == ESP_FTP))) {
      ESP_LOGE(LOG_ESPR, "Cannot start Radio already used");
    } else {
      switch (data[0]) {
      case ESP_TELEMETRY:
        STARTBTEDR_MODE(ESP_TELEMETRY, espTelemetryStart());
        break;
      case ESP_TRAINER:
        STARTBLE_MODE(ESP_TRAINER, espTrainerStart());
        break;
      case ESP_JOYSTICK:
        STARTBLE_MODE(ESP_JOYSTICK, espJoystickStart());
        break;
      case ESP_AUDIO:
        STARTBTEDR_MODE(ESP_AUDIO, espAudioStart());
        break;
      case ESP_FTP:
        // STARTWIFI_MODE(ESP_FTP, espFTPStart());
        break;
      case ESP_IMU:
        // START_MODE(ESP_IMU, espIMUStart());
        break;
      default:
        break;
      }
    }
    break;

  case ESP_ROOTCMD_STOP_MODE:
    if (len != 1)
      break;
    ESP_LOGI(LOG_ESPR, "Root Cmd RX: Stopping Mode %d", data[0]);
    switch (data[0]) {
    case ESP_TELEMETRY:
      if(espTelemetryRunning()) {
        espTelemetryStop();
        g_radioIsBTEDR = false;
      }
      break;
    case ESP_TRAINER:
      if (espTrainerRunning()) {
        espTrainerStop();
        g_radioIsBLE = false;
      }
      break;
    case ESP_JOYSTICK:
      if (espJoystickRunning()) {
        espJoystickStop();
        g_radioIsBLE = false;
      }
      break;
    case ESP_AUDIO:
      if (espAudioRunning()) {
        espAudioStop();
        g_radioIsBTEDR = false;
      }
      break;
    case ESP_FTP:
      //    if(espFTPRunning()) {
      //      espFTPStop();
      //      radioUsed = false;
      //    }
      break;
    case ESP_IMU:
      //    if(espFTPRunning()) {
      //      espFTPStop();
      //    }
      break;

    default:
      break;
    }
    break;

  case ESP_ROOTCMD_ACTIVE_MODES:
    ESP_LOGI(LOG_ESPR, "Root Cmd RX: Get Active Modes");

  case ESP_ROOTCMD_RESTART:
    ESP_LOGI(LOG_ESPR, "Root Cmd RX: Rebooting...");
    esp_restart();
    break;

  case ESP_ROOTCMD_VERSION:
    writeCommand(ESP_ROOT, ESP_ROOTCMD_VERSION, (const uint8_t *)&espVersion,
                 sizeof(espversion));
    break;

  case ESP_ROOTCMD_CON_EVENT: // Shouldn't be much here, events are generated
    ESP_LOGI(LOG_ESPR, "Root Cmd RX: Connection Event");
    if (len < 1)
      break;
    ESP_LOGI(LOG_ESPR, "  Con Mgr. Event %d", data[0]);
    connectionEventRX(data[0], data + 1, len - 1);
    break;

  case ESP_ROOTCMD_CON_MGMNT:
    ESP_LOGI(LOG_ESPR, "Root Cmd RX: Connection Command Received");
    if (len < 1)
      break;
    ESP_LOGI(LOG_ESPR, "  Con Mgr. Cmd %d", data[0]);
    connectionCommandRX(data[0], data + 1, len - 1);
    break;

  case ESP_ROOTCMD_SET_VALUE: {
    ESP_LOGI(LOG_ESPR, "Root Cmd RX: Setting, Setting value");
    // First 4 Characters are the Variable, Remainder is the Data
    if (len > 4) {
      char variable[5];
      memcpy(variable, data, 4);
      variable[4] = '\0';
      // ESP_LOGI("SETT", "Radio Set %s", variable);
      for (unsigned int i = 0; i < SETTINGS_COUNT; i++) {
        if (!strcmp(variable, espSettingsIndex[i].variable)) {
          // Found the variable, make sure it's the same size
          if (len - SETTING_LEN == espSettingsIndex[i].len) {
            memcpy(espSettingsIndex[i].ptr, data + 4, len - 4);
            ESP_LOGI("ROOT", "Radio Set %s Success", espSettingsIndex[i].variable);
          }
          break;
        }
      }
    }
    break;
  }
  case ESP_ROOTCMD_GET_VALUE: {
    ESP_LOGI(LOG_ESPR, "Root Cmd RX: Setting, Requested value");
    uint8_t buffer[50];
    // First 4 Characters are the Variable, Remainder is the Data
    if (len == 4) {
      char variable[5];
      memcpy(variable, data, 4);
      variable[4] = '\0';
      // ESP_LOGI("SETT", "Radio Requesting %s", variable);
      for (unsigned int i = 0; i < SETTINGS_COUNT; i++) {
        if (!strcmp(variable, espSettingsIndex[i].variable)) {
          ESP_LOGI("ROOT", "Found Variable %s.. Sending it", espSettingsIndex[i].variable);
          memcpy(buffer, espSettingsIndex[i].variable, SETTING_LEN);
          memcpy(buffer + 4, espSettingsIndex[i].ptr, espSettingsIndex[i].len);
          writeCommand(ESP_ROOT, ESP_ROOTCMD_SET_VALUE, buffer,
                       espSettingsIndex[i].len + SETTING_LEN);
        }
      }
    }
    break;
  }
  }
}
