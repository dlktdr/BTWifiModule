#include "cobs.h"

int cobs_encode(const uint8_t* buffer, int size, uint8_t* encodedBuffer)
{
  int read_index  = 0;
  int write_index = 1;
  int code_index  = 0;
  uint8_t code    = 1;

  while (read_index < size) {
    if (buffer[read_index] == 0) {
      encodedBuffer[code_index] = code;
      code = 1;
      code_index = write_index++;
      read_index++;
    } else {
      encodedBuffer[write_index++] = buffer[read_index++];
      code++;
      if (code == 0xFF) {
          encodedBuffer[code_index] = code;
          code = 1;
          code_index = write_index++;
      }
    }
  }
  encodedBuffer[code_index] = code;
  return write_index;
}

int cobs_decode(const uint8_t* encodedBuffer, int size, uint8_t* decodedBuffer)
{
  if (size == 0)
      return 0;
  int read_index  = 0;
  int write_index = 0;
  uint8_t code       = 0;
  uint8_t i          = 0;
  while (read_index < size) {
    code = encodedBuffer[read_index];
    if (read_index + code > size && code != 1) {
      return 0;
    }
    read_index++;
    for (i = 1; i < code; i++) {
      decodedBuffer[write_index++] = encodedBuffer[read_index++];
    }
    if (code != 0xFF && read_index != size) {
      decodedBuffer[write_index++] = '\0';
    }
  }

  return write_index;
}
