#include "stdio.h"
#include <stdint.h>
#include <string.h>

int main(void)
{
  /* check endianness */
  uint32_t x = 0x01020304u;
  uint8_t m[4];
  memcpy(m, &x, 4);
  printf("%02X %02X %02X %02X\n", m[0], m[1], m[2], m[3]);

  return 0;
}
