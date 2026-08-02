#include <stddef.h>
#include <stdio.h>

int main(void)
{
  int arr[5] = {10, 20, 30, 40, 50};

  int *ip = arr;
  for (size_t i = 0; i < 5; i++)
  {
    printf("int*:   %p -> %p (%td bytes)\n", (void *)ip, (void *)(ip + i),
           (char *)(ip + i) - (char *)ip);
  }

  int *a = &arr[0];
  int *b = &arr[3];
  ptrdiff_t c = (char *)b - (char *)a;

  printf("%td\n", b - a);
  printf("%td\n", c);

  return 0;
}
