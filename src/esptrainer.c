#include <stdio.h>
#include "esptrainer.h"
#include "esp_log.h"

#define TRAINER_TAG "TRAINER"

bool trainerstarted=false;

int espTrainerStart()
{
  ESP_LOGI(TRAINER_TAG, "Trainer Start");
  if(trainerstarted) return -1;
  trainerstarted = true;

  return 0;
}

void espTrainerStop()
{
  ESP_LOGI(TRAINER_TAG, "Trainer Stop");
  if(!trainerstarted) return; 
  
  trainerstarted = false;
}

bool espTrainerRunning()
{
  return trainerstarted;
}

// Trainer data received 
void espTrainerData(const uint8_t *data, uint8_t len)
{
  if(len == sizeof(channeldata)) {
    const channeldata *chdata = (const channeldata *)data;
    for(int i=0; i < MAX_OUTPUT_CHANNELS; i++) {
      if(chdata->channelmask & 1<<i)
        printf("CH%d[%d] ", i+1, chdata->ch[i]);
    }
    printf("\r\n");

  } else {
    ESP_LOGE(TRAINER_TAG, "Unknown Data");
  }
}

void espTrainerCommand(uint8_t command, const uint8_t *data, uint8_t len)
{
  ESP_LOGI(TRAINER_TAG, "Got A Command %d", command);
  switch(command) {
    case ESP_TRAINERCMD_SET_MASTER:
      break;
    case ESP_TRAINERCMD_SET_SLAVE:
      break;
  }
}
