#pragma once

#include <esp_log.h>
#include <stdbool.h>
#include <stdint.h>

#define MAX_OUTPUT_CHANNELS 32

#define ESP_PACKET_TYPE_MSK 0x0F
#define ESP_PACKET_CMD_BIT 6
#define ESP_PACKET_ACK_BIT 7
#define ESP_PACKET_ISCMD(t) (t & (1 << ESP_PACKET_CMD_BIT))
#define ESP_PACKET_ISACK(t) (t & (1 << ESP_PACKET_ACK_BIT))

enum ESPModes {
  ESP_ROOT,
  ESP_TELEMETRY,
  ESP_TRAINER,
  ESP_JOYSTICK,
  ESP_AUDIO,
  ESP_FTP,
  ESP_IMU,
  ESP_MAX
};

enum ESPRootCmds {
  ESP_ROOTCMD_START_MODE,
  ESP_ROOTCMD_STOP_MODE,
  ESP_ROOTCMD_ACTIVE_MODES, // Request & return mask of running modes
  ESP_ROOTCMD_RESTART,      // Reboot ESP
  ESP_ROOTCMD_VERSION,      // Request Version
  ESP_ROOTCMD_CON_EVENT,    // ESP Connection event
  ESP_ROOTCMD_CON_MGMNT,    // Set ESP Connection Parameters
  ESP_ROOTCMD_SET_VALUE,    // Set a value
  ESP_ROOTCMD_GET_VALUE,    // Request a value
};

enum ESPConnectionEvents {
  ESP_EVT_MESSAGE,           // String value of status
  ESP_EVT_DISCOVER_STARTED,
  ESP_EVT_DISCOVER_COMPLETE,
  ESP_EVT_DEVICE_FOUND,      // A connectable device was found
  ESP_EVT_CONNECTED,         // Periodically send this event.
  ESP_EVT_DISCONNECTED,      // Periodically send this event.
  ESP_EVT_PIN_REQUEST,
  ESP_EVT_IP_OBTAINED
};

enum ESPConnectionManagment {
  ESP_CON_DISCOVER_START,
  ESP_CON_DISCOVER_STOP,
  ESP_CON_CONNECT,
  ESP_CON_DISCONNECT,
};

enum ESPWifiModes {
  ESP_WIFI_STATION,
  ESP_WIFI_AP,
};

enum ESPTrainerCmds {
  ESP_TRAINERCMD_SET_MASTER,
  ESP_TRAINERCMD_SET_SLAVE,
};


// Channel Format
typedef struct {
  int16_t ch[MAX_OUTPUT_CHANNELS];
  uint32_t channelmask;  // Valid Channels
} channeldata;

typedef struct {
  uint8_t bteaddr[6];
  uint8_t rssi;
  char name[30];
} scanresult;

typedef struct {
  uint8_t maj;
  uint8_t min;
  uint8_t rev;
  uint8_t sha[10];
} espversion;


typedef struct {
  uint8_t event;  // Event ID
  uint8_t data[50];
} espevent;

// ESP Settings
typedef struct {
  char variable[5];
  void *ptr;
  int len;
} espsettingslink;

// ESP Settings
typedef struct {
  char name[40];
  char wifimac[18];
  char blemac[18];
  char ssid[30];
  char ip[16];
  char subnet[16];
  char staticip[16];
  uint8_t dhcpMode;
  uint8_t wifiStationMode;
} espsettings;

#define SETTING_LINK_ARR(name, _array) {name, (void*)_array, sizeof(_array)}
#define SETTING_LINK_VAR(name, variable) {name, &variable, sizeof(variable)}
#define SETTINGS_COUNT (sizeof(espSettingsIndex)/sizeof(espsettingslink))
#define SETTING_LEN 4

extern espsettings espSettings;