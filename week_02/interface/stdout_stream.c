#include "stdout_stream.h"

/* This backend is write-only. Kept as a real function (rather than a NULL
 * slot) so the failure mode is an explicit error code. */
static int stdout_read(void *vctx, uint8_t *buf, size_t len)
{
  (void)vctx;
  (void)buf;
  (void)len;
  return IO_ERR_UNSUPPORTED;
}

static int stdout_write(void *vctx, const uint8_t *buf, size_t len)
{
  stdout_stream_ctx_t *ctx = vctx;
  size_t n = fwrite(buf, 1, len, ctx->out);

  if (n < len && ferror(ctx->out))
    return IO_ERR_IO;

  ctx->written += n;
  return (int)n;
}

io_stream_t stdout_stream_open(stdout_stream_ctx_t *ctx)
{
  ctx->out = stdout;
  ctx->written = 0;

  io_stream_t s = {
      .read = stdout_read,
      .write = stdout_write,
      .ctx = ctx,
  };
  return s;
}
