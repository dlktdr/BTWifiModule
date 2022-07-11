#include <stdio.h>
#include "espaudio.h"
#include "esp_log.h"

#define AUDIO_TAG "AUD"

int espAudioStart()
{
  return -1;
}

void espAudioStop()
{

}

bool espAudioRunning()
{
  return false;
}

void espAudioData(const uint8_t *data, uint8_t len)
{
 // Read audio data, resample and write to bluetooth

}

void espAudioCommand(uint8_t command, const uint8_t *data, uint8_t len)
{

}
