#ifndef RING_BUF_H
#define RING_BUF_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct
{
  uint8_t *buf;
  size_t capacity;
  size_t head, tail;
  bool full;
  size_t count;
} ringbuf_t;

void rb_init(ringbuf_t *rb, uint8_t *storage, size_t capacity);
bool rb_put(ringbuf_t *rb, uint8_t byte);
bool rb_get(ringbuf_t *rb, uint8_t *out);
size_t rb_count(const ringbuf_t *rb); /* we don't want to mutate the buffer */

#endif /* RING_BUF_H*/
