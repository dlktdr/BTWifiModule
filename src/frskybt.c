/* From OpenTX https://github.com/opentx
 */

#include "frskybt.h"

#include <stdbool.h>

#include "bt_server.h"
#include "esp_log.h"
#include "settings.h"

#define FRSKYBT_TAG "FRSKYBT"

#define LEN_BLUETOOTH_ADDR 16
#define BLUETOOTH_LINE_LENGTH 32
#define BLUETOOTH_PACKET_SIZE 14

uint16_t channeldata[BT_CHANNELS];

/**
 * @brief Displays the decoded channel values and time since last receive
 *
 * @param btdata
 * @param len
 */

void logBTFrame(bool valid, char message[])
{
  static int64_t ltime = 0;
  int64_t timestamp = esp_timer_get_time() - ltime;
  ltime = esp_timer_get_time();
  if (!valid) {
    ESP_LOGE(FRSKYBT_TAG, "(%05lld)Unable to decode data, %s", timestamp, message);
  } else {
    ESP_LOGI(
        FRSKYBT_TAG,
        "(%05lld)Ch1[%04d] Ch2[%04d] Ch3[%04d] Ch4[%04d] Ch5[%04d] Ch6[%04d] Ch7[%04d] Ch8[%04d]",
        timestamp, channeldata[0], channeldata[1], channeldata[2], channeldata[3], channeldata[4],
        channeldata[5], channeldata[6], channeldata[7]);
  }
}

static uint8_t buffer[BLUETOOTH_LINE_LENGTH + 1];
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
  uint8_t *cur = buffer;
  bufferIndex = 0;
  crc = 0x00;

  buffer[bufferIndex++] = START_STOP;  // start byte
  pushByte(TRAINER_FRAME);             // trainer frame type?
  for (int channel = 0; channel < BT_CHANNELS; channel += 2, cur += 3) {
    uint16_t channelValue1 = chan_vals[channel];
    uint16_t channelValue2 = chan_vals[channel + 1];

    pushByte(channelValue1 & 0x00ff);
    pushByte(((channelValue1 & 0x0f00) >> 4) + ((channelValue2 & 0x00f0) >> 4));
    pushByte(((channelValue2 & 0x000f) << 4) + ((channelValue2 & 0x0f00) >> 8));
  }

  buffer[bufferIndex++] = crc;
  buffer[bufferIndex++] = START_STOP;  // end byte

  // Copy data to array
  memcpy(addr, buffer, bufferIndex);

  return bufferIndex;
}

//----------------------------------
// Receieve Code
//----------------------------------

enum { STATE_DATA_IDLE, STATE_DATA_START, STATE_DATA_XOR, STATE_DATA_IN_FRAME };

static uint8_t _otxbuffer[BLUETOOTH_LINE_LENGTH + 2] = {START_STOP};
static uint8_t *otxbuffer = _otxbuffer + 1;
static uint8_t rsndbuf[BLUETOOTH_LINE_LENGTH + 2];
uint8_t rsndbufindex = 0;
uint8_t otxbufferIndex = 0;
bool btprocessed = false;

void appendTrainerByte(uint8_t data)
{
  if (otxbufferIndex < BLUETOOTH_LINE_LENGTH) {
    otxbuffer[otxbufferIndex++] = data;
  } else {
    ESP_LOGE(FRSKYBT_TAG, "OTX Buffer Overflow");
    otxbufferIndex = 0;
  }
}

void processTrainerFrame(const uint8_t *otxbuffer)
{
  for (uint8_t channel = 0, i = 1; channel < BT_CHANNELS; channel += 2, i += 3) {
    // +-500 != 512, but close enough.
    channeldata[channel] = otxbuffer[i] + ((otxbuffer[i + 1] & 0xf0) << 4);
    channeldata[channel + 1] = ((otxbuffer[i + 1] & 0x0f) << 4) + ((otxbuffer[i + 2] & 0xf0) >> 4) +
                               ((otxbuffer[i + 2] & 0x0f) << 8);
  }

  if (settings.role == ROLE_BLE_PERIPHERAL) {
    rsndbuf[rsndbufindex++] = 0x7e;
    /*printf("BTDatOut ");
    for(int i=0; i < rsndbufindex; i++) {
      printf("%.2x ", rsndbuf[i]);
    }
    printf("\n");*/
    btp_sendChannelData(rsndbuf, rsndbufindex);
  }
}

void frSkyProcessByte(uint8_t data)
{
  static uint8_t dataState = STATE_DATA_IDLE;

  switch (dataState) {
    case STATE_DATA_START:
      if (data == START_STOP) {
        dataState = STATE_DATA_IN_FRAME;
        otxbufferIndex = 0;
        rsndbufindex = 0;
      } else {
        appendTrainerByte(data);
      }
      break;

    case STATE_DATA_IN_FRAME:
      if (data == BYTE_STUFF) {
        dataState = STATE_DATA_XOR;  // XOR next byte
      } else if (data == START_STOP) {
        dataState = STATE_DATA_IN_FRAME;
        otxbufferIndex = 0;
        rsndbufindex = 0;
      } else {
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
        case START_STOP:  // Illegal situation, as we have START_STOP, try to start from the
                          // beginning
          otxbufferIndex = 0;
          rsndbufindex = 0;
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
        rsndbufindex = 0;
        dataState = STATE_DATA_START;
      } else {
        appendTrainerByte(data);
      }
      break;
  }

  if (otxbufferIndex >= BLUETOOTH_PACKET_SIZE) {
    if(rsndbufindex <= BLUETOOTH_LINE_LENGTH+1)
      rsndbuf[rsndbufindex++] = data;
    uint8_t crc = 0x00;
    for (int i = 0; i < BLUETOOTH_PACKET_SIZE - 1; i++) {
      crc ^= otxbuffer[i];
    }
    if (crc == otxbuffer[BLUETOOTH_PACKET_SIZE - 1]) {
      if (otxbuffer[0] == TRAINER_FRAME) {
        processTrainerFrame(otxbuffer);
       // logBTFrame(true, "");
      } else {
        //logBTFrame(false, "Not a trainer frame");
      }
    } else {
      //logBTFrame(false, "CRC Fault");
    }
    dataState = STATE_DATA_IDLE;
  } else {
    // Create a copy, split at start/stop
    if(rsndbufindex <= BLUETOOTH_LINE_LENGTH+1)
      rsndbuf[rsndbufindex++] = data;
  }
}

void processFrame(const uint8_t *frame, uint8_t len)
{
  for (int i = 0; i < len; i++) {
    frSkyProcessByte(frame[i]);
  }
}