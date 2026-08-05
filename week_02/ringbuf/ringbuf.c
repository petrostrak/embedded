#include "ringbuf.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static inline bool rb_empty(const ringbuf_t *rb)
{
  return !rb->full && rb->head == rb->tail;
}

static inline size_t rb_next(const ringbuf_t *rb, size_t idx)
{
  return (idx + 1 == rb->capacity) ? 0 : idx + 1;
}

void rb_init(ringbuf_t *rb, uint8_t *storage, size_t capacity)
{
  rb->buf = storage;
  rb->capacity = capacity;
  rb->head = 0;
  rb->tail = 0;
  rb->full = false;
  rb->count = 0;
}

bool rb_put(ringbuf_t *rb, uint8_t byte)
{
  if (rb->full)
    return false;

  rb->buf[rb->head] = byte;
  rb->head = rb_next(rb, rb->head);
  rb->full = (rb->head == rb->tail);
  rb->count++;

  return true;
}

bool rb_get(ringbuf_t *rb, uint8_t *out)
{
  if (rb_empty(rb))
    return false; /* empty and *out is untouched */

  *out = rb->buf[rb->tail];
  rb->tail = rb_next(rb, rb->tail);
  rb->full = false;
  rb->count--;

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
