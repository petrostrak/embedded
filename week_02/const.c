#include <stdint.h>
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

  char *const q = b; /* must initialise here */
  // char *const q;     /* error: uninitialised const - useless for ever */

  *q = 'W'; /* can write through the pointer */
  q[1] = '0';

  /* cannot point to another object */
  // q = a;
  // q++;

  typedef struct
  {
    char const *name;
    uint8_t id;
  } cmd_t;

  static const cmd_t commands[] = {
      {"reset", 1},
  };

  // commands[0].name = 'X'; /* error! */

  char *c = commands[0].name; /* linter warning: initialising 'char *' with an
                                 expression of type 'const char *const' */
  c[0] = 'X';                 /* it compiles but corrupts the memory */

  return 0;
}
