#ifndef STDOUT_STREAM_H
#define STDOUT_STREAM_H

#include <stdio.h>

#include "io_stream.h"

/* Receiver for the stdout backend. */
typedef struct
{
  FILE *out;      /* where bytes go               */
  size_t written; /* running total, just for show  */
} stdout_stream_ctx_t;

/* Write-only stream over stdout. read() always fails. */
io_stream_t stdout_stream_open(stdout_stream_ctx_t *ctx);

#endif /* STDOUT_STREAM_H */
