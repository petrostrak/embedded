#ifndef IO_STREAM_H
#define IO_STREAM_H

#include <stddef.h>
#include <stdint.h>

/*
 * io_stream_t: hand-rolled interface.
 *
 *   read/write  -> the method set (the "vtable", inlined into the struct)
 *   ctx         -> the receiver (the "this" pointer), opaque to callers
 *
 * Contract every backend must honour:
 *   - returns >= 0 : number of bytes actually transferred (may be < len)
 *   - returns    0 : from read() means end of stream
 *   - returns  < 0 : error (see IO_ERR_* below)
 *   - a method may be NULL if the backend does not support it; consumers
 *     must check before calling.
 */
typedef struct
{
  int (*read)(void *ctx, uint8_t *buf, size_t len);
  int (*write)(void *ctx, const uint8_t *buf, size_t len);
  void *ctx;
} io_stream_t;

#define IO_ERR_UNSUPPORTED (-1) /* method not available on this backend */
#define IO_ERR_FULL (-2)        /* no space left to write               */
#define IO_ERR_IO (-3)          /* underlying I/O failure               */

#endif /* IO_STREAM_H */
