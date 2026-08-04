#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* ---- the interface ---- */
typedef int (*sink_fn)(void *ctx, uint8_t *buf, size_t len);

/* ---- implementation 1: sum the bytes ---- */
typedef struct
{
  uint32_t sum;
} sum_ctx;

static int sink_sum(void *vctx, uint8_t *buf, size_t len)
{
  sum_ctx *c = vctx;
  for (size_t i = 0; i < len; i++)
    c->sum += buf[i];
  return (int)len;
}

/* ---- implementation 2: append to a fixed buffer ---- */
typedef struct
{
  uint8_t *out;
  size_t cap, used;
} buf_ctx;

static int sink_buf(void *vctx, uint8_t *buf, size_t len)
{
  buf_ctx *c = vctx;
  if (len > c->cap - c->used)
    return -1; /* would overflow */
  memcpy(c->out + c->used, buf, len);
  c->used += len;
  return (int)len;
}

/* ---- implementation 3: discard ---- */
static int sink_null(void *vctx, uint8_t *buf, size_t len)
{
  (void)vctx;
  (void)buf; /* silence unused warnings */
  return (int)len;
}

/* ---- the dispatch table ---- */
typedef enum
{
  SINK_SUM = 0,
  SINK_BUF,
  SINK_NULL,
  SINK_COUNT
} sink_kind;

static const sink_fn sinks[SINK_COUNT] = {
    [SINK_SUM] = sink_sum,
    [SINK_BUF] = sink_buf,
    [SINK_NULL] = sink_null,
};

static sink_fn sink_lookup(sink_kind k)
{
  if (k < 0 || k >= SINK_COUNT)
    return NULL;
  return sinks[k];
}

/* ---- generic driver: knows nothing about any implementation ---- */
static int feed(sink_fn sink, void *ctx, uint8_t *data, size_t n)
{
  if (sink == NULL)
    return -1;
  size_t done = 0;
  while (done < n)
  {
    int r = sink(ctx, data + done, n - done);
    if (r <= 0)
      return r < 0 ? r : -1;
    done += (size_t)r;
  }
  return (int)done;
}

int main(void)
{
  uint8_t data[] = {1, 2, 3, 4, 5};

  sum_ctx s = {0};
  feed(sink_lookup(SINK_SUM), &s, data, sizeof data);
  printf("sum  = %u\n", s.sum); /* 15 */

  uint8_t out[8];
  buf_ctx b = {.out = out, .cap = sizeof out, .used = 0};
  feed(sink_lookup(SINK_BUF), &b, data, sizeof data);
  printf("used = %zu\n", b.used); /* 5 */

  printf("bad  = %d\n", feed(sink_lookup(SINK_COUNT), NULL, data, 5)); /* -1 */
  return 0;
}
