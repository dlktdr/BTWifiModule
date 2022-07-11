#pragma once

#include "espdefs.h"

int espTrainerStart();
void espTrainerStop();
bool espTrainerRunning();
void espTrainerData(const uint8_t *data, uint8_t len);
void espTrainerCommand(uint8_t command, const uint8_t *data, uint8_t len);
