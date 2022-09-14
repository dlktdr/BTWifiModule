#pragma once

#include "espdefs.h"

// Main functions to radio
int espTrainerStart();
void espTrainerStop();
void espTrainerExec();
bool espTrainerRunning();
void espTrainerData(const uint8_t *data, uint8_t len);
void espTrainerCommand(uint8_t command, const uint8_t *data, uint8_t len);
void espTrainerSend(const channeldata *chans);
void espTrainerDiscoverStart();
void espTrainerDiscoverStop();

// Internal Functions, Bluetooth
void espTrainerDeviceDiscovered(const char *address);
void espTrainerRFDataReceived(const channeldata *chans);
void espTrainerDiscoverComplete();