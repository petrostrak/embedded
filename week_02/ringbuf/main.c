#include "ringbuf.h"
#include <stdint.h>
#include <stdio.h>

void print_status(ringbuf_t *rb)
{
  printf("h=%zu  t=%zu  count  %zu\n", rb->head, rb->tail, rb->count);
}

int main(void)
{
  ringbuf_t rb;
  uint8_t storage[4] = {0};
  rb_init(&rb, storage, 4);

  print_status(&rb);
  _Bool flag = rb_put(&rb, 'A');
  if (!flag)
    printf("coundn't put 'A'\n");

  print_status(&rb);
  flag = rb_put(&rb, 'B');
  if (!flag)
    printf("coundn't put 'B'\n");

  print_status(&rb);
  uint8_t out = 'A';
  flag = rb_get(&rb, &out);
  if (!flag)
    printf("coundn't get 'A'\n");

  print_status(&rb);
  flag = rb_put(&rb, 'C');
  if (!flag)
    printf("coundn't put 'C'\n");

  print_status(&rb);
  out = 'B';
  flag = rb_get(&rb, &out);
  if (!flag)
    printf("coundn't get 'A'\n");
  print_status(&rb);

  return 0;
}
