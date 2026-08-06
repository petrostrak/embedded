#ifndef MEM_STREAM_H
#define MEM_STREAM_H

#include "io_stream.h"

/* Receiver for the memory backend. Caller owns the storage: no malloc here. */
typedef struct
{
  uint8_t *buf; /* caller-supplied backing array */
  size_t cap;   /* size of buf                   */
  size_t len;   /* bytes written so far          */
  size_t pos;   /* read cursor                   */
} mem_stream_ctx_t;

/*
 * Wire up ctx over [buf, buf+cap) and return a stream that talks to it.
 * The returned stream keeps a pointer to ctx, so ctx must outlive the stream.
 */
io_stream_t mem_stream_open(mem_stream_ctx_t *ctx, uint8_t *buf, size_t cap);

/* Make previously written bytes readable from the start again. */
void mem_stream_rewind(mem_stream_ctx_t *ctx);

#endif /* MEM_STREAM_H */
