#include "espjoystick.h"
#include "bt.h"
#include "esp_log.h"
#include "joystick/bt_joystick.h"
#include "joystick/hidd_le_prf_int.h"
#include <stdio.h>

#define JOYSTICK_TAG "JOY"

bool joystickstarted = false;

int espJoystickStart() {
  ESP_LOGI(JOYSTICK_TAG, "Joystick Starting");
  if (joystickstarted)
    return -1;
  joystickstarted = true;
  BTInit();
  BTJoyInit();
  return 0;
}

void espJoystickStop() {
  ESP_LOGI(JOYSTICK_TAG, "Joystick Stop");
  if (joystickstarted) {
    bt_disable();
  }
  joystickstarted = false;
}

bool espJoystickRunning() { return joystickstarted; }

void espJoystickExec()
{

}

void espJoystickData(const uint8_t *data, uint8_t len) {
  if (len == sizeof(channeldata)) {
    /*const channeldata *chdata = (const channeldata *)data;
    for(int i=0; i < 8; i++) {
      if(chdata->channelmask & 1<<i)
        printf("CH%d[%d] ", i+1, chdata->ch[i]);
    }
    printf("\r\n");*/
    if (btjoystickconnected)
      hid_SendJoystickChannels((uint16_t *)data);
  } else {
    ESP_LOGE(JOYSTICK_TAG, "Unknown Data");
  }
}

void espJoystickCommand(uint8_t command, const uint8_t *data, uint8_t len) {}
void espJoystickDiscoverStart() {}
void espJoystickDiscoverStop() {}