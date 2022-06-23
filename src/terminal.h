#pragma once

#include "defines.h"

// NOTE:: Not used at the moment.
//        Future planning/thinking to enable more robust packet
//        format for features that could be added while keeping
//        interface cleaner
//

// Packet Format
typedef struct {
  uint8_t len;       // Data length. Max 255 per packet
  uint8_t type;      // Packet Type
  uint8_t data[256]; // User Data
  uint16_t crc;      // CRC16:xxxx of the packet
} packet_s;

// Struct for a PACKET_TYPE_8_CHANNEL
// channelmask is the bitmask of the valid channels
// Keeping SBUS Style 11 Bits of channel info

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
  uint16_t ch32 : 11; // 352 bits, 44 bytes
  uint32_t channelmask;
} channeldata_s; // 48 bytes total

// Packet Type Field
//    Bit [0:4] - Packet Type
//    Bit [5]   - Require an acknowledge to be returned
//    Bit [6]   - Continuation Packet

//    0 1 2 3 4 5 6 7
//                  1  (0:6) Packets Remaining
//                1    (6:7) ACK required
//    m m m m m 0 0 0  (0:5) Packet Type



//    x x x x x x 1 x -
// If Bit7 clear (type&^0x80) then,
//   Type is defines as the below ennums
// If Bit7 set then Bits0:6 = Remaining Packets to receive

enum {
  PACKET_TYPE_ACK=0,       // Got it.
  PACKET_TYPE_NAK,         // Didn't Ack, need resend
  PACKET_TYPE_SETMODE,     // Mode change request
  PACKET_TYPE_COMMAND,     // A Command (Do something in current mode)
  PACKET_TYPE_8_CHANNEL,   // Contains 8  11bit Channels
  PACKET_TYPE_16_CHANNEL,  // Contains 16 11bit Channels
  PACKET_TYPE_32_CHANNEL,  // Contains 32 11bit Channels
  PACKET_TYPE_TELEMETRY,   // Contains a telemetery sensor
  PACKET_TYPE_AUDIO,       // Contains streamed audio data
};

void runUARTHead();
void logBTFrame(const char btdata[], int len);
