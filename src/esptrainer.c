#include "esptrainer.h"
#include "bt.h"
#include "bt_client/bt_client.h"
#include "bt_server/bt_server.h"
#include "esp_log.h"
#include "terminal.h"
#include <stdio.h>

#define TRAINER_TAG "TRAINER"

bool trainerstarted = false;
bool trainerismaster = false;
bool isDiscoveryStarted = false;

int espTrainerStart() {
  if (trainerstarted)
    return -1;

  if (trainerismaster) {
    ESP_LOGI(TRAINER_TAG, "Trainer Master Start");
    // Start Master Mode
    BTInit();
    btpInit();
    writeCommand(ESP_TRAINER,
                 ESP_TRAINERCMD_SET_MASTER,
                 NULL,
                 0);
  } else {
    ESP_LOGI(TRAINER_TAG, "Trainer Slave Start");
    // Start Slave Mode
    BTInit();
    btcInit();
    writeCommand(ESP_TRAINER,
                ESP_TRAINERCMD_SET_MASTER,
                NULL,
                0);
  }

  trainerstarted = true;
  return 0;
}

void espTrainerStop() {
  ESP_LOGI(TRAINER_TAG, "Trainer Stop");
  if (trainerstarted)
    bt_disable();
  trainerstarted = false;
}

bool espTrainerRunning() { return trainerstarted; }

void espTrainerExec()
{

}

// Trainer data received
void espTrainerData(const uint8_t *data, uint8_t len) {
  if (len == sizeof(channeldata)) {
    const channeldata *chdata = (const channeldata *)data;
    for (int i = 0; i < MAX_OUTPUT_CHANNELS; i++) {
      if (chdata->channelmask & 1 << i)
        printf("CH%d[%d] ", i + 1, chdata->ch[i]);
    }
    printf("\r\n");

  } else {
    ESP_LOGE(TRAINER_TAG, "Unknown Data");
  }
}

void espTrainerCommand(uint8_t command, const uint8_t *data, uint8_t len) {
  ESP_LOGI(TRAINER_TAG, "Got A Command %d", command);
  bool switchmodes = false;
  switch (command) {
  case ESP_TRAINERCMD_SET_MASTER:
    if (!trainerismaster)
      switchmodes = true;
    break;
  case ESP_TRAINERCMD_SET_SLAVE:
    if (!trainerismaster)
      switchmodes = true;
    break;
  }

  if (switchmodes) {
    trainerismaster = !trainerismaster;
    espTrainerStop();
    espTrainerStart();
  }
}

void espTrainerSend(const channeldata *chans) {
  writePacket((const uint8_t *)chans, sizeof(channeldata), false, ESP_TRAINER);
}

void espTrainerDiscoverStart()
{
  ESP_LOGI(TRAINER_TAG, "Trainer Discovery Requested");
  if(isDiscoveryStarted || !trainerismaster) {
    ESP_LOGE(TRAINER_TAG, "Discovery already started or not master mode");
    return;
  }
  btc_start_scan();
  writeEvent(ESP_EVT_DISCOVER_STARTED, NULL, 0);
  isDiscoveryStarted = true;
}

void espTrainerDeviceDiscovered(const char *address)
{
  ESP_LOGI(TRAINER_TAG, "Device discovered %s", address);
  writeEvent(ESP_EVT_DEVICE_FOUND, (uint8_t*)address, strlen(address)+1);
}

void espTrainerRFDataReceived(const channeldata *chans)
{

}

void espTrainerDiscoverComplete()
{
  ESP_LOGI(TRAINER_TAG, "Discoverery Complete");
  writeEvent(ESP_EVT_DISCOVER_COMPLETE, NULL, 0);
  isDiscoveryStarted = false;
}

void espTrainerDiscoverStop()
{
  ESP_LOGI(TRAINER_TAG, "Trainer Discovery Stop Requested");
  isDiscoveryStarted = false;

}