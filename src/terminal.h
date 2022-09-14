#pragma once

#include "defines.h"
#include "espdefs.h"

// Packet Format
#define PACKET_OVERHEAD 3
#define ESP_MAX_PACKET_LEN  250 // COBS, without adding an extra byte
#define ESP_MAX_PACKET_DATA (ESP_MAX_PACKET_LEN - PACKET_OVERHEAD)
typedef struct PACKED {
  uint8_t type;
  uint8_t crcl;      // CRC16:low byte
  uint8_t crch;      // CRC16:high byte
  uint8_t data[ESP_MAX_PACKET_DATA]; // User Data
  uint8_t len;       // Data length, not transmitted
} packet_s;


typedef uint8_t espmode;

void mainTask(void *n);
void uartRXTask(void *n);
void logBTFrame(const char btdata[], int len);
void writePacket(const uint8_t *dat, int len, bool iscmd, uint8_t mode);
void writeData(uint8_t mode, const uint8_t *dat, int len);
void writeCommand(uint8_t mode, uint8_t command, const uint8_t *dat, int len);
void writeEvent(uint8_t event, const uint8_t* dat, int len);
void writeAck();