#include <stdio.h>

int main(void)
{
  char a[] = "hello";
  char b[] = "world";

  const char *p = a;

  /* cannot write through the pointer */
  // p[0] = 'H';
  // *p = 'H';

  /* the rest is fine */
  p = b;
  p = b + 1;
  p++;

  printf("%c\n", *p); /* can read through it */
  a[0] = 'H';         /* fine - 'a' is not const, only our view of ti is */
  printf("%s\n", a);  /* Hello */

  return 0;
}
