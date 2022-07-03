#pragma once

#include <stdint.h>

enum ESPTrainerCmds {
  ESP_TRAINERCMD_SET_MASTER,
  ESP_TRAINERCMD_SET_SLAVE,
  ESP_TRAINERCMD_COUNT,
};

void espTrainerData(const char *data, uint8_t len);
void espTrainerCommand(uint8_t command, const uint8_t *data, uint8_t len);
