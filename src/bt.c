// Part of setTrainer to calculate CRC
// From OpenTX

#include "bt.h"


#define LEN_BLUETOOTH_ADDR              16
#define MAX_BLUETOOTH_DISTANT_ADDR      6
#define BLUETOOTH_LINE_LENGTH           32

static uint8_t bufferIndex;
static uint8_t buffer[BLUETOOTH_LINE_LENGTH+1];
uint16_t chan_vals[BT_CHANNELS];
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
int setTrainer(uint8_t *addr)
{
    // Allocate Channel Mappings, Set Default to all Center
    uint8_t * cur = buffer;
    bufferIndex = 0;
    crc = 0x00;

    buffer[bufferIndex++] = START_STOP; // start byte
    pushByte(0x80); // trainer frame type?
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