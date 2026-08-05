#include "ringbuf.h"
#include <stdint.h>

void rb_init(ringbuf_t *rb, uint8_t *storage, size_t capacity)
{
  rb->buf = storage;
  rb->capacity = capacity;
  rb->head = 0;
  rb->tail = 0;
  rb->full = false;
}
