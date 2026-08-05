#include "ringbuf.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

void rb_init(ringbuf_t *rb, uint8_t *storage, size_t capacity)
{
  rb->buf = storage;
  rb->capacity = capacity;
  rb->head = 0;
  rb->tail = 0;
  rb->full = false;
}

bool rb_put(ringbuf_t *rb, uint8_t byte)
{
  if (rb->full)
    return false;

  rb->buf[rb->head] = byte;

  size_t next = rb->head + 1;
  if (next == rb->capacity)
    next = 0;
  rb->head = next;

  rb->full = (rb->head == rb->tail);

  return true;
}

bool rb_get(ringbuf_t *rb, uint8_t *out)
{
  if (!rb->full && rb->head == rb->tail)
    return false; /* empty and *out is untouched */

  *out = rb->buf[rb->tail];

  size_t next = rb->tail + 1;
  if (next == rb->capacity)
    next = 0;
  rb->tail = next;

  rb->full = false;
  return true;
}

size_t rb_count(const ringbuf_t *rb)
{
  if (rb->full)
    return rb->capacity;
  if (rb->head >= rb->tail)
    return rb->head - rb->tail;
  return rb->capacity + rb->head - rb->tail;
}
