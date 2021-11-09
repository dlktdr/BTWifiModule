#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define START_STOP 0x7E
#define BYTE_STUFF 0x7D
#define STUFF_MASK 0x20
#define BT_CHANNELS 8
#define BLUETOOTH_LINE_LENGTH           32

extern uint16_t chan_vals[BT_CHANNELS];
int setTrainer(uint8_t *addr);