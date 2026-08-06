#include <stdio.h>
#include <string.h>

#include "stream_util.h"

int stream_write_all(io_stream_t *s, const uint8_t *buf, size_t len)
{
  if (s->write == NULL)
    return IO_ERR_UNSUPPORTED;

  size_t done = 0;
  while (done < len)
  {
    int n = s->write(s->ctx, buf + done, len - done);
    if (n < 0)
      return n; /* propagate the backend's error code */
    if (n == 0)
      return IO_ERR_FULL; /* no progress: avoid spinning forever */
    done += (size_t)n;
  }
  return 0;
}

int stream_puts(io_stream_t *s, const char *str)
{
  return stream_write_all(s, (const uint8_t *)str, strlen(str));
}

int stream_hexdump(io_stream_t *s, const uint8_t *data, size_t len)
{
  char line[96];

  for (size_t off = 0; off < len; off += 16)
  {
    size_t n = (len - off < 16) ? len - off : 16;
    int p = snprintf(line, sizeof line, "%08zx  ", off);

    for (size_t i = 0; i < 16; i++)
    {
      if (i < n)
        p +=
            snprintf(line + p, sizeof line - (size_t)p, "%02x ", data[off + i]);
      else
        p += snprintf(line + p, sizeof line - (size_t)p, "   ");
      if (i == 7)
        line[p++] = ' ';
    }

    p += snprintf(line + p, sizeof line - (size_t)p, " |");
    for (size_t i = 0; i < n; i++)
    {
      uint8_t c = data[off + i];
      line[p++] = (c >= 0x20 && c < 0x7f) ? (char)c : '.';
    }
    line[p++] = '|';
    line[p++] = '\n';

    int rc = stream_write_all(s, (const uint8_t *)line, (size_t)p);
    if (rc < 0)
      return rc;
  }
  return 0;
}
