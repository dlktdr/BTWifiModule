#pragma once

#include "espdefs.h"

void connectionCommandRX(int conCmd, const uint8_t * data, int len);
void connectionEventRX(int event, const uint8_t *data, int len);
void sendEvent(int event, const uint8_t *data, int len);
