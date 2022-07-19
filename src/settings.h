#pragma once

#include "bt_client/bt_client.h"
#include "defines.h"
#include "espdefs.h"

typedef struct {
  // uint8_t version // Todo the version info here, check on load it matches
  // otherwise ignore
  char rmtbtaddr[13];
  uint8_t mode;
} settings_t;

extern settings_t settings;
extern volatile bool settings_ok;

void loadSettings();
void saveSettings();