#include "mem_stream.h"
#include "stdout_stream.h"
#include "stream_util.h"
#include <stdio.h>

static const uint8_t payload[] =
    "io_stream_t: one interface, two backends.\x00\x01\x02\xff";

int main(void)
{
  /* --- backend 2: stdout --------------------------------------------- */
  stdout_stream_ctx_t out_ctx;
  io_stream_t out = stdout_stream_open(&out_ctx);

  stream_puts(&out, "== hexdump straight to stdout ==\n");
  stream_hexdump(&out, payload, sizeof payload - 1);

  /* --- backend 1: memory buffer -------------------------------------- */
  uint8_t backing[512];
  mem_stream_ctx_t mem_ctx;
  io_stream_t mem = mem_stream_open(&mem_ctx, backing, sizeof backing);

  /* Exact same consumer call, different backend. */
  stream_puts(&mem, "== hexdump captured in memory ==\n");
  stream_hexdump(&mem, payload, sizeof payload - 1);

  printf("\ncaptured %zu bytes in the memory stream, reading it back:\n\n",
         mem_ctx.len);

  /* Drain the memory stream through read() and relay it to stdout. */
  mem_stream_rewind(&mem_ctx);
  uint8_t chunk[32];
  for (;;)
  {
    int n = mem.read(mem.ctx, chunk, sizeof chunk);
    if (n <= 0)
      break;
    stream_write_all(&out, chunk, (size_t)n);
  }

  /* The stdout backend is write-only. */
  printf("\nstdout read() -> %d (IO_ERR_UNSUPPORTED)\n",
         out.read(out.ctx, chunk, sizeof chunk));
  printf("bytes written to stdout via the stream: %zu\n", out_ctx.written);

  /* Writing past the end of a memory stream is a clean error, not a crash. */
  uint8_t tiny_backing[4];
  mem_stream_ctx_t tiny_ctx;
  io_stream_t tiny =
      mem_stream_open(&tiny_ctx, tiny_backing, sizeof tiny_backing);
  printf("overflowing a 4-byte memory stream -> %d (IO_ERR_FULL)\n",
         stream_puts(&tiny, "more than four bytes"));

  return 0;
}
