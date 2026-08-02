#include <stdio.h>

#define SUCCESS 0

int a = 5;              // .data
int result;             // .bss
const int tax_rate = 5; // .rodata

int add(int a, int b); // .text

int main(void) // .text
{
  static int b = 10; // .data

  result = add(a, b);
  printf("The result of the addition is: %d\n",
         result); // .rodata (string literal)
  return SUCCESS;
}

int add(int a, int b)
{
  int tax = tax_rate; // stack
  return a + b + tax;
}
