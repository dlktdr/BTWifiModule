#pragma once
/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/


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

/* Attributes State Machine */
enum
{
    IDX_SVC,
    IDX_CHAR_A,
    IDX_CHAR_VAL_A,
    IDX_CHAR_CFG_A,

    HRS_IDX_NB,
};