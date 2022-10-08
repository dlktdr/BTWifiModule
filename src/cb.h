#pragma once

#include <stdlib.h>
#include <string.h>


typedef struct circular_buffer {
  char *buffer;      // data buffer
  char *buffer_end;  // end of data buffer
  size_t capacity;   // maximum number of items in the buffer
  size_t count;      // number of items in the buffer
  char *head;        // pointer to head
  char *tail;        // pointer to tail
} circular_buffer;

void cb_init(circular_buffer *cb, size_t capacity)
{
  cb->buffer = malloc(capacity);
  if (cb->buffer == NULL) {
    // TODO: handle error
  }
  cb->buffer_end = cb->buffer + capacity;
  cb->capacity = capacity;
  cb->count = 0;
  cb->head = cb->buffer;
  cb->tail = cb->buffer;
}

void cb_free(circular_buffer *cb) { free(cb->buffer); }

int cb_push_back(circular_buffer *cb, char *item)
{
  if (cb->count == cb->capacity) return -1;

  *cb->head = *item;

  cb->head++;
  if (cb->head == cb->buffer_end) cb->head = cb->buffer;
  cb->count++;
  return 0;
}

int cb_pop_front(circular_buffer *cb, char *item)
{
  if (cb->count == 0) {
    return -1;
  }
  *item = *cb->tail;

  cb->tail++;
  if (cb->tail == cb->buffer_end) cb->tail = cb->buffer;
  cb->count--;
  return 0;
}