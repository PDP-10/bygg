#include <stdio.h>

void foo(void)
{
  int c = getchar();

  if (c != EOF) {
    foo();
    putchar(c);
  }
}

int main(int argc, char* argv[])
{
  foo();
}

