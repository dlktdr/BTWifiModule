/* From OpenTX https://github.com/opentx
 */

#include <stdbool.h>
#include "frskybt.h"


#define LEN_BLUETOOTH_ADDR 16
#define BLUETOOTH_LINE_LENGTH 32
#define BLUETOOTH_PACKET_SIZE 14

static uint8_t buffer[BLUETOOTH_LINE_LENGTH+1];
static uint8_t bufferIndex;
static uint8_t crc;

void pushByte(uint8_t byte)
{
    crc ^= byte;
    if (byte == START_STOP || byte == BYTE_STUFF) {
        buffer[bufferIndex++] = BYTE_STUFF;
        byte ^= STUFF_MASK;
    }
    buffer[bufferIndex++] = byte;
}

/* Builds Trainer Data
*     Returns the length of the encoded PPM + CRC
*     Data saved into addr pointer
*/

int setTrainer(uint8_t *addr, uint16_t chan_vals[BT_CHANNELS])
{
    // Allocate Channel Mappings, Set Default to all Center
    uint8_t * cur = buffer;
    bufferIndex = 0;
    crc = 0x00;

    buffer[bufferIndex++] = START_STOP; // start byte
    pushByte(TRAINER_FRAME); // trainer frame type?
    for (int channel=0; channel < BT_CHANNELS; channel+=2, cur+=3) {
        uint16_t channelValue1 = chan_vals[channel];
        uint16_t channelValue2 = chan_vals[channel+1];

        pushByte(channelValue1 & 0x00ff);
        pushByte(((channelValue1 & 0x0f00) >> 4) + ((channelValue2 & 0x00f0) >> 4));
        pushByte(((channelValue2 & 0x000f) << 4) + ((channelValue2 & 0x0f00) >> 8));
    }

    buffer[bufferIndex++] = crc;
    buffer[bufferIndex++] = START_STOP; // end byte

    // Copy data to array
    memcpy(addr,buffer,bufferIndex);

    return bufferIndex;
}

//----------------------------------
// Receieve Code
//----------------------------------

enum {STATE_DATA_IDLE,
STATE_DATA_START,
STATE_DATA_XOR,
STATE_DATA_IN_FRAME
};

uint8_t otxbuffer[BLUETOOTH_LINE_LENGTH+1];
uint8_t otxbufferIndex = 0;
uint16_t chan_out[BT_CHANNELS];
bool btprocessed = false;

void appendTrainerByte(uint8_t data)
{
  if (otxbufferIndex < BLUETOOTH_LINE_LENGTH) {
    otxbuffer[otxbufferIndex++] = data;
    // we check for "DisConnected", but the first byte could be altered (if received in state STATE_DATA_XOR)
    if (data == '\n') {
        otxbufferIndex = 0;
    }
  }
}

void processTrainerFrame(const uint8_t * otxbuffer)
{  
  for (uint8_t channel=0, i=1; channel<BT_CHANNELS; channel+=2, i+=3) {
    // +-500 != 512, but close enough.
    chan_out[channel] = otxbuffer[i] + ((otxbuffer[i+1] & 0xf0) << 4);
    chan_out[channel+1] = ((otxbuffer[i+1] & 0x0f) << 4) + ((otxbuffer[i+2] & 0xf0) >> 4) + ((otxbuffer[i+2] & 0x0f) << 8);
  }
}

void processTrainerByte(uint8_t data)
{
  static uint8_t dataState = STATE_DATA_IDLE;

  switch (dataState) {
    case STATE_DATA_START:
      if (data == START_STOP) {
        dataState = STATE_DATA_IN_FRAME;
        otxbufferIndex = 0;
      }
      else {
        appendTrainerByte(data);
      }
      break;

    case STATE_DATA_IN_FRAME:
      if (data == BYTE_STUFF) {
        dataState = STATE_DATA_XOR; // XOR next byte
      }
      else if (data == START_STOP) {
        dataState = STATE_DATA_IN_FRAME;
        otxbufferIndex = 0;
      }
      else {
        appendTrainerByte(data);
      }
      break;

    case STATE_DATA_XOR:
      switch (data) {
        case BYTE_STUFF ^ STUFF_MASK:
        case START_STOP ^ STUFF_MASK:
          // Expected content, save the data
          appendTrainerByte(data ^ STUFF_MASK);
          dataState = STATE_DATA_IN_FRAME;
          break;
        case START_STOP:  // Illegal situation, as we have START_STOP, try to start from the beginning
          otxbufferIndex = 0;
          dataState = STATE_DATA_IN_FRAME;
          break;
        default:  
          // Illegal situation, start looking for a new START_STOP byte
          dataState = STATE_DATA_START;
          break;
      }
      break;

    case STATE_DATA_IDLE:
      if (data == START_STOP) {
        otxbufferIndex = 0;
        dataState = STATE_DATA_START;
      }
      else {
        appendTrainerByte(data);
      }
      break;
  }

  if (otxbufferIndex >= BLUETOOTH_PACKET_SIZE) {
    uint8_t crc = 0x00;
    for (int i = 0; i < BLUETOOTH_PACKET_SIZE - 1; i++) {
      crc ^= otxbuffer[i];
    }
    if (crc == otxbuffer[BLUETOOTH_PACKET_SIZE - 1]) {
      if (otxbuffer[0] == TRAINER_FRAME) {
        processTrainerFrame(otxbuffer);
        btprocessed = true;
      }
    }
    dataState = STATE_DATA_IDLE;
  }
}

int processTrainer(const char *data, int len, uint16_t channels[BT_CHANNELS])
{
  btprocessed = false;
  for(int i=0; i < len; i++) {
    processTrainerByte(data[i]);
  }
  if(!btprocessed)
    return -1;
  
  memcpy(channels, chan_out, sizeof(chan_out));
  return 0;
}