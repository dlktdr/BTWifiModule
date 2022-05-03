#pragma once

#include "defines.h"
#include "bt_client.h"

typedef struct {
  char rmtbtaddr[13];
  role_t role;
} settings_t;

extern settings_t settings;
extern volatile bool settings_ok;

void loadSettings();
void saveSettings();