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

  char *const q = b;

  *q = 'W'; /* can write through the pointer */
  q[1] = '0';

  /* cannot point to another object */
  // q = a;

  return 0;
}
