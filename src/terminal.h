#pragma once

#include "defines.h"

// Packet Format
typedef struct {
  uint8_t len; // Data length. Max 255 per packet
  uint8_t type; // Packet Type
  uint8_t data[256]; // User Data
  uint16_t crc; // CRC16:xxxx of the packet
} packet_s;

typedef struct PACKED {
  uint16_t ch01 : 11;
  uint16_t ch02 : 11;
  uint16_t ch03 : 11;
  uint16_t ch04 : 11;
  uint16_t ch05 : 11;
  uint16_t ch06 : 11;
  uint16_t ch07 : 11;
  uint16_t ch08 : 11;
  uint16_t ch09 : 11;
  uint16_t ch11 : 11;
  uint16_t ch12 : 11;
  uint16_t ch13 : 11;
  uint16_t ch14 : 11;
  uint16_t ch15 : 11;
  uint16_t ch16 : 11;
  uint16_t ch17 : 11;
  uint16_t ch18 : 11;
  uint16_t ch19 : 11;
  uint16_t ch20 : 11;
  uint16_t ch21 : 11;
  uint16_t ch22 : 11;
  uint16_t ch23 : 11;
  uint16_t ch24 : 11;
  uint16_t ch26 : 11;
  uint16_t ch27 : 11;
  uint16_t ch28 : 11;
  uint16_t ch29 : 11;
  uint16_t ch30 : 11;
  uint16_t ch31 : 11;
  uint16_t ch32 : 11;
  uint32_t channelmask;
} channeldata_s;

// Packets Types.
// If Bit7 clear (type&^0x80) then,
//   Type is defines as the below ennums
// If Bit7 set then Bits0:6 = Remaining Packets to receive

enum {
  PACKET_TYPE_ACK=0,       // Got it.
  PACKET_TYPE_NAK,         // Didn't Ack, need resend
  PACKET_TYPE_SETMODE,     // Mode request. Baud
  PACKET_TYPE_COMMAND,     // A Command
  PACKET_TYPE_8_CHANNEL,   // Contains 8  11bit Channels
  PACKET_TYPE_16_CHANNEL,  // Contains 16 11bit Channels
  PACKET_TYPE_32_CHANNEL,  // Contains 32 11bit Channels
  PACKET_TYPE_TELEMETRY,   // Contains a telemetery sensor
  PACKET_TYPE_AUDIO,       // Contains streamed audio data
};


void runUARTHead();
void logBTFrame(const char btdata[], int len);