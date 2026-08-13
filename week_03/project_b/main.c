#include "stdint.h"
#include <stdint.h>
#include <string.h>

float bits_to_float(uint32_t i) { return *(float *)&i; }

float fix_bits_to_float(uint32_t i)
{
  float f;
  memcpy(&f, &i, sizeof f);
  return f;
}

int main(void) { return 0; }
