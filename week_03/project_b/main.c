#include "stdint.h"
#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

float bits_to_float(uint32_t i) { return *(float *)&i; }

float fix_bits_to_float(uint32_t i)
{
  float f;
  memcpy(&f, &i, sizeof f);
  return f;
}

uint64_t mask(int n) { return (1 << n) - 1; } /* safe for n <= 31 */

uint64_t fix_mask(int n)
{
  return ((uint64_t)1 << n) - 1;
} /* safe for n <= 63 */

int out_of_bounds(int n)
{
  int a[n];
  for (int i = 0; i <= 10; i++) /* writes a[10] */
    a[i] = i;

  return n;
}

int fix_out_of_bounds(int n)
{
  int a[n];
  for (size_t i = 0; i < sizeof a / sizeof a[0]; i++)
    a[i] = i;

  return n;
}

int main(void)
{
  float f;
  uint64_t m;
  int b;

  f = bits_to_float(32);
  printf("%0.f\n", f);

  m = mask(32);
  printf("%" PRIu64 "\n", m);

  b = out_of_bounds(10);
  printf("%d\n", b);
}
