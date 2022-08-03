#pragma once

#include <stdint.h>

int cobs_encode(const uint8_t* buffer, int size, uint8_t* encodedBuffer);
int cobs_decode(const uint8_t* encodedBuffer, int size, uint8_t* decodedBuffer);
