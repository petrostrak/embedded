#include <stdint.h>
#include <stdio.h>

uint32_t add(uint32_t a, uint32_t b) { return a + b; }

int main(void)
{
  printf("%p\n", (void *)add); // prints the code address
  return 0;
}
