#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* interface of function pointers */
typedef struct
{
  int (*read)(void *ctx, uint8_t *buf, size_t len);
  int (*write)(void *ctx, const uint8_t *buf, size_t len);
} io_vtable;

/* interface value */
typedef struct
{
  const io_vtable *vt;
  void *self;
} io_stream_t;

/* a byte array */
typedef struct
{
  uint8_t *buf;
  size_t cap;
  size_t len;
  size_t pos;
} membuf;

static int mem_read(void *self, uint8_t *buf, size_t len)
{
  membuf *m = self; /* cast back to the real type */
  size_t avail = m->len - m->pos;

  if (len > avail)
    len = avail; /* may return less than asked */
  memcpy(buf, m->buf + m->pos, len);
  m->pos += len;
  return (int)len;
}

static int mem_write(void *self, const uint8_t *buf, size_t len)
{
  membuf *m = self;
  size_t space = m->cap - m->len;

  if (len > space)
    len = space; /* full: writes less, or 0 */
  memcpy(m->buf + m->len, buf, len);
  m->len += len;
  return (int)len;
}

static const io_vtable mem_vt = {.read = mem_read, .write = mem_write};

static int out_read(void *self, uint8_t *buf, size_t len)
{
  (void)self;
  (void)buf;
  (void)len;
  return -1; /* you cannot read stdout */
}

static int out_write(void *self, const uint8_t *buf, size_t len)
{
  (void)self; /* no state needed */
  return (int)fwrite(buf, 1, len, stdout);
}

static const io_vtable out_vt = {.read = out_read, .write = out_write};

static io_stream_t stdout_as_stream(void)
{
  return (io_stream_t){.vt = &out_vt, .self = NULL};
}

static void say(io_stream_ts, const char *text)
{
  s.vt->write(s.self, (const uint8_t *)text, strlen(text));
}
