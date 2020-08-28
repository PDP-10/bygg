#include <stdio.h>

typedef unsigned char byte;

int main(int argc, char* argv[])
{
  FILE* f;
  int len;

  for (;;) {
    len = getchar();
    if (len == EOF) break;
    len += getchar() << 8;
    while (len-- > 0) {
      putchar(getchar());
    }
  }
}
