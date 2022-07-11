#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <esp_log.h>

#define ESP_PACKET_TYPE_MSK 0x0F
#define ESP_PACKET_CMD_BIT 6
#define ESP_PACKET_ACK_BIT 7
#define ESP_PACKET_ISCMD(t) (t&(1<<ESP_PACKET_CMD_BIT))
#define ESP_PACKET_ISACKREQ(t) (t&(1<<ESP_PACKET_ACK_BIT))

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
  ESP_ROOTCMD_ACKNAK=0,
  ESP_ROOTCMD_START_MODE,
  ESP_ROOTCMD_STOP_MODE,
  ESP_ROOTCMD_RESTART,
  ESP_ROOTCMD_GET_VER,
  ESP_ROOTCMD_EVENT,
};


enum ESPEvents {
  ESP_EVT_DISCOVER_STARTED,
  ESP_EVT_DISCOVER_COMPLETE,
  ESP_EVT_DEVICE_FOUND,
  ESP_EVT_CONNECTED,
  ESP_EVT_DISCONNECTED,
  ESP_EVT_PIN_REQUEST,
  ESP_EVT_IP_OBTAINED
};

enum ESPTrainerCmds {
  ESP_TRAINERCMD_SET_MASTER,
  ESP_TRAINERCMD_SET_SLAVE,
  ESP_TRAINERCMD_COUNT,
};

// Channel Data (Joystick, Trainer)

#define MAX_OUTPUT_CHANNELS 32

// Channel Format
typedef struct  {
  int16_t ch[MAX_OUTPUT_CHANNELS];
  uint32_t channelmask; // Valid Channels
} channeldata;
