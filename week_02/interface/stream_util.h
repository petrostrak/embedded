#ifndef STREAM_UTIL_H
#define STREAM_UTIL_H

#include "io_stream.h"

/*
 * Consumers. These only ever touch io_stream_t: they have no idea which
 * backend they were handed, and they never look inside ctx.
 */

/* Push len bytes, looping over short writes. 0 on success, negative on error.
 */
int stream_write_all(io_stream_t *s, const uint8_t *buf, size_t len);

/* Same, for a NUL-terminated string. */
int stream_puts(io_stream_t *s, const char *str);

/* Classic offset / hex / ascii dump, emitted through the stream. */
int stream_hexdump(io_stream_t *s, const uint8_t *data, size_t len);

#endif /* STREAM_UTIL_H */
