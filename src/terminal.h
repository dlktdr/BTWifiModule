#pragma once

#include "defines.h"

// Packet Format
typedef struct {
  uint8_t len;       // Data length. Max 255 per packet
  uint8_t type;
  uint16_t crc;      // CRC16:xxxx of the packet
  uint8_t data[257]; // User Data
} packet_s;

#define PACKET_OVERHEAD 4

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

#define ESP_PACKET_CMD_BIT 6
#define ESP_PACKET_ACK_BIT 7
#define ESP_PACKET_ISCMD(t) (t&(1<<ESP_PACKET_CMD_BIT))
#define ESP_PACKET_ISACKREQ(t) (t&(1<<ESP_PACKET_ACK_BIT))

typedef uint8_t espmode;

void runUARTHead(void *n);
void logBTFrame(const char btdata[], int len);