/* A simple example to explore the preprocess */
#include <stdio.h>

#define GREETING "Hello, World"
#define SQUARE(x) ((x) * (x))

int main(void)
{
  printf("%s %d\n", GREETING, SQUARE(4));
  return 0;
}
