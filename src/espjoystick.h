#pragma once

#include "espdefs.h"

int  espJoystickStart();
void espJoystickStop();
bool espJoystickRunning();
void espJoystickData(const uint8_t *data, uint8_t len);
void espJoystickCommand(uint8_t command, const uint8_t *data, uint8_t len);
