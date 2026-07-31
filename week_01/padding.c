#include <stdalign.h>
#include <stdio.h>

int main(void)
{
  typedef struct
  {
    char a;   // 0 1
    short b;  // 2 3 4 5 6 7
    double c; // 8 9 10 11 12 13 17 18 19
    int d;    // 20
  } Example;  // 24

  printf("%zu\n", alignof(Example));
  printf("%zu\n", sizeof(Example));

  return 0;
}
