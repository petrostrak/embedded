#include <stdio.h>

#define BIT(n) (1UL << (n))

int main(void)
{
  printf("%d\n", BIT(1) == 0b00000010);
  printf("%d\n", BIT(3) == 0b00001000);
  printf("%d\n", BIT(5) == 0b00100000);

  return 0;
}
