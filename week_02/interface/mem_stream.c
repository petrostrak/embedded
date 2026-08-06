#include <string.h>

#include "mem_stream.h"

static int mem_read(void *vctx, uint8_t *buf, size_t len)
{
  mem_stream_ctx_t *ctx = vctx;
  size_t avail = ctx->len - ctx->pos;
  size_t n = (len < avail) ? len : avail;

  memcpy(buf, ctx->buf + ctx->pos, n);
  ctx->pos += n;
  return (int)n; /* 0 means end of stream */
}

static int mem_write(void *vctx, const uint8_t *buf, size_t len)
{
  mem_stream_ctx_t *ctx = vctx;
  size_t space = ctx->cap - ctx->len;
  size_t n = (len < space) ? len : space;

  if (n == 0 && len > 0)
    return IO_ERR_FULL;

  memcpy(ctx->buf + ctx->len, buf, n);
  ctx->len += n;
  return (int)n; /* short write is legal; caller loops */
}

io_stream_t mem_stream_open(mem_stream_ctx_t *ctx, uint8_t *buf, size_t cap)
{
  ctx->buf = buf;
  ctx->cap = cap;
  ctx->len = 0;
  ctx->pos = 0;

  io_stream_t s = {
      .read = mem_read,
      .write = mem_write,
      .ctx = ctx,
  };
  return s;
}

void mem_stream_rewind(mem_stream_ctx_t *ctx) { ctx->pos = 0; }
