#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define DEV_CAPACITY 32u
#define ERASED_BYTE 0xFFu

struct device
{
  uint8_t storage[DEV_CAPACITY];
  size_t offset; /* cursor; invariant: offset <= DEV_CAPACITY */
};

typedef enum
{
  OP_READ = 0,
  OP_WRITE,
  OP_ERASE,
  OP_COUNT
} opcode;

/* All three share one signature so they can live in the same table. */
static int op_read(void *ctx, uint8_t *buf, size_t len);
static int op_write(void *ctx, uint8_t *buf, size_t len);
static int op_erase(void *ctx, uint8_t *buf, size_t len);

typedef int (*fn)(void *ctx, uint8_t *buf, size_t len);

/* Array of OP_COUNT const pointers to int (void *, uint8_t *, size_t) */
static const fn ops[OP_COUNT] = {
    [OP_READ] = op_read,
    [OP_WRITE] = op_write,
    [OP_ERASE] = op_erase,
};

int handle(size_t opcode, void *ctx, uint8_t *buf, size_t len)
{
  if (opcode >= sizeof ops / sizeof ops[0])
    return -1;
  if (ops[opcode] == NULL) /* guards holes if the table is sparse */
    return -1;
  return ops[opcode](ctx, buf, len);
}

static int op_read(void *ctx, uint8_t *buf, size_t len)
{
  struct device *dev = ctx;

  if (dev == NULL || buf == NULL)
    return -1;
  if (len > DEV_CAPACITY - dev->offset) /* no underflow: offset <= capacity */
    return -2;

  memcpy(buf, dev->storage + dev->offset, len);
  dev->offset += len;
  return (int)len;
}

static int op_write(void *ctx, uint8_t *buf, size_t len)
{
  struct device *dev = ctx;

  if (dev == NULL || buf == NULL)
    return -1;
  if (len > DEV_CAPACITY - dev->offset)
    return -2;

  memcpy(dev->storage + dev->offset, buf, len);
  dev->offset += len;
  return (int)len;
}

static int op_erase(void *ctx, uint8_t *buf, size_t len)
{
  struct device *dev = ctx;

  (void)buf;
  if (dev == NULL)
    return -1;
  if (len > DEV_CAPACITY)
    return -2;

  memset(dev->storage, ERASED_BYTE, len);
  dev->offset = 0;
  return (int)len;
}

static void dump(const char *label, const uint8_t *p, size_t n)
{
  printf("%-10s", label);
  for (size_t i = 0; i < n; i++)
    printf(" %02X", p[i]);
  printf("  |");
  for (size_t i = 0; i < n; i++)
    putchar((p[i] >= 0x20 && p[i] < 0x7F) ? (char)p[i] : '.');
  printf("|\n");
}

int main(void)
{
  struct device dev = {{0}, 0};
  uint8_t msg[] = {'h', 'e', 'l', 'l', 'o'};
  uint8_t out[sizeof msg];
  int rc;

  rc = handle(OP_WRITE, &dev, msg, sizeof msg);
  printf("write -> %d\n", rc);

  dev.offset = 0; /* rewind before reading back */
  rc = handle(OP_READ, &dev, out, sizeof out);
  printf("read  -> %d\n", rc);
  dump("data:", out, sizeof out);

  rc = handle(OP_ERASE, &dev, NULL, sizeof msg);
  printf("erase -> %d\n", rc);

  rc = handle(OP_READ, &dev, out, sizeof out);
  printf("read  -> %d\n", rc);
  dump("data:", out, sizeof out);

  printf("bad opcode -> %d\n", handle(99, &dev, out, sizeof out));
  printf("overrun    -> %d\n", handle(OP_WRITE, &dev, msg, DEV_CAPACITY + 1));

  return 0;
}
