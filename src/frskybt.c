/* From OpenTX https://github.com/opentx
 */

#include "frskybt.h"

#include <stdbool.h>
#include <esp_timer.h>

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
// Receive Code
//----------------------------------
//
// Both the Bluetooth Trainer frame (0x80 + 12 channel bytes + CRC, 14 inner
// bytes) and the S.Port telemetry frame forwarded by EdgeTX (8 byte
// SportTelemetryPacket + CRC, 9 inner bytes) use the same 0x7E delimited,
// 0x7D byte-stuffed framing. Instead of relying on a fixed 14 byte length we
// delimit frames on the raw 0x7E and classify them by their unstuffed length.

enum { STATE_DATA_IDLE, STATE_DATA_IN_FRAME, STATE_DATA_XOR };

#define FRSKY_SPORT_PACKET_SIZE 9                  // 8 byte S.Port payload + XOR CRC
#define TRAINER_PACKET_SIZE BLUETOOTH_PACKET_SIZE  // 0x80 + 12 channels + XOR CRC

static uint8_t rxFrame[BLUETOOTH_LINE_LENGTH];
static uint8_t rxFrameIndex = 0;
static uint8_t dataState = STATE_DATA_IDLE;

/**
 * @brief Builds a 0x7E delimited, byte-stuffed frame from an already CRC'd
 *        payload (the CRC is expected to be the last payload byte).
 *
 * @return length written into dst
 */
static uint8_t buildFramedFrame(uint8_t *dst, const uint8_t *payload, uint8_t len)
{
  uint8_t idx = 0;
  dst[idx++] = START_STOP;
  for (uint8_t i = 0; i < len; i++) {
    uint8_t byte = payload[i];
    if (byte == START_STOP || byte == BYTE_STUFF) {
      dst[idx++] = BYTE_STUFF;
      byte ^= STUFF_MASK;
    }
    dst[idx++] = byte;
  }
  dst[idx++] = START_STOP;
  return idx;
}

static void appendFrameByte(uint8_t data)
{
  if (rxFrameIndex < sizeof(rxFrame)) {
    rxFrame[rxFrameIndex++] = data;
  } else {
    ESP_LOGE(FRSKYBT_TAG, "RX Buffer Overflow");
    rxFrameIndex = 0;
    dataState = STATE_DATA_IDLE;
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
    uint8_t outbuf[2 * TRAINER_PACKET_SIZE + 2];
    uint8_t outlen = buildFramedFrame(outbuf, otxbuffer, TRAINER_PACKET_SIZE);
    btp_sendChannelData(outbuf, outlen);
  }
}

/**
 * @brief Forwards an 8 byte S.Port telemetry packet (plus trailing CRC) to the
 *        connected BLE central, preserving the framing EdgeTX expects.
 *
 *        frame[0]   = physical id
 *        frame[1]   = prim id (0x10 = data frame)
 *        frame[2..3]= data id (little endian)
 *        frame[4..7]= value (little endian)
 *        frame[8]   = XOR CRC of the previous 8 bytes
 */
static void processSportFrame(const uint8_t *frame)
{
  if (settings.role == ROLE_BLE_PERIPHERAL) {
    uint8_t outbuf[2 * FRSKY_SPORT_PACKET_SIZE + 2];
    uint8_t outlen = buildFramedFrame(outbuf, frame, FRSKY_SPORT_PACKET_SIZE);
    btp_sendChannelData(outbuf, outlen);
  }

  ESP_LOGD(FRSKYBT_TAG, "S.Port physical=%02X prim=%02X data=%04X", frame[0], frame[1],
           (uint16_t)(frame[2] | (frame[3] << 8)));
}

static void handleFrame(const uint8_t *frame, uint8_t len)
{
  if (len < 2) return;

  uint8_t crc = 0x00;
  for (uint8_t i = 0; i < len - 1; i++) {
    crc ^= frame[i];
  }
  if (crc != frame[len - 1]) {
    // logBTFrame(false, "CRC Fault");
    return;
  }

  if (len == TRAINER_PACKET_SIZE && frame[0] == TRAINER_FRAME) {
    processTrainerFrame(frame);
    // logBTFrame(true, "");
  } else if (len == FRSKY_SPORT_PACKET_SIZE) {
    processSportFrame(frame);
  } else {
    // logBTFrame(false, "Unknown frame");
  }
}

void frSkyProcessByte(uint8_t data)
{
  switch (dataState) {
    case STATE_DATA_IDLE:
      if (data == START_STOP) {
        rxFrameIndex = 0;
        dataState = STATE_DATA_IN_FRAME;
      }
      break;

    case STATE_DATA_IN_FRAME:
      if (data == BYTE_STUFF) {
        dataState = STATE_DATA_XOR;  // XOR next byte
      } else if (data == START_STOP) {
        // Raw 0x7E can never appear inside a stuffed frame, so it delimits the end.
        handleFrame(rxFrame, rxFrameIndex);
        rxFrameIndex = 0;
        dataState = STATE_DATA_IDLE;
      } else {
        appendFrameByte(data);
      }
      break;

    case STATE_DATA_XOR:
      appendFrameByte(data ^ STUFF_MASK);
      dataState = STATE_DATA_IN_FRAME;
      break;
  }
}

void processFrame(const uint8_t *frame, uint8_t len)
{
  for (int i = 0; i < len; i++) {
    frSkyProcessByte(frame[i]);
  }
}