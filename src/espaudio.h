#pragma once

#include "espdefs.h"

int  espAudioStart();
void espAudioStop();
bool espAudioRunning();
void espAudioData(const uint8_t *data, uint8_t len);
void espAudioCommand(uint8_t command, const uint8_t *data, uint8_t len);
