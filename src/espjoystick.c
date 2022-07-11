#include <stdio.h>
#include "espjoystick.h"
#include "esp_log.h"

#define JOYSTICK_TAG "JOY"

int espJoystickStart()
{
  return -1;
}

void espJoystickStop()
{

}

bool espJoystickRunning()
{
  return false;
}

void espJoystickData(const uint8_t *data, uint8_t len)
{
  // Read audio data, resample and write to bluetooth
  
}

void espJoystickCommand(uint8_t command, const uint8_t *data, uint8_t len)
{

}
