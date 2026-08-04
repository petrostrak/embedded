#include <stddef.h>
#include <stdio.h>

int main(void)
{
  int arr[5] = {10, 20, 30, 40, 50};

  int *ip = arr;
  for (size_t i = 0; i < 5; i++)
  {
    printf("int*:   %p -> %p (%td bytes)\n", (void *)ip, (void *)(ip + i),
           (unsigned char *)(ip + i) - (unsigned char *)ip);
  }

  int *a = &arr[0];
  int *b = &arr[3];
  ptrdiff_t c = (char *)b - (char *)a;

  printf("%td\n", b - a);
  printf("%td\n", c);

  unsigned char *p = (unsigned char *)arr;
  for (size_t i = 0; i < sizeof arr; ++i)
    printf("%p\ti:%zu\n", (void *)(p + i), i);

  return 0;
}
