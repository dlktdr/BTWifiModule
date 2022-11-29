#pragma once

#include "espdefs.h"

int  espTelemetryStart();
void espTelemetryStop();
void espTelemetryExec();
bool espTelemetryRunning();
void espTelemetryData(const uint8_t *data, uint8_t len);
void espTelemetryCommand(uint8_t command, const uint8_t *data, uint8_t len);
void espTelemetryDiscoverStart();
void espTelemetryDiscoverStop();
