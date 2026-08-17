#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

#define BIT(n) (UINT64_C(1) << (n))

#define BIT_SET(x, n) ((x) |= BIT(n))
#define BIT_CLEAR(x, n) ((x) &= ~BIT(n))
#define BIT_TOGGLE(x, n) ((x) ^= BIT(n))
#define BIT_TEST(x, n) ((x) & BIT(n)) != 0

#define BIT_WRITE(x, n, v) ((v) ? BIT_SET(x, n) : BIT_CLEAR(x, n))

int main(void)
{
  printf("%d\n", BIT(1) == 0b00000010);
  printf("%d\n", BIT(3) == 0b00001000);
  printf("%d\n", BIT(5) == 0b00100000);

  uint64_t bit = 0xF7;
  printf("%" PRIx64 "\n", bit);
  BIT_SET(bit, 3);
  printf("%" PRIx64 "\n", bit);

  return 0;
}
