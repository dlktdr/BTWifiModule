#pragma once

#include "defines.h"
#include "espdefs.h"

// Packet Format
typedef struct PACKED {
  uint8_t type;
  uint8_t crcl;      // CRC16:low byte
  uint8_t crch;      // CRC16:high byte
  uint8_t data[257]; // User Data
  uint8_t len;       // Data length, not transmitted
} packet_s;

#define PACKET_OVERHEAD 3

/* Packet Type Format
 * Bits 0:4 - Type(32 Possible ESPModes, Mode 0=Base Module)
 *        5 - PartialPacket=1
 *        6 - Command=1/Data=0
 *        7 - Require Ack=1
 * If Command Bit(6) = 1
 *   data[0] = Command
 *   data[1:255] = UserData
 * If command bit(6) = 0
 *   data[0:255] = DataStream

 */


typedef uint8_t espmode;

void mainTask(void *n);
void uartRXTask(void *n);
void logBTFrame(const char btdata[], int len);
void writePacket(const uint8_t *dat, int len, bool iscmd, uint8_t mode);
void writeData(uint8_t mode, const uint8_t *dat, int len);
void writeCommand(uint8_t mode, uint8_t command, const uint8_t *dat, int len);
void writeAckNak(uint8_t mode, bool ack, const char *message);