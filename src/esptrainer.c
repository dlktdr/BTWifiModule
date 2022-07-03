#include "esptrainer.h"
#include "esp_log.h"

#define LOGS "TRNR"

void espTrainerData(const char *data, uint8_t len)
{
  ESP_LOGI(LOGS, "p");
}

void espTrainerCommand(uint8_t command, const uint8_t *data, uint8_t len)
{
  ESP_LOGI(LOGS, "I got a command %d", command);
}
