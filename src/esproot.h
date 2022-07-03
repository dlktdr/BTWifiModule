#pragma once

#include <stdint.h>



void espRootData(const char *data, uint8_t len);
void espRootCommand(uint8_t command, const uint8_t *data, uint8_t len);
