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
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void btpInit();
void btp_disconnect();
int btp_sendChannelData(uint8_t *data, int len);

#ifdef __cplusplus
}
#endif

extern volatile bool btp_connected;