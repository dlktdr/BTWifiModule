#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define START_STOP 0x7E
#define BYTE_STUFF 0x7D
#define TRAINER_FRAME 0x80
#define STUFF_MASK 0x20
#define BT_CHANNELS 8

enum {
  BT_CRC_FAULT=-1,
  BT_SUCCESS=0,
};

int setTrainer(uint8_t *addr, uint16_t chan_vals[BT_CHANNELS]);
void frSkyProcessByte(uint8_t data);
void processFrame(const uint8_t *frame, uint8_t len);