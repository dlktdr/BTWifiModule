#include "esproot.h"

enum ESPRootCmds {
  ESP_ROOTCMD_START_MODE,
  ESP_ROOTCMD_STOP_MODE,
  ESP_ROOTCMD_RESTART,
  ESP_ROOTCMD_GET_VER,
  ESP_ROOTCMD_COUNT,
};

void espRootData(const uint8_t *data, uint8_t len) {

}

void espRootCommand(uint8_t command, const uint8_t *data, uint8_t len)
{
  switch(command) {
  case ESP_ROOTCMD_START_MODE:
    break;
  case ESP_ROOTCMD_STOP_MODE:
    break;
  case ESP_ROOTCMD_RESTART:
    break;
  case ESP_ROOTCMD_GET_VER:
    break;
  case ESP_ROOTCMD_COUNT:
    break;
  }
}
