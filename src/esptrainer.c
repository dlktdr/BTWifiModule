#include "esptrainer.h"
#include "esp_log.h"

#define LOGS "TRNR"

// Channel Format
typedef struct  {
  int16_t ch[32];
  uint32_t channelmask; // Valid Channels
} channeldata;


void espTrainerData(const uint8_t *data, uint8_t len)
{
  printf("d\r\n");
//  ESP_LOGI(LOGS, "%d", len);
}

void espTrainerCommand(uint8_t command, const uint8_t *data, uint8_t len)
{
  printf("c\r\n");
//  ESP_LOGI(LOGS, "%d", command);
}
